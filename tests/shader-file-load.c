#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "shader.h"

int main(void)
{
  int ret = 0;

  struct uvr_shader_destroy shaderd;
  memset(&shaderd, 0, sizeof(shaderd));

  /*
   * 0. Vertex Shader
   * 1. Fragment Shader
   */
  struct uvr_utils_file uvr_shader[2];

  uvr_utils_log(UVR_WARNING, "LOADING VERTEX SHADER");

  uvr_shader[0] = uvr_utils_file_load(VERTEX_SHADER_SPIRV);
  if (!uvr_shader[0].bytes) { ret = 1 ; goto exit_distroy_shader ; }

  char *shader_code = malloc(uvr_shader[0].byteSize + 1);
  memcpy(shader_code, uvr_shader[0].bytes, uvr_shader[0].byteSize);
  shader_code[uvr_shader[0].byteSize - 1] = '\0';

  fprintf(stdout, "%s\n", shader_code);
  free(shader_code);

  uvr_utils_log(UVR_WARNING, "LOADING FRAGMENT SHADER");

  uvr_shader[1] = uvr_utils_file_load(FRAGMENT_SHADER_SPIRV);
  if (!uvr_shader[1].bytes) { ret = 1 ; goto exit_distroy_shader ; }

exit_distroy_shader:
  free(uvr_shader[0].bytes);
  free(uvr_shader[1].bytes);

  return ret;
}
