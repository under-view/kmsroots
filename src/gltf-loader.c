#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "gltf-loader.h"


/* TODO: move upstream in cglm */
static inline void glm_mat4_make(float *vec, mat4 dest)
{
	dest[0][0] = vec[0];  dest[0][1] = vec[1];   dest[0][2] = vec[2];  dest[0][3] = vec[3];
	dest[1][0] = vec[4];  dest[1][1] = vec[5];   dest[1][2] = vec[6];  dest[1][3] = vec[7];
	dest[2][0] = vec[8];  dest[2][1] = vec[9];   dest[2][2] = vec[10]; dest[2][3] = vec[11];
	dest[3][0] = vec[12]; dest[3][1] = vec[13];  dest[3][2] = vec[14]; dest[3][3] = vec[15];
}


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
 * struct uvr_gltf_loader_gltf_buffer_data (Underview Renderer GLTF Loader GLTF Buffer data)
 *
 * members:
 * @bufferType         - Stores the type of vertex data contained in buffer at offset of larger buffer
 * @byteOffset         - The byte offset of where the vertex indices are located within the larger buffer.
 * @bufferSize         - The size in bytes of buffer managed by buffer view
 * @bufferElementCount - The amount of elements in buffer
 * @bufferElementSize  - The size of each element contained within a buffer
 * @meshIndex          - Mesh associated with buffer
 */
struct uvr_gltf_loader_gltf_buffer_data {
	enum uvr_gltf_loader_vertex_type bufferType;
	uint32_t                         byteOffset;
	uint32_t                         bufferSize;
	uint32_t                         bufferElementCount;
	uint32_t                         bufferElementSize;
	uint16_t                         meshIndex;
};


struct uvr_gltf_loader_vertex uvr_gltf_loader_vertex_buffer_create(struct uvr_gltf_loader_vertex_buffer_create_info *uvrgltf)
{
	cgltf_size i, j, k, bufferViewElementType, bufferViewComponentType, meshCount, gltfBufferDataCount = 0;
	cgltf_buffer *buffer = NULL;
	cgltf_buffer_view *bufferView = NULL;
	cgltf_data *gltfData = uvrgltf->gltfFile.gltfData;

	uint32_t vertexBufferElementCounts[8], indexBufferElementCounts[8];
	struct uvr_gltf_loader_gltf_buffer_data *gltfBufferData = NULL;

	/* Retrieve amount of buffer views associate with buffers[uvrgltf->bufferIndex].buffer */
	for (i = 0; i < gltfData->meshes_count; i++) {
		for (j = 0; j < gltfData->meshes[i].primitives_count; j++) {
			for (k = 0; k < gltfData->meshes[i].primitives[j].attributes_count; k++) {
				bufferView = gltfData->meshes[i].primitives[j].attributes[k].data->buffer_view;
				buffer = bufferView->buffer;
				if (buffer->index != uvrgltf->bufferIndex) continue;
				gltfBufferDataCount++;
			}
			bufferView = gltfData->meshes[i].primitives[j].indices->buffer_view;
			buffer = bufferView->buffer;
			if (buffer->index != uvrgltf->bufferIndex) continue;
			gltfBufferDataCount++;
		}
	}

	gltfBufferData = alloca(gltfBufferDataCount * sizeof(struct uvr_gltf_loader_gltf_buffer_data));
	memset(gltfBufferData, 0, gltfBufferDataCount * sizeof(struct uvr_gltf_loader_gltf_buffer_data));
	memset(indexBufferElementCounts, 0, sizeof(indexBufferElementCounts));
	memset(vertexBufferElementCounts, 0, sizeof(vertexBufferElementCounts));
	gltfBufferDataCount=0;

	// TODO: account for accessor bufferOffset
	/*
	 * Retrieve important elements from buffer views/accessors associated with
	 * each GLTF mesh that's associated with buffers[uvrgltf->bufferIndex].buffer.
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
				 * Don't populated gltfBufferData array if the current bufferViews
				 * buffer not equal to the one we want
				 */
				if (buffer->index != uvrgltf->bufferIndex)
					continue;

				bufferViewElementType = gltfData->meshes[i].primitives[j].attributes[k].data->type;
				bufferViewComponentType = gltfData->meshes[i].primitives[j].attributes[k].data->component_type;

