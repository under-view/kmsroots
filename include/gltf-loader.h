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
 *	on success struct uvr_gltf_loader_file
 *	on failure struct uvr_gltf_loader_file { with member nulled }
 */
struct uvr_gltf_loader_file uvr_gltf_loader_file_load(struct uvr_gltf_loader_file_load_info *uvrgltf);


/*
 * enum uvr_gltf_loader_vertex_type (Underview Renderer GLTF Loader Vertex Type)
 *
 * Used to determine contents contained at offset of larger buffer
 */
enum uvr_gltf_loader_vertex_type {
	UVR_GLTF_LOADER_VERTEX_POSITION = 0x00000001,
	UVR_GLTF_LOADER_VERTEX_COLOR    = 0x00000002,
	UVR_GLTF_LOADER_VERTEX_TEXTURE  = 0x00000003,
	UVR_GLTF_LOADER_VERTEX_NORMAL   = 0x00000004,
	UVR_GLTF_LOADER_VERTEX_TANGENT  = 0x00000005,
	UVR_GLTF_LOADER_VERTEX_INDEX    = 0x00000006,
};


/*
 * struct uvr_gltf_loader_vertex_data (Underview Renderer GLTF Loader Vertex data)
 *
 * members:
 * @bufferType         - Stores the type of vertex data contained in buffer at offset of larger buffer
 * @byteOffset         - The byte offset of where the vertex indices are located within the larger buffer.
 * @bufferSize         - The size in bytes of buffer managed by buffer view
 * @bufferElementCount - The amount of elements in buffer
 * @bufferElementSize  - The size of each element contained within a buffer
 * @meshIndex          - Mesh associated with buffer
 */
struct uvr_gltf_loader_vertex_data {
	enum uvr_gltf_loader_vertex_type bufferType;
	uint32_t                         byteOffset;
	uint32_t                         bufferSize;
	uint32_t                         bufferElementCount;
	uint32_t                         bufferElementSize;
	uint32_t                         meshIndex;
};


/*
 * struct uvr_gltf_loader_vertex (Underview Renderer GLTF Loader Vertex)
 *
 * members:
 * @verticesData      - Pointer to an array of information about a given mesh->primitive->indices &
 *                      mesh->primitive->attributes. Accessor + buffer view information.
 * @verticesDataCount - Amount of elements in @verticesData array
 * @bufferData        - The actual bytes of data for an individual buffer
 * @bufferIndex       - The index in the "buffers" array of give GLTF file
 * @meshCount         - Amount of meshes associated with a @bufferIndex buffer in "buffers" array.
 */
struct uvr_gltf_loader_vertex {
	struct uvr_gltf_loader_vertex_data *verticesData;
	uint32_t                           verticesDataCount;
	unsigned char                      *bufferData;
	uint32_t                           bufferSize;
	uint32_t                           bufferIndex;
	uint32_t                           meshCount;
};


/*
 * struct uvr_gltf_loader_vertex_buffers_create_info (Underview Renderer GLTF Loader Vertex Buffers Create Information)
 *
 * members:
 * @gltfFile    - Must pass a valid struct uvr_gltf_loader_file for cgltf_data @gltfData member
 * @bufferIndex - Index of buffer in GLTF file buffers array
 */
struct uvr_gltf_loader_vertex_buffers_create_info {
	struct uvr_gltf_loader_file gltfFile;
	cgltf_int                   bufferIndex;
};


/*
 * uvr_gltf_loader_vertex_buffers_create: Function loops through all meshes and finds the associated buffer view
 *                                        for a given mesh primitive indices and attributes. Then returns in the
 *                                        member @verticesData an array of byteOffset, total bufferSize at that offset,
 *                                        the size of each element within the buffer at a given offset, the type of data
 *                                        at said offset within the larger buffer, and the associated mesh index. Function
 *                                        will only acquire buffer view data associated with @bufferIndex in the "buffers"
 *                                        array of a GLTF file. @verticesData Must be free by the application with
 *                                        uvr_gltf_loader_destroy(3).
 *
 *
 * args:
 * @uvrgltf - Must pass a pointer to a struct uvr_gltf_loader_vertex_buffers_create_info
 * return:
 *	on success struct uvr_gltf_loader_vertex { with member being pointer to an array }
 *	on failure struct uvr_gltf_loader_vertex { with member nulled }
 */
struct uvr_gltf_loader_vertex uvr_gltf_loader_vertex_buffers_create(struct uvr_gltf_loader_vertex_buffers_create_info *uvrgltf);


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
 * struct uvr_gltf_loader_texture_image_create_info (Underview Renderer GLTF Loader Texture Image Create Information)
 *
 * members:
 * @gltfFile  - Must pass a valid struct uvr_gltf_loader_file for cgltf_data @gltfData member
 * @directory - Must pass a pointer to a string detailing the directory of where all images are stored.
 *              Absolute path to a file that resides in the same directory as the image files will work too.
 */
