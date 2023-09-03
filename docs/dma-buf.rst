.. default-domain:: C

dma-buf
=======

Header: kmsroots/dma-buf.h

Table of contents (click to go)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

======
Macros
======

=====
Enums
=====

1. :c:enum:`kmr_dma_buf_sync_flags`

======
Unions
======

=======
Structs
=======

1. :c:struct:`kmr_dma_buf_import_sync_file_create_info`
#. :c:struct:`kmr_dma_buf_export_sync_file`
#. :c:struct:`kmr_dma_buf_export_sync_file_create_info`
#. :c:struct:`kmr_dma_buf_export_sync_file_destroy_info`

=========
Functions
=========

1. :c:func:`kmr_dma_buf_import_sync_file_create`
#. :c:func:`kmr_dma_buf_export_sync_file_create`
#. :c:func:`kmr_dma_buf_export_sync_file_destroy`

API Documentation
~~~~~~~~~~~~~~~~~

Similar comments may be found in `wlroots dmabuf.h`_ as this API endpoint is
largely just a copy of what's in `wlroots`_.

======================
kmr_dma_buf_sync_flags
======================

.. c:enum:: kmr_dma_buf_sync_flags

	.. c:macro::
		KMR_DMA_BUF_SYNC_READ
		KMR_DMA_BUF_SYNC_WRITE
		KMR_DMA_BUF_SYNC_RW

	:c:macro:`KMR_DMA_BUF_SYNC_READ`
		| Value set to ``1``

	:c:macro:`KMR_DMA_BUF_SYNC_WRITE`
		| Value set to ``2``

	:c:macro:`KMR_DMA_BUF_SYNC_RW`
		| Value set to ``2``

=========================================================================================================================================

========================================
kmr_dma_buf_import_sync_file_create_info
========================================

.. c:struct:: kmr_dma_buf_import_sync_file_create_info

	.. c:member::
		uint8_t                dmaBufferFdsCount;
		int                    *dmaBufferFds;
		int                    syncFileFd;
		kmr_dma_buf_sync_flags syncFlags;

	:c:member:`dmaBufferFdsCount`
		| Array size of :c:member:`dmaBufferFds`

	:c:member:`dmaBufferFds`
		| Pointer to an array of file descriptors to DMA-BUF's of size :c:member:`dmaBufferFdsCount`.

	:c:member:`syncFileFd`
		| File descriptor to a graphics API synchronization primitive.
		| May be acquired in Vulkan via (`VkSemaphoreGetFdInfoKHR`_ -> `vkGetSemaphoreFdKHR`_) or
		| by making a call to :c:func:`kmr_vk_sync_obj_export_external_sync_fd`

	:c:member:`syncFlags`
		| Flags used to determine permission allowed after import

===================================
kmr_dma_buf_import_sync_file_create
===================================

.. c:function::  int kmr_dma_buf_import_sync_file_create(struct kmr_dma_buf_import_sync_file_create_info *kmrdma);

	Import single graphics API synchronization primitive file descriptor into an array of DMA-BUF
	file descriptors with ``DMA_BUF_IOCTL_IMPORT_SYNC_FILE``.

	Parameters:
		| **kmrdma:** pointer to a ``struct`` :c:struct:`kmr_dma_buf_import_sync_file_create_info`

	Returns:
		| **on success:** 0
		| **on failure:** -1

=========================================================================================================================================

============================
kmr_dma_buf_export_sync_file
============================

.. c:struct:: kmr_dma_buf_export_sync_file

	.. c:member::
		uint8_t syncFileFdsCount;
		int     *syncFileFds;

	:c:member:`syncFileFdsCount`
		| Array size of :c:member:`syncFileFds`

	:c:member:`syncFileFds`
		| Pointer to an array of file descriptors of size :c:member:`syncFileFdsCount`.
		| These file descriptors make be import to a graphics API primitive.
		| In Vulkan you can imported via
		| (`VkImportSemaphoreFdInfoKHR`_ -> `vkImportSemaphoreFdKHR`_)
		| or by making a call to :c:func:`kmr_vk_sync_obj_import_external_sync_fd`

========================================
kmr_dma_buf_export_sync_file_create_info
========================================

.. c:struct:: kmr_dma_buf_export_sync_file_create_info

	.. c:member::
		uint8_t                dmaBufferFdsCount;
		int                    *dmaBufferFds;
		kmr_dma_buf_sync_flags syncFlags;

	:c:member:`dmaBufferFdsCount`
		| Array size of :c:member:`dmaBufferFds`

	:c:member:`dmaBufferFds`
		| Pointer to an array of file descriptors to DMA-BUF's of size :c:member:`dmaBufferFdsCount`.

	:c:member:`syncFlags`
		| Flags used to determine permission allowed by file descriptor after export

===================================
kmr_dma_buf_export_sync_file_create
===================================

.. c:function:: struct kmr_dma_buf_export_sync_file kmr_dma_buf_export_sync_file_create(struct kmr_dma_buf_export_sync_file_create_info *kmrdma);

	Exports an array of synchronization file descriptors from an array of
	DMA-BUF file descriptors with ``DMA_BUF_IOCTL_EXPORT_SYNC_FILE``.

	Parameters:
		| **kmrdma:** pointer to a ``struct`` :c:struct:`kmr_dma_buf_export_sync_file_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_dma_buf_export_sync_file`
		| **on failure:** ``struct`` :c:struct:`kmr_dma_buf_export_sync_file` { with members nulled }

=========================================
kmr_dma_buf_export_sync_file_destroy_info
=========================================

.. c:struct:: kmr_dma_buf_export_sync_file_destroy_info

	.. c:member::
		uint32_t                            count;
		struct kmr_dma_buf_export_sync_file *data;

	:c:member:`count`
		| Must pass the amount of elements in ``struct`` :c:struct:`kmr_dma_buf_export_sync_file`

	:c:member:`data`
		| Must pass a pointer to an array of valid ``struct`` :c:struct:`kmr_dma_buf_export_sync_file`
		| Free'd members

		.. code-block::

			struct kmr_dma_buf_export_sync_file {
				int *syncFileFds;
			}

====================================
kmr_dma_buf_export_sync_file_destroy
====================================

.. c:function:: void kmr_dma_buf_export_sync_file_destroy(struct kmr_dma_buf_export_sync_file_destroy_info *kmrdma);

	frees any allocated memory defined by user

	Parameters:
		| **kmrdma:**
		| Pointer to a ``struct`` :c:struct:`kmr_dma_buff_export_sync_file_destroy_info` containing
		| all memory created during application lifetime in need free'ing

=========================================================================================================================================

.. _wlroots dmabuf.h: https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/include/render/dmabuf.h
.. _wlroots: https://gitlab.freedesktop.org/wlroots/wlroots
.. _VkSemaphoreGetFdInfoKHR: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSemaphoreGetFdInfoKHR.html
.. _vkGetSemaphoreFdKHR: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetSemaphoreFdKHR.html
.. _VkImportSemaphoreFdInfoKHR: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImportSemaphoreFdInfoKHR.html
.. _vkImportSemaphoreFdKHR: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkImportSemaphoreFdKHR.html
