#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gltf-loader.h"

int main(void)
{
  struct uvr_gltf_loader_destroy gltfFileDestroy;
  memset(&gltfFileDestroy, 0, sizeof(struct uvr_gltf_loader_destroy));

  struct uvr_gltf_loader_file gltfFile;
  struct uvr_gltf_loader_vertex gltfFileVertex;
  struct uvr_gltf_loader_node gltfFileNode;

  struct uvr_gltf_loader_file_load_info gltfFileLoadInfo;
  gltfFileLoadInfo.fileName = GLTF_MODEL;

  gltfFile = uvr_gltf_loader_file_load(&gltfFileLoadInfo);
  if (!gltfFile.gltfData)
    return 1;

  struct uvr_gltf_loader_vertex_buffers_create_info gltfVertexBuffersInfo;
  gltfVertexBuffersInfo.gltfFile = gltfFile;
  gltfVertexBuffersInfo.bufferIndex = 0;

  gltfFileVertex = uvr_gltf_loader_vertex_buffers_create(&gltfVertexBuffersInfo);
  if (!gltfFileVertex.verticesData)
    goto exit_gltf_loader_file_free_cgltf_data;

  struct uvr_gltf_loader_node_create_info gltfFileNodeInfo;
  gltfFileNodeInfo.gltfLoaderFile = gltfFile;
  gltfFileNodeInfo.sceneIndex = 0;

  gltfFileNode = uvr_gltf_loader_node_create(&gltfFileNodeInfo);
  if (!gltfFileNode.nodeData)
    goto exit_gltf_loader_file_free_vertex_data;

  uvr_gltf_loader_node_display_matrix_transform(&gltfFileNode);

  gltfFileDestroy.uvr_gltf_loader_node_cnt = 1;
  gltfFileDestroy.uvr_gltf_loader_node = &gltfFileNode;
  gltfFileDestroy.uvr_gltf_loader_vertex_cnt = 1;
  gltfFileDestroy.uvr_gltf_loader_vertex = &gltfFileVertex;
  gltfFileDestroy.uvr_gltf_loader_file_cnt = 1;
  gltfFileDestroy.uvr_gltf_loader_file = &gltfFile;
  uvr_gltf_loader_destroy(&gltfFileDestroy);
  return 0;

exit_gltf_loader_file_free_vertex_data:
  gltfFileDestroy.uvr_gltf_loader_vertex_cnt = 1;
  gltfFileDestroy.uvr_gltf_loader_vertex = &gltfFileVertex;
exit_gltf_loader_file_free_cgltf_data:
  gltfFileDestroy.uvr_gltf_loader_file_cnt = 1;
  gltfFileDestroy.uvr_gltf_loader_file = &gltfFile;
  uvr_gltf_loader_destroy(&gltfFileDestroy);
  return 1;
}
