#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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


struct uvr_gltf_loader_vertex uvr_gltf_loader_vertex_buffers_get(struct uvr_gltf_loader_vertex_buffers_get_info *uvrgltf)
{
  cgltf_size i, j, k, bufferViewElementType, meshCount, verticesDataCount = 0;
  cgltf_buffer *buffer = NULL;
  cgltf_buffer_view *bufferView = NULL;
  cgltf_data *gltfData = uvrgltf->gltfFile.gltfData;

  struct uvr_gltf_loader_vertex_data *verticesData = NULL;

  /* Retrieve amount of buffer views associate with buffers[uvrgltf->bufferIndex].buffer */
  for (i = 0; i < gltfData->meshes_count; i++) {
    for (j = 0; j < gltfData->meshes[i].primitives_count; j++) {
      for (k = 0; k < gltfData->meshes[i].primitives[j].attributes_count; k++) { // Normally would want to avoid, but in this case it's fine
        bufferView = gltfData->meshes[i].primitives[j].attributes[k].data->buffer_view;
        buffer = bufferView->buffer;
        if (buffer->index != uvrgltf->bufferIndex) continue;
        verticesDataCount++;
      }
      bufferView = gltfData->meshes[i].primitives[j].indices->buffer_view;
      buffer = bufferView->buffer;
      if (buffer->index != uvrgltf->bufferIndex) continue;
      verticesDataCount++;
    }
  }

  verticesData = calloc(verticesDataCount, sizeof(struct uvr_gltf_loader_vertex_data));
  if (!verticesData) {
    uvr_utils_log(UVR_DANGER, "[x] calloc(verticesData): %s", strerror(errno));
    goto exit_error_uvr_gltf_loader_vertex_buffers_get;
  }

  uint32_t bufferSize = gltfData->buffers[uvrgltf->bufferIndex].size;
  unsigned char *bufferData = calloc(bufferSize, sizeof(unsigned char));
  if (!bufferData) {
    uvr_utils_log(UVR_DANGER, "[x] calloc(bufferData): %s", strerror(errno));
    goto exit_error_uvr_gltf_loader_vertex_buffers_get_free_vertices;
  }

  memcpy(bufferData, gltfData->buffers[uvrgltf->bufferIndex].data, bufferSize);
  verticesDataCount=0;

