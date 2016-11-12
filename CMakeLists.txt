cmake_minimum_required(VERSION 3.7.0)
project(vgraphplay)

message(STATUS "Configuring for build type: " ${CMAKE_BUILD_TYPE})

find_package(Vulkan REQUIRED)
message(STATUS "Found Vulkan headers: " ${Vulkan_INCLUDE_DIRS})
message(STATUS "Found Vulkan libs:    " ${Vulkan_LIBRARIES})

include_directories(AFTER SYSTEM ${Vulkan_INCLUDE_DIRS})

add_executable(vgraphplay WIN32 main.cpp)
target_link_libraries(vgraphplay ${Vulkan_LIBRARIES})

if(MINGW)
  set_target_properties(vgraphplay PROPERTIES
    LINK_FLAGS "-Wl,--subsystem,console")
endif(MINGW)
