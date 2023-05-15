#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "gltf-loader.h"


struct kmr_gltf_loader_file kmr_gltf_loader_file_load(struct kmr_gltf_loader_file_load_info *kmsgltf)
{
	cgltf_result res = cgltf_result_max_enum;
	cgltf_data *gltfData = NULL;

	cgltf_options options;
	memset(&options, 0, sizeof(cgltf_options));

	res = cgltf_parse_file(&options, kmsgltf->fileName, &gltfData);
	if (res != cgltf_result_success) {
		kmr_utils_log(KMR_DANGER, "[x] cgltf_parse_file: Could not load %s", kmsgltf->fileName);
		goto exit_error_kmr_gltf_loader_file_load;
	}

	res = cgltf_load_buffers(&options, gltfData, kmsgltf->fileName);
	if (res != cgltf_result_success) {
		kmr_utils_log(KMR_DANGER, "[x] cgltf_load_buffers: Could not load buffers in %s", kmsgltf->fileName);
		goto exit_error_kmr_gltf_loader_file_load_free_gltfData;
	}

	res = cgltf_validate(gltfData);
	if (res != cgltf_result_success) {
		kmr_utils_log(KMR_DANGER, "[x] cgltf_validate: Failed to load content in %s", kmsgltf->fileName);
		goto exit_error_kmr_gltf_loader_file_load_free_gltfData;
	}

	return (struct kmr_gltf_loader_file) { .gltfData = gltfData };

exit_error_kmr_gltf_loader_file_load_free_gltfData:
	if (gltfData)
		cgltf_free(gltfData);
exit_error_kmr_gltf_loader_file_load:
	return (struct kmr_gltf_loader_file) { .gltfData = NULL };
}


struct kmr_gltf_loader_mesh kmr_gltf_loader_mesh_create(struct kmr_gltf_loader_mesh_create_info *kmsgltf)
{
	cgltf_size i, j, k;
	uint32_t bufferOffset, bufferType, bufferViewElementType, bufferViewComponentType;
	uint32_t bufferElementCount, bufferElementSize, vertexIndex, firstIndex = 0, meshCount = 0;

	cgltf_buffer_view *bufferView = NULL;
	cgltf_data *gltfData = kmsgltf->gltfLoaderFile.gltfData;

	vec4 vec4Dest;
	void *finalAddress = NULL;
	struct kmr_gltf_loader_mesh_mesh_data *meshData = NULL;

	/* Allocate large enough buffer to store all mesh data in array */
	meshData = calloc(gltfData->meshes_count, sizeof(struct kmr_gltf_loader_mesh_mesh_data));
	if (!meshData) {
		kmr_utils_log(KMR_DANGER, "[x] calloc(meshData): %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_mesh_create;
	}

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
					meshData[i].vertexBufferData = calloc(bufferElementCount, sizeof(struct kmr_gltf_loader_mesh_data));
					if (!meshData[i].vertexBufferData) {
						kmr_utils_log(KMR_DANGER, "[x] calloc(meshData[%u].vertexBufferData): %s", i, strerror(errno));
					}

					meshData[i].vertexBufferDataCount = bufferElementCount;
					meshData[i].vertexBufferDataSize = bufferElementCount * sizeof(struct kmr_gltf_loader_mesh_data);
				}

				for (vertexIndex = 0; vertexIndex < bufferElementCount; vertexIndex++) {
					// Base buffer data adress + base byte offset address + (index * bufferElementSize) = address in buffer where data resides
					finalAddress = gltfData->buffers[kmsgltf->bufferIndex].data + bufferOffset + (vertexIndex * bufferElementSize);

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
					goto exit_error_kmr_gltf_loader_mesh_create_free_meshData;
				}

				meshData[i].indexBufferDataCount = bufferElementCount;
				meshData[i].indexBufferDataSize = bufferElementCount * sizeof(uint32_t);
			}

			for (vertexIndex = 0; vertexIndex < bufferElementCount; vertexIndex++) {
				// Base buffer data adress + base byte offset address + (index * bufferElementSize) = address in buffer where data resides
				finalAddress = gltfData->buffers[kmsgltf->bufferIndex].data + bufferOffset + (vertexIndex * bufferElementSize);
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
						goto exit_error_kmr_gltf_loader_mesh_create_free_meshData;
				}
			}

			meshData[i].firstIndex = firstIndex;
			firstIndex += bufferElementCount;
		}
	}

	return (struct kmr_gltf_loader_mesh) { .bufferIndex = kmsgltf->bufferIndex, .meshDataCount = gltfData->meshes_count, .meshData = meshData };

exit_error_kmr_gltf_loader_mesh_create_free_meshData:
	for (i = 0; i < meshCount; i++) {
		free(meshData[i].vertexBufferData);
		free(meshData[i].indexBufferData);
	}
	free(meshData);
exit_error_kmr_gltf_loader_mesh_create:
	return (struct kmr_gltf_loader_mesh) { .bufferIndex = 0, .meshDataCount = 0, .meshData = NULL };
}


