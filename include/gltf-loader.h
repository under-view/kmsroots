#ifndef KMR_GLTF_LOADER_H
#define KMR_GLTF_LOADER_H

#include "utils.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include <cglm/cglm.h>


/*
 * struct kmr_gltf_loader_file (kmsroots GLTF File)
 *
 * members:
 * @gltfData - Buffer that stores a given gltf file's content
 */
struct kmr_gltf_loader_file {
	cgltf_data *gltfData;
};


/*
 * struct kmr_gltf_loader_file_load_info (kmsroots GLTF Loader File Information)
 *
 * members:
 * @fileName - Must pass the path to the gltf file to load
 */
struct kmr_gltf_loader_file_load_info {
	const char *fileName;
};


/*
 * kmr_gltf_loader_file_load: This function is used to parse and load gltf files content
 *                            into memory. struct kmr_gltf_loader_file member bytes can
 *                            be free'd with a call to kmr_gltf_loader_destroy(3).
 *
 * parameters:
 * @kmsgltf - Must pass a pointer to a struct kmr_gltf_loader_file_load_info
 * returns:
 *	on success struct kmr_gltf_loader_file
 *	on failure struct kmr_gltf_loader_file { with member nulled }
 */
struct kmr_gltf_loader_file kmr_gltf_loader_file_load(struct kmr_gltf_loader_file_load_info *kmsgltf);


/*
 * struct kmr_gltf_loader_mesh_data (kmsroots GLTF Loader Mesh Data)
 *
 * Struct member order is arbitrary. SHOULD NOT BE USED DIRECTLY. Advise to create second stack buffer
 * and copy data over to it. Members populated with vertices from GLTF file buffer.
 *
 * @position - Vertex position coordinates
 * @normal   - Vertex normal (direction vertex points)
 * @texCoord - Texture Coordinate
 * @color    - Color
 */
struct kmr_gltf_loader_mesh_data {
	vec3 position;
	vec3 normal;
	vec2 texCoord;
	vec3 color;
};


/*
 * struct kmr_gltf_loader_mesh_mesh_data (kmsroots GLTF Loader Mesh Data)
 *
 * @firstIndex            - Base index within the index buffer. Calculated in kmr_gltf_loader_mesh_create(3)
 *                          firstIndex = firstIndex + bufferElementCount (GLTF file accessor[index].count)
 *                          Can be used by the application to fill in vkCmdDrawIndexed(3) function.
 * @indexBufferData       - Buffer of index data belonging to mesh populated from GLTF file buffer at @bufferIndex.
 * @indexBufferDataCount  - Amount of elements in @indexBufferData.
 * @indexBufferDataSize   - The total size in bytes of the @indexBufferData array.
 * @vertexBufferData      - Arbitrary structure to an array of vertices populated from GLTF file buffer at @bufferIndex.
 * @vertexBufferDataCount - Amount of elements in @vertexBufferData array.
 * @vertexBufferDataSize  - The total size in bytes of the @vertexBufferData array.
 */
struct kmr_gltf_loader_mesh_mesh_data {
	uint32_t                         firstIndex;
	uint32_t                         *indexBufferData;
	uint32_t                         indexBufferDataCount;
	uint32_t                         indexBufferDataSize;
	struct kmr_gltf_loader_mesh_data *vertexBufferData;
	uint32_t                         vertexBufferDataCount;
	uint32_t                         vertexBufferDataSize;
};


/*
 * struct kmr_gltf_loader_mesh (kmsroots GLTF Loader Mesh)
 *
 * members:
 * @bufferIndex   - The index in the "buffers" array of give GLTF file.
 * @meshDataCount - Amount of meshes associated with a @bufferIndex GLTF buffer. The array size of @meshData array.
 * @meshData      - Pointer to an array of struct kmr_gltf_loader_mesh_mesh_data storing all important data
 *                  related to each mesh.
 */
struct kmr_gltf_loader_mesh {
	uint16_t                              bufferIndex;
	uint16_t                              meshDataCount;
	struct kmr_gltf_loader_mesh_mesh_data *meshData;
};


/*
 * struct kmr_gltf_loader_mesh_create_info (kmsroots GLTF Loader Mesh Create Information)
 *
 * members:
 * @gltfLoaderFile - Must pass a valid struct kmr_gltf_loader_file for cgltf_data @gltfData member
 * @bufferIndex    - Index of buffer in GLTF file buffers array
 */
struct kmr_gltf_loader_mesh_create_info {
	struct kmr_gltf_loader_file gltfLoaderFile;
	uint16_t                    bufferIndex;
};


