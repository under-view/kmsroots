.. default-domain:: C

gltf-loader
===========

Header: kmsroots/gltf-loader.h

Table of contents (click to go)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

======
Macros
======

=====
Enums
=====

1. :c:enum:`kmr_gltf_loader_gltf_object_type`

======
Unions
======

=======
Structs
=======

1. :c:struct:`kmr_gltf_loader_file`
#. :c:struct:`kmr_gltf_loader_file_create_info`
#. :c:struct:`kmr_gltf_loader_mesh_vertex_data`
#. :c:struct:`kmr_gltf_loader_mesh_data`
#. :c:struct:`kmr_gltf_loader_mesh`
#. :c:struct:`kmr_gltf_loader_mesh_create_info`
#. :c:struct:`kmr_gltf_loader_texture_image`
#. :c:struct:`kmr_gltf_loader_texture_image_create_info`
#. :c:struct:`kmr_gltf_loader_cgltf_texture_transform`
#. :c:struct:`kmr_gltf_loader_cgltf_texture_view`
#. :c:struct:`kmr_gltf_loader_cgltf_pbr_metallic_roughness`
#. :c:struct:`kmr_gltf_loader_material_data`
#. :c:struct:`kmr_gltf_loader_material`
#. :c:struct:`kmr_gltf_loader_material_create_info`
#. :c:struct:`kmr_gltf_loader_node_data`
#. :c:struct:`kmr_gltf_loader_node`
#. :c:struct:`kmr_gltf_loader_node_create_info`

=========
Functions
=========

1. :c:func:`kmr_gltf_loader_file_create`
#. :c:func:`kmr_gltf_loader_file_destroy`
#. :c:func:`kmr_gltf_loader_mesh_create`
#. :c:func:`kmr_gltf_loader_mesh_destroy`
#. :c:func:`kmr_gltf_loader_texture_image_create`
#. :c:func:`kmr_gltf_loader_texture_image_destroy`
#. :c:func:`kmr_gltf_loader_material_create`
#. :c:func:`kmr_gltf_loader_material_destroy`
#. :c:func:`kmr_gltf_loader_node_create`
#. :c:func:`kmr_gltf_loader_node_destroy`
#. :c:func:`kmr_gltf_loader_node_display_matrix_transform`

=================
Function Pointers
=================

API Documentation
~~~~~~~~~~~~~~~~~

====================
kmr_gltf_loader_file
====================

.. c:struct:: kmr_gltf_loader_file

	.. c:member::
		cgltf_data *gltfData;

	:c:member:`gltfData`
		| Buffer that stores a given gltf file's metadata.

================================
kmr_gltf_loader_file_create_info
================================

.. c:struct:: kmr_gltf_loader_file_create_info

	.. c:member::
		const char *fileName;

	:c:member:`fileName`
		| Must pass the path to the gltf file to load.

===========================
kmr_gltf_loader_file_create
===========================

.. c:function:: struct kmr_gltf_loader_file *kmr_gltf_loader_file_create(struct kmr_gltf_loader_file_create_info *gltfFileInfo);

	This function is used to parse and load gltf files content into memory.

	Parameters:
		| **gltfFileInfo**
		| Must pass a pointer to a ``struct`` :c:struct:`kmr_gltf_loader_file_create_info`

	Returns:
		| **on success:** Pointer to a ``struct`` :c:struct:`kmr_gltf_loader_file`
		| **on failure:** NULL

============================
kmr_gltf_loader_file_destroy
============================

.. c:function:: void kmr_gltf_loader_file_destroy(struct kmr_gltf_loader_file *gltfFile);

	Frees any allocated memory and closes FD's (if open) created after
	:c:func:`kmr_gltf_loader_file_create` call.

	Parameters:
		| **gltfFile**
		| Pointer to a valid ``struct`` :c:struct:`kmr_gltf_loader_file`

	.. code-block::

		/* Free'd members */
		struct kmr_gltf_loader_file {
			cgltf_data *gltfData;
		}

