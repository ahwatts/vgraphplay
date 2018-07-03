add_executable(vgraphplay
  gfx/Commands.cpp
  gfx/Device.cpp
  gfx/Pipeline.cpp
  gfx/Presentation.cpp
  gfx/System.cpp
  gfx/VulkanExt.cpp
  gfx/VulkanOutput.cpp
  vgraphplay.cpp)

target_link_libraries(vgraphplay
  Boost::log
  Boost::filesystem
  Vulkan::Vulkan
  glfw)
