#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "gltf-loader.h"


/************************************************************
 * START OF kmr_gltf_loader_file_{create,destroy} FUNCTIONS *
 ************************************************************/

struct kmr_gltf_loader_file *
kmr_gltf_loader_file_create (struct kmr_gltf_loader_file_create_info *gltfFileInfo)
{
	cgltf_options options;
	cgltf_result res = cgltf_result_max_enum;

	struct kmr_gltf_loader_file *gltfFile = NULL;

	gltfFile = calloc(1, sizeof(struct kmr_gltf_loader_file));
	if (!gltfFile) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		return NULL;
	}

	memset(&options, 0, sizeof(cgltf_options));
	res = cgltf_parse_file(&options, gltfFileInfo->fileName, &gltfFile->gltfData);
	if (res != cgltf_result_success) {
		kmr_utils_log(KMR_DANGER, "[x] cgltf_parse_file: Could not load %s", gltfFileInfo->fileName);
		goto exit_error_kmr_gltf_loader_file_load;
	}

	res = cgltf_load_buffers(&options, gltfFile->gltfData, gltfFileInfo->fileName);
	if (res != cgltf_result_success) {
		kmr_utils_log(KMR_DANGER, "[x] cgltf_load_buffers: Could not load buffers in %s", gltfFileInfo->fileName);
		goto exit_error_kmr_gltf_loader_file_load;
	}

	res = cgltf_validate(gltfFile->gltfData);
	if (res != cgltf_result_success) {
		kmr_utils_log(KMR_DANGER, "[x] cgltf_validate: Failed to load content in %s", gltfFileInfo->fileName);
		goto exit_error_kmr_gltf_loader_file_load;
	}

	return gltfFile;

exit_error_kmr_gltf_loader_file_load:
	kmr_gltf_loader_file_destroy(gltfFile);
	return NULL;
}


void
kmr_gltf_loader_file_destroy (struct kmr_gltf_loader_file *gltfFile)
{
	if (!gltfFile)
		return;

	cgltf_free(gltfFile->gltfData);
	free(gltfFile);
}

/**********************************************************
 * END OF kmr_gltf_loader_file_{create,destroy} FUNCTIONS *
 **********************************************************/


/************************************************************
 * START OF kmr_gltf_loader_mesh_{create,destroy} FUNCTIONS *
 ************************************************************/