=========================================================================================================================================

================================
kmr_gltf_loader_mesh_vertex_data
================================

.. c:struct:: kmr_gltf_loader_mesh_vertex_data

	.. c:member::
		vec3 position;
		vec3 normal;
		vec2 texCoord;
		vec3 color;

	Struct member order is arbitrary. **SHOULD NOT BE USED DIRECTLY**. Advise to create second stack buffer
	and copy data over to it. Members populated with vertices from GLTF file buffer.

	:c:member:`position`
		| Vertex position coordinates

	:c:member:`normal`
		| Vertex normal (direction vertex points)

	:c:member:`texCoord`
		| Texture coordinates

	:c:member:`color`
		| Color

=========================
kmr_gltf_loader_mesh_data
=========================

.. c:struct:: kmr_gltf_loader_mesh_data

	.. c:member::
		uint32_t                                firstIndex;
		uint32_t                                *indexBufferData;
		uint32_t                                indexBufferDataCount;
		uint32_t                                indexBufferDataSize;
		struct kmr_gltf_loader_mesh_vertex_data *vertexBufferData;
		uint32_t                                vertexBufferDataCount;
		uint32_t                                vertexBufferDataSize;

	:c:member:`firstIndex`
		| Array index within the index buffer. Calculated in :c:func:`kmr_gltf_loader_mesh_create`
		| firstIndex = firstIndex + bufferElementCount (GLTF file accessor[index].count).
		| Can be used by the application to fill in `vkCmdDrawIndexed(3)`_ function.

	:c:member:`indexBufferData`
		| Buffer of index data belonging to mesh populated from GLTF file buffer at
		| ``struct`` :c:struct:`kmr_gltf_loader_mesh` { ``bufferIndex`` }.

	:c:member:`indexBufferDataCount`
		| Amount of elements in :c:member:`indexBufferData` array.

	:c:member:`indexBufferDataSize`
		| The total size in bytes of the :c:member:`indexBufferData` array.

	:c:member:`vertexBufferData`
		| Pointer to a buffer containing position vertices, normal,
		| texture coordinates, and color populated from GLTF file buffer at
		| ``struct`` :c:struct:`kmr_gltf_loader_mesh` { ``bufferIndex`` }.

	:c:member:`vertexBufferDataCount`
		| Amount of elements in :c:member:`vertexBufferData` array.

	:c:member:`vertexBufferDataSize`
		| The total size in bytes of the :c:member:`vertexBufferData` array.

====================
kmr_gltf_loader_mesh
====================

.. c:struct:: kmr_gltf_loader_mesh

	.. c:member::
		uint16_t                         bufferIndex;
		struct kmr_gltf_loader_mesh_data *meshData;
		uint16_t                         meshDataCount;

	:c:member:`bufferIndex`
		| The index in the "buffers" (json key) array of give GLTF file.

	:c:member:`meshData`
		| Pointer to an array of ``struct`` :c:struct:`kmr_gltf_loader_mesh_data`
		| storing all important data related to each mesh.

	:c:member:`meshDataCount`
		| Amount of meshes associated with a ``bufferIndex``.
		| The array size of ``meshData`` array.

================================
kmr_gltf_loader_mesh_create_info
================================

.. c:struct:: kmr_gltf_loader_mesh_create_info

	.. c:member::
		struct kmr_gltf_loader_file *gltfFile;
		uint16_t                    bufferIndex;

	:c:member:`gltfFile`
		| Must pass a valid pointer to a ``struct`` :c:struct:`kmr_gltf_loader_file`
		| for cgltf_data ``gltfData`` member

	:c:member:`bufferIndex`
		| Index of buffer in GLTF file "buffers" (json key) array

===========================
kmr_gltf_loader_mesh_create
===========================

