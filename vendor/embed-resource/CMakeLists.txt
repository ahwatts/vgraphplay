# cmake_minimum_required(VERSION 3.5)
# project(EmbedResource)

function(embed_resources out_var)
  set(result)
  foreach(in_f ${ARGN})
    file(RELATIVE_PATH src_f ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/${in_f})
    set(out_f "${PROJECT_BINARY_DIR}/${in_f}.cpp")
    add_custom_command(OUTPUT ${out_f}
      COMMAND embed-resource ${out_f} ${src_f}
      DEPENDS ${in_f}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Building binary file for embedding ${out_f}"
      VERBATIM)
    list(APPEND result "${out_f}")
  endforeach()
  set(${out_var} "${result}" PARENT_SCOPE)
endfunction()

add_executable(embed-resource embedresource.cpp)
target_compile_features(embed-resource PUBLIC cxx_std_14)
set_target_properties(embed-resource PROPERTIES CXX_VISIBILITY_PRESET hidden)
target_include_directories(embed-resource PUBLIC .)
target_link_libraries(embed-resource PUBLIC Boost::filesystem)
