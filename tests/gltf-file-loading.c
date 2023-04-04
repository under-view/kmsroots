#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gltf-loader.h"

int main(void)
{
	struct uvr_gltf_loader_destroy gltfLoaderFileDestroy;
	memset(&gltfLoaderFileDestroy, 0, sizeof(struct uvr_gltf_loader_destroy));

	struct uvr_gltf_loader_file gltfLoaderFile;
	struct uvr_gltf_loader_mesh gltfLoaderFileMesh;
	struct uvr_gltf_loader_node gltfLoaderFileNode;

	struct uvr_gltf_loader_file_load_info gltfLoaderFileLoadInfo;
	gltfLoaderFileLoadInfo.fileName = GLTF_MODEL;

	gltfLoaderFile = uvr_gltf_loader_file_load(&gltfLoaderFileLoadInfo);
	if (!gltfLoaderFile.gltfData)
		return 1;

	struct uvr_gltf_loader_mesh_create_info gltfMeshInfo;
	gltfMeshInfo.gltfLoaderFile = gltfLoaderFile;
	gltfMeshInfo.bufferIndex = 0;

	gltfLoaderFileMesh = uvr_gltf_loader_mesh_create(&gltfMeshInfo);
	if (!gltfLoaderFileMesh.meshData)
		goto exit_gltf_loader_file_free_cgltf_data;

	struct uvr_gltf_loader_node_create_info gltfLoaderFileNodeInfo;
	gltfLoaderFileNodeInfo.gltfLoaderFile = gltfLoaderFile;
	gltfLoaderFileNodeInfo.sceneIndex = 0;

	gltfLoaderFileNode = uvr_gltf_loader_node_create(&gltfLoaderFileNodeInfo);
	if (!gltfLoaderFileNode.nodeData)
		goto exit_gltf_loader_file_free_vertex_data;

	uvr_gltf_loader_node_display_matrix_transform(&gltfLoaderFileNode);

	gltfLoaderFileDestroy.uvr_gltf_loader_node_cnt = 1;
	gltfLoaderFileDestroy.uvr_gltf_loader_node = &gltfLoaderFileNode;
	gltfLoaderFileDestroy.uvr_gltf_loader_mesh_cnt = 1;
	gltfLoaderFileDestroy.uvr_gltf_loader_mesh = &gltfLoaderFileMesh;
	gltfLoaderFileDestroy.uvr_gltf_loader_file_cnt = 1;
	gltfLoaderFileDestroy.uvr_gltf_loader_file = &gltfLoaderFile;
	uvr_gltf_loader_destroy(&gltfLoaderFileDestroy);
	return 0;

exit_gltf_loader_file_free_vertex_data:
	gltfLoaderFileDestroy.uvr_gltf_loader_mesh_cnt = 1;
	gltfLoaderFileDestroy.uvr_gltf_loader_mesh = &gltfLoaderFileMesh;
exit_gltf_loader_file_free_cgltf_data:
	gltfLoaderFileDestroy.uvr_gltf_loader_file_cnt = 1;
	gltfLoaderFileDestroy.uvr_gltf_loader_file = &gltfLoaderFile;
	uvr_gltf_loader_destroy(&gltfLoaderFileDestroy);
	return 1;
}
