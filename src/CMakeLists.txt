
add_library(engine
  core/Application.cpp
  core/Window.cpp
  math/Math.cpp
  platform/VulkanContext.cpp
  render/Mesh.cpp
  render/Pipeline.cpp
  render/Renderer.cpp
  render/Shader.cpp
  resources/ResourceManager.cpp
  scene/Camera.cpp
  scene/Scene.cpp
)

# expose <project-root>/include so all engine sources pick up #include <engine/...>
target_include_directories(engine PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)

# pull in Vulkan, GLFW, GLM to engine itself
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
