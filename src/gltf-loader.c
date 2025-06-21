#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include <cglm/cglm.h>
#include <cando/cando.h>

#include "gltf-loader.h"

/**********************************************
 * Start of kmr_gltf_loader_file_* functions. *
 **********************************************/

/*
 * @brief Structure defining kmsroots GLTF Loader File.
 *
 * @member err       - Stores information about the error that occured
 *                     for the given instance and may later be retrieved
 *                     by caller.
 * @member free      - If structure allocated with calloc(3) member will be
 *                     set to true so that, we know to call free(3) when
 *                     destroying the instance.
 * @member gltf_data - Buffer that stores a given gltf file's content.
 */
struct kmr_gltf_loader_file
{
	struct cando_log_error_struct err;
	bool                          free;
	cgltf_data                    *gltf_data;
};


struct kmr_gltf_loader_file *
kmr_gltf_loader_file_create (struct kmr_gltf_loader_file *p_gltf_file,
                             const void *p_gltf_info)
{
	cgltf_options options;

	cgltf_result res = cgltf_result_max_enum;

	struct kmr_gltf_loader_file *gltf_file = p_gltf_file;

	const struct kmr_gltf_loader_file_create_info *gltf_info = p_gltf_info;

	if (!gltf_file) {
		gltf_file = calloc(1, sizeof(struct kmr_gltf_loader_file));
		if (!gltf_file) {
			cando_log_error("calloc: %s", strerror(errno));
			return NULL;
		}
	}

	memset(&options, 0, sizeof(cgltf_options));
	res = cgltf_parse_file(&options, gltf_info->fname, &(gltf_file->gltf_data));
	if (res != cgltf_result_success) {
		cando_log_error("cgltf_parse_file: Could not load %s", gltf_info->fname);
		kmr_gltf_loader_file_destroy(gltf_file);
		return NULL;
	}

	res = cgltf_load_buffers(&options, gltf_file->gltf_data, gltf_info->fname);
	if (res != cgltf_result_success) {
		cando_log_error("cgltf_load_buffers: Could not load buffers in %s", gltf_info->fname);
		kmr_gltf_loader_file_destroy(gltf_file);
		return NULL;
	}

	res = cgltf_validate(gltf_file->gltf_data);
	if (res != cgltf_result_success) {
		cando_log_error("cgltf_validate: Failed to load content in %s", gltf_info->fname);
		kmr_gltf_loader_file_destroy(gltf_file);
		return NULL;
	}

	return gltf_file;
}


void
kmr_gltf_loader_file_destroy (struct kmr_gltf_loader_file *gltf)
{
	if (!gltf)
		return;

	cgltf_free(gltf->gltf_data);

	if (gltf->free) {
		free(gltf);
	} else {
		memset(gltf, 0, sizeof(struct kmr_gltf_loader_file));
	}
}


int
kmr_gltf_loader_file_get_sizeof (void)
{
	return sizeof(struct kmr_gltf_loader_file);
}

/*******************************************
 * End of kmr_gltf_loader_file_* functions *
 *******************************************/


/*********************************************
 * Start of kmr_gltf_loader_mesh_* functions *
 *********************************************/

/*
 * @brief Structure defining kmsroots GLTF Loader Mesh Vertex Data.
 *
 *        Struct member order is arbitrary. SHOULD NOT BE USED DIRECTLY.
 *        Advise to create second stack buffer and copy data over to it.
 *        Members populated with vertices from GLTF file buffer.
 *
 * @member position  - Vertex position coordinates.
 * @member normal    - Vertex normal (direction vertex points).
 * @member tex_coord - Texture coordinate.
 * @member color     - Color.
 */
struct kmr_gltf_loader_mesh_vertex_data
{
	vec3 position;
	vec3 normal;
	vec2 tex_coord;
	vec3 color;
};