				gltfBufferData[gltfBufferDataCount].byteOffset = bufferView->offset;
				gltfBufferData[gltfBufferDataCount].bufferSize = bufferView->size;
				gltfBufferData[gltfBufferDataCount].meshIndex = meshCount = i;

				// Acquire accessor count
				gltfBufferData[gltfBufferDataCount].bufferElementCount = gltfData->meshes[i].primitives[j].attributes[k].data->count;
				vertexBufferElementCounts[i] = gltfBufferData[gltfBufferDataCount].bufferElementCount;

				switch (gltfData->meshes[i].primitives[j].attributes[k].type) {
					case cgltf_attribute_type_texcoord:
						gltfBufferData[gltfBufferDataCount].bufferElementSize = cgltf_calc_size(bufferViewElementType, bufferViewComponentType);
						gltfBufferData[gltfBufferDataCount].bufferType = UVR_GLTF_LOADER_VERTEX_TEXTURE;
						break;
					case cgltf_attribute_type_normal:
						gltfBufferData[gltfBufferDataCount].bufferElementSize = cgltf_calc_size(bufferViewElementType, bufferViewComponentType);
						gltfBufferData[gltfBufferDataCount].bufferType = UVR_GLTF_LOADER_VERTEX_NORMAL;
						break;
					case cgltf_attribute_type_color:
						gltfBufferData[gltfBufferDataCount].bufferElementSize = cgltf_calc_size(bufferViewElementType, bufferViewComponentType);
						gltfBufferData[gltfBufferDataCount].bufferType = UVR_GLTF_LOADER_VERTEX_COLOR;
						break;
					case cgltf_attribute_type_position:
						gltfBufferData[gltfBufferDataCount].bufferElementSize = cgltf_calc_size(bufferViewElementType, bufferViewComponentType);
						gltfBufferData[gltfBufferDataCount].bufferType = UVR_GLTF_LOADER_VERTEX_POSITION;
						break;
					case cgltf_attribute_type_tangent:
						gltfBufferData[gltfBufferDataCount].bufferElementSize = cgltf_calc_size(bufferViewElementType, bufferViewComponentType);
						gltfBufferData[gltfBufferDataCount].bufferType = UVR_GLTF_LOADER_VERTEX_TANGENT;
						break;
					default:
						continue;
				}

