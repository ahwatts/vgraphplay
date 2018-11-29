#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTex;

layout(binding = 1) uniform sampler2D unifTexture;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(inColor * texture(unifTexture, inTex).rgb, 1.0);
}
