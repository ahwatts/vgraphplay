cmake_minimum_required(VERSION 3.7.0)
project(vgraphplay)
set(CMAKE_CXX_STANDARD 11)

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

find_package(Threads REQUIRED)

set(Boost_USE_STATIC_LIBS "On")
find_package(Boost REQUIRED COMPONENTS filesystem log)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

include_directories(AFTER SYSTEM
  Boost::boost
  Vulkan::Vulkan
  glfw
  glm)

add_subdirectory(vgraphplay)
add_subdirectory(shaders)