/*
 * kmr_gltf_loader_mesh_create: Function loops through all meshes and finds the associated accessor->buffer view
 *                              for a given buffer at @bufferIndex. After retrieves all information required to
 *                              understand the contents of a buffer. The function then creates multiple meshes
 *                              with appropriate data (struct kmr_gltf_loader_mesh_data) so the application
 *                              only need to call function and create their vertex buffer + index buffer array
 *                              based upon what's already populated. Converts GLTF buffer to a buffer that Vulkan
 *                              can understand seperating each buffer, by their mesh index in GLTF file "meshes"
 *                              array. @meshData must be free'd by the application with call to kmr_gltf_loader_destroy(3).
 *
 * parameters:
 * @kmsgltf - Must pass a pointer to a struct kmr_gltf_loader_mesh_create_info
 * returns:
 *	on success struct kmr_gltf_loader_mesh { with member being pointer to an array }
 *	on failure struct kmr_gltf_loader_mesh { with member nulled }
 */
struct kmr_gltf_loader_mesh kmr_gltf_loader_mesh_create(struct kmr_gltf_loader_mesh_create_info *kmsgltf);


/*
 * struct kmr_gltf_loader_texture_image (kmsroots GLTF Loader Texture Image)
 *
 * members:
 * @imageCount      - Amount of images associated with a given GLTF file
 * @totalBufferSize - Collective size of each image associated with a given GLTF file.
 *                    Best utilized when creating single VkBuffer.
 * @imageData       - Pointer to an array of image meta data and pixel buffer.
 */
struct kmr_gltf_loader_texture_image {
	uint32_t                      imageCount;
	uint32_t                      totalBufferSize;
	struct kmr_utils_image_buffer *imageData;
};


/*
 * struct kmr_gltf_loader_texture_image_create_info (kmsroots GLTF Loader Texture Image Create Information)
 *
 * members:
 * @gltfLoaderFile - Must pass a valid struct kmr_gltf_loader_file for cgltf_data @gltfData member
 * @directory      - Must pass a pointer to a string detailing the directory of where all images are stored.
 *                   Absolute path to a file that resides in the same directory as the image files will work too.
 */
struct kmr_gltf_loader_texture_image_create_info {
	struct kmr_gltf_loader_file gltfLoaderFile;
	const char                  *directory;
};


/*
 * kmr_gltf_loader_texture_image_create: Function Loads all images associated with gltf file into memory.
 *                                       To free @pixels and @imageData call kmr_gltf_loader_destroy(3).
 *
 * parameters:
 * @kmsgltf - Must pass a pointer to a struct kmr_gltf_loader_texture_image_create_info
 * returns:
 *	on success struct kmr_gltf_loader_texture_image { with member being pointer to an array }
 *	on failure struct kmr_gltf_loader_texture_image { with member nulled }
 */
struct kmr_gltf_loader_texture_image kmr_gltf_loader_texture_image_create(struct kmr_gltf_loader_texture_image_create_info *kmsgltf);


/*
 * struct kmr_gltf_loader_cgltf_texture_transform (kmsroots GLTF Loader CGLTF Texture Transform)
 *
 * members:
 * @offsets  -
 * @rotation -
 * @scales   -
 */
struct kmr_gltf_loader_cgltf_texture_transform {
	float offsets[2];
	float rotation;
	float scales[2];
};


/*
 * struct kmr_gltf_loader_cgltf_texture_view (kmsroots GLTF Loader CGLTF Texture View)
 *
 * members:
 * @textureIndex     - Index in "textures" GLTF file array
 * @imageIndex       - Index in "images" GTLF file array that belongs to the texture at @textureIndex
 * @scale            - The scalar parameter applied to each vector of the texture.
 * @textureTransform -
 */
struct kmr_gltf_loader_cgltf_texture_view {
	uint32_t                                       textureIndex;
	uint32_t                                       imageIndex;
	float                                          scale;
	struct kmr_gltf_loader_cgltf_texture_transform textureTransform;
};


/*
 * struct kmr_gltf_loader_cgltf_pbr_metallic_roughness (kmsroots GLTF Loader CGLTF Physically-Based Rendering Metallic Roughness)
 *
 * members:
 * @baseColorTexture         -
 * @metallicRoughnessTexture - Textures for metalness and roughness properties are packed together in a single texture (image).
 *                             Used for readability should hopefully let application writing know we are acquring texture->image
 *                             associated with @metallicRoughnessTexture.
 *
 * Bellow define the metallic-roughness material model
 * @baseColorFactor          - The "main" color of the object surface (RBGA).
 * @metallicFactor           - Describes how much the reflective behavior of the material resembles that of a metal.
 *                             Values range from 0.0 (non-metal) to 1.0 (metal).
 * @roughnessFactor          - Indicating how rough the surface is, affecting the light scattering.
 *                             Values range from 0.0 (smooth) to 1.0 (rough).
 */
