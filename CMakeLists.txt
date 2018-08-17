cmake_minimum_required(VERSION 3.11.0)
project(vgraphplay)

if(WIN32)
  if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};../deps/boost-msys;../deps/glfw-msys;../deps/glm")
  elseif(MSVC AND "${CMAKE_SIZEOF_VOID_P}" STREQUAL 8)
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};../deps/boost-msvc-x64;../deps/glfw-msvc-x64;../deps/glm")
  elseif(MSVC)
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH};../deps/boost-msvc;../deps/glfw-msvc;../deps/glm")
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

add_subdirectory(vendor/embed-resource)

compile_spirv(SPIRV_SHADERS
  shaders/unlit.frag
  shaders/unlit.vert)

embed_resources(EMBEDDED_SHADERS ${SPIRV_SHADERS})

add_executable(vgraphplay
  vgraphplay/Application.cpp
  # vgraphplay/gfx/Commands.cpp
  # vgraphplay/gfx/Device.cpp
  # vgraphplay/gfx/Pipeline.cpp
  # vgraphplay/gfx/Presentation.cpp
  vgraphplay/gfx/System.cpp
  vgraphplay/gfx/VulkanExt.cpp
  vgraphplay/gfx/VulkanOutput.cpp
  vgraphplay/vgraphplay.cpp
  ${EMBEDDED_SHADERS})
target_compile_features(vgraphplay PUBLIC cxx_std_11)
target_include_directories(vgraphplay PUBLIC vendor/embed-resource)
target_link_libraries(vgraphplay
  Boost::log_setup
  Boost::log
  Boost::filesystem
  Vulkan::Vulkan
  glfw)

if(CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(vgraphplay PUBLIC "-Wall" "-Og" "-pg" "-ggdb")
  set_target_properties(vgraphplay PROPERTIES LINK_FLAGS "-pg")
endif()
