add_library(vgraphplay_engine
  gfx/Commands.cpp
  gfx/Device.cpp
  gfx/Pipeline.cpp
  gfx/Presentation.cpp
  gfx/System.cpp
  gfx/VulkanExt.cpp
  gfx/VulkanOutput.cpp)

target_include_directories(vgraphplay_engine
  PRIVATE
  ${Boost_INCLUDE_DIRS}
  ${Vulkan_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIRS})

add_executable(vgraphplay
  vgraphplay.cpp)

target_link_libraries(vgraphplay
  vgraphplay_engine
  Boost::log_setup
  Boost::log
  Boost::filesystem
  Vulkan::Vulkan
  glfw)

add_executable(hello_triangle
  15_hello_triangle.cpp)

target_link_libraries(hello_triangle
  vgraphplay_engine
  Boost::log_setup
  Boost::log
  Boost::filesystem
  Vulkan::Vulkan
  glfw)

