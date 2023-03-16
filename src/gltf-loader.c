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


struct uvr_gltf_loader_indices uvr_gltf_loader_indices_get_vertex_index_buffers(struct uvr_gltf_loader_file *uvrgltf)
{
  cgltf_size i, j, indicesInfoCount = 0;
  cgltf_buffer_view *bufferView = NULL;
  cgltf_data *gltfData = uvrgltf->gltfData;

  struct uvr_gltf_loader_indices_info *indicesInfo = NULL;

  for (i = 0; i < gltfData->meshes_count; i++)
    for (j = 0; j < gltfData->meshes[i].primitives_count; j++)
      indicesInfoCount++;

  indicesInfo = calloc(indicesInfoCount, sizeof(struct uvr_gltf_loader_indices_info));
  if (!indicesInfo) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_error_uvr_gltf_loader_buffer_view_indices_acquire;
  }

  indicesInfoCount=0;
  for (i = 0; i < gltfData->meshes_count; i++) {
    for (j = 0; j < gltfData->meshes[i].primitives_count; j++) {
      if (!gltfData->meshes[i].primitives[j].indices) continue;
      bufferView = gltfData->meshes[i].primitives[j].indices->buffer_view;
      indicesInfo[indicesInfoCount].indices = gltfData->meshes[i].primitives[j].indices_index;
      indicesInfo[indicesInfoCount].byteOffset = bufferView->offset;
      indicesInfo[indicesInfoCount].bufferSize = bufferView->size;
      indicesInfoCount++;
    }
  }

  return (struct uvr_gltf_loader_indices) { .indicesInfo = indicesInfo, .indicesInfoCount = indicesInfoCount };

exit_error_uvr_gltf_loader_buffer_view_indices_acquire:
  return (struct uvr_gltf_loader_indices) { .indicesInfo = NULL, .indicesInfoCount = 0 };
}


void uvr_gltf_loader_destroy(struct uvr_gltf_loader_destroy *uvrgltf)
{
  uint32_t i;
  for (i = 0; i < uvrgltf->uvr_gltf_loader_file_cnt; i++) {
    if (uvrgltf->uvr_gltf_loader_file[i].gltfData)
      cgltf_free(uvrgltf->uvr_gltf_loader_file[i].gltfData);
  }
  for (i = 0; i < uvrgltf->uvr_gltf_loader_indices_cnt; i++) {
    if (uvrgltf->uvr_gltf_loader_indices[i].indicesInfo)
      free(uvrgltf->uvr_gltf_loader_indices[i].indicesInfo);
  }
}
