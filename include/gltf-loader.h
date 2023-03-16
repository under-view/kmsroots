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
  UVR_GLTF_LOADER_VERTICES_POSITION = (1 << 0),
  UVR_GLTF_LOADER_VERTICES_COLOR    = (1 << 1),
  UVR_GLTF_LOADER_VERTICES_TEXTURE  = (1 << 2),
  UVR_GLTF_LOADER_VERTICES_NORMAL   = (1 << 3),
  UVR_GLTF_LOADER_VERTICES_INDEX    = (1 << 4),
};


/*
 * struct uvr_gltf_loader_vertices_info (Underview Renderer GLTF Loader Indices Info)
 *
 * members:
 * @bufferType - Stores the type of vertex data contained in buffer at offset of larger buffer
 * @byteOffset - The byte offset of where the vertex indices are located within the larger buffer.
 * @bufferSize - The size in bytes of the vertex indices buffer
 */
struct uvr_gltf_loader_vertices_data {
  enum uvr_gltf_loader_vertices_type bufferType;
  uint32_t                           byteOffset;
  uint32_t                           bufferSize;
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
 * uvr_gltf_loader_vertices_get_buffers: Function acquires location of index buffer data contained in larger
 *                                       buffer. @indicesInfo member is a pointer to an array of buffer
 *                                       offset's and buffer byte sizes for the vertex index buffers.
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
 * struct uvr_gltf_loader_destroy (Underview Renderer Shader Destroy)
 *
 * members:
 * @uvr_gltf_loader_file_cnt     - Must pass the amount of elements in struct uvr_gltf_loader_file array
 * @uvr_gltf_loader_file         - Must pass a pointer to an array of valid struct uvr_gltf_loader_file { free'd  members: cgltf_data *gltfData }
 * @uvr_gltf_loader_vertices_cnt - Must pass the amount of elements in struct uvr_gltf_loader_vertices_cnt array
 * @uvr_gltf_loader_vertices     - Must pass a pointer to an array of valid struct uvr_gltf_loader_vertices { free'd  members: *indicesInfo }
 */
struct uvr_gltf_loader_destroy {
  uint32_t                        uvr_gltf_loader_file_cnt;
  struct uvr_gltf_loader_file     *uvr_gltf_loader_file;
  uint32_t                        uvr_gltf_loader_vertices_cnt;
  struct uvr_gltf_loader_vertices *uvr_gltf_loader_vertices;
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
