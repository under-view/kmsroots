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
 * struct uvr_gltf_loader_indices_info (Underview Renderer GLTF Loader Indices Info)
 *
 * members:
 * @indices    - The index of the accessor that links the bufferView with the vertex indices.
 * @byteOffset - The byte offset of where the vertex indices are located within the larger buffer.
 * @bufferSize - The size in bytes of the vertex indices buffer
 */
struct uvr_gltf_loader_indices_info {
  int indices;
  int byteOffset;
  int bufferSize;
};


/*
 * struct uvr_gltf_loader_indices (Underview Renderer GLTF Loader Indices)
 *
 * members:
 * @indicesInfo      - Pointer to an array of information about a given mesh->primitive->indices.
 *                     The "indices" index being the accessors array element associated with a
 *                     given buffer view. The buffer view then contains the index buffer
 *                     byte offset and buffer size.
 * @indicesInfoCount - Amount of elements in @indicesInfo array
 */
struct uvr_gltf_loader_indices {
  struct uvr_gltf_loader_indices_info *indicesInfo;
  uint32_t                            indicesInfoCount;
};


/*
 * uvr_gltf_loader_indices_vertex_index_buffers_acquire: Function acquires location of index buffer data contained in larger
 *                                                       buffer. @indicesInfo member is a pointer to an array of buffer
 *                                                       offset's and buffer byte sizes for the vertex index buffers.
 *
 *
 * args:
 * @uvrgltf - Must pass a pointer to a struct uvr_gltf_loader_file
 * return:
 *    on success struct uvr_gltf_loader_indices { with member being pointer to an array }
 *    on failure struct uvr_gltf_loader_indices { with member nulled }
 */
struct uvr_gltf_loader_indices uvr_gltf_loader_indices_vertex_index_buffers_acquire(struct uvr_gltf_loader_file *uvrgltf);


/*
 * struct uvr_gltf_loader_destroy (Underview Renderer Shader Destroy)
 *
 * members:
 * @uvr_gltf_loader_file_cnt    - Must pass the amount of elements in struct uvr_gltf_loader_file array
 * @uvr_gltf_loader_file        - Must pass a pointer to an array of valid struct uvr_gltf_loader_file { free'd  members: cgltf_data *gltfData }
 * @uvr_gltf_loader_indices_cnt - Must pass the amount of elements in struct uvr_gltf_loader_indices_cnt array
 * @uvr_gltf_loader_indices     - Must pass a pointer to an array of valid struct uvr_gltf_loader_indices { free'd  members: *indicesInfo }
 */
struct uvr_gltf_loader_destroy {
  uint32_t                       uvr_gltf_loader_file_cnt;
  struct uvr_gltf_loader_file    *uvr_gltf_loader_file;
  uint32_t                       uvr_gltf_loader_indices_cnt;
  struct uvr_gltf_loader_indices *uvr_gltf_loader_indices;
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
