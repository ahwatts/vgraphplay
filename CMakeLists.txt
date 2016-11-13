cmake_minimum_required(VERSION 3.7.0)
project(vgraphplay)
set(CMAKE_CXX_STANDARD 11)

message(STATUS "Configuring for build type: " ${CMAKE_BUILD_TYPE})

find_package(Vulkan REQUIRED)
message(STATUS "Found Vulkan headers: " ${Vulkan_INCLUDE_DIRS})
message(STATUS "Found Vulkan libs:    " ${Vulkan_LIBRARIES})

include_directories(AFTER SYSTEM ${Vulkan_INCLUDE_DIRS})

add_subdirectory(vgraphplay)
