#ifndef UVR_SHADER_H
#define UVR_SHADER_H

#include "common.h"
#include "utils.h"

/*
 * struct uvr_shader_file (Underview Renderer Shader File)
 *
 * members:
 * @bytes - Buffer that stores a given file's content
 * @bsize - Size of buffer storing a given file's content
 */
struct uvr_shader_file {
  char *bytes;
  long bsize;
};


/*
 * uvr_shader_file_load: Takes in a file and loads contents in to a buffer. This function is largely
 *                       only used to load shader files.
 *
 * args:
 * @filename - Must pass file to load
 * return:
 *    on success struct uvr_shader_file
 *    on failure struct uvr_shader_file { with member nulled }
 */
struct uvr_shader_file uvr_shader_file_load(const char *filename);


#endif
