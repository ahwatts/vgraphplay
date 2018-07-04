cmake_minimum_required(VERSION 3.7.0)
project(vgraphplay)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_PREFIX_PATH ../graphplay-deps)

if(CMAKE_COMPILER_IS_GNUCXX)
  add_compile_options("-Wall")
endif()

find_package(Threads REQUIRED)

set(Boost_USE_STATIC_LIBS "On")
find_package(Boost REQUIRED COMPONENTS filesystem log)

find_package(Vulkan REQUIRED)

find_package(glfw3 QUIET)
if(${glfw3_FOUND})
  message(STATUS "Found GLFW: " ${glfw3_CONFIG})
else()
  find_path(__GLFW_INCLUDE_DIRS NAMES GL/glfw3.h GLFW/glfw3.h)

  if(MSVC)
    find_library(__GLFW_LIBRARY NAMES glfw3dll)
  else()
    find_library(__GLFW_LIBRARY NAMES glfw3 glfw glfw3dll glfwdll)
  endif(MSVC)
  set(__GLFW_LIBRARIES ${__GLFW_LIBRARY})

  add_library(glfw STATIC IMPORTED)
  set_property(TARGET glfw PROPERTY IMPORTED_LOCATION ${__GLFW_LIBRARY})
  target_include_directories(glfw INTERFACE ${__GLFW_INCLUDE_DIRS})
  target_link_libraries(glfw INTERFACE ${__GLFW_LIBRARIES})
  message(STATUS "Found GLFW: " ${__GLFW_LIBRARY})
  message(STATUS "Found GLFW include: " ${__GLFW_INCLUDE_DIRS})
  message(STATUS "Found GLFW libs: " ${__GLFW_LIBRARIES})
endif()

find_package(glm QUIET)
if(${glm_FOUND})
  message(STATUS "Found glm: " ${glm_CONFIG})
else()
  find_path(__GLM_INCLUDE_PATH glm/glm.hpp PATHS ${CMAKE_PREFIX_PATH}/include NO_DEFAULT_PATH)
  find_path(__GLM_INCLUDE_PATH glm/glm.hpp)

  add_library(glm STATIC IMPORTED)
  target_include_directories(glm INTERFACE ${__GLM_INCLUDE_PATH})
  message(STATUS "Found glm include: " ${__GLM_INCLUDE_PATH})
endif(${glm_FOUND})

add_subdirectory(vgraphplay)
add_subdirectory(shaders)