/*
 * @brief Structure defining kmsroots GLTF Loader Mesh Data.
 *
 * @member first_index            - Array index within the index buffer. Calculated in kmr_gltf_loader_mesh_create()
 *                                  first_index = first_index + buff_element_count (GLTF file accessor[index].count).
 *                                  Can be used by the application to fill in vkCmdDrawIndexed(3) function.
 * @member index_buff_data        - Buffer of index data belonging to mesh populated from GLTF file buffer at
 *                                  struct kmr_gltf_loader_mesh { @buffer_idx }.
 * @member index_buff_data_count  - Amount of elements in @index_buff_data array.
 * @member index_buff_data_size   - The total size in bytes of the @index_buff_data array.
 * @member vertex_buff_data       - Pointer to a buffer containing position vertices, normal,
 *                                  texture coordinates, and color populated from GLTF file buffer at
 *                                  struct kmr_gltf_loader_mesh { @buffer_idx }.
 * @member vertex_buff_data_count - Amount of elements in @vertex_buff_data array.
 * @member vertex_buff_data_size  - The total size in bytes of the @vertex_buff_data array.
 */
struct kmr_gltf_loader_mesh_data
{
	uint32_t                                first_index;
	uint32_t                                index_buff_data[4096];
	uint32_t                                index_buff_data_count;
	uint32_t                                index_buff_data_size;
	struct kmr_gltf_loader_mesh_vertex_data vertex_buff_data[4096];
	uint32_t                                vertex_buff_data_count;
	uint32_t                                vertex_buff_data_size;
};


/*
 * @brief Structure defining kmsroots GLTF Loader Mesh.
 *
 * @member err             - Stores information about the error that occured
 *                           for the given instance and may later be retrieved
 *                           by caller.
 * @member free            - If structure allocated with calloc(3) member will be
 *                           set to true so that, we know to call free(3) when
 *                           destroying the instance.
 * @member buffer_idx      - The index in the "buffers" (json key) array of give GLTF file.
 * @member mesh_data_count - Amount of meshes associated with a @buffer_idx.
 *                           The array size of @mesh_data array.
 * @member mesh_data       - Pointer to an array of struct kmr_gltf_loader_mesh_data
 *                           storing all important data related to each mesh.
 */
struct kmr_gltf_loader_mesh
{
	struct cando_log_error_struct    err;
	bool                             free;
	uint16_t                         buffer_idx;
	uint16_t                         mesh_data_count;
	struct kmr_gltf_loader_mesh_data mesh_data[4096];
};


static void
p_populate_vertex_buffer (struct kmr_gltf_loader_mesh_data *mesh_data,
			  cgltf_accessor *vertices,
                          uint32_t buff_type,
                          void *data)
{
	vec4 vec4_dest;

	void *final_addr = NULL;

	uint32_t buff_offset, index;
	uint32_t buff_element_count, buff_element_size;
	uint32_t buff_view_element_type, buff_view_comp_type;

	/*
	 * buffer_view associated with accessor which
	 * is associated with a mesh->primitive->attribute.
	 */

	buff_offset = vertices->buffer_view->offset;
	buff_view_element_type = vertices->type;
	buff_view_comp_type = vertices->component_type;
	buff_element_count = vertices->count;
	buff_element_size = cgltf_calc_size(buff_view_element_type, buff_view_comp_type);

	mesh_data->vertex_buff_data_count = buff_element_count;
	mesh_data->vertex_buff_data_size = buff_element_count * sizeof(struct kmr_gltf_loader_mesh_vertex_data);

	for (index = 0; index < buff_element_count; index++) {
		/*
		 * Base buffer data adress + \
		 * base byte offset address + \
		 * (index * buff_element_size) = \
		 * address in buffer where data resides
		 */
		final_addr = data + buff_offset + (index * buff_element_size);

		switch (buff_type) {
			case cgltf_attribute_type_texcoord: /* Texture Coordinate Buffer */
				glm_vec2((float*) final_addr,
				mesh_data->vertex_buff_data[index].tex_coord);
				break;

			case cgltf_attribute_type_normal: /* Normal buffer */
				glm_vec3_normalize_to((float*) final_addr,
				mesh_data->vertex_buff_data[index].normal);
				break;

			case cgltf_attribute_type_position: /* Position buffer */
				glm_vec4((float*) final_addr, 1.0f, vec4_dest);
				glm_vec3(vec4_dest, mesh_data->vertex_buff_data[index].position);
				break;

			default:
				/*
				 * Color buffer (want values all set to 1.0f).
				 * If not defined in meshes->primitive->attribute.
				 */
				glm_vec3_one(mesh_data->vertex_buff_data[index].color);
				break;
		}
	}
}


