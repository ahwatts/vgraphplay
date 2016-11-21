add_executable(vgraphplay
  Graphics.cpp
  VulkanOutput.cpp
  System.cpp
  Win32.cpp
  vgraphplay.cpp)

target_link_libraries(vgraphplay ${Vulkan_LIBRARIES})