				gltfBufferDataCount++;
			}

			// Store index buffer data
			if (!gltfData->meshes[i].primitives[j].indices)
				continue;

			bufferView = gltfData->meshes[i].primitives[j].indices->buffer_view;
			buffer = bufferView->buffer;

			/*
			 * Don't populated gltfBufferData array if the current bufferViews
			 * buffer does not equal to the one we want
			 */
			if (buffer->index != uvrgltf->bufferIndex)
				continue;

			bufferViewElementType = gltfData->meshes[i].primitives[j].indices->type;
			bufferViewComponentType = gltfData->meshes[i].primitives[j].indices->component_type;

			gltfBufferData[gltfBufferDataCount].bufferElementCount = gltfData->meshes[i].primitives[j].indices->count;
			gltfBufferData[gltfBufferDataCount].bufferElementSize = cgltf_calc_size(bufferViewElementType, bufferViewComponentType);
			gltfBufferData[gltfBufferDataCount].bufferType = UVR_GLTF_LOADER_VERTEX_INDEX;
			gltfBufferData[gltfBufferDataCount].byteOffset = bufferView->offset;
			gltfBufferData[gltfBufferDataCount].bufferSize = bufferView->size;
			gltfBufferData[gltfBufferDataCount].meshIndex = meshCount = i;
			indexBufferElementCounts[i] = gltfBufferData[gltfBufferDataCount].bufferElementCount;
			gltfBufferDataCount++;
		}
	}
	/*
	 * To ensure accuracy with the amount of meshes associated with buffer
	 * @meshCount is assigned when meshIndex is assigned to give buffer
	 * in @gltfBufferData array. Do to loops beginning at 0 add 1 to final
	 * number to get an acurrate count.
	 */
	meshCount++;

	struct uvr_gltf_loader_vertex_mesh_data *meshData = NULL;
	uint32_t byteOffset, bufferElementSize, bufferElementCount, bufferType, bufferSize, vertexIndex;
	uint32_t prevMeshIndex = 0, firstIndex = 0;
	void *finalAddress = NULL;
	vec4 vec4Dest;

	meshData = calloc(meshCount, sizeof(struct uvr_gltf_loader_vertex_mesh_data));
	if (!meshData) {
		uvr_utils_log(UVR_DANGER, "[x] calloc(meshData): %s", strerror(errno));
		goto exit_error_uvr_gltf_loader_vertex_buffer_create;
	}


	for (j = 0; j < meshCount; j++) {
		meshData[j].vertexBufferDataCount = vertexBufferElementCounts[j];
		meshData[j].vertexBufferDataSize = vertexBufferElementCounts[j] * sizeof(struct uvr_gltf_loader_vertex_data);
		meshData[j].vertexBufferData = calloc(vertexBufferElementCounts[j], sizeof(struct uvr_gltf_loader_vertex_data));
		if (!meshData[j].vertexBufferData) {
			uvr_utils_log(UVR_DANGER, "[x] calloc(meshData[%u].vertexBufferData): %s", j, strerror(errno));
			goto exit_error_uvr_gltf_loader_vertex_buffer_create_free_meshData;
		}

		meshData[j].indexBufferDataCount = indexBufferElementCounts[j];
		meshData[j].indexBufferDataSize = indexBufferElementCounts[j] * sizeof(uint16_t);
		meshData[j].indexBufferData = calloc(indexBufferElementCounts[j], sizeof(uint16_t));
		if (!meshData[j].indexBufferData) {
			uvr_utils_log(UVR_DANGER, "[x] calloc(meshData[%u].indexBufferData): %s", j, strerror(errno));
			goto exit_error_uvr_gltf_loader_vertex_buffer_create_free_meshData;
		}
	}

	prevMeshIndex = gltfBufferData[0].meshIndex;
	j=0; // Represents the current index in meshes array.

	/*
	 * Converts GLTF buffer to
	 * 	- struct uvr_gltf_loader_vertex_data [] { .pos, .normal, .texCoord, .color }
	 * 	- struct uvr_gltf_loader_index_data [] { .indices }
	 */
	for (i = 0; i < gltfBufferDataCount; i++) {
		bufferType = gltfBufferData[i].bufferType;
		bufferElementCount = gltfBufferData[i].bufferElementCount;
		byteOffset = gltfBufferData[i].byteOffset;
		bufferElementSize = gltfBufferData[i].bufferElementSize;
		bufferSize = gltfBufferData[i].bufferSize;

		/*
		 * Update @curVertexDataIndex if @meshIndex changes
		 */
		if (prevMeshIndex != gltfBufferData[i].meshIndex) {
			prevMeshIndex = gltfBufferData[i].meshIndex;
			j++;
		}

		if (bufferType == UVR_GLTF_LOADER_VERTEX_INDEX) {
			// Base buffer data adress + base byte offset address = address in buffer where data resides
			finalAddress = uvrgltf->gltfFile.gltfData->buffers[uvrgltf->bufferIndex].data + byteOffset;
			memcpy(meshData[j].indexBufferData, finalAddress, bufferSize);

			meshData[j].firstIndex = firstIndex; // firstIndex = element in @indexBufferData | indexBufferData[firstIndex]
			firstIndex += bufferElementCount;
		} else {
			for (vertexIndex = 0; vertexIndex < bufferElementCount; vertexIndex++) {
				// Base buffer data adress + base byte offset address + (index * bufferElementSize) = address in buffer where data resides
				finalAddress = uvrgltf->gltfFile.gltfData->buffers[uvrgltf->bufferIndex].data + byteOffset + (vertexIndex * bufferElementSize);

				switch (bufferType) {
					case UVR_GLTF_LOADER_VERTEX_TEXTURE: // Texture Coordinate Buffer
						glm_vec2((float*)finalAddress, meshData[j].vertexBufferData[vertexIndex].texCoord);
						break;
					case UVR_GLTF_LOADER_VERTEX_NORMAL: // Normal buffer
						glm_vec3_normalize_to((float*)finalAddress, meshData[j].vertexBufferData[vertexIndex].normal);
						break;
					case UVR_GLTF_LOADER_VERTEX_POSITION: // position buffer
						glm_vec4((float*)finalAddress, 1.0f, vec4Dest);
						glm_vec3(vec4Dest, meshData[j].vertexBufferData[vertexIndex].position);
						break;
					default:
						continue;
				}

				// color buffer (want values all set to 1.0f)
				glm_vec3_one(meshData[j].vertexBufferData[vertexIndex].color);
			}
		}
	}

	return (struct uvr_gltf_loader_vertex) { .bufferIndex = uvrgltf->bufferIndex, .meshDataCount = meshCount, .meshData = meshData };

