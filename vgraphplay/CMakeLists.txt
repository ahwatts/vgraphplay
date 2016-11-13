add_executable(vgraphplay
  utils.cpp
  vgraphplay.cpp)

target_link_libraries(vgraphplay ${Vulkan_LIBRARIES})
