#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTex;

layout(set = 0, binding = 0) uniform Transformations {
    mat4x4 model;
    mat4x4 view;
    mat4x4 projection;
} xforms;

out gl_PerVertex {
    vec4 gl_Position;
};

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outTex;

void main() {
    gl_Position = xforms.projection * xforms.view * xforms.model * vec4(inPosition, 1.0);
    outColor = inColor;
    outTex = inTex;
}
