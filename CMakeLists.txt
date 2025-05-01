
# CMakeLists.txt (project root)
cmake_minimum_required(VERSION 3.10)
project(VulkanEngine LANGUAGES CXX)



# only enable in Debug
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(ENABLE_VALIDATION_LAYERS)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
list(APPEND REQUIRED_INSTANCE_EXTENSIONS VK_EXT_debug_utils)
# locate either glslangValidator or glslang
find_program(GLSLANG_EXECUTABLE
  NAMES glslangValidator glslang
  HINTS $ENV{VULKAN_SDK}/bin
)
if(NOT GLSLANG_EXECUTABLE)
  message(FATAL_ERROR "glslangValidator (or glslang) not found; please install the Vulkan SDK or glslang-tools.")
endif()

# directory for compiled shaders
set(SPIRV_OUT ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SPIRV_OUT})

# let the code know where to look
add_definitions(-DSPIRV_OUT="${SPIRV_OUT}")

# compile our GLSL → SPIR-V
add_custom_command(
  OUTPUT ${SPIRV_OUT}/vert.spv ${SPIRV_OUT}/frag.spv
  COMMAND
    ${GLSLANG_EXECUTABLE} -V -S vert
      ${CMAKE_SOURCE_DIR}/assets/shaders/vert.glsl
      -o ${SPIRV_OUT}/vert.spv
  COMMAND
    ${GLSLANG_EXECUTABLE} -V -S frag
      ${CMAKE_SOURCE_DIR}/assets/shaders/frag.glsl
      -o ${SPIRV_OUT}/frag.spv
  DEPENDS
    ${CMAKE_SOURCE_DIR}/assets/shaders/vert.glsl
    ${CMAKE_SOURCE_DIR}/assets/shaders/frag.glsl
  COMMENT "Compiling GLSL shaders → SPIR-V"
)
add_custom_target(Shaders DEPENDS ${SPIRV_OUT}/vert.spv ${SPIRV_OUT}/frag.spv)

# 1) build the engine
add_subdirectory(src)

# 2) ensure engine waits on our Shaders
add_dependencies(engine Shaders)

# 3) build the final app
add_executable(VulkanEngine src/main.cpp)
target_include_directories(VulkanEngine PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(VulkanEngine PRIVATE
  engine
  Vulkan::Vulkan
  ${GLFW_LIBRARIES}
  glm::glm
)

