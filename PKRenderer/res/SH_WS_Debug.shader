#version 450

#Cull Back
#ZTest LEqual
#ZWrite True

#pragma PROGRAM_VERTEX

layout(set = 0, binding = 0) uniform UniformBufferObject
{
    mat4 model;
    mat4 viewproj;
} ubo;

layout(push_constant) uniform offset{ vec4 offset_x; };

layout(location = 0) in vec3 in_POSITION;
layout(location = 1) in vec2 in_TEXCOORD0;
layout(location = 2) in vec3 in_COLOR;

layout(location = 0) out vec3 vs_COLOR;
layout(location = 1) out vec2 vs_TEXCOORD0;

void main()
{
    gl_Position = ubo.viewproj * ubo.model * vec4(in_POSITION + offset_x.xyz, 1.0);
    vs_COLOR = in_COLOR;
    vs_TEXCOORD0 = in_TEXCOORD0;
}

#pragma PROGRAM_FRAGMENT

layout(location = 0) in vec3 vs_COLOR;
layout(location = 1) in vec2 vs_TEXCOORD0;
layout(set = 0, binding = 1) uniform sampler2D tex1;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(texture(tex1, vs_TEXCOORD0).xyz * vs_COLOR, 1.0);
}