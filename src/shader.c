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
    uvr_utils_log(UVR_DANGER, "[x] fread: %s", strerror(errno));
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
