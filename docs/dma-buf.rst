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

=========
Functions
=========

1. :c:func:`kmr_dma_buf_import_sync_file_create`
#. :c:func:`kmr_dma_buf_export_sync_file_create`
#. :c:func:`kmr_dma_buf_export_sync_file_destroy`

=================
Function Pointers
=================

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
		| Value set to ``3``

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
		| by making a call to :c:func:`kmr_vk_sync_obj_export_external_sync_fd`. Will be closed on
		| success or failure.

	:c:member:`syncFlags`
		| Flags used to determine permission allowed after import

===================================
kmr_dma_buf_import_sync_file_create
===================================

.. c:function::  int kmr_dma_buf_import_sync_file_create(struct kmr_dma_buf_import_sync_file_create_info *importSyncFileInfo);

	Import a single file descriptor to a graphics API synchronization primitive
	into an array of DMA-BUF file descriptors with ``drmIoctl(DMA_BUF_IOCTL_IMPORT_SYNC_FILE)``.

	Parameters:
		| **importSyncFileInfo:** pointer to a ``struct`` :c:struct:`kmr_dma_buf_import_sync_file_create_info`

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
		| Pointer to an array of file descriptors used for synchronization
		| of size :c:member:`syncFileFdsCount`. These file descriptors may
		| be imported to a graphics API primitive. In Vulkan you can import via
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
		| Pointer to an array of file descriptors. These file descriptors point to
		| DMA-BUF's of size :c:member:`dmaBufferFdsCount`.

	:c:member:`syncFlags`
		| Flags used to determine permission allowed by file descriptor after export

===================================
kmr_dma_buf_export_sync_file_create
===================================

.. c:function:: struct kmr_dma_buf_export_sync_file *kmr_dma_buf_export_sync_file_create(struct kmr_dma_buf_export_sync_file_create_info *exportSyncFileInfo);

	Exports an array of synchronization file descriptors from an array of
	DMA-BUF file descriptors with ``drmIoctl(DMA_BUF_IOCTL_EXPORT_SYNC_FILE)``.

	Parameters:
		| **exportSyncFileInfo:** pointer to a ``struct`` :c:struct:`kmr_dma_buf_export_sync_file_create_info`

	Returns:
		| **on success:** Pointer to a ``struct`` :c:struct:`kmr_dma_buf_export_sync_file`
		| **on failure:** NULL

====================================
kmr_dma_buf_export_sync_file_destroy
====================================

.. c:function:: void kmr_dma_buf_export_sync_file_destroy(struct kmr_dma_buf_export_sync_file *exportSyncFile);

	Frees any allocated memory and closes FD's (if open) created after
	:c:func:`kmr_dma_buf_export_sync_file_create` call.

	Parameters:
		| **exportSyncFile:** Pointer to a valid ``struct`` :c:struct:`kmr_dma_buf_export_sync_file`

		.. code-block::

			/* Free'd members with fd's closed */
			struct kmr_dma_buf_export_sync_file {
				int *syncFileFds;
			}

=========================================================================================================================================

.. _wlroots dmabuf.h: https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/include/render/dmabuf.h
.. _wlroots: https://gitlab.freedesktop.org/wlroots/wlroots
.. _VkSemaphoreGetFdInfoKHR: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSemaphoreGetFdInfoKHR.html
.. _vkGetSemaphoreFdKHR: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkGetSemaphoreFdKHR.html
.. _VkImportSemaphoreFdInfoKHR: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImportSemaphoreFdInfoKHR.html
.. _vkImportSemaphoreFdKHR: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkImportSemaphoreFdKHR.html
