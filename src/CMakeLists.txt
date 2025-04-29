
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(engine STATIC ${ENGINE_SOURCES})

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
  ${GLFW_INCLUDE_DIRS}
  ${CMAKE_SOURCE_DIR}/external         # <— so we can #include "vk_mem_alloc.h"
)

if (WIN32)
  target_compile_definitions(engine PRIVATE VK_USE_PLATFORM_WIN32_KHR)
elseif (${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  target_compile_definitions(engine PRIVATE
    VK_USE_PLATFORM_XLIB_KHR
    VK_USE_PLATFORM_WAYLAND_KHR
  )
endif()