struct uvr_gltf_loader_texture_image_create_info {
	struct uvr_gltf_loader_file gltfFile;
	const char                  *directory;
};


/*
 * uvr_gltf_loader_texture_image_create: Function Loads all images associated with gltf file into memory.
 *                                       To free @pixels and @imageData call uvr_gltf_loader_destroy(3).
 *
 * args:
 * @uvrgltf - Must pass a pointer to a struct uvr_gltf_loader_texture_image_create_info
 * return:
 *	on success struct uvr_gltf_loader_texture_image { with member being pointer to an array }
 *	on failure struct uvr_gltf_loader_texture_image { with member nulled }
 */
struct uvr_gltf_loader_texture_image uvr_gltf_loader_texture_image_create(struct uvr_gltf_loader_texture_image_create_info *uvrgltf);


/*
 * struct uvr_gltf_loader_cgltf_texture_transform (Underview Renderer GLTF Loader CGLTF Texture Transform)
 *
 * members:
 * @offsets  -
 * @rotation -
 * @scales   -
 */
struct uvr_gltf_loader_cgltf_texture_transform {
	float offsets[2];
	float rotation;
	float scales[2];
};


/*
 * struct uvr_gltf_loader_cgltf_texture_view (Underview Renderer GLTF Loader CGLTF Texture View)
 *
 * members:
 * @textureIndex     - Index in "textures" GLTF file array
 * @imageIndex       - Index in "images" GTLF file array that belongs to the texture at @textureIndex
 * @scale            - The scalar parameter applied to each vector of the texture.
 * @textureTransform -
 */
struct uvr_gltf_loader_cgltf_texture_view {
	uint32_t                                       textureIndex;
	uint32_t                                       imageIndex;
	float                                          scale;
	struct uvr_gltf_loader_cgltf_texture_transform textureTransform;
};


/*
 * struct uvr_gltf_loader_cgltf_pbr_metallic_roughness (Underview Renderer GLTF Loader CGLTF Physically-Based Rendering Metallic Roughness)
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
struct uvr_gltf_loader_cgltf_pbr_metallic_roughness {
	struct uvr_gltf_loader_cgltf_texture_view baseColorTexture;
	struct uvr_gltf_loader_cgltf_texture_view metallicRoughnessTexture;
	float                                     baseColorFactor[4];
	float                                     metallicFactor;
	float                                     roughnessFactor;
};


/*
 * struct uvr_gltf_loader_material_data (Underview Renderer GLTF Loader Material Data)
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
struct uvr_gltf_loader_material_data {
	uint32_t                                             meshIndex;
	char                                                 materialName[32];
	struct uvr_gltf_loader_cgltf_pbr_metallic_roughness  pbrMetallicRoughness;
	struct uvr_gltf_loader_cgltf_texture_view            normalTexture;
	struct uvr_gltf_loader_cgltf_texture_view            occlusionTexture;
};


/*
 * struct uvr_gltf_loader_material (Underview Renderer GLTF Loader Material)
 *
 * members:
 * @materialData      - Pointer to an array of struct uvr_gltf_loader_material_data
 * @materialDataCount - Amount of elements in @materialData array
 */
struct uvr_gltf_loader_material {
	struct uvr_gltf_loader_material_data *materialData;
	uint32_t                             materialDataCount;
};


/*
 * struct uvr_gltf_loader_material_create_info (Underview Renderer GLTF Loader Material Create Information)
 *
 * members:
 * @gltfFile  - Must pass a valid struct uvr_gltf_loader_file for cgltf_data @gltfData member
 */
struct uvr_gltf_loader_material_create_info {
	struct uvr_gltf_loader_file gltfFile;
};


/*
 * uvr_gltf_loader_material_create: Function Loads necessary material information associated with gltf file into memory.
 *                                  To free @materialData call uvr_gltf_loader_destroy(3) or free(@materialData).
 *
 * args:
 * @uvrgltf - Must pass a pointer to a struct uvr_gltf_loader_material_create_info
 * return:
 *	on success struct uvr_gltf_loader_material { with member being pointer to an array }
 *	on failure struct uvr_gltf_loader_material { with member nulled }
 */
struct uvr_gltf_loader_material uvr_gltf_loader_material_create(struct uvr_gltf_loader_material_create_info *uvrgltf);


/*
 * enum uvr_gltf_loader_gltf_object_type (Underview Renderer GLTF Loader GLTF Object Type)
 */
enum uvr_gltf_loader_gltf_object_type {
	UVR_GLTF_LOADER_GLTF_NODE = 0x00000001,
	UVR_GLTF_LOADER_GLTF_MESH = 0x00000002,
	UVR_GLTF_LOADER_GLTF_SKIN = 0x00000003,
	UVR_GLTF_LOADER_GLTF_CAMERA = 0x00000004,
};