static void
p_populate_index_buffer (struct kmr_gltf_loader_mesh_data *mesh_data,
                         cgltf_accessor *indices,
                         void *data,
                         uint32_t *first_index)
{
	void *final_addr = NULL;

	uint32_t buff_offset, index;
	uint32_t buff_element_count, buff_element_size;
	uint32_t buff_view_element_type, buff_view_comp_type;

	buff_offset = indices->buffer_view->offset;
	buff_view_element_type = indices->type;
	buff_view_comp_type = indices->component_type;
	buff_element_count = indices->count;
	buff_element_size = cgltf_calc_size(buff_view_element_type, buff_view_comp_type);

	mesh_data->index_buff_data_count = buff_element_count;
	mesh_data->index_buff_data_size = buff_element_count * sizeof(uint32_t);

	for (index = 0; index < buff_element_count; index++) {
		/*
		 * Base buffer data adress + \
		 * base byte offset address + \
		 * (index * buff_element_size) = \
		 * address in buffer where data resides
		 */
		final_addr = data + buff_offset + (index * buff_element_size);

		switch (buff_view_comp_type) {
			case cgltf_component_type_r_8u:
				mesh_data->index_buff_data[index] = *((uint8_t*)final_addr);
				break;

			case cgltf_component_type_r_16u:
				mesh_data->index_buff_data[index] = *((uint16_t*)final_addr);
				break;

			case cgltf_component_type_r_32u:
				mesh_data->index_buff_data[index] = *((uint32_t*)final_addr);
				break;

			default:
				cando_log_error("Somethings gone horribly wrong here. "
						"GLTF buffer indices section doesn't "
						"have correct data type\n");
				break;
		}
	}

	mesh_data->first_index = *first_index;
	*first_index += buff_element_count;
}


struct kmr_gltf_loader_mesh *
kmr_gltf_loader_mesh_create (struct kmr_gltf_loader_mesh *p_mesh,
                             const void *p_mesh_info)
{
	void *data = NULL;

	cgltf_size i, j, k;

	cgltf_data *gltf_data = NULL;

	uint32_t first_index = 0, buffer_type;

	struct kmr_gltf_loader_mesh *mesh = p_mesh;
	struct kmr_gltf_loader_mesh_data *mesh_data = NULL;

	cgltf_accessor *vertices = NULL, *indices = NULL;

	const struct kmr_gltf_loader_mesh_create_info *mesh_info = p_mesh_info;

	if (!mesh) {
		mesh = calloc(1, sizeof(struct kmr_gltf_loader_mesh));
		if (!mesh) {
			cando_log_error("calloc: %s", strerror(errno));
			return NULL;
		}
	}

	mesh_data = &(mesh->mesh_data[0]);
	gltf_data = mesh_info->gltf_file->gltf_data;
	mesh->mesh_data_count = gltf_data->meshes_count;
	data = gltf_data->buffers[mesh_info->buffer_idx].data;

	// TODO: account for accessor buff_offset

	/*
	 * Retrieve important elements from buffer views/accessors associated with
	 * each GLTF mesh that's associated with buffers[kmsgltf->buffer_idx].buffer.
	 * Mesh->primitive->attribute->accessor->buffer_view->buffer
	 * Mesh->primitive->indices->accessor->buffer_view->buffer
	 * Normally would want to avoid, but in this case it's fine
	 */
	for (i = 0; i < gltf_data->meshes_count; i++) {
		for (j = 0; j < gltf_data->meshes[i].primitives_count; j++) {
			for (k = 0; k < gltf_data->meshes[i].primitives[j].attributes_count; k++) {
				vertices = gltf_data->meshes[i].primitives[j].attributes[k].data;
				buffer_type = gltf_data->meshes[i].primitives[j].attributes[k].type;
				p_populate_vertex_buffer(&(mesh_data[i]), vertices, buffer_type, data);
			}

			indices = gltf_data->meshes[i].primitives[j].indices;
			if (!indices)
				continue;

			p_populate_index_buffer(&(mesh_data[i]), indices, data, &first_index);
		}
	}

	return mesh;
}


