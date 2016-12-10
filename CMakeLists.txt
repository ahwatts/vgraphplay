cmake_minimum_required(VERSION 3.7.0)
project(vgraphplay)
set(CMAKE_CXX_STANDARD 11)

message(STATUS "Configuring for build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_PREFIX_PATH ../graphplay-deps)

if(CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "-Wall")
  set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -pg")

  if(NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")
  endif(NOT APPLE)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "-Wall")
endif(CMAKE_COMPILER_IS_GNUCXX)

find_package(Boost REQUIRED COMPONENTS filesystem log)
message(STATUS "Found Boost version: " ${Boost_VERSION})
message(STATUS "Found Boost headers: " ${Boost_INCLUDE_DIRS})
message(STATUS "Found Boost libs:    " ${Boost_LIBRARIES})

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED CONFIG)
message(STATUS "Found GLFW: " ${glfw3_DIR})
find_package(glm REQUIRED)
message(STATUS "Found GLM: " ${glm_DIR})

include_directories(AFTER SYSTEM
  ${Boost_INCLUDE_DIRS}
  ${Vulkan_INCLUDE_DIRS}
  glfw
  glm)

add_subdirectory(vgraphplay)
add_subdirectory(shaders)
