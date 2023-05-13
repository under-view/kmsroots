#include <stdio.h>
#include <string.h>
#include "shader.h"

#define VK_SHADER_STAGE_VERTEX_BIT 0x00000001
#define VK_SHADER_STAGE_FRAGMENT_BIT 0x00000010

int main(void)
{
	int ret = 0;

	struct kmr_shader_destroy shaderd;
	memset(&shaderd, 0, sizeof(shaderd));

	const char vertexShader[] =
		"#version 450\n"  // GLSL 4.5
		"#extension GL_ARB_separate_shader_objects : enable\n"
		"layout(location = 0) out vec3 outColor;\n\n"
		"layout(location = 0) in vec2 inPosition;\n"
		"layout(location = 1) in vec3 inColor;\n"
		"void main() {\n"
		"   gl_Position = vec4(inPosition, 0.0, 1.0);\n"
		"   outColor = inColor;\n"
		"}";

	const char fragmentShader[] =
		"#version 450\n"
		"#extension GL_ARB_separate_shader_objects : enable\n"
		"layout(location = 0) in vec3 inColor;\n"
		"layout(location = 0) out vec4 outColor;\n"
		"void main() { outColor = vec4(inColor, 1.0); }";

	struct kmr_shader_spirv_create_info vertexShaderCreateInfo;
	vertexShaderCreateInfo.kind = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderCreateInfo.source = vertexShader;
	vertexShaderCreateInfo.filename = "vert.spv";
	vertexShaderCreateInfo.entryPoint = "main";

	struct kmr_shader_spirv_create_info fragmentShaderCreateInfo;
	fragmentShaderCreateInfo.kind = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.source = fragmentShader;
	fragmentShaderCreateInfo.filename = "frag.spv";
	fragmentShaderCreateInfo.entryPoint = "main";

	/*
	 * 0. Vertex Shader
	 * 1. Fragment Shader
	 */
	struct kmr_shader_spirv kmr_shader[2];

	kmr_utils_log(KMR_WARNING, "LOADING VERTEX SHADER");
	fprintf(stdout, "%s\n\n", vertexShader);

	kmr_shader[0] = kmr_shader_compile_buffer_to_spirv(&vertexShaderCreateInfo);
	if (!kmr_shader[0].bytes) { ret = 1 ; goto exit_distroy_shader ; }

	kmr_utils_log(KMR_WARNING, "LOADING FRAGMENT SHADER");
	fprintf(stdout, "%s\n", fragmentShader);

	kmr_shader[1] = kmr_shader_compile_buffer_to_spirv(&fragmentShaderCreateInfo);
	if (!kmr_shader[1].bytes) { ret = 1 ; goto exit_distroy_shader ; }

exit_distroy_shader:
	shaderd.kmr_shader_spirv_cnt = ARRAY_LEN(kmr_shader);
	shaderd.kmr_shader_spirv = kmr_shader;
	kmr_shader_destroy(&shaderd);

	return ret;
}
