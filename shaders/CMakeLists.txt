set(SHADER_SOURCES
  unlit.vert
  unlit.frag)

set(SHADER_OUTPUTS
  unlit.vert.spv
  unlit.frag.spv)

add_custom_target(spirv_shaders ALL
  DEPENDS ${SHADER_OUTPUTS})

foreach(SHADER_SOURCE IN LISTS SHADER_SOURCES)
  add_custom_command(
    OUTPUT "${SHADER_SOURCE}.spv"
    COMMAND glslangValidator -V -o "${SHADER_SOURCE}.spv" "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_SOURCE}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_SOURCE}")
endforeach(SHADER_SOURCE)
