#version 450  // GLSL 4.5
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outTexCoord;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;
layout (location = 3) in vec3 inColor;

layout (set = 0, binding = 0) uniform uniform_buffer_scene {
	mat4 projection;
	mat4 view;
	vec4 lightPosition;
	vec4 viewPosition;
} uboScene;

// Used in conjunction with dynamic uniform buffer
layout(set = 0, binding = 1) uniform uniform_buffer_scene_model {
	mat4 model;
} uboModel;

// Won't be included when shader is compiled as it's not in use
layout(push_constant) uniform uniform_push_model {
	mat4 model;
} pushModel;

void main() {
	outNormal = inNormal;
	outColor = inColor;
	outTexCoord = inTexCoord;
	gl_Position = uboScene.projection * uboScene.view * uboModel.model * vec4(inPosition.xyz, 1.0);

	vec4 pos = uboScene.view * vec4(inPosition, 1.0);
	outNormal = mat3(uboScene.view) * inNormal;
	outLightVec = uboScene.lightPosition.xyz - pos.xyz;
	outViewVec = uboScene.viewPosition.xyz - pos.xyz;
}