exit_error_uvr_gltf_loader_vertex_buffer_create_free_meshData:
	for (j = 0; j < meshCount; j++) {
		free(meshData[j].vertexBufferData);
		free(meshData[j].indexBufferData);
	}
	free(meshData);
exit_error_uvr_gltf_loader_vertex_buffer_create:
	return (struct uvr_gltf_loader_vertex) { .bufferIndex = 0, .meshDataCount = 0, .meshData = NULL };
}


struct uvr_gltf_loader_texture_image uvr_gltf_loader_texture_image_create(struct uvr_gltf_loader_texture_image_create_info *uvrgltf)
{
	struct uvr_gltf_loader_texture_image_data *imageData = NULL;
	uint32_t curImage = 0, totalBufferSize = 0;
	char *imageFile = NULL;

	cgltf_data *gltfData = uvrgltf->gltfFile.gltfData;

	imageData = calloc(gltfData->images_count, sizeof(struct uvr_gltf_loader_texture_image_data));
	if (!imageData) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_uvr_gltf_loader_texture_image_create;
	}

	/* Load all images associated with GLTF file into memory */
	for (curImage = 0; curImage < gltfData->images_count; curImage++) {
		imageFile = uvr_utils_concat_file_to_dir(uvrgltf->directory, gltfData->images[curImage].uri, (1<<8));

		imageData[curImage].pixels = (void *) stbi_load(imageFile, &imageData[curImage].imageWidth, &imageData[curImage].imageHeight, &imageData[curImage].imageChannels, STBI_rgb_alpha);
		if (!imageData[curImage].pixels) {
			uvr_utils_log(UVR_DANGER, "[x] stbi_load: failed to load %s", imageFile);
			free(imageFile); imageFile = NULL;
			goto exit_error_uvr_gltf_loader_texture_image_create_free_image_data;
		}

		if (imageData[curImage].imageChannels == 3) {
			imageData[curImage].imageChannels++;
		}

		free(imageFile); imageFile = NULL;
		imageData[curImage].imageSize = (imageData[curImage].imageWidth * imageData[curImage].imageHeight) * imageData[curImage].imageChannels;
		totalBufferSize += imageData[curImage].imageSize;
	}

	return (struct uvr_gltf_loader_texture_image) { .imageCount = gltfData->images_count, .totalBufferSize = totalBufferSize, .imageData = imageData };

exit_error_uvr_gltf_loader_texture_image_create_free_image_data:
	for (curImage = 0; curImage < gltfData->images_count; curImage++)
		if (imageData[curImage].pixels)
			stbi_image_free(imageData[curImage].pixels);
	free(imageData);
exit_error_uvr_gltf_loader_texture_image_create:
	return (struct uvr_gltf_loader_texture_image) { .imageCount = 0, .totalBufferSize = 0, .imageData = NULL };
}


struct uvr_gltf_loader_material uvr_gltf_loader_material_create(struct uvr_gltf_loader_material_create_info *uvrgltf)
{
	uint32_t i, j, materialCount = 0;
	cgltf_material *material = NULL;
	struct uvr_gltf_loader_material_data *materialData = NULL;

	cgltf_data *gltfData = uvrgltf->gltfFile.gltfData;

	for (i = 0; i < gltfData->meshes_count; i++) {
		for (j = 0; j < gltfData->meshes[i].primitives_count; j++) {
			material = gltfData->meshes[i].primitives[j].material;
			if (material) materialCount++;
		}
	}

	materialData = calloc(materialCount, sizeof(struct uvr_gltf_loader_material_data));
	if (!materialData) {
		uvr_utils_log(UVR_DANGER, "[x] calloc(materialData): %s", strerror(errno));
		goto exit_error_uvr_gltf_loader_material_create;
	}

