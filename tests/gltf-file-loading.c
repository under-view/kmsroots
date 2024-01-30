#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gltf-loader.h"

int main(void)
{
	int ret = 0;

	struct kmr_gltf_loader_file *gltfLoaderFile = NULL;
	struct kmr_gltf_loader_mesh *gltfLoaderFileMesh = NULL;
	struct kmr_gltf_loader_node *gltfLoaderFileNode = NULL;

	struct kmr_gltf_loader_file_create_info gltfLoaderFileCreateInfo;
	struct kmr_gltf_loader_mesh_create_info gltfMeshInfo;
	struct kmr_gltf_loader_node_create_info gltfLoaderFileNodeInfo;

	gltfLoaderFileCreateInfo.fileName = GLTF_MODEL;
	gltfLoaderFile = kmr_gltf_loader_file_create(&gltfLoaderFileCreateInfo);
	if (!gltfLoaderFile) { ret = 1; goto exit_error_gltf_file_loading; }

	gltfMeshInfo.gltfFile = gltfLoaderFile;
	gltfMeshInfo.bufferIndex = 0;
	gltfLoaderFileMesh = kmr_gltf_loader_mesh_create(&gltfMeshInfo);
	if (!gltfLoaderFileMesh) { ret = 1; goto exit_error_gltf_file_loading; }

	gltfLoaderFileNodeInfo.gltfFile = gltfLoaderFile;
	gltfLoaderFileNodeInfo.sceneIndex = 0;
	gltfLoaderFileNode = kmr_gltf_loader_node_create(&gltfLoaderFileNodeInfo);
	if (!gltfLoaderFileNode) { ret = 1; goto exit_error_gltf_file_loading; }

	kmr_gltf_loader_node_display_matrix_transform(gltfLoaderFileNode);

exit_error_gltf_file_loading:
	kmr_gltf_loader_node_destroy(gltfLoaderFileNode);
	kmr_gltf_loader_mesh_destroy(gltfLoaderFileMesh);
	kmr_gltf_loader_file_destroy(gltfLoaderFile);
	return ret;
}