struct kmr_gltf_loader_mesh *
kmr_gltf_loader_mesh_create (struct kmr_gltf_loader_mesh_create_info *meshInfo)
{
	cgltf_size i, j, k;
	uint32_t bufferOffset, bufferType, bufferViewElementType, bufferViewComponentType;
	uint32_t bufferElementCount, bufferElementSize, vertexIndex, firstIndex = 0;

	cgltf_data *gltfData = NULL;
	cgltf_buffer_view *bufferView = NULL;

	vec4 vec4Dest;
	void *finalAddress = NULL;

	struct kmr_gltf_loader_mesh *mesh = NULL;
	struct kmr_gltf_loader_mesh_data *meshData = NULL;

	mesh = calloc(1, sizeof(struct kmr_gltf_loader_mesh));
	if (!mesh) {
		kmr_utils_log(KMR_DANGER, "[x] calloc(mesh): %s", strerror(errno));
		return NULL;
	}

	/* Allocate large enough buffer to store all mesh data in array */
	gltfData = meshInfo->gltfFile->gltfData;
	meshData = calloc(gltfData->meshes_count, sizeof(struct kmr_gltf_loader_mesh_data));
	if (!meshData) {
		kmr_utils_log(KMR_DANGER, "[x] calloc(meshData): %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_mesh_create;
	}

	mesh->meshData = meshData;
	mesh->meshDataCount = gltfData->meshes_count;

	// TODO: account for accessor bufferOffset
	/*
	 * Retrieve important elements from buffer views/accessors associated with
	 * each GLTF mesh that's associated with buffers[kmsgltf->bufferIndex].buffer.
	 * Mesh->primitive->attribute->accessor->bufferView->buffer
	 * Mesh->primitive->indices->accessor->bufferView->buffer
	 * Normally would want to avoid, but in this case it's fine
	 */
	for (i = 0; i < gltfData->meshes_count; i++) {
		for (j = 0; j < gltfData->meshes[i].primitives_count; j++) {
			for (k = 0; k < gltfData->meshes[i].primitives[j].attributes_count; k++) {

				// bufferView associated with accessor which is associated with a mesh->primitive->attribute
				bufferView = gltfData->meshes[i].primitives[j].attributes[k].data->buffer_view;

				bufferOffset = bufferView->offset;
				bufferViewElementType = gltfData->meshes[i].primitives[j].attributes[k].data->type;
				bufferViewComponentType = gltfData->meshes[i].primitives[j].attributes[k].data->component_type;
				bufferElementCount = gltfData->meshes[i].primitives[j].attributes[k].data->count;
				bufferElementSize = cgltf_calc_size(bufferViewElementType, bufferViewComponentType);
				bufferType = gltfData->meshes[i].primitives[j].attributes[k].type;

				if (!meshData[i].vertexBufferData) {
					meshData[i].vertexBufferData = calloc(bufferElementCount, sizeof(struct kmr_gltf_loader_mesh_vertex_data));
					if (!meshData[i].vertexBufferData) {
						kmr_utils_log(KMR_DANGER, "[x] calloc(meshData[%u].vertexBufferData): %s", i, strerror(errno));
					}

					meshData[i].vertexBufferDataCount = bufferElementCount;
					meshData[i].vertexBufferDataSize = bufferElementCount * sizeof(struct kmr_gltf_loader_mesh_vertex_data);
				}

				for (vertexIndex = 0; vertexIndex < bufferElementCount; vertexIndex++) {
					// Base buffer data adress + base byte offset address + (index * bufferElementSize) = address in buffer where data resides
					finalAddress = gltfData->buffers[meshInfo->bufferIndex].data + bufferOffset + (vertexIndex * bufferElementSize);

					switch (bufferType) {
						case cgltf_attribute_type_texcoord: // Texture Coordinate Buffer
							glm_vec2((float*) finalAddress, meshData[i].vertexBufferData[vertexIndex].texCoord);
							break;
						case cgltf_attribute_type_normal: // Normal buffer
							glm_vec3_normalize_to((float*) finalAddress, meshData[i].vertexBufferData[vertexIndex].normal);
							break;
						case cgltf_attribute_type_position: // position buffer
							glm_vec4((float*) finalAddress, 1.0f, vec4Dest);
							glm_vec3(vec4Dest, meshData[i].vertexBufferData[vertexIndex].position);
							break;
						default:
							// color buffer (want values all set to 1.0f). if not defined in meshes->primitive->attribute
							glm_vec3_one(meshData[i].vertexBufferData[vertexIndex].color);
							break;
					}
				}
			}

			// Store index buffer data
			if (!gltfData->meshes[i].primitives[j].indices)
				continue;

			bufferView = gltfData->meshes[i].primitives[j].indices->buffer_view;
	
			bufferOffset = bufferView->offset;
			bufferViewElementType = gltfData->meshes[i].primitives[j].indices->type;
			bufferViewComponentType = gltfData->meshes[i].primitives[j].indices->component_type;
			bufferElementCount = gltfData->meshes[i].primitives[j].indices->count;
			bufferElementSize = cgltf_calc_size(bufferViewElementType, bufferViewComponentType);

			if (!meshData[i].indexBufferData) {
				meshData[i].indexBufferData = calloc(bufferElementCount, sizeof(uint32_t));
				if (!meshData[i].indexBufferData) {
					kmr_utils_log(KMR_DANGER, "[x] calloc(meshData[%u].indexBufferData): %s", i, strerror(errno));
					goto exit_error_kmr_gltf_loader_mesh_create;
				}

				meshData[i].indexBufferDataCount = bufferElementCount;
				meshData[i].indexBufferDataSize = bufferElementCount * sizeof(uint32_t);
			}

			for (vertexIndex = 0; vertexIndex < bufferElementCount; vertexIndex++) {
				// Base buffer data adress + base byte offset address + (index * bufferElementSize) = address in buffer where data resides
				finalAddress = gltfData->buffers[meshInfo->bufferIndex].data + bufferOffset + (vertexIndex * bufferElementSize);
				switch (bufferViewComponentType) {
					case cgltf_component_type_r_8u:
						meshData[i].indexBufferData[vertexIndex] = *((uint8_t*) finalAddress);
						break;
					case cgltf_component_type_r_16u:
						meshData[i].indexBufferData[vertexIndex] = *((uint16_t*) finalAddress);
						break;
					case cgltf_component_type_r_32u:
						meshData[i].indexBufferData[vertexIndex] = *((uint32_t*) finalAddress);
						break;
					default:
						kmr_utils_log(KMR_DANGER, "[x] Somethings gone horribly wrong here. GLTF buffer indices section doesn't have correct data type");
						goto exit_error_kmr_gltf_loader_mesh_create;
				}
			}

			meshData[i].firstIndex = firstIndex;
			firstIndex += bufferElementCount;
		}
	}

	return mesh;

exit_error_kmr_gltf_loader_mesh_create:
	kmr_gltf_loader_mesh_destroy(mesh);
	return NULL;
}


void
kmr_gltf_loader_mesh_destroy (struct kmr_gltf_loader_mesh *mesh)
{
	uint32_t i;

	if (!mesh)
		return;

	for (i = 0; i < mesh->meshDataCount; i++) {
		free(mesh->meshData[i].vertexBufferData);
		free(mesh->meshData[i].indexBufferData);
	}

	free(mesh->meshData);
	free(mesh);
}

/**********************************************************
 * END OF kmr_gltf_loader_mesh_{create,destroy} FUNCTIONS *
 **********************************************************/


/*********************************************************************
 * START OF kmr_gltf_loader_texture_image_{create,destroy} FUNCTIONS *
 *********************************************************************/

struct kmr_gltf_loader_texture_image *
kmr_gltf_loader_texture_image_create (struct kmr_gltf_loader_texture_image_create_info *textureImageInfo)
{
	cgltf_data *gltfData = NULL;
	uint32_t curImage = 0, totalBufferSize = 0;

	struct kmr_gltf_loader_texture_image *textureImage = NULL;
	struct kmr_utils_image_buffer_create_info imageDataCreateInfo;

	textureImage = calloc(1, sizeof(struct kmr_gltf_loader_texture_image));
	if (!textureImage) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_texture_image_create;
	}

	gltfData = textureImageInfo->gltfFile->gltfData;

	textureImage->imageData = calloc(gltfData->images_count, sizeof(struct kmr_utils_image_buffer));
	if (!textureImage->imageData) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_texture_image_create;
	}

	imageDataCreateInfo.maxStrLen = (1<<8);

	/* Load all images associated with GLTF file into memory */
	for (curImage = 0; curImage < gltfData->images_count; curImage++) {
		imageDataCreateInfo.directory = textureImageInfo->directory;
		imageDataCreateInfo.filename = gltfData->images[curImage].uri;

		textureImage->imageData[curImage] = kmr_utils_image_buffer_create(&imageDataCreateInfo);
		if (!textureImage->imageData[curImage].pixels)
			goto exit_error_kmr_gltf_loader_texture_image_create;

		textureImage->imageData[curImage].imageBufferOffset = totalBufferSize;
		totalBufferSize += textureImage->imageData[curImage].imageSize;
	}

	textureImage->totalBufferSize = totalBufferSize;
	textureImage->imageCount = gltfData->images_count;
	return textureImage;

