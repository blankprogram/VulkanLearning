
cmake_minimum_required(VERSION 3.10)
project(VulkanLearning)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Vulkan REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

add_executable(VulkanLearning main.cpp)

target_include_directories(VulkanLearning PRIVATE
    ${Vulkan_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
)

target_link_libraries(VulkanLearning
    Vulkan::Vulkan
    ${GLFW_LIBRARIES}
)

if (WIN32)
    target_compile_definitions(VulkanLearning PRIVATE VK_USE_PLATFORM_WIN32_KHR)
elseif (UNIX)
    target_compile_definitions(VulkanLearning PRIVATE VK_USE_PLATFORM_WAYLAND_KHR VK_USE_PLATFORM_XLIB_KHR)
endif()
