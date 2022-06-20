#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 v_Color;
layout(location = 0) out vec4 o_Color;

void main() { o_Color = vec4(v_Color, 1.0); }
