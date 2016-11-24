// -*- mode: glsl; c-basic-offset: 4; -*-

#version 450 core

#define MAX_LIGHTS 10
struct Light {
    bool enabled;
    vec3 position;
    vec4 color;
    uint specular_exp;
};

layout (location=0) in vec3 position;
layout (location=1) in vec4 color;

layout (set=0, binding=0) uniform uniforms {
    mat4x4 model;
    mat3x3 model_inv_trans_3;
    mat4x4 view;
    mat4x4 view_inv;
    mat4x4 projection;
    Light lights[MAX_LIGHTS];
};

layout (location=0) out vec4 v_color;

void main(void) {
    gl_Position = projection * view * model * vec4(position, 1.0);
    v_color = color;
}
