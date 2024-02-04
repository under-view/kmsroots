#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <shaderc/shaderc.h>

#include "shader.h"


/********************************************************
 * START OF kmr_shader_spirv_{create,destroy} FUNCTIONS *
 ********************************************************/

/*
 * Allows one to specify what type of shader they want to create SPIR-V bytes from
 * key: VkShaderStageFlagBits, value: shaderc_shader_kind
 */
static const unsigned int shader_map_table[] = {
	[0x00000000] = shaderc_glsl_infer_from_source,
	[0x00000001] = shaderc_glsl_vertex_shader,
	[0x00000002] = shaderc_glsl_tess_control_shader,
	[0x00000004] = shaderc_glsl_tess_evaluation_shader,
	[0x00000008] = shaderc_glsl_geometry_shader,
	[0x00000010] = shaderc_glsl_fragment_shader,
	[0x00000020] = shaderc_glsl_compute_shader,
};


struct kmr_shader_spirv *
kmr_shader_spirv_create (struct kmr_shader_spirv_create_info *spirvInfo)
{
	unsigned int ret = 0;

	shaderc_compiler_t compiler = NULL;
	shaderc_compile_options_t options = NULL;

	struct kmr_shader_spirv *spirv = NULL;

	if (!spirvInfo->source) {
		kmr_utils_log(KMR_DANGER, "[x] spirvInfo->source: Must pass character buffer with shader code");
		goto exit_error_kmr_shader_spirv_create;
	}

	spirv = calloc(1, sizeof(struct kmr_shader_spirv));
	if (!spirv) {
		kmr_utils_log(KMR_DANGER, "[x] spirvInfo->source: Must pass character buffer with shader code");
		goto exit_error_kmr_shader_spirv_create;
	}

	compiler = shaderc_compiler_initialize();
	if (!compiler) {
		kmr_utils_log(KMR_DANGER, "[x] shaderc_compiler_initialize: Failed initialize shaderc_compiler_t");
		goto exit_error_kmr_shader_spirv_create;
	}

	options = shaderc_compile_options_initialize();
	if (!options) {
		kmr_utils_log(KMR_DANGER, "[x] shaderc_compiler_initialize: Failed initialize shaderc_compile_options_t");
		goto exit_error_kmr_shader_spirv_create;
	}

	shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_size);

	spirv->result = (void *) shaderc_compile_into_spv(compiler, spirvInfo->source, strlen(spirvInfo->source),
	                                                  shader_map_table[spirvInfo->kind], spirvInfo->filename,
	                                                  spirvInfo->entryPoint, options);

	if (!spirv->result) {
		kmr_utils_log(KMR_DANGER, "[x] shaderc_compile_into_spv: %s", shaderc_result_get_error_message(spirv->result));
		goto exit_error_kmr_shader_spirv_create;
	}

	ret = shaderc_result_get_compilation_status(spirv->result);
	if (ret != shaderc_compilation_status_success) {
		kmr_utils_log(KMR_DANGER, "[x] shaderc_result_get_compilation_status: %s", shaderc_result_get_error_message(spirv->result));
		goto exit_error_kmr_shader_spirv_create;
	}

	spirv->byteSize = shaderc_result_get_length(spirv->result);
	spirv->bytes = (const unsigned char *) shaderc_result_get_bytes(spirv->result);

	// Have to free results later
	shaderc_compile_options_release(options);
	shaderc_compiler_release(compiler);

	return spirv;

exit_error_kmr_shader_spirv_create:
	if (options)
		shaderc_compile_options_release(options);
	if (compiler)
		shaderc_compiler_release(compiler);
	kmr_shader_spirv_destroy(spirv);
	return NULL;
}


void
kmr_shader_spirv_destroy (struct kmr_shader_spirv *spirv)
{
	if (!spirv)
		return;

	if (spirv->result)
		shaderc_result_release(spirv->result);

	free(spirv);
}

/******************************************************
 * END OF kmr_shader_spirv_{create,destroy} FUNCTIONS *
 ******************************************************/
