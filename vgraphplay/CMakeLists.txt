add_executable(vgraphplay
  Graphics.cpp
  VulkanOutput.cpp
  vgraphplay.cpp)

target_link_libraries(vgraphplay
  Boost::log
  Boost::filesystem
  Vulkan::Vulkan
  glfw)
