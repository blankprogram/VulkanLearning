
add_library(engine STATIC
  core/Application.cpp
  core/Window.cpp
  math/Math.cpp
  platform/VulkanContext.cpp
  render/Mesh.cpp
  render/Pipeline.cpp
  render/Renderer.cpp
  resources/Texture.cpp
  scene/Camera.cpp
  utils/VulkanHelpers.cpp
  render/Material.cpp
)

target_include_directories(engine PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(engine PUBLIC
  Vulkan::Vulkan
  ${GLFW_LIBRARIES}
  glm::glm
)

if (WIN32)
  target_compile_definitions(engine PRIVATE VK_USE_PLATFORM_WIN32_KHR)
elseif (UNIX)
  target_compile_definitions(engine PRIVATE
    VK_USE_PLATFORM_XLIB_KHR
    VK_USE_PLATFORM_WAYLAND_KHR
  )
endif()
