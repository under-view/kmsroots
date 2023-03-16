#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "gltf-loader.h"


struct uvr_gltf_loader_file uvr_gltf_loader_file_load(struct uvr_gltf_loader_file_load_info *uvrgltf)
{
  cgltf_result res = cgltf_result_max_enum;
  cgltf_data *gltfData = NULL;

  cgltf_options options;
  memset(&options, 0, sizeof(cgltf_options));

  res = cgltf_parse_file(&options, uvrgltf->fileName, &gltfData);
  if (res != cgltf_result_success) {
    uvr_utils_log(UVR_DANGER, "[x] cgltf_parse_file: Could not load %s", uvrgltf->fileName);
    goto exit_error_uvr_gltf_loader_file_load;
  }

  res = cgltf_load_buffers(&options, gltfData, uvrgltf->fileName);
  if (res != cgltf_result_success) {
    uvr_utils_log(UVR_DANGER, "[x] cgltf_load_buffers: Could not load buffers in %s", uvrgltf->fileName);
    goto exit_error_uvr_gltf_loader_file_load_free_gltfData;
  }

  res = cgltf_validate(gltfData);
  if (res != cgltf_result_success) {
    uvr_utils_log(UVR_DANGER, "[x] cgltf_validate: Failed to load content in %s", uvrgltf->fileName);
    goto exit_error_uvr_gltf_loader_file_load_free_gltfData;
  }

  return (struct uvr_gltf_loader_file) { .gltfData = gltfData };

exit_error_uvr_gltf_loader_file_load_free_gltfData:
  if (gltfData)
    cgltf_free(gltfData);
exit_error_uvr_gltf_loader_file_load:
  return (struct uvr_gltf_loader_file) { .gltfData = NULL };
}


struct uvr_gltf_loader_vertices uvr_gltf_loader_vertices_get_buffers(struct uvr_gltf_loader_file *uvrgltf)
{
  cgltf_size i, j, verticesDataCount = 0;
  cgltf_buffer_view *bufferView = NULL;
  cgltf_data *gltfData = uvrgltf->gltfData;

  struct uvr_gltf_loader_vertices_data *verticesData = NULL;

  for (i = 0; i < gltfData->meshes_count; i++)
    for (j = 0; j < gltfData->meshes[i].primitives_count; j++)
      verticesDataCount++;

  verticesData = calloc(verticesDataCount, sizeof(struct uvr_gltf_loader_vertices_data));
  if (!verticesData) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_error_uvr_gltf_loader_vertices_get_buffers;
  }

  verticesDataCount=0;
  for (i = 0; i < gltfData->meshes_count; i++) {
    for (j = 0; j < gltfData->meshes[i].primitives_count; j++) {
      if (!gltfData->meshes[i].primitives[j].indices) continue;
      bufferView = gltfData->meshes[i].primitives[j].indices->buffer_view;
      // verticesData[verticesDataCount].indices = gltfData->meshes[i].primitives[j].indices_index;
      verticesData[verticesDataCount].byteOffset = bufferView->offset;
      verticesData[verticesDataCount].bufferSize = bufferView->size;
      verticesDataCount++;
    }
  }

  return (struct uvr_gltf_loader_vertices) { .verticesData = verticesData, .verticesDataCount = verticesDataCount };

exit_error_uvr_gltf_loader_vertices_get_buffers:
  return (struct uvr_gltf_loader_vertices) { .verticesData = NULL, .verticesDataCount = 0 };
}


void uvr_gltf_loader_destroy(struct uvr_gltf_loader_destroy *uvrgltf)
{
  uint32_t i;
  for (i = 0; i < uvrgltf->uvr_gltf_loader_file_cnt; i++) {
    if (uvrgltf->uvr_gltf_loader_file[i].gltfData)
      cgltf_free(uvrgltf->uvr_gltf_loader_file[i].gltfData);
  }
  for (i = 0; i < uvrgltf->uvr_gltf_loader_vertices_cnt; i++) {
    if (uvrgltf->uvr_gltf_loader_vertices[i].verticesData)
      free(uvrgltf->uvr_gltf_loader_vertices[i].verticesData);
  }
}
