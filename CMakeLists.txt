cmake_minimum_required(VERSION 3.31.0)
project(vgraphplay)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMake")

# Add option to enable/disable C++ 20 module
option(ENABLE_CPP20_MODULE "Enable C++ 20 module support for Vulkan" OFF)

# Enable C++ module dependency scanning only if C++ 20 module is enabled
if(ENABLE_CPP20_MODULE)
  set(CMAKE_CXX_SCAN_FOR_MODULES ON)
endif()

function(compile_spirv out_var)
  set(result)
  foreach(in_file ${ARGN})
    file(RELATIVE_PATH src_file ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${in_file})
    set(out_file ${PROJECT_BINARY_DIR}/${in_file}.spv)
    get_filename_component(out_dir ${out_file} DIRECTORY)
    file(MAKE_DIRECTORY ${out_dir})
    file(RELATIVE_PATH dst_file ${CMAKE_SOURCE_DIR} ${out_file})
    add_custom_command(
      OUTPUT ${out_file}
      COMMAND glslangValidator -V -o ${dst_file} ${src_file}
      DEPENDS ${in_file}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Building SPIR-V binary ${dst_file}"
      VERBATIM)
    list(APPEND result "${dst_file}")
  endforeach()
  set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

find_package(Boost REQUIRED COMPONENTS filesystem log)
find_package(Vulkan 1.4.335 REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# set up Vulkan C++ module only if enabled
if(ENABLE_CPP20_MODULE)
  add_library(VulkanCppModule)
  add_library(Vulkan::cppm ALIAS VulkanCppModule)

  target_compile_definitions(VulkanCppModule
    PUBLIC 
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1)
  target_include_directories(VulkanCppModule PUBLIC "${Vulkan_INCLUDE_DIR}")
  target_link_libraries(VulkanCppModule PUBLIC Vulkan::Vulkan)

  set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)

  # Add MSVC-specific compiler options for proper C++ module support
  if(MSVC)
    target_compile_options(VulkanCppModule PRIVATE
      /std:c++latest      # Use latest C++ standard for better module support
      /permissive-        # Standards conformance mode
      /Zc:__cplusplus     # Enable correct __cplusplus macro
      /EHsc               # Enable C++ exception handling
      /Zc:preprocessor    # Use conforming preprocessor
      /translateInclude)  # Automatically translate #include to import for standard library    
  endif()

  target_sources(VulkanCppModule
    PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES
    BASE_DIRS
    "${Vulkan_INCLUDE_DIR}"
    FILES
    "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    "${Vulkan_INCLUDE_DIR}/vulkan/vulkan_video.cppm")

  # Add the vulkan.cppm file directly as a source file
  target_sources(VulkanCppModule
    PRIVATE
    "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    "${Vulkan_INCLUDE_DIR}/vulkan/vulkan_video.cppm")
else()
  # Create a dummy interface library when C++ 20 module is disabled
  add_library(VulkanCppModule INTERFACE)
  add_library(Vulkan::cppm ALIAS VulkanCppModule)
  target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
  target_compile_definitions(VulkanCppModule
    INTERFACE 
    VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1)
endif()

# GLM changed their library link target in a way that we can't really detect.
if(TARGET glm::glm)
  set(glm_library "glm::glm")
elseif(TARGET glm)
  set(glm_library "glm")
else()
  message(FATAL_ERROR "No glm link target found")
endif()

add_subdirectory(vendor/embed-resource)
add_subdirectory(vendor/stb)

compile_spirv(SPIRV_SHADERS
  shaders/unlit.frag
  shaders/unlit.vert)

embed_resources(EMBEDDED_SHADERS ${SPIRV_SHADERS})
embed_resources(EMBEDDED_TEXTURES textures/warren.jpg)

add_executable(vgraphplay
  vgraphplay/vulkan.h
  vgraphplay/Application.h
  vgraphplay/Application.cpp
  vgraphplay/gfx/System.h
  vgraphplay/gfx/System.cpp
  vgraphplay/VulkanExt.cpp
  vgraphplay/VulkanOutput.h
  vgraphplay/VulkanOutput.cpp
  vgraphplay/vgraphplay.cpp
  ${EMBEDDED_SHADERS}
  ${EMBEDDED_TEXTURES})
target_compile_features(vgraphplay PUBLIC cxx_std_14)
target_include_directories(vgraphplay PUBLIC vendor/embed-resource)
target_link_libraries(vgraphplay
  Boost::log
  Boost::filesystem
  Vulkan::cppm
  glfw
  ${glm_library}
  stb)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
  target_link_libraries(vgraphplay rt)
endif()

# if(CMAKE_COMPILER_IS_GNUCXX)
#   target_compile_options(vgraphplay PUBLIC "-Wall" "-Og" "-pg" "-ggdb")
#   set_target_properties(vgraphplay PROPERTIES LINK_FLAGS "-pg")
# endif()