  // TODO: account for accessor bufferOffset
  /*
   * Mesh->primitive->attribute->accessor->bufferView->buffer
   * Mesh->primitive->indices->accessor->bufferView->buffer
   * Normally would want to avoid, but in this case it's fine
   */
  for (i = 0; i < gltfData->meshes_count; i++) {
    for (j = 0; j < gltfData->meshes[i].primitives_count; j++) {
      for (k = 0; k < gltfData->meshes[i].primitives[j].attributes_count; k++) {
        /*
         * Store texture, normal, color, position, tangent information
         * at a given offset in a buffer
         */

        // bufferView associated with accessor which is associated with a mesh->primitive->attribute
        bufferView = gltfData->meshes[i].primitives[j].attributes[k].data->buffer_view;
        buffer = bufferView->buffer;

        /*
         * Don't populated verticesData array if the current bufferViews
         * buffer not equal to the one we want
         */
        if (buffer->index != uvrgltf->bufferIndex)
          continue;

        bufferViewElementType = gltfData->meshes[i].primitives[j].attributes[k].data->type;

        verticesData[verticesDataCount].byteOffset = bufferView->offset;
        verticesData[verticesDataCount].bufferSize = bufferView->size;
        verticesData[verticesDataCount].meshIndex = meshCount = i;

        switch (gltfData->meshes[i].primitives[j].attributes[k].type) {
          case cgltf_attribute_type_texcoord:
            verticesData[verticesDataCount].bufferElementSize = cgltf_num_components(bufferViewElementType);
            verticesData[verticesDataCount].bufferType = UVR_GLTF_LOADER_VERTEX_TEXTURE;
            break;
          case cgltf_attribute_type_normal:
            verticesData[verticesDataCount].bufferElementSize = cgltf_num_components(bufferViewElementType);
            verticesData[verticesDataCount].bufferType = UVR_GLTF_LOADER_VERTEX_NORMAL;
            break;
          case cgltf_attribute_type_color:
            verticesData[verticesDataCount].bufferElementSize = cgltf_num_components(bufferViewElementType);
            verticesData[verticesDataCount].bufferType = UVR_GLTF_LOADER_VERTEX_COLOR;
            break;
          case cgltf_attribute_type_position:
            verticesData[verticesDataCount].bufferElementSize = cgltf_num_components(bufferViewElementType);
            verticesData[verticesDataCount].bufferType = UVR_GLTF_LOADER_VERTEX_POSITION;
            break;
          case cgltf_attribute_type_tangent:
            verticesData[verticesDataCount].bufferElementSize = cgltf_num_components(bufferViewElementType);
            verticesData[verticesDataCount].bufferType = UVR_GLTF_LOADER_VERTEX_TANGENT;
            break;
          default:
            continue;
        }

        verticesDataCount++;
      }

      // Store index buffer data
      if (!gltfData->meshes[i].primitives[j].indices)
        continue;

      bufferView = gltfData->meshes[i].primitives[j].indices->buffer_view;
      buffer = bufferView->buffer;

      /*
       * Don't populated verticesData array if the current bufferViews
       * buffer does not equal to the one we want
       */
      if (buffer->index != uvrgltf->bufferIndex)
        continue;

      bufferViewElementType = gltfData->meshes[i].primitives[j].indices->type;

      verticesData[verticesDataCount].bufferElementCount = gltfData->meshes[i].primitives[j].indices->count;
      verticesData[verticesDataCount].bufferElementSize = cgltf_num_components(bufferViewElementType);
      verticesData[verticesDataCount].bufferType = UVR_GLTF_LOADER_VERTEX_INDEX;
      verticesData[verticesDataCount].byteOffset = bufferView->offset;
      verticesData[verticesDataCount].bufferSize = bufferView->size;
      verticesData[verticesDataCount].meshIndex = meshCount = i;
      verticesDataCount++;
    }
  }

  /*
   * To ensure accuracy with the amount of meshes associated with buffer
   * @meshCount is assigned when meshIndex is assigned to give buffer
   * in @verticesData array. Do to loops beginning at 0 add 1 to final
   * number to get an acurrate count.
   */
  meshCount++;
  return (struct uvr_gltf_loader_vertex) { .verticesData = verticesData, .verticesDataCount = verticesDataCount, .bufferData = bufferData,
                                           .bufferSize = bufferSize, .bufferIndex = uvrgltf->bufferIndex, .meshCount = meshCount };

exit_error_uvr_gltf_loader_vertex_buffers_get_free_vertices:
  free(verticesData);
exit_error_uvr_gltf_loader_vertex_buffers_get:
  return (struct uvr_gltf_loader_vertex) { .verticesData = NULL, .verticesDataCount = 0, .bufferData = NULL,
                                           .bufferSize = 0, .bufferIndex = 0, .meshCount = 0 };
}


struct uvr_gltf_loader_texture_image uvr_gltf_loader_texture_image_get(struct uvr_gltf_loader_texture_image_get_info *uvrgltf)
{
  struct uvr_gltf_loader_texture_image_data *imageData = NULL;
  uint32_t curImage = 0, totalBufferSize = 0;
  char *imageFile = NULL;

  cgltf_data *gltfData = uvrgltf->gltfFile.gltfData;

