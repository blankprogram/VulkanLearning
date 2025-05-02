
cmake_minimum_required(VERSION 3.20)
project(Voxel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

find_program(GLSLANG_EXECUTABLE glslangValidator REQUIRED)

file(GLOB_RECURSE GLSL_SHADERS assets/shaders/*.vert assets/shaders/*.frag assets/shaders/*.comp)
set(SPIRV_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SPIRV_DIR})

foreach(SHADER ${GLSL_SHADERS})
    get_filename_component(FILENAME ${SHADER} NAME)
    set(SPIRV_OUT "${SPIRV_DIR}/${FILENAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV_OUT}
        COMMAND ${GLSLANG_EXECUTABLE} -V ${SHADER} -o ${SPIRV_OUT}
        DEPENDS ${SHADER}
    )
    list(APPEND SPIRV_OUTPUTS ${SPIRV_OUT})
endforeach()

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)

add_executable(Voxel ${SRC_FILES} ${SPIRV_OUTPUTS})

target_include_directories(Voxel PRIVATE include)



target_link_libraries(Voxel PRIVATE Vulkan::Vulkan ${GLFW_LIBRARIES} glm::glm)