struct kmr_gltf_loader_cgltf_pbr_metallic_roughness {
	struct kmr_gltf_loader_cgltf_texture_view baseColorTexture;
	struct kmr_gltf_loader_cgltf_texture_view metallicRoughnessTexture;
	float                                     baseColorFactor[4];
	float                                     metallicFactor;
	float                                     roughnessFactor;
};


/*
 * struct kmr_gltf_loader_material_data (kmsroots GLTF Loader Material Data)
 *
 * Copy of struct cgltf_material slightly modified. Will add more members based upon need.
 *
 * members:
 * @meshIndex            - Index in "meshes" array of GLTF file that material belongs to.
 * @materialName         - Name given to material block contained in GLTF file
 * @pbrMetallicRoughness - "Physically-Based Rendering Metallic Roughness Model" - Allows renderers to
 *                         display objects with a realistic appearance under different lighting conditions.
 *                         Stores required data for PBR.
 * @normalTexture        -
 * @occlusionTexture     -
 */
struct kmr_gltf_loader_material_data {
	uint32_t                                             meshIndex;
	char                                                 materialName[32];
	struct kmr_gltf_loader_cgltf_pbr_metallic_roughness  pbrMetallicRoughness;
	struct kmr_gltf_loader_cgltf_texture_view            normalTexture;
	struct kmr_gltf_loader_cgltf_texture_view            occlusionTexture;
};


/*
 * struct kmr_gltf_loader_material (kmsroots GLTF Loader Material)
 *
 * members:
 * @materialData      - Pointer to an array of struct kmr_gltf_loader_material_data
 * @materialDataCount - Amount of elements in @materialData array
 */
struct kmr_gltf_loader_material {
	struct kmr_gltf_loader_material_data *materialData;
	uint32_t                             materialDataCount;
};


/*
 * struct kmr_gltf_loader_material_create_info (kmsroots GLTF Loader Material Create Information)
 *
 * members:
 * @gltfLoaderFile - Must pass a valid struct kmr_gltf_loader_file for cgltf_data @gltfData member
 */
struct kmr_gltf_loader_material_create_info {
	struct kmr_gltf_loader_file gltfLoaderFile;
};


/*
 * kmr_gltf_loader_material_create: Function Loads necessary material information associated with gltf file into memory.
 *                                  To free @materialData call kmr_gltf_loader_destroy(3) or free(@materialData).
 *
 * parameters:
 * @kmsgltf - Must pass a pointer to a struct kmr_gltf_loader_material_create_info
 * returns:
 *	on success struct kmr_gltf_loader_material { with member being pointer to an array }
 *	on failure struct kmr_gltf_loader_material { with member nulled }
 */
struct kmr_gltf_loader_material kmr_gltf_loader_material_create(struct kmr_gltf_loader_material_create_info *kmsgltf);


/*
 * enum kmr_gltf_loader_gltf_object_type (kmsroots GLTF Loader GLTF Object Type)
 */
enum kmr_gltf_loader_gltf_object_type {
	KMR_GLTF_LOADER_GLTF_NODE = 0x00000001,
	KMR_GLTF_LOADER_GLTF_MESH = 0x00000002,
	KMR_GLTF_LOADER_GLTF_SKIN = 0x00000003,
	KMR_GLTF_LOADER_GLTF_CAMERA = 0x00000004,
};


/*
 * struct kmr_gltf_loader_node_data (kmsroots GLTF Loader Node Data)
 *
 * members:
 * @objectType      - Type of GLTF object that attached to node
 * @objectIndex     - The index in GLTF file "Insert Object Name" array. If @objectType is a
 *                    mesh this index is the index in the GLTF file "meshes" array.
 * @nodeIndex       - Index in the GLTF file "nodes" array for child node
 * @parentNodeIndex - Index in the GLTF file "nodes" array for parent node
 * @matrixTransform - If matrix property not already defined. Value is T * R * S.
 *                    T - translation
 *                    R - Rotation
 *                    S - scale
 *                    Final matrix transform is (identity matrix * TRS parent matrix) * (identity matrix * TRS child matrix)
 */
struct kmr_gltf_loader_node_data {
	enum kmr_gltf_loader_gltf_object_type objectType;
	uint32_t                              objectIndex;
	uint32_t                              nodeIndex;
	uint32_t                              parentNodeIndex;
	float                                 matrixTransform[4][4];
};


