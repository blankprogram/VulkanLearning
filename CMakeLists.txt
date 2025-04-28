
cmake_minimum_required(VERSION 3.10)
project(VulkanEngine LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan     REQUIRED)
find_package(glm        REQUIRED)     # if GLM is installed via your distro
find_package(PkgConfig  REQUIRED)
pkg_search_module(GLFW   REQUIRED glfw3)

add_subdirectory(src)      # <-- builds the `engine` static‐lib

add_executable(app main.cpp)
target_include_directories(app PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(app PRIVATE
  engine
  Vulkan::Vulkan
  ${GLFW_LIBRARIES}
  glm::glm
)

add_custom_command(TARGET app POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets/shaders
    $<TARGET_FILE_DIR:app>/shaders
)
