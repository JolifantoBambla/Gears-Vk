#include <gvk.hpp>

namespace gvk
{
	std::mutex window::sSubmitMutex;

	void window::enable_resizing(bool aEnable)
	{
		mShallBeResizable = aEnable;
		
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	
	void window::request_srgb_framebuffer(bool aRequestSrgb)
	{
		// Which formats are supported, depends on the surface.
		mSurfaceFormatSelector = [lSrgbFormatRequested = aRequestSrgb](const vk::SurfaceKHR & surface) {
			// Get all the formats which are supported by the surface:
			auto srfFrmts = context().physical_device().getSurfaceFormatsKHR(surface);

			// Init with a default format...
			auto selSurfaceFormat = vk::SurfaceFormatKHR{
				vk::Format::eB8G8R8A8Unorm,
				vk::ColorSpaceKHR::eSrgbNonlinear
			};

			// ...and try to possibly find one which is definitely supported or better suited w.r.t. the surface.
			if (!(srfFrmts.size() == 1 && srfFrmts[0].format == vk::Format::eUndefined)) {
				for (const auto& e : srfFrmts) {
					if (lSrgbFormatRequested) {
						if (avk::is_srgb_format(e.format)) {
							selSurfaceFormat = e;
							break;
						}
					}
					else {
						if (!avk::is_srgb_format(e.format)) {
							selSurfaceFormat = e;
							break;
						}
					}
				}
			}

			// In any case, return a format
			return selSurfaceFormat;
		};

		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::set_presentaton_mode(gvk::presentation_mode aMode)
	{
		mPresentationModeSelector = [lPresMode = aMode](const vk::SurfaceKHR& surface) {
			// Supported presentation modes must be queried from a device:
			auto presModes = context().physical_device().getSurfacePresentModesKHR(surface);

			// Select a presentation mode:
			auto selPresModeItr = presModes.end();
			switch (lPresMode) {
			case gvk::presentation_mode::immediate:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eImmediate);
				break;
			case gvk::presentation_mode::relaxed_fifo:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eFifoRelaxed);
				break;
			case gvk::presentation_mode::fifo:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eFifo);
				break;
			case gvk::presentation_mode::mailbox:
				selPresModeItr = std::find(std::begin(presModes), std::end(presModes), vk::PresentModeKHR::eMailbox);
				break;
			default:
				throw gvk::runtime_error("should not get here");
			}
			if (selPresModeItr == presModes.end()) {
				LOG_WARNING_EM("No presentation mode specified or desired presentation mode not available => will select any presentation mode");
				selPresModeItr = presModes.begin();
			}

			return *selPresModeItr;
		};

		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::set_number_of_samples(vk::SampleCountFlagBits aNumSamples)
	{
		mNumberOfSamplesGetter = [lSamples = aNumSamples]() { return lSamples; };

		mMultisampleCreateInfoBuilder = [this]() {
			auto samples = mNumberOfSamplesGetter();
			return vk::PipelineMultisampleStateCreateInfo()
				.setSampleShadingEnable(vk::SampleCountFlagBits::e1 == samples ? VK_FALSE : VK_TRUE) // disable/enable?
				.setRasterizationSamples(samples)
				.setMinSampleShading(1.0f) // Optional
				.setPSampleMask(nullptr) // Optional
				.setAlphaToCoverageEnable(VK_FALSE) // Optional
				.setAlphaToOneEnable(VK_FALSE); // Optional
		};

		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::set_number_of_presentable_images(uint32_t aNumImages)
	{
		mNumberOfPresentableImagesGetter = [lNumImages = aNumImages]() { return lNumImages; };

		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::set_number_of_concurrent_frames(window::frame_id_t aNumConcurrent)
	{
		mNumberOfConcurrentFramesGetter = [lNumConcurrent = aNumConcurrent]() { return lNumConcurrent; };

		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::set_additional_back_buffer_attachments(std::vector<avk::attachment> aAdditionalAttachments)
	{
		mAdditionalBackBufferAttachmentsGetter = [lAdditionalAttachments = std::move(aAdditionalAttachments)]() { return lAdditionalAttachments; };

		// If the window has already been created, the new setting can't 
		// be applied unless the window is being recreated.
		if (is_alive()) {
			mRecreationRequired = true;
		}
	}

	void window::open()
	{
		context().dispatch_to_main_thread([this]() {
			// Ensure, previous work is done:
			context().work_off_event_handlers();

			// Share the graphics context between all windows
			auto* sharedContex = context().get_window_for_shared_context();
			// Bring window into existance:
			glfwWindowHint(GLFW_RESIZABLE, get_config_shall_be_resizable() ? GLFW_TRUE : GLFW_FALSE);
			auto* handle = glfwCreateWindow(mRequestedSize.mWidth, mRequestedSize.mHeight,
				mTitle.c_str(),
				mMonitor.has_value() ? mMonitor->mHandle : nullptr,
				sharedContex);
			if (nullptr == handle) {
				// No point in continuing
				throw gvk::runtime_error("Failed to create window with the title '" + mTitle + "'");
			}
			mHandle = window_handle{ handle };
			initialize_after_open();

			// There will be some pending work regarding this newly created window stored within the
			// context's events, like creating a swap chain and so on. 
			// Why wait? Invoke them now!
			context().work_off_event_handlers();
		});
	}

	bool window::get_config_shall_be_resizable() const
	{
		return mShallBeResizable;
	}

	vk::SurfaceFormatKHR window::get_config_surface_format(const vk::SurfaceKHR & aSurface)
	{
		if (!mSurfaceFormatSelector) {
			// Set the default:
			request_srgb_framebuffer(false);
		}
		// Determine the format:
		return mSurfaceFormatSelector(aSurface);
	}

	vk::PresentModeKHR window::get_config_presentation_mode(const vk::SurfaceKHR & aSurface)
	{
		if (!mPresentationModeSelector) {
			// Set the default:
			set_presentaton_mode(gvk::presentation_mode::mailbox);
		}
		// Determine the presentation mode:
		return mPresentationModeSelector(aSurface);
	}

	vk::SampleCountFlagBits window::get_config_number_of_samples()
	{
		if (!mNumberOfSamplesGetter) {
			// Set the default:
			set_number_of_samples(vk::SampleCountFlagBits::e1);
		}
		// Determine the number of samples:
		return mNumberOfSamplesGetter();
	}

	vk::PipelineMultisampleStateCreateInfo window::get_config_multisample_state_create_info()
	{
		if (!mMultisampleCreateInfoBuilder) {
			// Set the default:
			set_number_of_samples(vk::SampleCountFlagBits::e1);
		}
		// Get the config struct:
		return mMultisampleCreateInfoBuilder();
	}

	uint32_t window::get_config_number_of_presentable_images()
	{
		if (!mNumberOfPresentableImagesGetter) {
			auto srfCaps = context().physical_device().getSurfaceCapabilitiesKHR(surface());
			auto imageCount = srfCaps.minImageCount + 1u;
			if (srfCaps.maxImageCount > 0) { // A value of 0 for maxImageCount means that there is no limit
				imageCount = glm::min(imageCount, srfCaps.maxImageCount);
			}
			return imageCount;
		}
		return mNumberOfPresentableImagesGetter();
	}

	window::frame_id_t window::get_config_number_of_concurrent_frames()
	{
		if (!mNumberOfConcurrentFramesGetter) {
			return get_config_number_of_presentable_images();
		}
		return mNumberOfConcurrentFramesGetter();
	}

	std::vector<avk::attachment> window::get_additional_back_buffer_attachments()
	{
		if (!mAdditionalBackBufferAttachmentsGetter) {
			return {};
		}
		else {
			return mAdditionalBackBufferAttachmentsGetter();
		}
	}

	void window::handle_lifetime(avk::command_buffer aCommandBuffer, std::optional<frame_id_t> aFrameId)
	{
		std::scoped_lock<std::mutex> guard(sSubmitMutex);
		if (!aFrameId.has_value()) {
			aFrameId = current_frame();
		}

		aCommandBuffer->invoke_post_execution_handler(); // Yes, do it now!
		
		auto& refTpl = mLifetimeHandledCommandBuffers.emplace_back(aFrameId.value(), std::move(aCommandBuffer));
		// ^ Prefer code duplication over recursive_mutex
	}

	void window::handle_lifetime(std::optional<avk::command_buffer> aCommandBuffer, std::optional<frame_id_t> aFrameId)
	{
		if (!aCommandBuffer.has_value()) {
			LOG_WARNING("std::optional<command_buffer> submitted and it has no value.");
			return;
		}
		handle_lifetime(std::move(aCommandBuffer.value()), aFrameId);
	}

	std::vector<avk::semaphore> window::remove_all_present_semaphore_dependencies_for_frame(frame_id_t aPresentFrameId)
	{
		// No need to protect against concurrent access since that would be misuse of this function.
		// This shall never be called from the invokee callbacks as being invoked through a parallel invoker.
		
		// Find all to remove
		auto to_remove = std::remove_if(
			std::begin(mPresentSemaphoreDependencies), std::end(mPresentSemaphoreDependencies),
			[maxTTL = aPresentFrameId - number_of_frames_in_flight()](const auto& tpl) {
				return std::get<frame_id_t>(tpl) <= maxTTL;
			});
		// return ownership of all the semaphores to remove to the caller
		std::vector<avk::semaphore> moved_semaphores;
		for (decltype(to_remove) it = to_remove; it != std::end(mPresentSemaphoreDependencies); ++it) {
			moved_semaphores.push_back(std::move(std::get<avk::semaphore>(*it)));
		}
		// Erase and return
		mPresentSemaphoreDependencies.erase(to_remove, std::end(mPresentSemaphoreDependencies));
		return moved_semaphores;
	}

	std::vector<avk::command_buffer> window::clean_up_command_buffers_for_frame(frame_id_t aPresentFrameId)
	{
		// No need to protect against concurrent access since that would be misuse of this function.
		// This shall never be called from the invokee callbacks as being invoked through a parallel invoker.

		// Up to the frame with id 'maxTTL', all command buffers can be safely removed
		const auto maxTTL = aPresentFrameId - number_of_frames_in_flight();
		
		// 2. SINGLE USE COMMAND BUFFERS
		// Can not use the erase-remove idiom here because that would invalidate iterators and references
		// HOWEVER: "[...]unless the erased elements are at the end or the beginning of the container,
		// in which case only the iterators and references to the erased elements are invalidated." => Let's do that!
		auto eraseBegin = std::begin(mLifetimeHandledCommandBuffers);
		std::vector<avk::command_buffer> removedBuffers;
		if (std::end(mLifetimeHandledCommandBuffers) == eraseBegin || std::get<frame_id_t>(*eraseBegin) > maxTTL) {
			return removedBuffers;
		}
		// There are elements that we can remove => find position until where:
		auto eraseEnd = eraseBegin;
		while (eraseEnd != std::end(mLifetimeHandledCommandBuffers) && std::get<frame_id_t>(*eraseEnd) <= maxTTL) {
			// return ownership of all the command_buffers to remove to the caller
			removedBuffers.push_back(std::move(std::get<avk::command_buffer>(*eraseEnd)));
			++eraseEnd;
		}
		mLifetimeHandledCommandBuffers.erase(eraseBegin, eraseEnd);
		return removedBuffers;
	}

	void window::fill_in_present_semaphore_dependencies_for_frame(std::vector<vk::Semaphore>& aSemaphores, std::vector<vk::PipelineStageFlags>& aWaitStages, frame_id_t aFrameId) const
	{
		for (const auto& [frameId, sem] : mPresentSemaphoreDependencies) {
			if (frameId == aFrameId) {
				aSemaphores.push_back(sem->handle());
				aWaitStages.push_back(sem->semaphore_wait_stage());
			}
		}
	}

	void window::sync_before_render()
	{
		// Wait for the fence before proceeding, GPU -> CPU synchronization via fence
		const auto ci = current_in_flight_index();
		auto& cf = current_fence();
		assert(cf.handle() == mFramesInFlightFences[current_in_flight_index()]->handle());
		cf.wait_until_signalled();
		cf.reset();

		// Keep house with the in-flight images:
		//   However, we don't know which index this fence had been mapped to => we have to search
		for (auto& mapping : mImagesInFlightFenceIndices) {
			if (ci == mapping) {
				mapping = -1;
				break;
			}
		}

		// At this point we are certain that the frame which has used the current fence before is done.
		//  => Clean up the resources of that previous frame!
		auto semaphoresToBeFreed = remove_all_present_semaphore_dependencies_for_frame(current_frame());
		auto commandBuffersToBeFreed 	= clean_up_command_buffers_for_frame(current_frame());

		//
		//
		//
		//	TODO: Recreate swap chain probably somewhere here
		//  Potential problems:
		//	 - How to handle the fences? Is waitIdle enough?
		//	 - A problem might be the multithreaded access to this function... hmm... or is it??
		//      => Now would be the perfect time to think about how to handle parallel invokers
		//		   Only Command Buffer generation should be parallelized anyways, submission should 
		//		   be done on ONE thread, hence access to this method would be syncronized inherently, right?!
		//
		//	What about the following: Tie an instance of invokee to ONE AND EXACTLY ONE window*?!
		//	 => Then, the render method would create a command_buffer, which is then gathered (per window!) and passed on to this method.
		//
		//
		//

		try
		{
			// Get the next image from the swap chain, GPU -> GPU sync from previous present to the following acquire
			auto& imgAvailableSem = image_available_semaphore_for_frame();

			// Update previous image index before getting a new image index for the current frame:
			mPreviousFrameImageIndex = mCurrentFrameImageIndex;
			
			context().device().acquireNextImageKHR(
				swap_chain(), // the swap chain from which we wish to acquire an image 
				// At this point, I have to rant about the `timeout` parameter:
				// The spec says: "timeout specifies how long the function waits, in nanoseconds, if no image is available."
				// HOWEVER, don't think that a numeric_limit<int64_t>::max() will wait for nine quintillion nanoseconds!
				//    No, instead it will return instantly, yielding an invalid swap chain image index. OMG, WTF?!
				// Long story short: make sure to pass the UNSINGEDint64_t's maximum value, since only that will disable the timeout.
				std::numeric_limits<uint64_t>::max(), // a timeout in nanoseconds for an image to become available. Using the maximum value of a 64 bit unsigned integer disables the timeout. [1]
				imgAvailableSem.handle(), // The next two parameters specify synchronization objects that are to be signaled when the presentation engine is finished using the image [1]
				nullptr,
				&mCurrentFrameImageIndex); // a variable to output the index of the swap chain image that has become available. The index refers to the VkImage in our swapChainImages array. We're going to use that index to pick the right command buffer. [1]

			// It could be that the image index that has been returned is currently in flight.
			// There's no guarantee that we'll always get a nice cycling through the indices.
			// => Must handle this case!
			assert(current_image_index() == mCurrentFrameImageIndex);
			if (mImagesInFlightFenceIndices[current_image_index()] >= 0) {
				LOG_DEBUG_VERBOSE(fmt::format("Frame #{}: Have to issue an extra fence-wait because swap chain returned image[{}] but fence[{}] is currently in use.", current_frame(), mCurrentFrameImageIndex, mImagesInFlightFenceIndices[current_image_index()]));
				auto& xf = mFramesInFlightFences[mImagesInFlightFenceIndices[current_image_index()]];
				xf->wait_until_signalled();
				// But do not reset! Otherwise we will wait forever at the next wait_until_signalled that will happen for sure.
			}

			// Set the image available semaphore to be consumed:
			mCurrentFrameImageAvailableSemaphore = std::ref(imgAvailableSem);
		}
		catch (vk::OutOfDateKHRError omg) {
			LOG_INFO(fmt::format("Swap chain out of date at presentKHR-call[{}]. Waiting for better times...", omg.what()));
		}
	}
	
	void window::render_frame()
	{
		try
		{
			const auto& cf = current_fence();

			// EXTERN -> WAIT 
			std::vector<vk::Semaphore> renderFinishedSemaphores;
			std::vector<vk::PipelineStageFlags> renderFinishedSemaphoreStages; 
			fill_in_present_semaphore_dependencies_for_frame(renderFinishedSemaphores, renderFinishedSemaphoreStages, current_frame());

			if (!has_consumed_current_image_available_semaphore()) {
				LOG_WARNING(fmt::format("Frame #{}: User has not consumed the 'image available semaphore'. Render results might be corrupted. Use consume_current_image_available_semaphore() every frame!", current_frame()));
				auto& imgAvailable = consume_current_image_available_semaphore();
				renderFinishedSemaphores.push_back(imgAvailable.handle());
				renderFinishedSemaphoreStages.push_back(imgAvailable.semaphore_wait_stage());
			}

			// TODO: What if the user has not submitted any renderFinishedSemaphores?
			
			// WAIT -> SIGNAL
			auto& signalSemaphore = current_initiate_present_semaphore();

			auto submitInfo = vk::SubmitInfo()
				.setWaitSemaphoreCount(static_cast<uint32_t>(renderFinishedSemaphores.size()))
				.setPWaitSemaphores(renderFinishedSemaphores.data())
				.setPWaitDstStageMask(renderFinishedSemaphoreStages.data())
				.setCommandBufferCount(0u) // Submit ZERO command buffers :O
				.setSignalSemaphoreCount(1u)
				.setPSignalSemaphores(signalSemaphore.handle_addr());
			// SIGNAL + FENCE, actually:
			assert(mPresentQueue);
			mPresentQueue->handle().submit(1u, &submitInfo, cf.handle());

			// SIGNAL -> PRESENT
			auto presentInfo = vk::PresentInfoKHR()
				.setWaitSemaphoreCount(1u)
				.setPWaitSemaphores(signalSemaphore.handle_addr())
				.setSwapchainCount(1u)
				.setPSwapchains(&swap_chain())
				.setPImageIndices(&mCurrentFrameImageIndex)
				.setPResults(nullptr);
			mPresentQueue->handle().presentKHR(presentInfo);
			
			// increment frame counter
			++mCurrentFrame;
		}
		catch (vk::OutOfDateKHRError omg) {
			LOG_INFO(fmt::format("Swap chain out of date at presentKHR-call[{}]. Waiting for better times...", omg.what()));
		}
	}
	
	void window::add_queue_family_ownership(avk::queue& aQueue)
	{
		mQueueFamilyIndicesGetter.emplace_back([pQueue = &aQueue](){ return pQueue->family_index(); });
	}

	void window::set_present_queue(avk::queue& aPresentQueue)
	{
		mPresentQueue = &aPresentQueue;
	}		
	
}