/*
 * struct kmr_gltf_loader_node (kmsroots GLTF Loader Node)
 *
 * members:
 * @nodeData      - Pointer to an array of struct kmr_gltf_loader_node_data
 * @nodeDataCount - Amount of elements in @nodeData array
 */
struct kmr_gltf_loader_node {
	struct kmr_gltf_loader_node_data *nodeData;
	uint32_t                         nodeDataCount;
};


/*
 * struct kmr_gltf_loader_node_create_info (kmsroots GLTF Loader Node Create Information)
 *
 * members:
 * @gltfLoaderFile - Must pass a valid struct kmr_gltf_loader_file for cgltf_data @gltfData member
 * @sceneIndex     - Index in GLTF file "scenes" array.
 */
struct kmr_gltf_loader_node_create_info {
	struct kmr_gltf_loader_file gltfLoaderFile;
	uint32_t                    sceneIndex;
};


/*
 * kmr_gltf_loader_node_create: Calculates final translation * rotatation * scale matrix for all nodes associated with a scene.
 *                              Along with final matrix transform data function also returns the parent and child index in the
 *                              GLTF file object "nodes" array, the type of node/object (i.e "mesh,skin,camera,etc..."), and
 *                              the index of that object in that GLTF file "Insert Object Name" array.
 *
 * parameters:
 * @kmsgltf - Must pass a pointer to a struct kmr_gltf_loader_node_create_info
 * returns:
 *	on success struct kmr_gltf_loader_node { with member being pointer to an array }
 *	on failure struct kmr_gltf_loader_node { with member nulled }
 */
struct kmr_gltf_loader_node kmr_gltf_loader_node_create(struct kmr_gltf_loader_node_create_info *kmsgltf);


/*
 * kmr_gltf_loader_node_create: Prints out matrix transform for each node in struct kmr_gltf_loader_node { @nodeData }
 *
 * parameters:
 * @kmsgltf - Must pass a pointer to a struct kmr_gltf_loader_node
 */
void kmr_gltf_loader_node_display_matrix_transform(struct kmr_gltf_loader_node *kmsgltf);


/*
 * struct kmr_gltf_loader_destroy (kmsroots Shader Destroy)
 *
 * members:
 * @kmr_gltf_loader_file_cnt          - Must pass the amount of elements in struct kmr_gltf_loader_file array
 * @kmr_gltf_loader_file              - Must pass a pointer to an array of valid struct kmr_gltf_loader_file { free'd members: @gltfData }
 * @kmr_gltf_loader_mesh_cnt          - Must pass the amount of elements in struct kmr_gltf_loader_mesh array
 * @kmr_gltf_loader_mesh              - Must pass a pointer to an array of valid struct kmr_gltf_loader_mesh { free'd members: @verticesData }
 * @kmr_gltf_loader_texture_image_cnt - Must pass the amount of elements in struct kmr_gltf_loader_texture_image array
 * @kmr_gltf_loader_texture_image     - Must pass a pointer to an array of valid struct kmr_gltf_loader_texture_image { free'd members: @imageData, @pixels }
 * @kmr_gltf_loader_material_cnt      - Must pass the amount of elements in struct kmr_gltf_loader_material array
 * @kmr_gltf_loader_material          - Must pass a pointer to an array of valid struct kmr_gltf_loader_material { free'd members: @materialData }
 * @kmr_gltf_loader_node_cnt          - Must pass the amount of elements in struct kmr_gltf_loader_node array
 * @kmr_gltf_loader_node              - Must pass a pointer to an array of valid struct kmr_gltf_loader_node { free'd members: @nodeData }
 */
struct kmr_gltf_loader_destroy {
	uint32_t                             kmr_gltf_loader_file_cnt;
	struct kmr_gltf_loader_file          *kmr_gltf_loader_file;
	uint32_t                             kmr_gltf_loader_mesh_cnt;
	struct kmr_gltf_loader_mesh          *kmr_gltf_loader_mesh;
	uint32_t                             kmr_gltf_loader_texture_image_cnt;
	struct kmr_gltf_loader_texture_image *kmr_gltf_loader_texture_image;
	uint32_t                             kmr_gltf_loader_material_cnt;
	struct kmr_gltf_loader_material      *kmr_gltf_loader_material;
	uint32_t                             kmr_gltf_loader_node_cnt;
	struct kmr_gltf_loader_node          *kmr_gltf_loader_node;
};


/*
 * kmr_gltf_loader_destroy: frees any allocated memory defined by customer
 *
 * parameters:
 * @kmsgltf - pointer to a struct kmr_gltf_loader_destroy contains all objects created during
 *            application lifetime in need freeing
 */
void kmr_gltf_loader_destroy(struct kmr_gltf_loader_destroy *kmsgltf);


#endif
