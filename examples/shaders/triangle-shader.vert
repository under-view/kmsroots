#version 450  // GLSL 4.5
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec3 outColor;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;

void main() {
	gl_Position = vec4(inPosition, 0.0, 1.0);
	outColor = inColor;
}
