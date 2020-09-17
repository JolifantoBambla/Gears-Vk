cmake_minimum_required(VERSION 3.13)
project(Gears_Vk)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

add_library(${PROJECT_NAME} STATIC)
target_include_directories(${PROJECT_NAME} PUBLIC
        framework/include)
target_sources(${PROJECT_NAME} PRIVATE
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
target_link_libraries(${PROJECT_NAME} PUBLIC avk)

# stb
include(cmake/stb.cmake)
target_include_directories(${PROJECT_NAME} PRIVATE ${stb_SOURCE_DIR})

# Assimp
include(cmake/assimp.cmake)
target_include_directories(${PROJECT_NAME} PUBLIC ${assimp_SOURCE_DIR}/include/assimp)
target_link_libraries(${PROJECT_NAME} PUBLIC assimp::assimp)

# GLFW
include(cmake/glfw.cmake)
target_include_directories(${PROJECT_NAME} PUBLIC ${glfw_SOURCE_DIR}/include/GLFW)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw)

# IMGUI
include(cmake/imgui.cmake)
target_include_directories(${PROJECT_NAME} PUBLIC ${imgui_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC imgui)

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Vulkan::Vulkan)

if (UNIX)
    # pthreads
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
endif (UNIX)




# TODO: build examples (if option is set)

# TODO: clean this up: https://stackoverflow.com/questions/26037954/cmake-target-link-libraries-interface-dependencies

# hello_world
add_executable(hello_world
        examples/hello_world/source/hello_world.cpp)
target_include_directories(hello_world PRIVATE ${PROJECT_NAME})
target_link_libraries(hello_world PRIVATE ${PROJECT_NAME})

# this is done by the post build helper on windows
file(COPY examples/hello_world/shaders DESTINATION ${CMAKE_BINARY_DIR})


# model_loader
add_executable(model_loader
        examples/model_loader/source/model_loader.cpp)
target_include_directories(model_loader PRIVATE ${PROJECT_NAME})
target_link_libraries(model_loader PRIVATE ${PROJECT_NAME})

# this is done by the post build helper on windows
file(COPY examples/model_loader/shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY assets/3rd_party/models/sponza/sponza_structure.obj DESTINATION ${CMAKE_BINARY_DIR}/assets)


# compute_image_processing
add_executable(compute_image_processing
        examples/compute_image_processing/source/compute_image_processing.cpp)
target_include_directories(compute_image_processing PRIVATE ${PROJECT_NAME})
target_link_libraries(compute_image_processing PRIVATE ${PROJECT_NAME})

# this is done by the post build helper on windows
file(COPY examples/compute_image_processing/shaders DESTINATION ${CMAKE_BINARY_DIR})
file(COPY assets/3rd_party/models/sponza/textures/lion.png DESTINATION ${CMAKE_BINARY_DIR}/assets)


# orca_loader
add_executable(orca_loader
        examples/orca_loader/source/orca_loader.cpp)
target_include_directories(orca_loader PRIVATE
        examples/orca_loader/source)
target_include_directories(orca_loader PRIVATE ${PROJECT_NAME})
target_link_libraries(orca_loader PRIVATE ${PROJECT_NAME})


## general assets stuff
# this is done be the post build helper on windows
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR})
#file(COPY assets/3rd_party/models/sponza/textures DESTINATION ${CMAKE_BINARY_DIR}/assets)
