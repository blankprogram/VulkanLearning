cmake_minimum_required(VERSION 3.20)
project(Voxel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# — Vulkan, GLFW, GLM —
find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

# — ImGui — 
# Make sure this path really points at your include/externals/imgui folder
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/include/externals/imgui)

# core imgui files
file(GLOB IMGUI_CORE
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/imgui_demo.cpp
)

# only the backends you actually need
set(IMGUI_BACKENDS
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

add_library(imgui STATIC
  ${IMGUI_CORE}
  ${IMGUI_BACKENDS}
)

target_include_directories(imgui PUBLIC
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
)

# imgui_impl_vulkan needs Vulkan, imgui_impl_glfw needs GLFW
target_link_libraries(imgui PUBLIC Vulkan::Vulkan ${GLFW_LIBRARIES})

# — shaders → SPIR-V —
find_program(GLSLANG_VALIDATOR glslangValidator REQUIRED)
file(GLOB_RECURSE GLSL_SHADERS
  ${CMAKE_SOURCE_DIR}/assets/shaders/*.vert
  ${CMAKE_SOURCE_DIR}/assets/shaders/*.frag
  ${CMAKE_SOURCE_DIR}/assets/shaders/*.comp
)

set(SPIRV_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SPIRV_DIR})

set(SPIRV_OUTPUTS)
foreach(SHADER ${GLSL_SHADERS})
  get_filename_component(FNAME ${SHADER} NAME)
  set(OUT ${SPIRV_DIR}/${FNAME}.spv)
  add_custom_command(
    OUTPUT ${OUT}
    COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER} -o ${OUT}
    DEPENDS ${SHADER}
  )
  list(APPEND SPIRV_OUTPUTS ${OUT})
endforeach()

# — your application —
file(GLOB_RECURSE SRC_FILES
  ${CMAKE_SOURCE_DIR}/src/*.cpp
)

add_executable(Voxel
  ${SRC_FILES}
  ${SPIRV_OUTPUTS}
)

target_include_directories(Voxel PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(Voxel PRIVATE
  Vulkan::Vulkan
  ${GLFW_LIBRARIES}
  glm::glm
  imgui
)

