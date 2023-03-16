#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "gltf-loader.h"

#define COMPONENT_TYPE_UNSIGNED_SHORT 5123
#define COMPONENT_TYPE_FLOAT 5126

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
  cgltf_size i, j, k, verticesDataCount = 0, bufferViewElementType;
  cgltf_buffer_view *bufferView = NULL;
  cgltf_data *gltfData = uvrgltf->gltfData;

  struct uvr_gltf_loader_vertices_data *verticesData = NULL;

  verticesData = calloc(gltfData->buffer_views_count, sizeof(struct uvr_gltf_loader_vertices_data));
  if (!verticesData) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_error_uvr_gltf_loader_vertices_get_buffers;
  }

  // TODO: account for accessor bufferOffset
  // Normally would want to avoid, but in this case it's fine
  for (i = 0; i < gltfData->meshes_count; i++) {
    for (j = 0; j < gltfData->meshes[i].primitives_count; j++) {
      for (k = 0; k < gltfData->meshes[i].primitives[j].attributes_count; k++) {

        // bufferView associated with accessor which is associated with a mesh->primitive->attribute
        bufferView = gltfData->meshes[i].primitives[j].attributes[k].data->buffer_view;
        bufferViewElementType = gltfData->meshes[i].primitives[j].attributes[k].data->type;

        verticesData[verticesDataCount].byteOffset = bufferView->offset;
        verticesData[verticesDataCount].bufferSize = bufferView->size;
        verticesData[verticesDataCount].meshIndex = i;

        switch (gltfData->meshes[i].primitives[j].attributes[k].type) {
          case cgltf_attribute_type_texcoord:
            verticesData[verticesDataCount].bufferElementSize = cgltf_num_components(bufferViewElementType);
            verticesData[verticesDataCount].bufferType = UVR_GLTF_LOADER_VERTICES_TEXTURE;
            break;
          case cgltf_attribute_type_normal:
            verticesData[verticesDataCount].bufferElementSize = cgltf_num_components(bufferViewElementType);
            verticesData[verticesDataCount].bufferType = UVR_GLTF_LOADER_VERTICES_NORMAL;
            break;
          case cgltf_attribute_type_color:
            verticesData[verticesDataCount].bufferElementSize = cgltf_num_components(bufferViewElementType);
            verticesData[verticesDataCount].bufferType = UVR_GLTF_LOADER_VERTICES_COLOR;
            break;
          case cgltf_attribute_type_position:
            verticesData[verticesDataCount].bufferElementSize = cgltf_num_components(bufferViewElementType);
            verticesData[verticesDataCount].bufferType = UVR_GLTF_LOADER_VERTICES_POSITION;
            break;
          case cgltf_attribute_type_tangent:
            verticesData[verticesDataCount].bufferElementSize = cgltf_num_components(bufferViewElementType);
            verticesData[verticesDataCount].bufferType = UVR_GLTF_LOADER_VERTICES_TANGENT;
            break;
          default:
            continue;
        }

        verticesDataCount++;
      }

      if (!gltfData->meshes[i].primitives[j].indices)
        continue;

      // Store buffer index buffer data
      bufferView = gltfData->meshes[i].primitives[j].indices->buffer_view;
      bufferViewElementType = gltfData->meshes[i].primitives[j].indices->type;

      verticesData[verticesDataCount].bufferElementSize = cgltf_num_components(bufferViewElementType);
      verticesData[verticesDataCount].bufferType = UVR_GLTF_LOADER_VERTICES_INDEX;
      verticesData[verticesDataCount].byteOffset = bufferView->offset;
      verticesData[verticesDataCount].bufferSize = bufferView->size;
      verticesData[verticesDataCount].meshIndex = i;
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
    if (uvrgltf->uvr_gltf_loader_file[i].gltfData) {
      cgltf_free(uvrgltf->uvr_gltf_loader_file[i].gltfData);
      uvrgltf->uvr_gltf_loader_file[i].gltfData = NULL;
    }
  }
  for (i = 0; i < uvrgltf->uvr_gltf_loader_vertices_cnt; i++) {
    if (uvrgltf->uvr_gltf_loader_vertices[i].verticesData) {
      free(uvrgltf->uvr_gltf_loader_vertices[i].verticesData);
      uvrgltf->uvr_gltf_loader_vertices[i].verticesData = NULL;
    }
  }
}
