add_executable(vgraphplay
  Graphics.cpp
  VulkanOutput.cpp
  vgraphplay.cpp)

target_link_libraries(vgraphplay
  ${Boost_LIBRARIES}
  ${Vulkan_LIBRARIES}
  glfw)
