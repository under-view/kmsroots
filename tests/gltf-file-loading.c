#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gltf-loader.h"

int main(void)
{
	struct kmr_gltf_loader_destroy gltfLoaderFileDestroy;
	memset(&gltfLoaderFileDestroy, 0, sizeof(struct kmr_gltf_loader_destroy));

	struct kmr_gltf_loader_file gltfLoaderFile;
	struct kmr_gltf_loader_mesh gltfLoaderFileMesh;
	struct kmr_gltf_loader_node gltfLoaderFileNode;

	struct kmr_gltf_loader_file_load_info gltfLoaderFileLoadInfo;
	gltfLoaderFileLoadInfo.fileName = GLTF_MODEL;

	gltfLoaderFile = kmr_gltf_loader_file_load(&gltfLoaderFileLoadInfo);
	if (!gltfLoaderFile.gltfData)
		return 1;

	struct kmr_gltf_loader_mesh_create_info gltfMeshInfo;
	gltfMeshInfo.gltfLoaderFile = gltfLoaderFile;
	gltfMeshInfo.bufferIndex = 0;

	gltfLoaderFileMesh = kmr_gltf_loader_mesh_create(&gltfMeshInfo);
	if (!gltfLoaderFileMesh.meshData)
		goto exit_gltf_loader_file_free_cgltf_data;

	struct kmr_gltf_loader_node_create_info gltfLoaderFileNodeInfo;
	gltfLoaderFileNodeInfo.gltfLoaderFile = gltfLoaderFile;
	gltfLoaderFileNodeInfo.sceneIndex = 0;

	gltfLoaderFileNode = kmr_gltf_loader_node_create(&gltfLoaderFileNodeInfo);
	if (!gltfLoaderFileNode.nodeData)
		goto exit_gltf_loader_file_free_vertex_data;

	kmr_gltf_loader_node_display_matrix_transform(&gltfLoaderFileNode);

	gltfLoaderFileDestroy.kmr_gltf_loader_node_cnt = 1;
	gltfLoaderFileDestroy.kmr_gltf_loader_node = &gltfLoaderFileNode;
	gltfLoaderFileDestroy.kmr_gltf_loader_mesh_cnt = 1;
	gltfLoaderFileDestroy.kmr_gltf_loader_mesh = &gltfLoaderFileMesh;
	gltfLoaderFileDestroy.kmr_gltf_loader_file_cnt = 1;
	gltfLoaderFileDestroy.kmr_gltf_loader_file = &gltfLoaderFile;
	kmr_gltf_loader_destroy(&gltfLoaderFileDestroy);
	return 0;

exit_gltf_loader_file_free_vertex_data:
	gltfLoaderFileDestroy.kmr_gltf_loader_mesh_cnt = 1;
	gltfLoaderFileDestroy.kmr_gltf_loader_mesh = &gltfLoaderFileMesh;
exit_gltf_loader_file_free_cgltf_data:
	gltfLoaderFileDestroy.kmr_gltf_loader_file_cnt = 1;
	gltfLoaderFileDestroy.kmr_gltf_loader_file = &gltfLoaderFile;
	kmr_gltf_loader_destroy(&gltfLoaderFileDestroy);
	return 1;
}
