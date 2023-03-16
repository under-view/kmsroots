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


struct uvr_gltf_loader_vertex uvr_gltf_loader_vertex_buffers_get(struct uvr_gltf_loader_file *uvrgltf)
{
  cgltf_size i, j, k, verticesDataCount = 0, bufferViewElementType;
  cgltf_buffer_view *bufferView = NULL;
  cgltf_data *gltfData = uvrgltf->gltfData;

  struct uvr_gltf_loader_vertex_data *verticesData = NULL;

  verticesData = calloc(gltfData->buffer_views_count, sizeof(struct uvr_gltf_loader_vertex_data));
  if (!verticesData) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_error_uvr_gltf_loader_vertex_buffers_get;
  }

  // TODO: account for accessor bufferOffset
  // Mesh->primitive->attribute->accessor->bufferView->buffer
  // Mesh->primitive->indices->accessor->bufferView->buffer
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

      if (!gltfData->meshes[i].primitives[j].indices)
        continue;

      // Store index buffer data
      bufferView = gltfData->meshes[i].primitives[j].indices->buffer_view;
      bufferViewElementType = gltfData->meshes[i].primitives[j].indices->type;

      verticesData[verticesDataCount].bufferElementSize = cgltf_num_components(bufferViewElementType);
      verticesData[verticesDataCount].bufferType = UVR_GLTF_LOADER_VERTEX_INDEX;
      verticesData[verticesDataCount].byteOffset = bufferView->offset;
      verticesData[verticesDataCount].bufferSize = bufferView->size;
      verticesData[verticesDataCount].meshIndex = i;
      verticesDataCount++;
    }
  }

  return (struct uvr_gltf_loader_vertex) { .verticesData = verticesData, .verticesDataCount = verticesDataCount };

exit_error_uvr_gltf_loader_vertex_buffers_get:
  return (struct uvr_gltf_loader_vertex) { .verticesData = NULL, .verticesDataCount = 0 };
}


struct uvr_gltf_loader_texture_image uvr_gltf_loader_texture_image_get(struct uvr_gltf_loader_file *uvrgltf, const char *directory)
{
  struct uvr_gltf_loader_texture_image_data *imageData = NULL;
  uint32_t curImage = 0, totalBufferSize = 0;
  char *imageFile = NULL;

  cgltf_data *gltfData = uvrgltf->gltfData;

  imageData = calloc(gltfData->images_count, sizeof(struct uvr_gltf_loader_texture_image_data));
  if (!imageData) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_error_uvr_gltf_loader_texture_image_get;
  }

  /* Load all images associated with GLTF file into memory */
  for (curImage = 0; curImage < gltfData->images_count; curImage++) {
    imageFile = uvr_utils_concat_file_to_dir(directory, gltfData->images[curImage].uri, (1<<8));

    imageData[curImage].pixels = (void *) stbi_load(imageFile, &imageData[curImage].imageWidth, &imageData[curImage].imageHeight, &imageData[curImage].imageChannels, STBI_rgb_alpha);
    if (!imageData[curImage].pixels) {
      uvr_utils_log(UVR_DANGER, "[x] stbi_load: failed to load %s", imageFile);
      free(imageFile); imageFile = NULL;
      goto exit_error_uvr_gltf_loader_texture_image_get_free_image_data;
    }

    free(imageFile); imageFile = NULL;
    imageData[curImage].imageSize = (imageData[curImage].imageWidth * imageData[curImage].imageHeight) * (imageData[curImage].imageChannels+1);
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
      free(uvrgltf->uvr_gltf_loader_vertex[i].verticesData);
      uvrgltf->uvr_gltf_loader_vertex[i].verticesData = NULL;
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
