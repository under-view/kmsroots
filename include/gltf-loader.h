#ifndef UVR_GLTF_LOADER_H
#define UVR_GLTF_LOADER_H

#include "utils.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"


/*
 * struct uvr_gltf_loader_file (Underview Renderer GLTF File)
 *
 * members:
 * @gltfData - Buffer that stores a given gltf file's content
 */
struct uvr_gltf_loader_file {
  cgltf_data *gltfData;
};


/*
 * struct uvr_gltf_loader_file_load_info (Underview Renderer GLTF Loader File Information)
 *
 * members:
 * @fileName - Must pass the path to the gltf file to load
 */
struct uvr_gltf_loader_file_load_info {
  const char *fileName;
};


/*
 * uvr_gltf_loader_file_load: This function is used to parse and load gltf files content
 *                            into memory. struct uvr_gltf_loader_file member bytes can
 *                            be free'd with a call to uvr_gltf_loader_destroy(3).
 *
 * args:
 * @uvrgltf - Must pass a pointer to a struct uvr_gltf_loader_file_load_info
 * return:
 *    on success struct uvr_gltf_loader_file
 *    on failure struct uvr_gltf_loader_file { with member nulled }
 */
struct uvr_gltf_loader_file uvr_gltf_loader_file_load(struct uvr_gltf_loader_file_load_info *uvrgltf);


/*
 * enum uvr_gltf_loader_vertices_type (Underview Renderer GLTF Loader Vertices Type)
 *
 * Used to determine contents contained at offset of larger buffer
 */
enum uvr_gltf_loader_vertices_type {
  UVR_GLTF_LOADER_VERTICES_POSITION = 0x00000001,
  UVR_GLTF_LOADER_VERTICES_COLOR    = 0x00000002,
  UVR_GLTF_LOADER_VERTICES_TEXTURE  = 0x00000003,
  UVR_GLTF_LOADER_VERTICES_NORMAL   = 0x00000004,
  UVR_GLTF_LOADER_VERTICES_TANGENT  = 0x00000005,
  UVR_GLTF_LOADER_VERTICES_INDEX    = 0x00000006,
};


/*
 * struct uvr_gltf_loader_vertices_info (Underview Renderer GLTF Loader Indices Info)
 *
 * members:
 * @bufferType        - Stores the type of vertex data contained in buffer at offset of larger buffer
 * @byteOffset        - The byte offset of where the vertex indices are located within the larger buffer.
 * @bufferSize        - The size in bytes of the vertex indices buffer
 * @bufferElementSize - The size of each element contained within a buffer
 * @meshIndex         - Mesh associated with buffer
 */
struct uvr_gltf_loader_vertices_data {
  enum uvr_gltf_loader_vertices_type bufferType;
  uint32_t                           byteOffset;
  uint32_t                           bufferSize;
  uint32_t                           bufferElementSize;
  uint32_t                           meshIndex;
};


/*
 * struct uvr_gltf_loader_vertices (Underview Renderer GLTF Loader Indices)
 *
 * members:
 * @verticesData      - Pointer to an array of information about a given mesh->primitive->indices.
 *                      The "indices" index being the accessors array element associated with a
 *                      given buffer view. The buffer view then contains the index buffer
 *                      byte offset and buffer size.
 * @verticesDataCount - Amount of elements in @indicesInfo array
 */
struct uvr_gltf_loader_vertices {
  struct uvr_gltf_loader_vertices_data *verticesData;
  uint32_t                             verticesDataCount;
};


/*
 * uvr_gltf_loader_vertices_get_buffers: Function loops through all meshes and finds the associated buffer view
 *                                       for a given mesh primitive indices and attributes. Then returns in the
 *                                       member @verticesData and array of byteOffset, total bufferSize, the size
 *                                       of each element within a buffer, and the associated mesh.
 *                                       buffer. @verticesData Must be free by the application.
 *
 *
 * args:
 * @uvrgltf - Must pass a pointer to a struct uvr_gltf_loader_file
 * return:
 *    on success struct uvr_gltf_loader_vertices { with member being pointer to an array }
 *    on failure struct uvr_gltf_loader_vertices { with member nulled }
 */