/*
 * struct uvr_gltf_loader_node_data (Underview Renderer GLTF Loader Node Data)
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
struct uvr_gltf_loader_node_data {
	enum uvr_gltf_loader_gltf_object_type objectType;
	uint32_t                              objectIndex;
	uint32_t                              nodeIndex;
	uint32_t                              parentNodeIndex;
	float                                 matrixTransform[4][4];
};


/*
 * struct uvr_gltf_loader_node (Underview Renderer GLTF Loader Node)
 *
 * members:
 * @nodeData      - Pointer to an array of struct uvr_gltf_loader_node_data
 * @nodeDataCount - Amount of elements in @nodeData array
 */
struct uvr_gltf_loader_node {
	struct uvr_gltf_loader_node_data *nodeData;
	uint32_t                         nodeDataCount;
};


/*
 * struct uvr_gltf_loader_node_create_info (Underview Renderer GLTF Loader Node Create Information)
 *
 * members:
 * @gltfLoaderFile - Must pass a valid struct uvr_gltf_loader_file for cgltf_data @gltfData member
 * @sceneIndex     - Index in GLTF file "scenes" array.
 */
struct uvr_gltf_loader_node_create_info {
	struct uvr_gltf_loader_file gltfLoaderFile;
	uint32_t                    sceneIndex;
};


/*
 * uvr_gltf_loader_node_create: Calculates final translation * rotatation * scale matrix for all nodes associated with a scene.
 *                              Along with final matrix transform data function also returns the parent and child index in the
 *                              GLTF file object "nodes" array, the type of node/object (i.e "mesh,skin,camera,etc..."), and
 *                              the index of that object in that GLTF file "Insert Object Name" array.
 *
 * args:
 * @uvrgltf - Must pass a pointer to a struct uvr_gltf_loader_node_create_info
 * return:
 *	on success struct uvr_gltf_loader_node { with member being pointer to an array }
 *	on failure struct uvr_gltf_loader_node { with member nulled }
 */
struct uvr_gltf_loader_node uvr_gltf_loader_node_create(struct uvr_gltf_loader_node_create_info *uvrgltf);


/*
 * uvr_gltf_loader_node_create: Prints out matrix transform for each node in struct uvr_gltf_loader_node { @nodeData }
 *
 * args:
 * @uvrgltf - Must pass a pointer to a struct uvr_gltf_loader_node
 */
void uvr_gltf_loader_node_display_matrix_transform(struct uvr_gltf_loader_node *uvrgltf);


/*
 * struct uvr_gltf_loader_destroy (Underview Renderer Shader Destroy)
 *
 * members:
 * @uvr_gltf_loader_file_cnt          - Must pass the amount of elements in struct uvr_gltf_loader_file array
 * @uvr_gltf_loader_file              - Must pass a pointer to an array of valid struct uvr_gltf_loader_file { free'd members: @gltfData }
 * @uvr_gltf_loader_vertex_cnt        - Must pass the amount of elements in struct uvr_gltf_loader_vertex array
 * @uvr_gltf_loader_vertex            - Must pass a pointer to an array of valid struct uvr_gltf_loader_vertex { free'd members: @verticesData }
 * @uvr_gltf_loader_texture_image_cnt - Must pass the amount of elements in struct uvr_gltf_loader_texture_image array
 * @uvr_gltf_loader_texture_image     - Must pass a pointer to an array of valid struct uvr_gltf_loader_texture_image { free'd members: @imageData, @pixels }
 * @uvr_gltf_loader_material_cnt      - Must pass the amount of elements in struct uvr_gltf_loader_material array
 * @uvr_gltf_loader_material          - Must pass a pointer to an array of valid struct uvr_gltf_loader_material { free'd members: @materialData }
 * @uvr_gltf_loader_node_cnt          - Must pass the amount of elements in struct uvr_gltf_loader_node array
 * @uvr_gltf_loader_node              - Must pass a pointer to an array of valid struct uvr_gltf_loader_node { free'd members: @nodeData }
 */
struct uvr_gltf_loader_destroy {
	uint32_t                             uvr_gltf_loader_file_cnt;
	struct uvr_gltf_loader_file          *uvr_gltf_loader_file;
	uint32_t                             uvr_gltf_loader_vertex_cnt;
	struct uvr_gltf_loader_vertex        *uvr_gltf_loader_vertex;
	uint32_t                             uvr_gltf_loader_texture_image_cnt;
	struct uvr_gltf_loader_texture_image *uvr_gltf_loader_texture_image;
	uint32_t                             uvr_gltf_loader_material_cnt;
	struct uvr_gltf_loader_material      *uvr_gltf_loader_material;
	uint32_t                             uvr_gltf_loader_node_cnt;
	struct uvr_gltf_loader_node          *uvr_gltf_loader_node;
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
