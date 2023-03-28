#version 450  // GLSL 4.5
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec3 outColor;
layout(location = 1) out vec2 outTexCoord;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(set = 0, binding = 0) uniform uniform_buffer_scene {
	mat4 view;
	mat4 projection;
} uboScene;

layout(set = 0, binding = 1) uniform uniform_buffer_scene_model {
	mat4 model;
} uboModel;

// Won't be included when shader is compiled as it's not in use
layout(push_constant) uniform uniform_push_model {
	mat4 model;
} pushModel;

void main() {
	gl_Position = uboScene.projection * uboScene.view * uboModel.model * vec4(inPosition, 1.0);
	outColor = inColor;
	outTexCoord = inTexCoord;
}