struct uvr_gltf_loader_vertices uvr_gltf_loader_vertices_get_buffers(struct uvr_gltf_loader_file *uvrgltf);


/*
 * struct uvr_gltf_loader_texture_image_data (Underview Renderer GLTF Loader Texture Image Data)
 *
 * members:
 * @pixels        - Pointer to actual pixel data
 * @imageWidth    - Width of image in pixels/texels
 * @imageHeight   - Height of image in pixels/texels
 * @imageChannels - Amount of color channels image has { RGB(3), RGBA(4) }
 * @imageSize     - Byte size of the image (@textureWidth * @textureHeight) * @textureChannels
 */
struct uvr_gltf_loader_texture_image_data {
  void   *pixels;
  int    imageWidth;
  int    imageHeight;
  int    imageChannels;
  size_t imageSize;
};


/*
 * struct uvr_gltf_loader_texture_image (Underview Renderer GLTF Loader Texture Image)
 *
 * members:
 * @imageCount      - Amount of images associated with a given GLTF file
 * @totalBufferSize - Collective size of each image associated with a given GLTF file.
 *                    Best utilized when creating single VkBuffer.
 * @imageData       - Pointer to an array of image data.
 */
struct uvr_gltf_loader_texture_image {
  uint32_t                                  imageCount;
  uint32_t                                  totalBufferSize;
  struct uvr_gltf_loader_texture_image_data *imageData;
};


/*
 * uvr_gltf_loader_texture_image_get: Function Loads all images associated with gltf file into memory.
 *                                    To free @pixels and @imageData call uvr_gltf_loader_destroy(3).
 *
 * args:
 * @uvrgltf   - Must pass a pointer to a struct uvr_gltf_loader_file
 * @directory - Must pass a pointer to a string detailing the directory of where all images are stored.
 * return:
 *    on success struct uvr_gltf_loader_texture_image { with member being pointer to an array }
 *    on failure struct uvr_gltf_loader_texture_image { with member nulled }
 */
struct uvr_gltf_loader_texture_image uvr_gltf_loader_texture_image_get(struct uvr_gltf_loader_file *uvrgltf, const char *directory);


/*
 * struct uvr_gltf_loader_destroy (Underview Renderer Shader Destroy)
 *
 * members:
 * @uvr_gltf_loader_file_cnt          - Must pass the amount of elements in struct uvr_gltf_loader_file array
 * @uvr_gltf_loader_file              - Must pass a pointer to an array of valid struct uvr_gltf_loader_file { free'd  members: cgltf_data *gltfData }
 * @uvr_gltf_loader_vertices_cnt      - Must pass the amount of elements in struct uvr_gltf_loader_vertices_cnt array
 * @uvr_gltf_loader_vertices          - Must pass a pointer to an array of valid struct uvr_gltf_loader_vertices { free'd  members: *indicesInfo }
 * @uvr_gltf_loader_texture_image_cnt - Must pass the amount of elements in struct uvr_gltf_loader_vertices_cnt array
 * @uvr_gltf_loader_texture_image     - Must pass a pointer to an array of valid struct uvr_gltf_loader_vertices { free'd  members: *indicesInfo }
 */
struct uvr_gltf_loader_destroy {
  uint32_t                             uvr_gltf_loader_file_cnt;
  struct uvr_gltf_loader_file          *uvr_gltf_loader_file;
  uint32_t                             uvr_gltf_loader_vertices_cnt;
  struct uvr_gltf_loader_vertices      *uvr_gltf_loader_vertices;
  uint32_t                             uvr_gltf_loader_texture_image_cnt;
  struct uvr_gltf_loader_texture_image *uvr_gltf_loader_texture_image;
};


/*
 * uvr_gltf_loader_destroy: frees any allocated memory defined by customer
 *
 * args:
 * @uvrgltf - pointer to a struct uvr_gltf_loader_destroy contains all objects created during
 *            application lifetime in need freeing
 */
void uvr_gltf_loader_destroy(struct uvr_gltf_loader_destroy *uvrgltf);


#endif
