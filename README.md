# Gears-Vk + Auto-Vk

*Gears-Vk* is a modern C++17-based rendering framework for the Vulkan 1.2 API.      
It aims to hit the sweet spot between programmer-convenience and efficiency while still supporting full Vulkan functionality.
To achieve this goal, this framework uses [*Auto-Vk*](https://github.com/cg-tuwien/Auto-Vk), a convenience and productivity layer atop [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp).

*Gears-Vk* is ready to go. If your system meets the system requirements (see section _Installation_ below), everything is set up to build an run right out of the box. Just open [`visual_studio/gears-vk.sln`](./visual_studio/gears-vk.sln), set one of the example projects as startup project, build and run.

*Note:* At the first run, the Post Build Helper tool is being built. Watch Visual Studio's "Output" tab for status messages and possible instructions.

# Installation

Currently, only Windows is supported as a development platform. The project setup is provided for Visual Studio 2019 only.

Requirements:
* Windows 10 
* Visual Studio 2019 with a Windows 10 SDK installed
* Vulkan SDK 1.2.141.0 or newer

Detailed information about project setup and resource management with Visual Studio are given in [`visual_studio/README.md`](./visual_studio/README.md)

# What's the difference between Gears-Vk and Auto-Vk?

*Auto-Vk* is a platform-agnostic convenience and productivity layer atop Vulkan-Hpp. 

*Gears-Vk* establishes the missing link to the operating system -- in this case Windows 10 -- and adds further functionality:
* Rendering environment configuration, such as enabling Vulkan extensions (e.g. if `VK_KHR_raytracing` shall be used, it selects an appropriate physical device and enables required flags and extensions)
* Window management (through GLFW)
* Game-loop/render-loop handling with convenient to use callback methods via the `gvk::invokee` interface (such as `initialize()`, `update()`, `render()`, where the former is called only once and the latter two are invoked each frame)
* User input handling
* A ready to use base class for object hierarchies: `gvk::transform`
* A ready to use user-controllable camera class `gvk::quake_camera` (which is derived from both, `gvk::transform` and `gvk::invokee`)
* Resource loading support for:
  * Images
  * 3D Models
  * Scenes in the ORCA format, see: [ORCA: Open Research Content Archive](https://developer.nvidia.com/orca)
* Material loading and conversion into a GPU-suitable format (`gvk::material` and `gvk::material_gpu_data`)
* Lightsource loading and conversion into a GPU-suitable format (`gvk::lightsource` and `gvk::lightsource_gpu_data`)
* Resource handling via Visual Studio's filters, i.e. just drag and drop assets and shaders that you'd like to use directly into Visual Studio's filter hierarchy and get them deployed to the target directory.
* A powerful Post Build Helper tool which is invoked as a custom build step.
  * It deploys assets and shaders to the target directory
  * Shaders are compiled into SPIR-V
  * If shader files contain errors, popup messages are created displaying the error, and providing a `[->VS]` button to navigate to the line that contains the error _within_ Visual Studio.
  * By default, "Debug" and "Release" build configurations symlink resources to save space, but "Publish" build configurations deploy all required files into the target directory so that a built program can easily be transfered to another PC. No more tedious resource gathering is required in such situations since that is all handled by the Post Build Helper.

# FAQs, Known Issues, Troubleshooting

**Q: I have troubles with asset management in Visual Studio. Any advice?**        
_A: Check out [Known Issues and Troubleshooting w.r.t. Resource Handling](https://github.com/cg-tuwien/cg_base/blob/master/visual_studio/README.md#known-issues-and-troubleshooting-wrt-asset-handling), which offers guidelines for the following cases:_      
[Build errors when adding assets](https://github.com/cg-tuwien/cg_base/blob/master/visual_studio/README.md#build-errors-when-adding-assets)         
[Asset is not deployed because it is not saved in the Visual Studio's filters-file](https://github.com/cg-tuwien/cg_base/blob/master/visual_studio/README.md#asset-is-not-deployed-because-it-is-not-saved-in-the-visual-studios-filters-file)     

**Q: I have troubles with the Post Build Helper. What to do?**        
_A: Check out [Known Issues and Troubleshooting w.r.t. CGB Post Build Helper](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#known-issues-and-troubleshooting-wrt-cgb-post-build-helper), which offers guidelines for the following cases:_        
[Application could not start at first try (maybe due to missing DLLs)](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#application-could-not-start-at-first-try-maybe-due-to-missing-dlls)        
[Error message about denied access to DLL files (DLLs are not re-deployed)](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#error-message-about-denied-access-to-dll-files-dlls-are-not-re-deployed)      
[Too few resources are being deployed](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#too-few-resources-are-being-deployed)      
[Slow performance when showing lists within CGB Post Build Helper](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#slow-performance-when-showing-lists-within-cgb-post-build-helper)      

**Q: The application takes a long time to load assets like 3D models and images. Can it be accelerated?**     
_A: If you are referring to the Debug-build, you can configure CGB Post Build Helper so that it deploys Release-DLLs of some external dependencies which should accelerate asset loading a lot._     
To do that, please open CGB Post Build Helper's [Settings dialogue](https://github.com/cg-tuwien/cg_base/tree/master/visual_studio#settings) and enable the option "Always deploy Release DLLs".

