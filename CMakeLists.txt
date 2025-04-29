
cmake_minimum_required(VERSION 3.10)
project(VulkanEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

# 1. Add engine library from src/
add_subdirectory(src)

# 2. Add app (entry point)

add_executable(app src/main.cpp)


# 3. Include and link properly
target_include_directories(app PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(app PRIVATE
  engine
  Vulkan::Vulkan
  ${GLFW_LIBRARIES}
  glm::glm
)