	/*
	 * Mesh->primitive->material->pbr_metallic_roughness->base_color_texture
	 * Mesh->primitive->material->pbr_metallic_roughness->metallic_roughness_texture
	 * Mesh->primitive->material->normal_texture
	 * Mesh->primitive->material->occlusion_texture
	 */
	materialCount=0;
	for (i = 0; i < gltfData->meshes_count; i++) {
		for (j = 0; j < gltfData->meshes[i].primitives_count; j++) {
			material = gltfData->meshes[i].primitives[j].material;
			if (!material) continue;

			memcpy(materialData[materialCount].materialName, material->name,
			       strnlen(material->name, STRUCT_MEMBER_SIZE(struct uvr_gltf_loader_material_data, materialName)));

			/* Physically-Based Rendering Metallic Roughness Model */
			materialData[materialCount].pbrMetallicRoughness.baseColorTexture.textureIndex = material->pbr_metallic_roughness.base_color_texture.index;
			materialData[materialCount].pbrMetallicRoughness.baseColorTexture.imageIndex = material->pbr_metallic_roughness.base_color_texture.texture->image_index;
			materialData[materialCount].pbrMetallicRoughness.baseColorTexture.scale = material->pbr_metallic_roughness.base_color_texture.scale;

			materialData[materialCount].pbrMetallicRoughness.metallicRoughnessTexture.textureIndex = material->pbr_metallic_roughness.metallic_roughness_texture.index;
			materialData[materialCount].pbrMetallicRoughness.metallicRoughnessTexture.imageIndex = material->pbr_metallic_roughness.metallic_roughness_texture.texture->image_index;
			materialData[materialCount].pbrMetallicRoughness.metallicRoughnessTexture.scale = material->pbr_metallic_roughness.metallic_roughness_texture.scale;

			materialData[materialCount].pbrMetallicRoughness.metallicFactor = material->pbr_metallic_roughness.metallic_factor;
			materialData[materialCount].pbrMetallicRoughness.roughnessFactor = material->pbr_metallic_roughness.roughness_factor;
			memcpy(materialData[materialCount].pbrMetallicRoughness.baseColorFactor, material->pbr_metallic_roughness.base_color_factor,
			       STRUCT_MEMBER_SIZE(struct uvr_gltf_loader_cgltf_pbr_metallic_roughness, baseColorFactor));

			materialData[materialCount].normalTexture.textureIndex = material->normal_texture.index;
			materialData[materialCount].normalTexture.imageIndex = material->normal_texture.texture->image_index;
			materialData[materialCount].normalTexture.scale = material->normal_texture.scale;

			materialData[materialCount].occlusionTexture.textureIndex = material->occlusion_texture.index;
			materialData[materialCount].occlusionTexture.imageIndex = material->occlusion_texture.texture->image_index;
			materialData[materialCount].occlusionTexture.scale = material->occlusion_texture.scale;

			materialData[materialCount].meshIndex = i;
			materialCount++;
		}
	}

	return (struct uvr_gltf_loader_material) { .materialDataCount = materialCount, .materialData = materialData };
exit_error_uvr_gltf_loader_material_create:
	return (struct uvr_gltf_loader_material) { .materialDataCount = 0, .materialData = NULL };
}


static void UNUSED print_matrix(mat4 matrix, const char *matrixName)
{
	uint32_t i, j;
	fprintf(stdout, "Matrix (%s) = {\n", matrixName);
	for (i = 0; i < ARRAY_LEN(matrix[0]); i++) {
		fprintf(stdout, "\t");
		for (j = 0; j < ARRAY_LEN(matrix[0]); j++)
			fprintf(stdout, "   %f   ", matrix[i][j]);
		fprintf(stdout, "\n");
	}
	fprintf(stdout, "}\n\n");
}


struct uvr_gltf_loader_node uvr_gltf_loader_node_create(struct uvr_gltf_loader_node_create_info *uvrgltf)
{
	uint32_t n, c, nodeDataCount = 0;
	cgltf_node *parentNode = NULL, *childNode = NULL;
	struct uvr_gltf_loader_node_data *nodeData = NULL;

	cgltf_data *gltfData = uvrgltf->gltfLoaderFile.gltfData;

	/* Acquire amount of nodes associate with scene */
	for (n = 0; n < gltfData->scenes[uvrgltf->sceneIndex].nodes_count; n++) {
		parentNode = gltfData->scenes[uvrgltf->sceneIndex].nodes[n];
		nodeDataCount += parentNode->children_count;
	}

