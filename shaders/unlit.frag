// -*- mode: glsl; c-basic-offset: 4; -*-

#version 450
#extension GL_ARB_separate_shader_objects: enable

layout (location=0) in vec3 vertexColor;
layout (location=0) out vec4 fragmentColor;

void main(void) {
    fragmentColor = vec4(vertexColor, 1.0);
    // FragColor = v_color;
}