void
kmr_gltf_loader_mesh_destroy (struct kmr_gltf_loader_mesh *mesh)
{
	if (!mesh)
		return;

	if (mesh->free) {
		free(mesh);
	} else {
		memset(mesh, 0, sizeof(struct kmr_gltf_loader_mesh));
	}
}


int
kmr_gltf_loader_mesh_get_sizeof (void)
{
	return sizeof(struct kmr_gltf_loader_mesh);
}

/*******************************************
 * End of kmr_gltf_loader_mesh_* functions *
 *******************************************/


/******************************************************
 * Start of kmr_gltf_loader_image_texture_* functions *
 ******************************************************/

/*
 * @brief Structure defining kmsroots GLTF Loader Texture Image.
 *
 * @image_count      - Amount of images associated with a given GLTF file
 * @total_buff_sz - Collective size of each image associated with a given GLTF file.
 *                    Best utilized when creating single VkBuffer.
 * @imageData       - Pointer to an array of image metadata and pixel buffer.
 */
struct kmr_gltf_loader_image_texture
{
	uint32_t                      image_count;
	uint32_t                      total_buff_sz;
	struct kmr_utils_image_buffer *imageData;
};


struct kmr_gltf_loader_image_texture *
kmr_gltf_loader_image_texture_create (struct kmr_gltf_loader_image_texture *p_texture,
                                      const void *p_image_texture_info)
{
	cgltf_data *gltf_data = NULL;

	uint32_t cur_img = 0, total_buff_sz = 0;

	struct kmr_gltf_loader_image_texture *texture = p_texture;
	struct kmr_gltf_loader_image_texture_create_info *image_texture_info = p_image_texture_info;

	struct kmr_utils_image_buffer_create_info image_data_create_info;

	if (!texture) {
		texture = calloc(1, sizeof(struct kmr_gltf_loader_image_texture));
		if (!texture) {
			cando_log_error("calloc: %s", strerror(errno));
			return NULL;
		}
	}

	gltf_data = image_texture_info->gltf_file->gltf_data;

	texture->image_data = calloc(gltf_data->images_count, sizeof(struct kmr_utils_image_buffer));
	if (!texture->image_data) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_image_texture_create;
	}

	image_data_create_info.maxStrLen = (1<<8);

	/* Load all images associated with GLTF file into memory */
	for (cur_img = 0; cur_img < gltf_data->images_count; cur_img++) {
		image_data_create_info.directory = image_texture_info->directory;
		image_data_create_info.filename = gltf_data->images[cur_img].uri;

		texture->image_data[cur_img] = kmr_utils_image_buffer_create(&image_data_create_info);
		if (!(texture->image_data[cur_img].pixels)) {
			kmr_gltf_loader_image_texture_destroy(texture);
			return NULL;
		}

		texture->image_data[cur_img].imageBufferOffset = total_buff_sz;
		total_buff_sz += texture->image_data[cur_img].imageSize;
	}

	texture->total_buff_sz = total_buff_sz;
	texture->image_count = gltf_data->images_count;

	return texture;
}


void
kmr_gltf_loader_image_texture_destroy (struct kmr_gltf_loader_image_texture *texture)
{
	uint32_t i;

	if (!texture)
		return;

	for (i=0; i < texture->image_count; i++) {
		if (texture->image_data[i].pixels)
			free(texture->image_data[i].pixels);
	}

	free(texture->image_data);
	free(texture);
}


/*******************************************************************
 * END OF kmr_gltf_loader_image_texture_{create,destroy} FUNCTIONS *
 *******************************************************************/


/****************************************************************
 * START OF kmr_gltf_loader_material_{create,destroy} FUNCTIONS *
 ****************************************************************/

static uint32_t
material_count_get (cgltf_data *gltf_data)
{
	uint32_t i, j, material_data_count = 0;

	for (i = 0; i < gltf_data->meshes_count; i++)
		for (j = 0; j < gltf_data->meshes[i].primitives_count; j++)
			if (gltf_data->meshes[i].primitives[j].material)
				material_data_count++;

	return material_data_count;
}


