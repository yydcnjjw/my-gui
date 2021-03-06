#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(set = 0, binding = 1) uniform UniformBufferPerObject {
    mat4 model;
} per_ubo;

layout(location = 0) in vec3 pos;
layout(location = 1) in vec2 tex_coord;

layout(location = 0) out vec2 frag_tex_coord;
layout(location = 1) out vec3 frag_color;

void main() {
    gl_Position =
        ubo.proj *
        ubo.view *
        per_ubo.model *
        vec4(pos, 1.0);
    frag_tex_coord = tex_coord;
    frag_color = pos;
}

// #version 450
// #extension GL_ARB_separate_shader_objects : enable

// layout(location = 0) in vec2 inPosition;
// layout(location = 1) in vec3 inColor;

// layout(location = 0) out vec3 fragColor;

// void main() {
//     gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 0.0, 1.0);
//     fragColor = inColor;
// }

// #version 450
// #extension GL_ARB_separate_shader_objects : enable

// layout(binding = 0) uniform UniformBufferObject {
//     mat4 model;
//     mat4 view;
//     mat4 proj;
// } ubo;

// layout(location = 0) in vec3 inPosition;
// layout(location = 1) in vec3 inColor;
// layout(location = 2) in vec2 inTexCoord;

// layout(location = 0) out vec3 fragColor;
// layout(location = 1) out vec2 fragTexCoord;

// void main() {
//     gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
//     fragColor = inColor;
//     fragTexCoord = inTexCoord;
// }


// #version 450
// #extension GL_ARB_separate_shader_objects : enable

// layout(binding = 0) uniform UniformBufferObject {
//     mat4 model;
//     mat4 view;
//     mat4 proj;
// } ubo;

// layout(location = 0) in vec3 inPosition;
// layout(location = 1) in vec3 inColor;
// layout(location = 2) in vec2 inTexCoord;
// layout(location = 3) in vec3 inInstacePos;

// layout(location = 0) out vec3 fragColor;
// layout(location = 1) out vec2 fragTexCoord;

// void main() {
//     vec3 _pos = inPosition + inInstacePos;
//     vec4 pos = vec4(_pos, 1.0);
//     gl_Position = ubo.proj * ubo.view * ubo.model * pos;
//     fragColor = inColor;
//     fragTexCoord = inTexCoord;
//     // vec3 _pos = inPosition + inInstacePos;
//     // vec4 pos = vec4(_pos, 1.0);
//     // // gl_Position = ubo.proj * ubo.view * ubo.model * pos;
//     // gl_Position = vec4(inPosition.x, inPosition.y, 0.0, 1.0);
//     // fragColor = inColor;
//     // fragTexCoord = inTexCoord;
// }