	/*
	 * For whatever odd reason some cglm functions don't appear to like CGLTF allocated heap memory.
	 * Define stack based variables and use memcpy to copy data from
	 * stack->heap & heap->stack.
	 */
	float matrix[16];
	vec4 rotation; vec3 translation, scale;
	mat4 parentNodeMatrix, childNodeMatrix, rotationMatrix;

	nodeData = calloc(nodeDataCount, sizeof(struct uvr_gltf_loader_node_data));
	if (!nodeData) {
		uvr_utils_log(UVR_DANGER, "[x] calloc(nodeData): %s", strerror(errno));
		goto exit_error_uvr_gltf_loader_material_create;
	}

	nodeDataCount = 0;
	for (n = 0; n < gltfData->scenes[uvrgltf->sceneIndex].nodes_count; n++) {
		parentNode = gltfData->scenes[uvrgltf->sceneIndex].nodes[n];

		/* Clear stack array */
		memset(translation, 0, sizeof(translation));
		memset(rotation, 0, sizeof(rotation));
		memset(scale, 0, sizeof(scale));
		memset(matrix, 0, sizeof(matrix));
		memset(rotationMatrix, 0, sizeof(rotationMatrix));
		memset(parentNodeMatrix, 0, sizeof(parentNodeMatrix));

		/* Copy from heap to stack */
		memcpy(translation, parentNode->translation, sizeof(translation));
		memcpy(rotation, parentNode->rotation, sizeof(rotation));
		memcpy(scale, parentNode->scale, sizeof(scale));
		memcpy(matrix, parentNode->matrix, sizeof(matrix));

		glm_mat4_identity(parentNodeMatrix);

		/*
		 * GLTF: The node's unit quaternion rotation in the order (x, y, z, w), where w is the scalar.
		 * NOTE: cglm stores quaternion as [x, y, z, w] in memory since v0.4.0 it was [w, x, y, z] before v0.4.0 ( v0.3.5 and earlier )
		 */
		if (parentNode->has_translation)
			glm_translate(parentNodeMatrix, translation);

		if (parentNode->has_rotation) {
			glm_quat_mat4(rotation, rotationMatrix);
			glm_mat4_mul(rotationMatrix, parentNodeMatrix, parentNodeMatrix);
		}

		if (parentNode->has_scale)
			glm_scale(parentNodeMatrix, scale);

		if (parentNode->has_matrix)
			glm_mat4_make(matrix, parentNodeMatrix); // Not CGLM function

		/* Start child node's loop */
		for (c = 0; c < parentNode->children_count; c++) {
			childNode = parentNode->children[c];

			/* Clear stack array */
			memset(translation, 0, sizeof(translation));
			memset(rotation, 0, sizeof(rotation));
			memset(scale, 0, sizeof(scale));
			memset(matrix, 0, sizeof(matrix));
			memset(childNodeMatrix, 0, sizeof(childNodeMatrix));
			memset(rotationMatrix, 0, sizeof(rotationMatrix));

			/* Copy from heap to stack */
			memcpy(translation, childNode->translation, sizeof(translation));
			memcpy(rotation, childNode->rotation, sizeof(rotation));
			memcpy(scale, childNode->scale, sizeof(scale));
			memcpy(matrix, childNode->matrix, sizeof(matrix));

			glm_mat4_identity(childNodeMatrix);
			if (childNode->has_translation)
				glm_translate(childNodeMatrix, translation);

			if (childNode->has_rotation) {
				glm_quat_mat4(rotation, rotationMatrix);
				glm_mat4_mul(rotationMatrix, childNodeMatrix, childNodeMatrix);
			}

			if (childNode->has_scale)
				glm_scale(childNodeMatrix, scale);

			if (childNode->has_matrix)
				glm_mat4_make(matrix, childNodeMatrix); // Not CGLM function

			/* Multiply the parent matrix by the child */
			glm_mat4_mul(parentNodeMatrix, childNodeMatrix, childNodeMatrix);

			/* copy final stack matrix into heap memory matrix */
			memcpy(nodeData[nodeDataCount].matrixTransform, childNodeMatrix, sizeof(childNodeMatrix));

			if (childNode->skin) {
				nodeData[nodeDataCount].objectIndex = childNode->skin->skin_index;
				nodeData[nodeDataCount].objectType = UVR_GLTF_LOADER_GLTF_SKIN;
			} else if (childNode->mesh) {
				nodeData[nodeDataCount].objectIndex = childNode->mesh->mesh_index;
				nodeData[nodeDataCount].objectType = UVR_GLTF_LOADER_GLTF_MESH;
			} else if (childNode->camera) {
				nodeData[nodeDataCount].objectIndex = childNode->camera->camera_index;
				nodeData[nodeDataCount].objectType = UVR_GLTF_LOADER_GLTF_CAMERA;
			} else {
				nodeData[nodeDataCount].objectIndex = childNode->node_index;
				nodeData[nodeDataCount].objectType = UVR_GLTF_LOADER_GLTF_NODE;
			}

			nodeData[nodeDataCount].parentNodeIndex = parentNode->node_index;
			nodeData[nodeDataCount].nodeIndex = childNode->node_index;
			nodeDataCount++;
		}
	}