struct kmr_gltf_loader_material *
kmr_gltf_loader_material_create (struct kmr_gltf_loader_material_create_info *materialInfo)
{
	cgltf_data *gltf_data = NULL;
	cgltf_material *gltfMaterial = NULL;
	uint32_t i, j, material_data_count = 0;

	struct kmr_gltf_loader_material *material = NULL;
	struct kmr_gltf_loader_material_data *material_data = NULL;

	material = calloc(1, sizeof(struct kmr_gltf_loader_material));
	if (!material) {
		kmr_utils_log(KMR_DANGER, "[x] calloc(material): %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_material_create;
	}

	gltf_data = materialInfo->gltf_file->gltf_data;
	material_data_count = material_count_get(gltf_data);

	material_data = calloc(material_data_count, sizeof(struct kmr_gltf_loader_material_data));
	if (!material_data) {
		kmr_utils_log(KMR_DANGER, "[x] calloc(material_data): %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_material_create;
	}

	material->material_data = material_data;

	/*
	 * Mesh->primitive->material->pbr_metallic_roughness->base_color_texture
	 * Mesh->primitive->material->pbr_metallic_roughness->metallic_roughness_texture
	 * Mesh->primitive->material->normal_texture
	 * Mesh->primitive->material->occlusion_texture
	 */
	material_data_count=0;
	for (i = 0; i < gltf_data->meshes_count; i++) {
		for (j = 0; j < gltf_data->meshes[i].primitives_count; j++) {
			gltfMaterial = gltf_data->meshes[i].primitives[j].material;
			if (!gltfMaterial) continue;

			material_data[material_data_count].material_name = strndup(gltfMaterial->name, (1<<6));

			/* Physically-Based Rendering Metallic Roughness Model */
			material_data[material_data_count].pbrMetallicRoughness.baseColorTexture.textureIndex = \
				cgltf_texture_index(gltf_data, gltfMaterial->pbr_metallic_roughness.base_color_texture.texture);
			material_data[material_data_count].pbrMetallicRoughness.baseColorTexture.imageIndex = \
				cgltf_image_index(gltf_data, gltfMaterial->pbr_metallic_roughness.base_color_texture.texture->image);
			material_data[material_data_count].pbrMetallicRoughness.baseColorTexture.scale = \
				gltfMaterial->pbr_metallic_roughness.base_color_texture.scale;
			material_data[material_data_count].pbrMetallicRoughness.metallicRoughnessTexture.textureIndex = \
				cgltf_texture_index(gltf_data, gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture);
			material_data[material_data_count].pbrMetallicRoughness.metallicRoughnessTexture.imageIndex = \
				cgltf_image_index(gltf_data, gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.texture->image);
			material_data[material_data_count].pbrMetallicRoughness.metallicRoughnessTexture.scale = \
				gltfMaterial->pbr_metallic_roughness.metallic_roughness_texture.scale;

			material_data[material_data_count].pbrMetallicRoughness.metallicFactor = gltfMaterial->pbr_metallic_roughness.metallic_factor;
			material_data[material_data_count].pbrMetallicRoughness.roughnessFactor = gltfMaterial->pbr_metallic_roughness.roughness_factor;
			memcpy(material_data[material_data_count].pbrMetallicRoughness.baseColorFactor,
			       gltfMaterial->pbr_metallic_roughness.base_color_factor,
			       STRUCT_MEMBER_SIZE(struct kmr_gltf_loader_cgltf_pbr_metallic_roughness, baseColorFactor));

			material_data[material_data_count].normalTexture.scale = gltfMaterial->normal_texture.scale;
			material_data[material_data_count].normalTexture.textureIndex = \
				cgltf_texture_index(gltf_data, gltfMaterial->normal_texture.texture);
			material_data[material_data_count].normalTexture.imageIndex = \
				cgltf_image_index(gltf_data, gltfMaterial->normal_texture.texture->image);

			material_data[material_data_count].occlusionTexture.scale = gltfMaterial->occlusion_texture.scale;
			material_data[material_data_count].occlusionTexture.textureIndex = \
				cgltf_texture_index(gltf_data, gltfMaterial->occlusion_texture.texture);
			material_data[material_data_count].occlusionTexture.imageIndex = \
				cgltf_image_index(gltf_data, gltfMaterial->occlusion_texture.texture->image);

			material_data[material_data_count].meshIndex = i;
			material_data_count++;
		}
	}

	material->material_data_count = material_data_count;
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

	for (i=0; i < material->material_data_count; i++) {
		free(material->material_data[i].material_name);
	}

	free(material->material_data);
	free(material);
}

/**************************************************************
 * END OF kmr_gltf_loader_material_{create,destroy} FUNCTIONS *
 **************************************************************/


/************************************************************
 * START OF kmr_gltf_loader_node_{create,destroy} FUNCTIONS *
 ************************************************************/

struct kmr_gltf_loader_node *
kmr_gltf_loader_node_create (struct kmr_gltf_loader_node_create_info *node_info)
{
	uint32_t n, c, node_data_count = 0;

	float matrix[16];
	vec4 rotation; vec3 translation, scale;
	mat4 parent_node_matrix, child_node_matrix, rotation_matrix;

	cgltf_data *gltf_data = NULL;
	cgltf_node *parent_node = NULL;
	cgltf_node *child_node = NULL;

	struct kmr_gltf_loader_node *node = NULL;
	struct kmr_gltf_loader_node_data *node_data = NULL;

	node = calloc(1, sizeof(struct kmr_gltf_loader_node));
	if (!node) {
		kmr_utils_log(KMR_DANGER, "[x] calloc(node): %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_material_create;
	}

	gltf_data = node_info->gltf_file->gltf_data;

	/* Acquire amount of nodes associate with scene */
	for (n = 0; n < gltf_data->scenes[node_info->scene_index].nodes_count; n++) {
		parent_node = gltf_data->scenes[node_info->scene_index].nodes[n];
		node_data_count += parent_node->children_count;
	}

	/*
	 * For whatever odd reason some cglm functions don't appear to like CGLTF allocated heap memory.
	 * Define stack based variables and use memcpy to copy data from
	 * stack->heap & heap->stack.
	 */

	node_data = calloc(node_data_count, sizeof(struct kmr_gltf_loader_node_data));
	if (!node_data) {
		kmr_utils_log(KMR_DANGER, "[x] calloc(node_data): %s", strerror(errno));
		goto exit_error_kmr_gltf_loader_material_create;
	}

	node_data_count = 0;
	node->node_data = node_data;
	for (n = 0; n < gltf_data->scenes[node_info->scene_index].nodes_count; n++) {
		parent_node = gltf_data->scenes[node_info->scene_index].nodes[n];

		/* Clear stack array */
		memset(translation, 0, sizeof(translation));
		memset(rotation, 0, sizeof(rotation));
		memset(scale, 0, sizeof(scale));
		memset(matrix, 0, sizeof(matrix));
		memset(rotation_matrix, 0, sizeof(rotation_matrix));
		memset(parent_node_matrix, 0, sizeof(parent_node_matrix));

		/* Copy from heap to stack */
		memcpy(translation, parent_node->translation, sizeof(translation));
		memcpy(rotation, parent_node->rotation, sizeof(rotation));
		memcpy(scale, parent_node->scale, sizeof(scale));
		memcpy(matrix, parent_node->matrix, sizeof(matrix));

		glm_mat4_identity(parent_node_matrix);

		/*
		 * GLTF: The node's unit quaternion rotation in the order (x, y, z, w), where w is the scalar.
		 * NOTE: cglm stores quaternion as [x, y, z, w] in memory since v0.4.0 it was [w, x, y, z] before v0.4.0 ( v0.3.5 and earlier )
		 */
		if (parent_node->has_translation)
			glm_translate(parent_node_matrix, translation);

		if (parent_node->has_rotation) {
			glm_quat_mat4(rotation, rotation_matrix);
			glm_mat4_mul(rotation_matrix, parent_node_matrix, parent_node_matrix);
		}

		if (parent_node->has_scale)
			glm_scale(parent_node_matrix, scale);

		if (parent_node->has_matrix)
			glm_mat4_make(matrix, parent_node_matrix); // Not CGLM function

		/* Start child node's loop */
		for (c = 0; c < parent_node->children_count; c++) {
			child_node = parent_node->children[c];

			/* Clear stack array */
			memset(translation, 0, sizeof(translation));
			memset(rotation, 0, sizeof(rotation));
			memset(scale, 0, sizeof(scale));
			memset(matrix, 0, sizeof(matrix));
			memset(child_node_matrix, 0, sizeof(child_node_matrix));
			memset(rotation_matrix, 0, sizeof(rotation_matrix));

			/* Copy from heap to stack */
			memcpy(translation, child_node->translation, sizeof(translation));
			memcpy(rotation, child_node->rotation, sizeof(rotation));
			memcpy(scale, child_node->scale, sizeof(scale));
			memcpy(matrix, child_node->matrix, sizeof(matrix));

			glm_mat4_identity(child_node_matrix);
			if (child_node->has_translation)
				glm_translate(child_node_matrix, translation);

			if (child_node->has_rotation) {
				glm_quat_mat4(rotation, rotation_matrix);
				glm_mat4_mul(rotation_matrix, child_node_matrix, child_node_matrix);
			}

			if (child_node->has_scale)
				glm_scale(child_node_matrix, scale);

			if (child_node->has_matrix)
				glm_mat4_make(matrix, child_node_matrix); // Not CGLM function

			/* Multiply the parent matrix by the child */
			glm_mat4_mul(parent_node_matrix, child_node_matrix, child_node_matrix);

			/* copy final stack matrix into heap memory matrix */
			memcpy(node_data[node_data_count].matrix_transform, child_node_matrix, sizeof(child_node_matrix));

			if (child_node->skin) {
				node_data[node_data_count].obj_index = cgltf_skin_index(gltf_data, child_node->skin);
				node_data[node_data_count].obj_type = KMR_GLTF_LOADER_GLTF_SKIN;
			} else if (child_node->mesh) {
				node_data[node_data_count].obj_index = cgltf_mesh_index(gltf_data, child_node->mesh);
				node_data[node_data_count].obj_type = KMR_GLTF_LOADER_GLTF_MESH;
			} else if (child_node->camera) {
				node_data[node_data_count].obj_index = cgltf_camera_index(gltf_data, child_node->camera);
				node_data[node_data_count].obj_type = KMR_GLTF_LOADER_GLTF_CAMERA;
			} else {
				node_data[node_data_count].obj_index = cgltf_node_index(gltf_data, child_node);
				node_data[node_data_count].obj_type = KMR_GLTF_LOADER_GLTF_NODE;
			}

			node_data[node_data_count].parent_node_index = cgltf_node_index(gltf_data, parent_node);
			node_data[node_data_count].node_index = cgltf_node_index(gltf_data, child_node);
			node_data_count++;
		}
	}

	node->node_data_count = node_data_count;
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

	free(node->node_data);
	free(node);	
}

/**********************************************************
 * END OF kmr_gltf_loader_node_{create,destroy} FUNCTIONS *
 **********************************************************/


/********************************************************************
 * START OF kmr_gltf_loader_node_display_matrix_transform FUNCTIONS *
 ********************************************************************/

void
kmr_gltf_loader_node_display_matrix_transform (struct kmr_gltf_loader_node *node_info)
{
	uint32_t n, i, j;

	const char *obj_name[] = {
		[KMR_GLTF_LOADER_GLTF_NODE] = "nodes",
		[KMR_GLTF_LOADER_GLTF_MESH] = "meshes",
		[KMR_GLTF_LOADER_GLTF_SKIN] = "skins",
		[KMR_GLTF_LOADER_GLTF_CAMERA] = "cameras",
	};

	fprintf(stdout, "\nGLTF File \"nodes\" Array Matrix Transforms = [\n");
	for (n = 0; n < node_info->node_data_count; n++) {
		fprintf(stdout, "[Parent:Child] [%s[%u]]\n",
		        obj_name[node_info->node_data[n].obj_type], 
		        node_info->node_data[n].obj_index);

		fprintf(stdout, "\t[%u:%u] = {\n",
		        node_info->node_data[n].parent_node_index,
		        node_info->node_data[n].node_index);

		for (i = 0; i < 4; i++) {
			fprintf(stdout, "\t");
			for (j = 0; j < 4; j++)
				fprintf(stdout, "   %f   ",
				        node_info->node_data[n].matrix_transform[i][j]);
			fprintf(stdout, "\n");
		}
		fprintf(stdout, "\t}\n\n");
	}
	fprintf(stdout, "]\n\n");
}

/*****************************************************************
 * END OF kmr_gltf_loader_node_display_matrix_transform FUNCTION *
 *****************************************************************/
