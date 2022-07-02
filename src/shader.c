#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "shader.h"


struct uvr_shader_file uvr_shader_file_load(const char *filename) {
  FILE *stream = NULL;
  char *bytes = NULL;
  long bsize = 0;

  /* Open the file in binary mode */
  stream = fopen(filename, "rb");
  if (!stream) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_shader_file_load(fopen[%s]): %s", filename, strerror(errno));
    goto exit_shader_file_load;
  }

  /* Go to the end of the file */
  bsize = fseek(stream, 0, SEEK_END);
  if (bsize == -1) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_shader_file_load(fseek): %s", strerror(errno));
    goto exit_shader_file_load_fclose;
  }

  /*
   * Get the current byte offset in the file.
   * Used to read current position. Thus returns
   * a number equal to the size of the buffer we
   * need to allocate
   */
  bsize = ftell(stream);
  if (bsize == -1) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_shader_file_load(ftell): %s", strerror(errno));
    goto exit_shader_file_load_fclose;
  }

  /* Jump back to the beginning of the file */
  rewind(stream);

  bytes = (char *) calloc(bsize, sizeof(char));
  if (!bytes) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_shader_file_load(calloc): %s", strerror(errno));
    goto exit_shader_file_load_fclose;
  }

  /* Read in the entire file */
  if (fread(bytes, bsize, 1, stream) == 0) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_shader_file_load(fread): %s", strerror(errno));
    goto exit_shader_file_load_free_bytes;
  }

  fclose(stream);

  return (struct uvr_shader_file) { .bytes = bytes, .bsize = bsize };

exit_shader_file_load_free_bytes:
  free(bytes);
exit_shader_file_load_fclose:
  fclose(stream);
exit_shader_file_load:
  return (struct uvr_shader_file) { .bytes = NULL, .bsize = 0 };
}


#ifdef INCLUDE_SHADERC

/*
 * Allows one to specify what type of shader they with to create SPIR-V bytes from
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


/* Compiles a shader to a SPIR-V binary */
struct uvr_shader_spirv uvr_shader_compile_buffer_to_spirv(struct uvr_shader_spirv_create_info *uvrshader) {
  char *bytes = NULL;
  long bsize = 0;

  shaderc_compiler_t compiler = NULL;
  shaderc_compile_options_t options = NULL;
  shaderc_compilation_result_t result = NULL;

  if (!uvrshader->source) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_shader_compile_bytes_to_spirv(uvrshader->source): Must pass character buffer with shader code");
    goto exit_shader_compile_bytes_to_spirv;
  }

  compiler = shaderc_compiler_initialize();
  if (!compiler) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_shader_compile_bytes_to_spirv(shaderc_compiler_initialize): Failed initialize shaderc_compiler_t");
    goto exit_shader_compile_bytes_to_spirv;
  }

  options = shaderc_compile_options_initialize();
  if (!options) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_shader_compile_bytes_to_spirv(shaderc_compiler_initialize): Failed initialize shaderc_compile_options_t");
    goto exit_shader_compile_bytes_to_spirv_compiler_release;
  }

  shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level_size);

  result = shaderc_compile_into_spv(compiler, uvrshader->source, strlen(uvrshader->source),
                                    shader_map_table[uvrshader->kind], uvrshader->filename,
                                    uvrshader->entry_point, options);

  if (!result) {
    uvr_utils_log(UVR_DANGER, "[x] %s", shaderc_result_get_error_message(result));
    goto exit_shader_compile_bytes_to_spirv_release_result;
  }

  if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
    uvr_utils_log(UVR_DANGER, "[x] %s", shaderc_result_get_error_message(result));
    goto exit_shader_compile_bytes_to_spirv_release_result;
  }

  bsize = shaderc_result_get_length(result);
  bytes = (char *) shaderc_result_get_bytes(result);

  // Have to free results later
  shaderc_compile_options_release(options);
  shaderc_compiler_release(compiler);

  return (struct uvr_shader_spirv) { .result = result, .bytes = bytes, .bsize = bsize };

exit_shader_compile_bytes_to_spirv_release_result:
  if (result)
    shaderc_result_release(result);
  if (options)
    shaderc_compile_options_release(options);
exit_shader_compile_bytes_to_spirv_compiler_release:
  if (compiler)
    shaderc_compiler_release(compiler);
exit_shader_compile_bytes_to_spirv:
  return (struct uvr_shader_spirv) { .result = NULL, .bytes = NULL, .bsize = 0 };
}
#endif


void uvr_shader_destroy(struct uvr_shader_destroy *uvrshader) {
  if (uvrshader->uvr_shader_file.bytes)
    free(uvrshader->uvr_shader_file.bytes);
#ifdef INCLUDE_SHADERC
  if (uvrshader->uvr_shader_spirv.result)
    shaderc_result_release(uvrshader->uvr_shader_spirv.result);
  uvrshader->uvr_shader_spirv.result = NULL;
#endif
  uvrshader->uvr_shader_file.bytes = NULL;
}
