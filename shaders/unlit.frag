// -*- mode: glsl; c-basic-offset: 4; -*-

#version 450

layout (location=0) in vec4 v_color;

layout (location=0) out vec4 FragColor;

void main(void) {
    FragColor = v_color;
}