struct kmr_gltf_loader_texture_image kmr_gltf_loader_texture_image_create(struct kmr_gltf_loader_texture_image_create_info *kmsgltf)
{
	struct kmr_utils_image_buffer *imageData = NULL;
	uint32_t curImage = 0, totalBufferSize = 0;

	cgltf_data *gltfData = kmsgltf->gltfLoaderFile.gltfData;

	imageData = calloc(gltfData->images_count, sizeof(struct kmr_utils_image_buffer));
	if (!imageData) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_texture_image_create;
	}

	struct kmr_utils_image_buffer_create_info imageDataCreateInfo;
	imageDataCreateInfo.maxStrLen = (1<<8);

	/* Load all images associated with GLTF file into memory */
	for (curImage = 0; curImage < gltfData->images_count; curImage++) {
		imageDataCreateInfo.directory = kmsgltf->directory;
		imageDataCreateInfo.filename = gltfData->images[curImage].uri;
		imageData[curImage] = kmr_utils_image_buffer_create(&imageDataCreateInfo);
		if (!imageData[curImage].pixels)
			goto exit_error_kmr_gltf_loader_texture_image_create_free_image_data;
		imageData[curImage].imageBufferOffset = totalBufferSize;
		totalBufferSize += imageData[curImage].imageSize;
	}

	return (struct kmr_gltf_loader_texture_image) { .imageCount = gltfData->images_count, .totalBufferSize = totalBufferSize, .imageData = imageData };

exit_error_kmr_gltf_loader_texture_image_create_free_image_data:
	for (curImage = 0; curImage < gltfData->images_count; curImage++)
		if (imageData[curImage].pixels)
			free(imageData[curImage].pixels);
	free(imageData);
exit_error_kmr_gltf_loader_texture_image_create:
	return (struct kmr_gltf_loader_texture_image) { .imageCount = 0, .totalBufferSize = 0, .imageData = NULL };
}


struct kmr_gltf_loader_material kmr_gltf_loader_material_create(struct kmr_gltf_loader_material_create_info *kmsgltf)
{
	uint32_t i, j, materialCount = 0;
	cgltf_material *material = NULL;
	struct kmr_gltf_loader_material_data *materialData = NULL;

	cgltf_data *gltfData = kmsgltf->gltfLoaderFile.gltfData;

	for (i = 0; i < gltfData->meshes_count; i++) {
		for (j = 0; j < gltfData->meshes[i].primitives_count; j++) {
			material = gltfData->meshes[i].primitives[j].material;
			if (material) materialCount++;
		}
	}

	materialData = calloc(materialCount, sizeof(struct kmr_gltf_loader_material_data));
	if (!materialData) {
		kmr_utils_log(KMR_DANGER, "[x] calloc(materialData): %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_material_create;
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
			       strnlen(material->name, STRUCT_MEMBER_SIZE(struct kmr_gltf_loader_material_data, materialName)));

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
			       STRUCT_MEMBER_SIZE(struct kmr_gltf_loader_cgltf_pbr_metallic_roughness, baseColorFactor));

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

	return (struct kmr_gltf_loader_material) { .materialDataCount = materialCount, .materialData = materialData };
exit_error_kmr_gltf_loader_material_create:
	return (struct kmr_gltf_loader_material) { .materialDataCount = 0, .materialData = NULL };
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


struct kmr_gltf_loader_node kmr_gltf_loader_node_create(struct kmr_gltf_loader_node_create_info *kmsgltf)
{
	uint32_t n, c, nodeDataCount = 0;
	cgltf_node *parentNode = NULL, *childNode = NULL;
	struct kmr_gltf_loader_node_data *nodeData = NULL;

	cgltf_data *gltfData = kmsgltf->gltfLoaderFile.gltfData;

	/* Acquire amount of nodes associate with scene */
	for (n = 0; n < gltfData->scenes[kmsgltf->sceneIndex].nodes_count; n++) {
		parentNode = gltfData->scenes[kmsgltf->sceneIndex].nodes[n];
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

	nodeData = calloc(nodeDataCount, sizeof(struct kmr_gltf_loader_node_data));
	if (!nodeData) {
		kmr_utils_log(KMR_DANGER, "[x] calloc(nodeData): %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_material_create;
	}

	nodeDataCount = 0;
	for (n = 0; n < gltfData->scenes[kmsgltf->sceneIndex].nodes_count; n++) {
		parentNode = gltfData->scenes[kmsgltf->sceneIndex].nodes[n];

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
				nodeData[nodeDataCount].objectType = KMR_GLTF_LOADER_GLTF_SKIN;
			} else if (childNode->mesh) {
				nodeData[nodeDataCount].objectIndex = childNode->mesh->mesh_index;
				nodeData[nodeDataCount].objectType = KMR_GLTF_LOADER_GLTF_MESH;
			} else if (childNode->camera) {
				nodeData[nodeDataCount].objectIndex = childNode->camera->camera_index;
				nodeData[nodeDataCount].objectType = KMR_GLTF_LOADER_GLTF_CAMERA;
			} else {
				nodeData[nodeDataCount].objectIndex = childNode->node_index;
				nodeData[nodeDataCount].objectType = KMR_GLTF_LOADER_GLTF_NODE;
			}

			nodeData[nodeDataCount].parentNodeIndex = parentNode->node_index;
			nodeData[nodeDataCount].nodeIndex = childNode->node_index;
			nodeDataCount++;
		}
	}

