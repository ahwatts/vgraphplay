// -*- mode: glsl; c-basic-offset: 4; -*-

#version 450 core
#extension GL_ARB_separate_shader_objects: enable

// #define MAX_LIGHTS 10
// struct Light {
//     bool enabled;
//     vec3 position;
//     vec4 color;
//     uint specular_exp;
// };

// layout (location=0) in vec3 position;
// layout (location=1) in vec4 color;

// layout (set=0, binding=0) uniform uniforms {
//     mat4x4 model;
//     mat3x3 model_inv_trans_3;
//     mat4x4 view;
//     mat4x4 view_inv;
//     mat4x4 projection;
//     Light lights[MAX_LIGHTS];
// };

// layout (location=0) out vec4 v_color;

out gl_PerVertex {
    vec4 gl_Position;
};

layout (location = 0) out vec3 vertexColor;

vec2 positions[3] = vec2[](vec2(0.0, -0.5), vec2(0.5, 0.5), vec2(-0.5, 0.5));
vec3 colors[3] = vec3[](vec3(1.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));

void main(void) {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    vertexColor = colors[gl_VertexIndex];
    // gl_Position = projection * view * model * vec4(position, 1.0);
    // v_color = color;
}
