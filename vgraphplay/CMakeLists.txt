add_executable(vgraphplay
  Graphics.cpp
  VulkanOutput.cpp
  vgraphplay.cpp)

target_link_libraries(vgraphplay
  Boost::filesystem
  Boost::log
  ${Vulkan_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  glfw)
