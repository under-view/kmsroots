#ifndef KMR_DMA_BUF_H
#define KMR_DMA_BUF_H

#include "utils.h"

/*
 * similar comments may be found in wlroots
 * https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/include/render/dmabuf.h
 */


/*
 * enum kmr_dma_buf_sync_flags (kmsroots DMA Buffer Synchronization Flags)
 *
 */
typedef enum _kmr_dma_buf_sync_flags {
	KMR_DMA_BUF_SYNC_READ  = (1 << 0),
	KMR_DMA_BUF_SYNC_WRITE = (2 << 0),
	KMR_DMA_BUF_SYNC_RW    = (KMR_DMA_BUF_SYNC_READ | KMR_DMA_BUF_SYNC_WRITE)
} kmr_dma_buf_sync_flags;


/*
 * struct kmr_dma_buf_import_sync_file_create_info (kmsroots DMA Buffer Import Synchronization File Descriptor Create Info)
 *
 * members:
 * @dmaBufferFdsCount - Array size of @dmaBufferFds
 * @dmaBufferFds      - Pointer to an array of file descriptors to DMA-BUF's of size @dmaBufferFdsCount.
 * @syncFileFd        - File descriptor to a graphics API synchronization primitive.
 * @syncFlags         - Flags used to determine permission allowed after import
 */
struct kmr_dma_buf_import_sync_file_create_info {
	uint8_t                dmaBufferFdsCount;
	int                    *dmaBufferFds;
	int                    syncFileFd;
	kmr_dma_buf_sync_flags syncFlags;
};


/*
 * kmr_dma_buf_import_sync_file_create: Import single graphics API synchronization primitive file descriptor into an array of DMA-BUF
 *                                      file descriptors with DMA_BUF_IOCTL_IMPORT_SYNC_FILE. @syncFileFd in vulkan may be acquired via
 *                                      (VkSemaphoreGetFdInfoKHR->vkGetSemaphoreFdKHR)
 *
 * parameters:
 * @kmrdma - pointer to a struct kmr_dma_buf_import_sync_file_create_info
 * returns:
 *	on success 0
 *	on failure -1
 */
int kmr_dma_buf_import_sync_file_create(struct kmr_dma_buf_import_sync_file_create_info *kmrdma);


/*
 * struct kmr_dma_buf_export_sync_file_create_info (kmsroots DMA Buffer Export Synchronization File Descriptor Create Info)
 *
 * members:
 * @dmaBufferFdsCount - Array size of @dmaBufferFds
 * @dmaBufferFds      - Pointer to an array of file descriptors to DMA-BUF's of size @dmaBufferFdsCount.
 * @syncFlags         - Flags used to determine permission allowed after export
 */
struct kmr_dma_buf_export_sync_file_create_info {
	uint8_t                dmaBufferFdsCount;
	int                    *dmaBufferFds;
	kmr_dma_buf_sync_flags syncFlags;
};


/*
 * struct kmr_dma_buf_export_sync_file (kmsroots DMA Buffer Export Synchronization File Descriptor)
 *
 * members:
 * @syncFileFdsCount - Array size of @syncFileFds
 * @syncFileFds      - Pointer to an array of file descriptors of size @syncFileFdsCount. These file descriptors
 *                     can then be imported by graphics API implementation.
 */
struct kmr_dma_buf_export_sync_file {
	uint8_t syncFileFdsCount;
	int     *syncFileFds;
};


/*
 * kmr_dma_buf_export_sync_file_create: Exports an array of synchronization file descriptors from an array of
 *                                      DMA-BUF file descriptors with DMA_BUF_IOCTL_EXPORT_SYNC_FILE. @syncFileFds
 *                                      in vulkan may be imported via (VkImportSemaphoreFdInfoKHR->vkImportSemaphoreFdKHR).
 *
 * parameters:
 * @kmrdma - pointer to a struct kmr_dma_buf_export_sync_file_create_info
 * returns:
 *	on success struct kmr_dma_buf_export_sync_file
 *	on failure struct kmr_dma_buf_export_sync_file { with members nulled }
 */
struct kmr_dma_buf_export_sync_file kmr_dma_buf_export_sync_file_create(struct kmr_dma_buf_export_sync_file_create_info *kmrdma);


/*
 * struct kmr_dma_buf_destroy (kmsroots DMA Buffer Destroy)
 *
 * members:
 *
 * @kmr_dma_buf_export_sync_file_cnt - Must pass the amount of elements in struct kmr_dma_buf_export_sync_file
 * @kmr_dma_buf_export_sync_file     - Must pass a pointer to an array of valid struct kmr_dma_buf_export_sync_file { free'd members: @syncFileFds }
 */
struct kmr_dma_buf_destroy {
	uint32_t                            kmr_dma_buf_export_sync_file_cnt;
	struct kmr_dma_buf_export_sync_file *kmr_dma_buf_export_sync_file;
};


/*
 * kmr_dma_buf_destroy: frees any allocated memory defined by user
 *
 * parameters:
 * @kmrdma - pointer to a struct kmr_vk_destroy contains all objects created during
 *          application lifetime in need freeing
 */
void kmr_dma_buf_destroy(struct kmr_dma_buf_destroy *kmrdma);


#endif
