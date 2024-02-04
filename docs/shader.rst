.. default-domain:: C

shader
======

Header: kmsroots/shader.h

Table of contents (click to go)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

======
Macros
======

=====
Enums
=====

======
Unions
======

=======
Structs
=======

1. :c:struct:`kmr_shader_spirv`
#. :c:struct:`kmr_shader_spirv_create_info`

=========
Functions
=========

1. :c:func:`kmr_session_spirv_create`
#. :c:func:`kmr_session_spirv_destroy`

=================
Function Pointers
=================

API Documentation
~~~~~~~~~~~~~~~~~

================
kmr_shader_spirv
================

.. c:struct:: kmr_shader_spirv

	.. c:member::
		void                *result;
		const unsigned char *bytes;
		unsigned long       byteSize;

	:c:member:`result`
		| Pointer to an opaque handle to the results of a call to any shaderc_compile_into_*()
		| Unfortunately we can't release/free until after the shader module is created.

	:c:member:`bytes`
		| Buffer that stores a spirv byte code.

	:c:member:`byteSize` 
		| Byte size of spirv byte code buffer.

============================
kmr_shader_spirv_create_info
============================

.. c:struct:: kmr_shader_spirv_create_info

	.. c:member::
		unsigned int kind;
		const char   *source;
		const char   *filename;
		const char   *entryPoint;

	More information can be found at `libshaderc/shaderc.h`_.

	:c:member:`kind`
		| Used to specify what type of shader to create SPIR-V bytes from
		| key: `VkShaderStageFlagBits`_, value: `shaderc_shader_kind`_

	:c:member:`source`
		| Pointer to a buffer containing actual shader code

	:c:member:`filename`
		| Used as a tag to identify the source string

	:c:member:`entryPoint`
		| Used to define the function name in the GLSL source that acts
		| as an entry point for the shader

=======================
kmr_shader_spirv_create
=======================

.. c:function:: struct kmr_shader_spirv *kmr_shader_spirv_create(struct kmr_shader_spirv_create_info *spirvInfo);

	Takes in a character buffer containing shader code, it then compiles
	char buff into SPIRV-bytes at runtime. These SPIRV-bytes can later be
	passed to vulkan.

	Parameters:
		| **spirvInfo**
		| Pointer to a ``struct`` :c:struct:`kmr_shader_spirv_create_info`

	Returns:
		| **on success:** pointer to ``struct`` :c:struct:`kmr_shader_spriv`
		| **on failure:** NULL

========================
kmr_shader_spirv_destroy
========================

.. c:function:: void kmr_shader_spirv_destroy(struct kmr_shader_spirv *spirv);

	Frees any allocated memory and closes FD’s (if open) created after
	:c:func:`kmr_shader_spirv_create` call

	Parameters:
		| **session**
		| Must pass a valid pointer to a ``struct`` :c:struct:`kmr_shader_spirv`

	.. code-block::

		/* Free'd members with fd's closed */
		struct kmr_shader_spirv {
			void *result;
		};

=========================================================================================================================================

.. _libshaderc/shaderc.h: https://github.com/google/shaderc/blob/main/libshaderc/include/shaderc/shaderc.h
.. _shaderc_shader_kind: https://github.com/google/shaderc/blob/main/libshaderc/include/shaderc/shaderc.h
.. _VkShaderStageFlagBits: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderStageFlagBits.html
