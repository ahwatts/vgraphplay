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

if(NOT WIN32)
  add_definitions(-DBOOST_LOG_DYN_LINK)
endif(NOT WIN32)

find_package(Vulkan REQUIRED)

find_package(glfw3 QUIET)
if(${glfw3_FOUND})
  message(STATUS "Found glfw: " ${glfw3_CONFIG})
else()
  find_path(GLFW_INCLUDE_DIRS NAMES GL/glfw3.h GLFW/glfw3.h)
  find_file(GLFW_FULL_HEADER_PATH NAMES GL/glfw3.h GLFW/glfw3.h)
  if(MSVC)
    find_library(GLFW_LIBRARY NAMES glfw3dll)
  else()
    find_library(GLFW_LIBRARY NAMES glfw3 glfw glfw3dll glfwdll)
  endif(MSVC)
  set(GLFW_LIBRARIES ${GLFW_LIBRARY})
  message(STATUS "Found GLFW include: " ${GLFW_INCLUDE_DIRS})
  message(STATUS "Found GLFW libs: " ${GLFW_LIBRARIES})

  add_library(glfw STATIC IMPORTED)
  set_property(TARGET glfw PROPERTY IMPORTED_LOCATION ${GLFW_LIBRARY})
  set_property(TARGET glfw PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${GLFW_INCLUDE_DIRS})
  set_property(TARGET glfw PROPERTY INTERFACE_LINK_LIBRARIES ${GLFW_LIBRARIES})
endif(${glfw3_FOUND})

find_package(glm QUIET)
if(${glm_FOUND})
  message(STATUS "Found glm: " ${glm_CONFIG})
else()
  find_path(GLM_INCLUDE_PATH glm/glm.hpp PATHS ${CMAKE_PREFIX_PATH}/include NO_DEFAULT_PATH)
  find_path(GLM_INCLUDE_PATH glm/glm.hpp)
  set(GLM_INCLUDE_DIRS ${GLM_INCLUDE_PATH})
  message(STATUS "Found glm include: " ${GLM_INCLUDE_DIRS})

  add_library(glm STATIC IMPORTED)
  set_property(TARGET glm PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${GLM_INCLUDE_DIRS})
endif(${glm_FOUND})

include_directories(AFTER SYSTEM
  ${Boost_INCLUDE_DIRS}
  ${Vulkan_INCLUDE_DIRS}
  glfw
  glm)

add_subdirectory(vgraphplay)
add_subdirectory(shaders)
