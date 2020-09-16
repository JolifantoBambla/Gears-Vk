cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY      https://github.com/ocornut/imgui.git
        GIT_TAG             v1.76
)

FetchContent_MakeAvailable(imgui)

add_library(imgui INTERFACE)
target_include_directories(imgui INTERFACE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/examples)
target_sources(imgui INTERFACE
        ${imgui_SOURCE_DIR}/imgui.cpp
        #${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/examples/imgui_impl_glfw.cpp
        #${imgui_SOURCE_DIR}/examples/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/examples/imgui_impl_vulkan.cpp)
