

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "*.cpp")



set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/include/externals/imgui)

file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/*.cpp
)

set(IMGUI_SOURCES
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

add_library(engine STATIC ${ENGINE_SOURCES} ${IMGUI_SOURCES})



target_include_directories(engine PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${GLFW_INCLUDE_DIRS}
)

target_link_libraries(engine PUBLIC
  Vulkan::Vulkan
  ${GLFW_LIBRARIES}
  glm::glm
)



target_include_directories(engine PUBLIC
  ${CMAKE_SOURCE_DIR}/include
  ${IMGUI_DIR}
  ${IMGUI_DIR}/backends
  ${GLFW_INCLUDE_DIRS}
)

if (WIN32)
  target_compile_definitions(engine PRIVATE VK_USE_PLATFORM_WIN32_KHR)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  target_compile_definitions(engine PRIVATE
    VK_USE_PLATFORM_XLIB_KHR
    VK_USE_PLATFORM_WAYLAND_KHR
  )
endif()
