cmake_minimum_required(VERSION 3.14)

include(FetchContent)

FetchContent_Declare(
    auto-vk
    GIT_REPOSITORY      https://github.com/cg-tuwien/Auto-Vk.git
    GIT_TAG             master #3d8495f2212c5806ade9f678de59cb9ff74cbf26
)

FetchContent_MakeAvailable(auto-vk)
