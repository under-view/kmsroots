.. default-domain:: C

Buffer
======

Header: kmsroots/buffer.h

Table of contents (click to go)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

======
Macros
======

=====
Enums
=====

1. :c:enum:`kmr_buffer_type`

======
Unions
======

=======
Structs
=======

1. :c:struct:`kmr_buffer_object`
#. :c:struct:`kmr_buffer`
#. :c:struct:`kmr_buffer_create_info`

=========
Functions
=========

1. :c:func:`kmr_buffer_create`

API Documentation
~~~~~~~~~~~~~~~~~

===============
kmr_buffer_type
===============

.. c:enum:: kmr_buffer_type

	.. c:macro::
		KMR_BUFFER_DUMP_BUFFER
		KMR_BUFFER_GBM_BUFFER
		KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS

	Buffer allocation options used by :c:func:`kmr_buffer_create`

	:c:macro:`KMR_BUFFER_DUMP_BUFFER`
		| Value set to ``0``

	:c:macro:`KMR_BUFFER_GBM_BUFFER`
		| Value set to ``1``

	:c:macro:`KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS`
		| Value set to ``2``

=================
kmr_buffer_object
=================

.. c:struct:: kmr_buffer_object

	.. c:member::
		struct gbm_bo *bo;
		unsigned      fbid;
		unsigned      format;
		uint64_t      modifier;
		unsigned      planeCount;
		unsigned      pitches[4];
		unsigned      offsets[4];
		int           dmaBufferFds[4];
		int           kmsfd;

	More information can be found at `DrmMode`_

	:c:member:`bo`
		| Handle to some GEM allocated buffer. Used to get GEM handles, DMA buffer fds
		| (fd associate with GEM buffer), pitches, and offsets for the buffer used by
		| DRI device (GPU)

	:c:member:`fbid`
		| Framebuffer ID

	:c:member:`format`
		| The format of an image details how each pixel color channels is laid out in
		| memory: (i.e. RAM, VRAM, etc...). So, basically the width in bits, type, and
		| ordering of each pixels color channels.

	:c:member:`modifier`
		| The modifier details information on how pixels should be within a buffer for different types
		| operations such as scan out or rendering. (i.e linear, tiled, compressed, etc...)
		| `Linux Window Systems with DRM`_

	:c:member:`planeCount`
		| Number of Planar Formats. The number of :c:member:`dmaBufferFds`, :c:member:`offsets`, :c:member:`pitches`
		| retrieved per plane. More information can be found : `Planar Formats`_

	:c:member:`pitches`
		| width in bytes for each plane

	:c:member:`offsets`
		| offset of each plane

	:c:member:`dmaBufferFds`
		| (PRIME fd) Stores file descriptors to buffers that can be shared across hardware

	:c:member:`kmsfd`
		| File descriptor to open DRI device

==========
kmr_buffer
==========

.. c:struct:: kmr_buffer

	.. c:member::
		struct gbm_device        *gbmDevice;
		unsigned int             bufferCount;
		struct kmr_buffer_object *bufferObjects;

	:c:member:`gbmDevice`
		| A handle used to allocate gbm buffers & surfaces

	:c:member:`bufferCount`
		| Array size of :c:member:`bufferObjects`

	:c:member:`bufferObjects`
		| Stores an array of ``struct gbm_bo``'s and corresponding information about
		| the individual buffer.

======================
kmr_buffer_create_info
======================

.. c:struct:: kmr_buffer_create_info

	.. c:member::
		enum kmr_buffer_type bufferType;
		unsigned int         kmsfd;
		unsigned int         bufferCount;
		unsigned int         width;
		unsigned int         height;
		unsigned int         bitDepth;
		unsigned int         bitsPerPixel;
		unsigned int         gbmBoFlags;
		unsigned int         pixelFormat;
		uint64_t             *modifiers;
		unsigned int         modifierCount;

	:c:member:`bufferType`
		| Determines what type of buffer to allocate (i.e Dump Buffer, GBM buffer)

	:c:member:`kmsfd`
		| Used by ``gbm_create_device()``. Must be a valid file descriptor
		| to a DRI device (GPU character device file)

	:c:member:`bufferCount`
		| The amount of buffers to allocate.
		| 	2 for double buffering
 		|	3 for triple buffering

	:c:member:`width`
		| Amount of pixels going width wise on screen. Need to allocate buffer of similar size.

	:c:member:`height`
		| Amount of pixels going height wise on screen. Need to allocate buffer of similar size.

	:c:member:`bitDepth`
		| `Bit depth`_

	:c:member:`bitsPerPixel`
		| Pass the amount of bits per pixel

	:c:member:`gbmBoFlags`
		| Flags to indicate gbm_bo usage. More info here: `gbm.h`_

	:c:member:`pixelFormat`
		| The format of an image details how each pixel color channels is laid out in
		| memory: (i.e. RAM, VRAM, etc...). So basically the width in bits, type, and
		| ordering of each pixels color channels.

	:c:member:`modifiers`
		| List of drm format modifier

	:c:member:`modifierCount`
		| Number of drm format modifiers passed

=================
kmr_buffer_create
=================

.. c:function:: struct kmr_buffer kmr_buffer_create(struct kmr_buffer_create_info *kmrbuff);

	Function creates multiple GPU buffers

	Parameters:
		| **kmrbuff:** Pointer to a ``struct`` :c:struct:`kmr_buffer_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_buffer`
		| **on failure:** ``struct`` :c:struct:`kmr_buffer` { with members nulled }

=========================================================================================================================================

.. _Linux Window Systems with DRM: https://01.org/linuxgraphics/Linux-Window-Systems-with-DRM
.. _Planar Formats: https://en.wikipedia.org/wiki/Planar_(computer_graphics)
.. _DrmMode: https://gitlab.freedesktop.org/mesa/drm/-/blob/main/include/drm/drm_mode.h#L589
.. _gbm.h: https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/gbm/main/gbm.h#L213
.. _Bit depth: https://petapixel.com/2018/09/19/8-12-14-vs-16-bit-depth-what-do-you-really-need/
