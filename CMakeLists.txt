cmake_minimum_required(VERSION 3.10)
project(VulkanLearning)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# — find Vulkan
find_package(Vulkan REQUIRED)

# — find GLFW via pkg-config
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

# — list all of our sources
set(SOURCES
  main.cpp
  VulkanContext.cpp
  Pipeline.cpp
)

# — create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# — include directories (Vulkan + GLFW + project root)
target_include_directories(${PROJECT_NAME} PRIVATE
  ${Vulkan_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# — link against Vulkan and GLFW
target_link_libraries(${PROJECT_NAME} PRIVATE
  Vulkan::Vulkan
  ${GLFW_LIBRARIES}
)

# — platform‐specific Vulkan surface macros
if (WIN32)
  target_compile_definitions(${PROJECT_NAME} PRIVATE VK_USE_PLATFORM_WIN32_KHR)
elseif (UNIX)
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    VK_USE_PLATFORM_WAYLAND_KHR
    VK_USE_PLATFORM_XLIB_KHR
  )
endif()
