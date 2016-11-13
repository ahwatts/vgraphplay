cmake_minimum_required(VERSION 3.7.0)
project(vgraphplay)

message(STATUS "Configuring for build type: " ${CMAKE_BUILD_TYPE})

if(CMAKE_COMPILER_IS_GNUCXX)
  if(CMAKE_CXX_COMPILER_VERSION STRLESS "4.8")
    set(CMAKE_CXX_FLAGS "-std=gnu++11 -Wall")
  else()
    set(CMAKE_CXX_FLAGS "-std=c++11 -Wall")
  endif(CMAKE_CXX_COMPILER_VERSION STRLESS "4.8")

  set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -pg")

  if(NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")
  endif(NOT APPLE)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "-std=c++11 -Wall")
endif(CMAKE_COMPILER_IS_GNUCXX)

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
