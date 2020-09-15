#pragma once
// ReSharper disable CppUnusedIncludeDirective

#include "include.hpp"

// -------------------- Gears-Vk includes --------------------
#include "cgb_exceptions.hpp"
#include "conversion_utils.hpp"

#include "context_state.hpp"

#include "cursor.hpp"

#include "context_generic_glfw_types.hpp"
#include "window_base.hpp"

#include "window.hpp"
#include "context_generic_glfw.hpp"

#include "math_utils.hpp"
#include "key_code.hpp"
#include "key_state.hpp"
#include "timer_frame_type.hpp"
#include "timer_interface.hpp"
#include "fixed_update_timer.hpp"
#include "varying_update_timer.hpp"
#include "input_buffer.hpp"
#include "composition_interface.hpp"

#include "vk_convenience_functions.hpp"

#include "settings.hpp"
#include "context_vulkan.hpp"

namespace gvk
{
#pragma region global data representing the currently active composition
	/**	@brief Get the current timer, which represents the current game-/render-time
	 *	\remark This is just a shortcut to @ref composition_interface::current()->time();
	 */
	inline timer_interface& time()
	{
		return composition_interface::current()->time();
	}

	/** @brief Get the current frame's input data
	 *	\remark This is just a shortcut to @ref composition_interface::current()->input();
	 */
	inline input_buffer& input()
	{
		return composition_interface::current()->input();
	}

	/** @brief Get access to the currently active objects
	 *	\remark This is just a shortcut to @ref *composition_interface::current();
	 */
	inline composition_interface* current_composition()
	{
		return composition_interface::current();
	}

	inline auto& context()
	{
		static context_vulkan sContext;
		return sContext;
	}
#pragma endregion 
}

#include "invokee.hpp"
#include "invoker_interface.hpp"
#include "sequential_invoker.hpp"

#include "transform.hpp"
#include "camera.hpp"
#include "quake_camera.hpp"
#include "material_config.hpp"
#include "material_gpu_data.hpp"
#include "material.hpp"
#include "lightsource.hpp"
#include "lightsource_gpu_data.hpp"
#include "model_types.hpp"
#include "animation.hpp"
#include "model.hpp"
#include "orca_scene.hpp"
#include "material_image_helpers.hpp"

#include "composition.hpp"
#include "setup.hpp"

#include "imgui_manager.hpp"

// ReSharper restore CppUnusedIncludeDirective
