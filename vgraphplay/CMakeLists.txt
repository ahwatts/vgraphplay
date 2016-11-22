add_executable(vgraphplay
  Graphics.cpp
  VulkanOutput.cpp
  vgraphplay.cpp)

target_link_libraries(vgraphplay
  ${Vulkan_LIBRARIES}
  ${GLFW_LIBRARIES})
