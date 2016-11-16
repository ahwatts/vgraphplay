add_executable(vgraphplay
  utils.cpp
  vgraphplay.cpp
  win32.cpp)

target_link_libraries(vgraphplay ${Vulkan_LIBRARIES})