.. c:function:: struct kmr_gltf_loader_mesh *kmr_gltf_loader_mesh_create(struct kmr_gltf_loader_mesh_create_info *meshInfo);

	Function loops through all meshes and finds the associated accessor->buffer view
	for a given buffer at ``bufferIndex``. After retrieves all information required to
	understand the contents of the multiple sections in the buffer. The function then
	creates multiple meshes with appropriate data (``struct`` :c:struct:`kmr_gltf_loader_mesh_data`)
	so the application only need to call function and create their vertex buffer + index
	buffer array's based upon what's already populated. Converts GLTF buffer to a buffer
	that Vulkan can understand seperating each buffer, by their mesh index in GLTF file
	"meshes" (json key) array.

	Parameters:
		| **meshInfo**
		| Must pass a pointer to a ``struct`` :c:struct:`kmr_gltf_loader_mesh_create_info`

	Returns:
		| **on success:** pointer to a ``struct`` :c:struct:`kmr_gltf_loader_mesh`
		| **on failure:** NULL

============================
kmr_gltf_loader_mesh_destroy
============================

.. c:function:: void kmr_gltf_loader_mesh_destroy(struct kmr_gltf_loader_mesh *mesh);

	Frees any allocated memory and closes FD's (if open) created after
	:c:func:`kmr_gltf_loader_mesh_create` call.

	Parameters:
		| **mesh**
		| Pointer to a valid ``struct`` :c:struct:`kmr_gltf_loader_mesh`

	.. code-block::

		/* Free'd members */
		struct kmr_gltf_loader_mesh {
			struct kmr_gltf_loader_mesh_data {
				uint32_t *indexBufferData;
				struct kmr_gltf_loader_mesh_vertex_data *vertexBufferData;
			}
			struct kmr_gltf_loader_mesh_data *meshData;
		}

=========================================================================================================================================

=============================
kmr_gltf_loader_texture_image
=============================

.. c:struct:: kmr_gltf_loader_texture_image

	.. c:member::
		uint32_t                      imageCount;
		uint32_t                      totalBufferSize;
		struct kmr_utils_image_buffer *imageData;

	:c:member:`imageCount`
		| Amount of images associated with a given GLTF file

	:c:member:`totalBufferSize`
		| Collective size of each image associated with a given GLTF file.
		| Best utilized when creating single `VkBuffer`_.

	:c:member:`imageData`
		| Pointer to an array of image metadata and pixel buffer.

=========================================
kmr_gltf_loader_texture_image_create_info
=========================================

.. c:struct:: kmr_gltf_loader_texture_image_create_info

	.. c:member::
		struct kmr_gltf_loader_file *gltfFile;
		const char                  *directory;

	:c:member:`gltfFile`
		| Must pass a valid pointer to ``struct`` :c:struct:`kmr_gltf_loader_file` for
		| cgltf_data ``gltfData`` member.

	:c:member:`directory`
		| Must pass a pointer to a string detailing the directory of
		| where all images are stored. Absolute path to a file that resides
		| in the same directory as the images will work too.

====================================
kmr_gltf_loader_texture_image_create
====================================

.. c:function:: struct kmr_gltf_loader_texture_image *kmr_gltf_loader_texture_image_create(struct kmr_gltf_loader_texture_image_create_info *textureImageInfo);

	Function Loads all images associated with gltf file into memory.

	Parameters:
		| **textureImageInfo**
		| Must pass a pointer to a ``struct`` :c:struct:`kmr_gltf_loader_texture_image_create_info`

	Returns:
		| **on success:** pointer to a ``struct`` :c:struct:`kmr_gltf_loader_texture_image`
		| **on failure:** NULL

=====================================
kmr_gltf_loader_texture_image_destroy
=====================================