  imageData = calloc(gltfData->images_count, sizeof(struct uvr_gltf_loader_texture_image_data));
  if (!imageData) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_error_uvr_gltf_loader_texture_image_get;
  }

  /* Load all images associated with GLTF file into memory */
  for (curImage = 0; curImage < gltfData->images_count; curImage++) {
    imageFile = uvr_utils_concat_file_to_dir(uvrgltf->directory, gltfData->images[curImage].uri, (1<<8));

    imageData[curImage].pixels = (void *) stbi_load(imageFile, &imageData[curImage].imageWidth, &imageData[curImage].imageHeight, &imageData[curImage].imageChannels, STBI_rgb_alpha);
    if (!imageData[curImage].pixels) {
      uvr_utils_log(UVR_DANGER, "[x] stbi_load: failed to load %s", imageFile);
      free(imageFile); imageFile = NULL;
      goto exit_error_uvr_gltf_loader_texture_image_get_free_image_data;
    }

    if (imageData[curImage].imageChannels == 3) {
      imageData[curImage].imageChannels++;
    }

    free(imageFile); imageFile = NULL;
    imageData[curImage].imageSize = (imageData[curImage].imageWidth * imageData[curImage].imageHeight) * imageData[curImage].imageChannels;
    totalBufferSize += imageData[curImage].imageSize;
  }

  return (struct uvr_gltf_loader_texture_image) { .imageCount = gltfData->images_count, .totalBufferSize = totalBufferSize, .imageData = imageData };

exit_error_uvr_gltf_loader_texture_image_get_free_image_data:
  for (curImage = 0; curImage < gltfData->images_count; curImage++)
    if (imageData[curImage].pixels)
      stbi_image_free(imageData[curImage].pixels);
  free(imageData);
exit_error_uvr_gltf_loader_texture_image_get:
  return (struct uvr_gltf_loader_texture_image) { .imageCount = 0, .totalBufferSize = 0, .imageData = NULL };
}


void uvr_gltf_loader_destroy(struct uvr_gltf_loader_destroy *uvrgltf)
{
  uint32_t i, j;
  for (i = 0; i < uvrgltf->uvr_gltf_loader_file_cnt; i++) {
    if (uvrgltf->uvr_gltf_loader_file[i].gltfData) {
      cgltf_free(uvrgltf->uvr_gltf_loader_file[i].gltfData);
      uvrgltf->uvr_gltf_loader_file[i].gltfData = NULL;
    }
  }
  for (i = 0; i < uvrgltf->uvr_gltf_loader_vertex_cnt; i++) {
    if (uvrgltf->uvr_gltf_loader_vertex[i].verticesData) {
      if (uvrgltf->uvr_gltf_loader_vertex[i].verticesData) { // Gating to prevent accident double free'ing
        free(uvrgltf->uvr_gltf_loader_vertex[i].verticesData);
        uvrgltf->uvr_gltf_loader_vertex[i].verticesData = NULL;
      }
      if (uvrgltf->uvr_gltf_loader_vertex[i].bufferData) { // Gating to prevent accident double free'ing
        free(uvrgltf->uvr_gltf_loader_vertex[i].bufferData);
        uvrgltf->uvr_gltf_loader_vertex[i].bufferData = NULL;
      }
    }
  }
  for (i = 0; i < uvrgltf->uvr_gltf_loader_texture_image_cnt; i++) {
    for (j = 0; j < uvrgltf->uvr_gltf_loader_texture_image[i].imageCount; j++) {
      if (uvrgltf->uvr_gltf_loader_texture_image[i].imageData[j].pixels) {
        stbi_image_free(uvrgltf->uvr_gltf_loader_texture_image[i].imageData[j].pixels);
        uvrgltf->uvr_gltf_loader_texture_image[i].imageData[j].pixels = NULL;
      }
    }
    if (uvrgltf->uvr_gltf_loader_texture_image[i].imageData) {
      free(uvrgltf->uvr_gltf_loader_texture_image[i].imageData);
      uvrgltf->uvr_gltf_loader_texture_image[i].imageData = NULL;
    }
  }
}