	return (struct kmr_gltf_loader_node) { .nodeDataCount = nodeDataCount, .nodeData = nodeData };
exit_error_kmr_gltf_loader_material_create:
	return (struct kmr_gltf_loader_node) { .nodeDataCount = 0, .nodeData = NULL };
}


void kmr_gltf_loader_node_display_matrix_transform(struct kmr_gltf_loader_node *kmsgltf)
{
	uint32_t n, i, j;

	const char *objectNames[] = {
		[KMR_GLTF_LOADER_GLTF_NODE] = "nodes",
		[KMR_GLTF_LOADER_GLTF_MESH] = "meshes",
		[KMR_GLTF_LOADER_GLTF_SKIN] = "skins",
		[KMR_GLTF_LOADER_GLTF_CAMERA] = "cameras",
	};

	fprintf(stdout, "\nGLTF File \"nodes\" Array Matrix Transforms = [\n");
	for (n = 0; n < kmsgltf->nodeDataCount; n++) {
		fprintf(stdout, "[Parent:Child] [%s[%u]]\n", objectNames[kmsgltf->nodeData[n].objectType], kmsgltf->nodeData[n].objectIndex);
		fprintf(stdout, "\t[%u:%u] = {\n", kmsgltf->nodeData[n].parentNodeIndex, kmsgltf->nodeData[n].nodeIndex);
		for (i = 0; i < 4; i++) {
			fprintf(stdout, "\t");
			for (j = 0; j < 4; j++)
				fprintf(stdout, "   %f   ", kmsgltf->nodeData[n].matrixTransform[i][j]);
			fprintf(stdout, "\n");
		}
		fprintf(stdout, "\t}\n\n");
	}
	fprintf(stdout, "]\n\n");
}


void kmr_gltf_loader_destroy(struct kmr_gltf_loader_destroy *kmsgltf)
{
	uint32_t i, j;
	for (i = 0; i < kmsgltf->kmr_gltf_loader_file_cnt; i++) {
		if (kmsgltf->kmr_gltf_loader_file[i].gltfData) {
			cgltf_free(kmsgltf->kmr_gltf_loader_file[i].gltfData);
			kmsgltf->kmr_gltf_loader_file[i].gltfData = NULL;
		}
	}
	for (i = 0; i < kmsgltf->kmr_gltf_loader_mesh_cnt; i++) {
		for (j = 0; j < kmsgltf->kmr_gltf_loader_mesh[i].meshDataCount; j++) {
			if (kmsgltf->kmr_gltf_loader_mesh[i].meshData[j].vertexBufferData) { // Gating to prevent accident double free'ing
				free(kmsgltf->kmr_gltf_loader_mesh[i].meshData[i].vertexBufferData);
				kmsgltf->kmr_gltf_loader_mesh[i].meshData[i].vertexBufferData = NULL;
			}
			if (kmsgltf->kmr_gltf_loader_mesh[i].meshData[j].indexBufferData) { // Gating to prevent accident double free'ing
				free(kmsgltf->kmr_gltf_loader_mesh[i].meshData[j].indexBufferData);
				kmsgltf->kmr_gltf_loader_mesh[i].meshData[j].indexBufferData = NULL;
			}
		}
		if (kmsgltf->kmr_gltf_loader_mesh[i].meshData) {
			free(kmsgltf->kmr_gltf_loader_mesh[i].meshData);
			kmsgltf->kmr_gltf_loader_mesh[i].meshData = NULL;
		}
	}
	for (i = 0; i < kmsgltf->kmr_gltf_loader_texture_image_cnt; i++) {
		for (j = 0; j < kmsgltf->kmr_gltf_loader_texture_image[i].imageCount; j++) {
			if (kmsgltf->kmr_gltf_loader_texture_image[i].imageData[j].pixels) {
				free(kmsgltf->kmr_gltf_loader_texture_image[i].imageData[j].pixels);
				kmsgltf->kmr_gltf_loader_texture_image[i].imageData[j].pixels = NULL;
			}
		}
		if (kmsgltf->kmr_gltf_loader_texture_image[i].imageData) {
			free(kmsgltf->kmr_gltf_loader_texture_image[i].imageData);
			kmsgltf->kmr_gltf_loader_texture_image[i].imageData = NULL;
		}
	}
	for (i = 0; i < kmsgltf->kmr_gltf_loader_material_cnt; i++) {
		if (kmsgltf->kmr_gltf_loader_material[i].materialData) {
			free(kmsgltf->kmr_gltf_loader_material[i].materialData);
			kmsgltf->kmr_gltf_loader_material[i].materialData = NULL;
		}
	}
	for (i = 0; i < kmsgltf->kmr_gltf_loader_node_cnt; i++) {
		if (kmsgltf->kmr_gltf_loader_node[i].nodeData) {
			free(kmsgltf->kmr_gltf_loader_node[i].nodeData);
			kmsgltf->kmr_gltf_loader_node[i].nodeData = NULL;
		}
	}
}
