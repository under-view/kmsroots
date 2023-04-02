#version 450  // GLSL 4.5
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 2) uniform sampler2D outTexSampler;

void main() {
	outColor = vec4(texture(outTexSampler, inTexCoord).rgb, 1.0);
}