.. c:function:: void kmr_gltf_loader_texture_image_destroy (struct kmr_gltf_loader_texture_image *textureImage);

	Frees any allocated memory and closes FD's (if open) created after
	:c:func:`kmr_gltf_loader_texture_image_create` call.

	Parameters:
		| **textureImage**
		| Pointer to a valid ``struct`` :c:struct:`kmr_gltf_loader_texture_image`

	.. code-block::

		/* Free'd members */
		struct kmr_gltf_loader_mesh {
			struct kmr_gltf_loader_mesh_data {
				uint32_t *indexBufferData;
				struct kmr_gltf_loader_mesh_vertex_data *vertexBufferData;
			}
			struct kmr_gltf_loader_mesh_data *meshData;
		}

=========================================================================================================================================

=======================================
kmr_gltf_loader_cgltf_texture_transform
=======================================

.. c:struct:: kmr_gltf_loader_cgltf_texture_transform

	.. c:member::
		float offsets[2];
		float rotation;
		float scales[2];

	More information can be found at `KHR_texture_transform`_.

	:c:member:`offsets`
		| Offset from texture coordinate (UV) origin.

	:c:member:`rotation`
		| Rotate texture coordinate (UV) this many radians
		| counter-clockwise from the origin.

	:c:member:`scales`
		| Scale factor for texture coordinate (UV).

==================================
kmr_gltf_loader_cgltf_texture_view
==================================

.. c:struct:: kmr_gltf_loader_cgltf_texture_view

	.. c:member::
		uint32_t                                       textureIndex;
		uint32_t                                       imageIndex;
		float                                          scale;
		struct kmr_gltf_loader_cgltf_texture_transform textureTransform;

	:c:member:`textureIndex`
		| Index in "textures" (json key) GLTF file array.

	:c:member:`imageIndex`
		| Index in "images" (json key) GTLF file array that belongs
		| to the texture at ``textureIndex``.

	:c:member:`scale`
		| The scalar parameter applied to each vector of the texture.

	:c:member:`textureTransform`
		| Contains data regarding texture coordinate scale factor, rotation in
		| radians, & offset from origin.

============================================
kmr_gltf_loader_cgltf_pbr_metallic_roughness
============================================

.. c:struct:: kmr_gltf_loader_cgltf_pbr_metallic_roughness

	.. c:member::
		struct kmr_gltf_loader_cgltf_texture_view baseColorTexture;
		struct kmr_gltf_loader_cgltf_texture_view metallicRoughnessTexture;
		float                                     baseColorFactor[4];
		float                                     metallicFactor;
		float                                     roughnessFactor;

	:c:member:`baseColorTexture`
		| The main texture to be applied on an object and metadata
		| in relates to texture.

	:c:member:`metallicRoughnessTexture`
		| Textures for metalness and roughness properties are packed together
		| in a single texture (image). Used for readability letting the
		| application writer know we are acquring texture->image
		| associated with GLTF file ``metallicRoughnessTexture``.

	**Bellow define the metallic-roughness material model**

	:c:member:`baseColorFactor`
		| The "main" color of the object surface (RBGA).

	:c:member:`metallicFactor`
		| Describes how much the reflective behavior of the material resembles
		| that of a metal. Values range from 0.0 (non-metal) to 1.0 (metal).

	:c:member:`roughnessFactor`
		| Indicating how rough the surface is, affecting the light scattering.
		| Values range from 0.0 (smooth) to 1.0 (rough).

=============================
kmr_gltf_loader_material_data
=============================

.. c:struct:: kmr_gltf_loader_material_data

	.. c:member::
		uint32_t                                             meshIndex;
		char                                                 *materialName;
		struct kmr_gltf_loader_cgltf_pbr_metallic_roughness  pbrMetallicRoughness;
		struct kmr_gltf_loader_cgltf_texture_view            normalTexture;
		struct kmr_gltf_loader_cgltf_texture_view            occlusionTexture;

	More information can be found at `GLTF 2.0 Reference Guide`_.

	:c:member:`meshIndex`
		| Index in "meshes" (json key) array of GLTF file that material belongs to.

	:c:member:`materialName`
		| Name given to material block contained in GLTF file.

	:c:member:`pbrMetallicRoughness`
		| "Physically-Based Rendering Metallic Roughness Model" - Allows renderers to
		| display objects with a realistic appearance under different lighting conditions.
		| Stores required data for PBR.

	:c:member:`normalTexture`
		| Stores a given texture tangent-space normal data,
		| that will be applied to the normals of the coordinates.

	:c:member:`occlusionTexture`
		| Stores data about areas of a surface that are occluded from light,
		| and thus rendered darker.

