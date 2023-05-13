#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "shader.h"

int main(void)
{
	int ret = 0;

	struct kmr_shader_destroy shaderd;
	memset(&shaderd, 0, sizeof(shaderd));

	/*
	 * 0. Vertex Shader
	 * 1. Fragment Shader
	 */
	struct kmr_utils_file kmr_shader[2];

	kmr_utils_log(KMR_WARNING, "LOADING VERTEX SHADER");

	kmr_shader[0] = kmr_utils_file_load(VERTEX_SHADER_SPIRV);
	if (!kmr_shader[0].bytes) { ret = 1 ; goto exit_distroy_shader ; }

	char *shader_code = malloc(kmr_shader[0].byteSize + 1);
	memcpy(shader_code, kmr_shader[0].bytes, kmr_shader[0].byteSize);
	shader_code[kmr_shader[0].byteSize - 1] = '\0';

	fprintf(stdout, "%s\n", shader_code);
	free(shader_code);

	kmr_utils_log(KMR_WARNING, "LOADING FRAGMENT SHADER");

	kmr_shader[1] = kmr_utils_file_load(FRAGMENT_SHADER_SPIRV);
	if (!kmr_shader[1].bytes) { ret = 1 ; goto exit_distroy_shader ; }

exit_distroy_shader:
	free(kmr_shader[0].bytes);
	free(kmr_shader[1].bytes);

	return ret;
}
