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
    goto exit_create_gltf_loader_file_free_gltf_data;

  gltfFileDestroy.uvr_gltf_loader_file_cnt = 1;
  gltfFileDestroy.uvr_gltf_loader_file = &gltfFile;
  gltfFileDestroy.uvr_gltf_loader_vertex_cnt = 1;
  gltfFileDestroy.uvr_gltf_loader_vertex = &gltfFileVertex;
  uvr_gltf_loader_destroy(&gltfFileDestroy);
  return 0;

exit_create_gltf_loader_file_free_gltf_data:
  gltfFileDestroy.uvr_gltf_loader_file_cnt = 1;
  gltfFileDestroy.uvr_gltf_loader_file = &gltfFile;
  uvr_gltf_loader_destroy(&gltfFileDestroy);
  return 1;
}