========================
kmr_gltf_loader_material
========================

.. c:struct:: kmr_gltf_loader_material

	.. c:member::
		struct kmr_gltf_loader_material_data *materialData;
		uint16_t                             materialDataCount;

	:c:member:`materialData`
		| Pointer to an array of ``struct`` :c:struct:`kmr_gltf_loader_material_data`.

	:c:member:`materialDataCount`
		| Amount of elements in ``materialData`` array.

====================================
kmr_gltf_loader_material_create_info
====================================

.. c:struct:: kmr_gltf_loader_material_create_info

	.. c:member::
		struct kmr_gltf_loader_file *gltfFile;

	:c:member:`gltfFile`
		| Must pass a valid pointer to a ``struct`` :c:struct:`kmr_gltf_loader_file`
		| for cgltf_data ``gltfData`` member.

===============================
kmr_gltf_loader_material_create
===============================

.. c:function:: struct kmr_gltf_loader_material *kmr_gltf_loader_material_create(struct kmr_gltf_loader_material_create_info *materialInfo);

	Function Loads necessary material information associated with gltf file into memory.

	Parameters:
		| **materialInfo**
		| Must pass a pointer to a ``struct`` :c:struct:`kmr_gltf_loader_material_create_infoi`

	Returns:
		| **on success:** pointer to a ``struct`` :c:struct:`kmr_gltf_loader_material`
		| **on failure:** NULL

================================
kmr_gltf_loader_material_destroy
================================

.. c:function:: void kmr_gltf_loader_material_destroy(struct kmr_gltf_loader_material *material);

	Frees any allocated memory and closes FD's (if open) created after
	:c:func:`kmr_gltf_loader_material_create` call.

	Parameters:
		| **material**
		| Pointer to a valid ``struct`` :c:struct:`kmr_gltf_loader_material`

	.. code-block::

		/* Free'd members */
		struct kmr_gltf_loader_material {
			struct kmr_gltf_loader_material_data {
				char *materialName;
			}
			struct kmr_gltf_loader_material_data *materialData;
		}

=========================================================================================================================================

================================
kmr_gltf_loader_gltf_object_type
================================

.. c:enum:: kmr_gltf_loader_gltf_object_type

	.. c:macro::
		KMR_GLTF_LOADER_GLTF_NODE
		KMR_GLTF_LOADER_GLTF_MESH
		KMR_GLTF_LOADER_GLTF_SKIN
		KMR_GLTF_LOADER_GLTF_CAMERA

	Options used by :c:struct:`kmr_gltf_loader_node_data`

	:c:macro:`KMR_GLTF_LOADER_GLTF_NODE`
		| Value set to ``0x0001``

	:c:macro:`KMR_GLTF_LOADER_GLTF_MESH`
		| Value set to ``0x0002``

	:c:macro:`KMR_GLTF_LOADER_GLTF_SKIN`
		| Value set to ``0x0003``

	:c:macro:`KMR_GLTF_LOADER_GLTF_CAMERA`
		| Value set to ``0x0004``

=========================
kmr_gltf_loader_node_data
=========================