exit_error_kmr_gltf_loader_texture_image_create:
	kmr_gltf_loader_texture_image_destroy(textureImage);
	return NULL;
}


void
kmr_gltf_loader_texture_image_destroy (struct kmr_gltf_loader_texture_image *textureImage)
{
	uint32_t i;

	if (!textureImage)
		return;

	for (i=0; i < textureImage->imageCount; i++) {
		if (textureImage->imageData[i].pixels)
			free(textureImage->imageData[i].pixels);
	}

	free(textureImage->imageData);
	free(textureImage);
}


/*******************************************************************
 * END OF kmr_gltf_loader_texture_image_{create,destroy} FUNCTIONS *
 *******************************************************************/


/****************************************************************
 * START OF kmr_gltf_loader_material_{create,destroy} FUNCTIONS *
 ****************************************************************/

static uint32_t
material_count_get (cgltf_data *gltfData)
{
	uint32_t i, j, materialDataCount = 0;

	for (i = 0; i < gltfData->meshes_count; i++)
		for (j = 0; j < gltfData->meshes[i].primitives_count; j++)
			if (gltfData->meshes[i].primitives[j].material)
				materialDataCount++;

	return materialDataCount;
}


struct kmr_gltf_loader_material *
kmr_gltf_loader_material_create (struct kmr_gltf_loader_material_create_info *materialInfo)
{
	cgltf_data *gltfData = NULL;
	cgltf_material *gltfMaterial = NULL;
	uint32_t i, j, materialDataCount = 0;

	struct kmr_gltf_loader_material *material = NULL;
	struct kmr_gltf_loader_material_data *materialData = NULL;

	material = calloc(1, sizeof(struct kmr_gltf_loader_material));
	if (!material) {
		kmr_utils_log(KMR_DANGER, "[x] calloc(material): %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_material_create;
	}

	gltfData = materialInfo->gltfFile->gltfData;
	materialDataCount = material_count_get(gltfData);

	materialData = calloc(materialDataCount, sizeof(struct kmr_gltf_loader_material_data));
	if (!materialData) {
		kmr_utils_log(KMR_DANGER, "[x] calloc(materialData): %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_material_create;
	}

	material->materialData = materialData;

	/*
	 * Mesh->primitive->material->pbr_metallic_roughness->base_color_texture
	 * Mesh->primitive->material->pbr_metallic_roughness->metallic_roughness_texture
	 * Mesh->primitive->material->normal_texture
	 * Mesh->primitive->material->occlusion_texture
	 */
	materialDataCount=0;
	for (i = 0; i < gltfData->meshes_count; i++) {
		for (j = 0; j < gltfData->meshes[i].primitives_count; j++) {
			gltfMaterial = gltfData->meshes[i].primitives[j].material;
			if (!gltfMaterial) continue;

			materialData[materialDataCount].materialName = strndup(gltfMaterial->name, (1<<6));

			/* Physically-Based Rendering Metallic Roughness Model */
			materialData[materialDataCount].pbrMetallicRoughness.baseColorTexture.textureIndex = \
				cgltf_texture_index(gltfData, gltfMaterial->pbr_metallic_roughness.base_color_texture.texture);
			materialData[materialDataCount].pbrMetallicRoughness.baseColorTexture.imageIndex = \
				cgltf_image_index(gltfData, gltfMaterial->pbr_metallic_roughness.base_color_texture.texture->image);
			materialData[materialDataCount].pbrMetallicRoughness.baseColorTexture.scale = \
				gltfMaterial->pbr_metallic_roughness.base_color_texture.scale;
			materialData[materialDataCount].pbrMetallicRoughness.metallicRoughnessTexture.textureIndex = \
				cgltf_texture_index(gltfData, gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture);
			materialData[materialDataCount].pbrMetallicRoughness.metallicRoughnessTexture.imageIndex = \
				cgltf_image_index(gltfData, gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture->image);
			materialData[materialDataCount].pbrMetallicRoughness.metallicRoughnessTexture.scale = \
				gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.scale;

			materialData[materialDataCount].pbrMetallicRoughness.metallicFactor = gltfMaterial->pbr_metallic_roughness.metallic_factor;
			materialData[materialDataCount].pbrMetallicRoughness.roughnessFactor = gltfMaterial->pbr_metallic_roughness.roughness_factor;
			memcpy(materialData[materialDataCount].pbrMetallicRoughness.baseColorFactor,
			       gltfMaterial->pbr_metallic_roughness.base_color_factor,
			       STRUCT_MEMBER_SIZE(struct kmr_gltf_loader_cgltf_pbr_metallic_roughness, baseColorFactor));

			materialData[materialDataCount].normalTexture.scale = gltfMaterial->normal_texture.scale;
			materialData[materialDataCount].normalTexture.textureIndex = \
				cgltf_texture_index(gltfData, gltfMaterial->normal_texture.texture);
			materialData[materialDataCount].normalTexture.imageIndex = \
				cgltf_image_index(gltfData, gltfMaterial->normal_texture.texture->image);

			materialData[materialDataCount].occlusionTexture.scale = gltfMaterial->occlusion_texture.scale;
			materialData[materialDataCount].occlusionTexture.textureIndex = \
				cgltf_texture_index(gltfData, gltfMaterial->occlusion_texture.texture);
			materialData[materialDataCount].occlusionTexture.imageIndex = \
				cgltf_image_index(gltfData, gltfMaterial->occlusion_texture.texture->image);

			materialData[materialDataCount].meshIndex = i;
			materialDataCount++;
		}
	}

	material->materialDataCount = materialDataCount;
	return material;

exit_error_kmr_gltf_loader_material_create:
	kmr_gltf_loader_material_destroy(material);
	return NULL;
}


void
kmr_gltf_loader_material_destroy (struct kmr_gltf_loader_material *material)
{
	uint32_t i;

	if (!material)
		return;

	for (i=0; i < material->materialDataCount; i++) {
		free(material->materialData[i].materialName);
	}

	free(material->materialData);
	free(material);
}

/**************************************************************
 * END OF kmr_gltf_loader_material_{create,destroy} FUNCTIONS *
 **************************************************************/


/************************************************************
 * START OF kmr_gltf_loader_node_{create,destroy} FUNCTIONS *
 ************************************************************/

struct kmr_gltf_loader_node *
kmr_gltf_loader_node_create (struct kmr_gltf_loader_node_create_info *nodeInfo)
{
	uint32_t n, c, nodeDataCount = 0;

	float matrix[16];
	vec4 rotation; vec3 translation, scale;
	mat4 parentNodeMatrix, childNodeMatrix, rotationMatrix;

	cgltf_data *gltfData = NULL;
	cgltf_node *parentNode = NULL;
	cgltf_node *childNode = NULL;

	struct kmr_gltf_loader_node *node = NULL;
	struct kmr_gltf_loader_node_data *nodeData = NULL;

	node = calloc(1, sizeof(struct kmr_gltf_loader_node));
	if (!node) {
		kmr_utils_log(KMR_DANGER, "[x] calloc(node): %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_material_create;
	}

	gltfData = nodeInfo->gltfFile->gltfData;

	/* Acquire amount of nodes associate with scene */
	for (n = 0; n < gltfData->scenes[nodeInfo->sceneIndex].nodes_count; n++) {
		parentNode = gltfData->scenes[nodeInfo->sceneIndex].nodes[n];
		nodeDataCount += parentNode->children_count;
	}

	/*
	 * For whatever odd reason some cglm functions don't appear to like CGLTF allocated heap memory.
	 * Define stack based variables and use memcpy to copy data from
	 * stack->heap & heap->stack.
	 */

	nodeData = calloc(nodeDataCount, sizeof(struct kmr_gltf_loader_node_data));
	if (!nodeData) {
		kmr_utils_log(KMR_DANGER, "[x] calloc(nodeData): %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_material_create;
	}

	nodeDataCount = 0;
	node->nodeData = nodeData;
	for (n = 0; n < gltfData->scenes[nodeInfo->sceneIndex].nodes_count; n++) {
		parentNode = gltfData->scenes[nodeInfo->sceneIndex].nodes[n];

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
				nodeData[nodeDataCount].objectIndex = cgltf_skin_index(gltfData, childNode->skin);
				nodeData[nodeDataCount].objectType = KMR_GLTF_LOADER_GLTF_SKIN;
			} else if (childNode->mesh) {
				nodeData[nodeDataCount].objectIndex = cgltf_mesh_index(gltfData, childNode->mesh);
				nodeData[nodeDataCount].objectType = KMR_GLTF_LOADER_GLTF_MESH;
			} else if (childNode->camera) {
				nodeData[nodeDataCount].objectIndex = cgltf_camera_index(gltfData, childNode->camera);
				nodeData[nodeDataCount].objectType = KMR_GLTF_LOADER_GLTF_CAMERA;
			} else {
				nodeData[nodeDataCount].objectIndex = cgltf_node_index(gltfData, childNode);
				nodeData[nodeDataCount].objectType = KMR_GLTF_LOADER_GLTF_NODE;
			}

			nodeData[nodeDataCount].parentNodeIndex = cgltf_node_index(gltfData, parentNode);
			nodeData[nodeDataCount].nodeIndex = cgltf_node_index(gltfData, childNode);
			nodeDataCount++;
		}
	}

	node->nodeDataCount = nodeDataCount;
	return node;

exit_error_kmr_gltf_loader_material_create:
	kmr_gltf_loader_node_destroy(node);
	return NULL;
}


void
kmr_gltf_loader_node_destroy (struct kmr_gltf_loader_node *node)
{
	if (!node)
		return;

	free(node->nodeData);
	free(node);	
}

/**********************************************************
 * END OF kmr_gltf_loader_node_{create,destroy} FUNCTIONS *
 **********************************************************/


/********************************************************************
 * START OF kmr_gltf_loader_node_display_matrix_transform FUNCTIONS *
 ********************************************************************/

void
kmr_gltf_loader_node_display_matrix_transform (struct kmr_gltf_loader_node *nodeInfo)
{
	uint32_t n, i, j;

	const char *objectNames[] = {
		[KMR_GLTF_LOADER_GLTF_NODE] = "nodes",
		[KMR_GLTF_LOADER_GLTF_MESH] = "meshes",
		[KMR_GLTF_LOADER_GLTF_SKIN] = "skins",
		[KMR_GLTF_LOADER_GLTF_CAMERA] = "cameras",
	};

	fprintf(stdout, "\nGLTF File \"nodes\" Array Matrix Transforms = [\n");
	for (n = 0; n < nodeInfo->nodeDataCount; n++) {
		fprintf(stdout, "[Parent:Child] [%s[%u]]\n", objectNames[nodeInfo->nodeData[n].objectType], nodeInfo->nodeData[n].objectIndex);
		fprintf(stdout, "\t[%u:%u] = {\n", nodeInfo->nodeData[n].parentNodeIndex, nodeInfo->nodeData[n].nodeIndex);
		for (i = 0; i < 4; i++) {
			fprintf(stdout, "\t");
			for (j = 0; j < 4; j++)
				fprintf(stdout, "   %f   ", nodeInfo->nodeData[n].matrixTransform[i][j]);
			fprintf(stdout, "\n");
		}
		fprintf(stdout, "\t}\n\n");
	}
	fprintf(stdout, "]\n\n");
}

/*****************************************************************
 * END OF kmr_gltf_loader_node_display_matrix_transform FUNCTION *
 *****************************************************************/
