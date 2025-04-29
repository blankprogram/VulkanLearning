
# CMakeLists.txt (project root)
cmake_minimum_required(VERSION 3.10)
project(VulkanEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

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
add_executable(app src/main.cpp)
target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(app PRIVATE
  engine
  Vulkan::Vulkan
  ${GLFW_LIBRARIES}
  glm::glm
)