.. c:struct:: kmr_gltf_loader_node_data

	.. c:member::
		enum kmr_gltf_loader_gltf_object_type objectType;
		uint32_t                              objectIndex;
		uint32_t                              nodeIndex;
		uint32_t                              parentNodeIndex;
		float                                 matrixTransform[4][4];

	:c:member:`objectType`
		| Type of GLTF object that attached to node

	:c:member:`objectIndex`
		| The index in GLTF file "Insert Object Name" array. If ``objectType`` is a
		| mesh this index is the index in the GLTF file "meshes" (json key) array.

	:c:member:`nodeIndex`
		| Index in the GLTF file "nodes" (json key) array for child node.

	:c:member:`parentNodeIndex`
		| Index in the GLTF file "nodes" array (json key) for parent node.

	:c:member:`matrixTransform`
		| If matrix property not already defined. Value is T * R * S.
		| T - translation
		| R - Rotation
		| S - scale
		| Final matrix transform is
		| (identity matrix * TRS parent matrix) * (identity matrix * TRS child matrix)

====================
kmr_gltf_loader_node
====================

.. c:struct:: kmr_gltf_loader_node

	.. c:member::
		struct kmr_gltf_loader_node_data *nodeData;
		uint32_t                         nodeDataCount;

	:c:member:`nodeData`
		| Pointer to an array of ``struct`` :c:struct:`kmr_gltf_loader_node_data`

	:c:member:`nodeDataCount`
		| Amount of elements in ``nodeData`` array.

================================
kmr_gltf_loader_node_create_info
================================

.. c:struct:: kmr_gltf_loader_node_create_info

	.. c:member::
		struct kmr_gltf_loader_file *gltfFile;
		uint32_t                    sceneIndex;

	:c:member:`gltfFile`
		| Must pass a valid pointer to a ``struct`` :c:struct:`kmr_gltf_loader_file`
		| for cgltf_data ``gltfData`` member.

	:c:member:`sceneIndex`
		| Index in GLTF file "scenes" (json key) array.

===========================
kmr_gltf_loader_node_create
===========================

.. c:function:: struct kmr_gltf_loader_node *kmr_gltf_loader_node_create(struct kmr_gltf_loader_node_create_info *nodeInfo);

	Calculates final translation * rotatation * scale matrix for all nodes associated with a scene.
	Along with final matrix transform data, function also returns the parent and child index in the
	GLTF file object "nodes" array, the type of node/object (i.e "mesh,skin,camera,etc..."), and
	the index of that object in the GLTF file "Insert Object Name" array.

	Parameters:
		| **nodeInfo**
		| Must pass a pointer to a ``struct`` :c:struct:`kmr_gltf_loader_node_create_info`

	Returns:
		| **on success:** pointer to a ``struct`` :c:struct:`kmr_gltf_loader_node`
		| **on failure:** NULL

============================
kmr_gltf_loader_node_destroy
============================

.. c:function:: void kmr_gltf_loader_node_destroy(struct kmr_gltf_loader_node *node);

	Frees any allocated memory and closes FD's (if open) created after
	:c:func:`kmr_gltf_loader_node_create` call.

	Parameters:
		| **node**
		| Pointer to a valid ``struct`` :c:struct:`kmr_gltf_loader_node`

	.. code-block::

		/* Free'd members */
		struct kmr_gltf_loader_node {
			struct kmr_gltf_loader_node_data *nodeData;
		}

=========================================================================================================================================

=============================================
kmr_gltf_loader_node_display_matrix_transform
=============================================

.. c:function:: void kmr_gltf_loader_node_display_matrix_transform(struct kmr_gltf_loader_node *node);

	Prints out matrix transform for each node in
	``struct`` :c:struct:`kmr_gltf_loader_node` { ``nodeData`` }.

	Parameters:
		| **node**
		| Must pass a pointer to a ``struct`` :c:struct:`kmr_gltf_loader_node`

=========================================================================================================================================

.. _VkBuffer: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBuffer.html
.. _vkCmdDrawIndexed(3): https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdDrawIndexed.html
.. _KHR_texture_transform: https://github.com/KhronosGroup/glTF/blob/main/extensions/2.0/Khronos/KHR_texture_transform/README.md
.. _GLTF 2.0 Reference Guide: https://www.khronos.org/files/gltf20-reference-guide.pdf
