cmake_minimum_required(VERSION 3.13)
project(Gears_Vk)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

add_library(${PROJECT_NAME} STATIC)
target_include_directories(${PROJECT_NAME} PUBLIC
        framework/include)
target_sources(${PROJECT_NAME} PUBLIC
        framework/src/animation.cpp
        framework/src/camera.cpp
        framework/src/cgb_exceptions.cpp
        framework/src/composition.cpp
        framework/src/composition_interface.cpp
        framework/src/context_generic_glfw.cpp
        framework/src/context_vulkan.cpp
        framework/src/fixed_update_timer.cpp
        framework/src/imgui_manager.cpp
        framework/src/input_buffer.cpp
        framework/src/log.cpp
        framework/src/material_image_helpers.cpp
        framework/src/math_utils.cpp
        framework/src/model.cpp
        framework/src/orca_scene.cpp
        framework/src/quake_camera.cpp
        framework/src/transform.cpp
        framework/src/varying_update_timer.cpp
        framework/src/vk_convenience_functions.cpp
        framework/src/window.cpp
        framework/src/window_base.cpp
        #framework/src/window_vulkan.cpp // this is a cg_base thing - remove?
        )

# add external include directories
target_include_directories(${PROJECT_NAME} PUBLIC
        external/universal/include)

# Auto-Vk
include(cmake/auto-vk.cmake)
target_include_directories(${PROJECT_NAME} PUBLIC ${auto-vk_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} avk)

# stb
include(cmake/stb.cmake)
target_include_directories(${PROJECT_NAME} PUBLIC ${stb_SOURCE_DIR})

# TODO: include & link other external libraries

# TODO: build examples (if option is set)