	return (struct uvr_gltf_loader_node) { .nodeDataCount = nodeDataCount, .nodeData = nodeData };
exit_error_uvr_gltf_loader_material_create:
	return (struct uvr_gltf_loader_node) { .nodeDataCount = 0, .nodeData = NULL };
}


void uvr_gltf_loader_node_display_matrix_transform(struct uvr_gltf_loader_node *uvrgltf)
{
	uint32_t n, i, j;

	const char *objectNames[] = {
		[UVR_GLTF_LOADER_GLTF_NODE] = "nodes",
		[UVR_GLTF_LOADER_GLTF_MESH] = "meshes",
		[UVR_GLTF_LOADER_GLTF_SKIN] = "skins",
		[UVR_GLTF_LOADER_GLTF_CAMERA] = "cameras",
	};

	fprintf(stdout, "\nGLTF File \"nodes\" Array Matrix Transforms = [\n");
	for (n = 0; n < uvrgltf->nodeDataCount; n++) {
		fprintf(stdout, "[Parent:Child] [%s[%u]]\n", objectNames[uvrgltf->nodeData[n].objectType], uvrgltf->nodeData[n].objectIndex);
		fprintf(stdout, "\t[%u:%u] = {\n", uvrgltf->nodeData[n].parentNodeIndex, uvrgltf->nodeData[n].nodeIndex);
		for (i = 0; i < 4; i++) {
			fprintf(stdout, "\t");
			for (j = 0; j < 4; j++)
				fprintf(stdout, "   %f   ", uvrgltf->nodeData[n].matrixTransform[i][j]);
			fprintf(stdout, "\n");
		}
		fprintf(stdout, "\t}\n\n");
	}
	fprintf(stdout, "]\n\n");
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
		for (j = 0; j < uvrgltf->uvr_gltf_loader_vertex[i].meshDataCount; j++) {
			if (uvrgltf->uvr_gltf_loader_vertex[i].meshData[j].vertexBufferData) { // Gating to prevent accident double free'ing
				free(uvrgltf->uvr_gltf_loader_vertex[i].meshData[i].vertexBufferData);
				uvrgltf->uvr_gltf_loader_vertex[i].meshData[i].vertexBufferData = NULL;
			}
			if (uvrgltf->uvr_gltf_loader_vertex[i].meshData[j].indexBufferData) { // Gating to prevent accident double free'ing
				free(uvrgltf->uvr_gltf_loader_vertex[i].meshData[j].indexBufferData);
				uvrgltf->uvr_gltf_loader_vertex[i].meshData[j].indexBufferData = NULL;
			}
		}
		if (uvrgltf->uvr_gltf_loader_vertex[i].meshData) {
			free(uvrgltf->uvr_gltf_loader_vertex[i].meshData);
			uvrgltf->uvr_gltf_loader_vertex[i].meshData = NULL;
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
	for (i = 0; i < uvrgltf->uvr_gltf_loader_material_cnt; i++) {
		if (uvrgltf->uvr_gltf_loader_material[i].materialData) {
			free(uvrgltf->uvr_gltf_loader_material[i].materialData);
			uvrgltf->uvr_gltf_loader_material[i].materialData = NULL;
		}
	}
	for (i = 0; i < uvrgltf->uvr_gltf_loader_node_cnt; i++) {
		if (uvrgltf->uvr_gltf_loader_node[i].nodeData) {
			free(uvrgltf->uvr_gltf_loader_node[i].nodeData);
			uvrgltf->uvr_gltf_loader_node[i].nodeData = NULL;
		}
	}
}
