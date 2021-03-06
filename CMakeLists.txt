cmake_minimum_required(VERSION 3.11.0)
project(vgraphplay)

if(WIN32)
  if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};../deps/boost-mingw;../deps/glfw-mingw;../deps/glm/cmake/glm")
  elseif(MSVC AND "${CMAKE_SIZEOF_VOID_P}" STREQUAL 8)
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};../deps/boost-msvc;../deps/glfw-msvc-x64;../deps/glm/cmake/glm")
  elseif(MSVC)
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};../deps/boost-msvc;../deps/glfw-msvc-x64;../deps/glm/cmake/glm")
  endif()
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

set(Boost_USE_STATIC_LIBS "On")
find_package(Boost REQUIRED COMPONENTS filesystem log)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

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
  vgraphplay/Application.cpp
  vgraphplay/gfx/System.cpp
  vgraphplay/VulkanExt.cpp
  vgraphplay/VulkanOutput.cpp
  vgraphplay/vgraphplay.cpp
  ${EMBEDDED_SHADERS}
  ${EMBEDDED_TEXTURES})
target_compile_features(vgraphplay PUBLIC cxx_std_11)
target_include_directories(vgraphplay PUBLIC vendor/embed-resource)
target_link_libraries(vgraphplay
  Boost::log
  Boost::filesystem
  Vulkan::Vulkan
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
