cmake_minimum_required(VERSION 3.7.0)
project(vgraphplay)
set(CMAKE_CXX_STANDARD 11)

message(STATUS "Configuring for build type: " ${CMAKE_BUILD_TYPE})

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "-Wall")
  set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -pg")

  if(NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")
  endif(NOT APPLE)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "-Wall")
endif(CMAKE_COMPILER_IS_GNUCXX)

find_package(Vulkan REQUIRED)
message(STATUS "Found Vulkan headers: " ${Vulkan_INCLUDE_DIRS})
message(STATUS "Found Vulkan libs:    " ${Vulkan_LIBRARIES})

include_directories(AFTER SYSTEM ${Vulkan_INCLUDE_DIRS})

add_subdirectory(vgraphplay)
