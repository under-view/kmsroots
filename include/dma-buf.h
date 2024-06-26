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
 *                      May be acquired in Vulkan via (VkSemaphoreGetFdInfoKHR -> vkGetSemaphoreFdKHR) or
 *                      by making a call to kmr_vk_sync_obj_export_external_sync_fd(). Will be closed on
 *                      success or failure.
 * @syncFlags         - Flags used to determine permission allowed after import
 */
struct kmr_dma_buf_import_sync_file_create_info {
	uint8_t                dmaBufferFdsCount;
	int                    *dmaBufferFds;
	int                    syncFileFd;
	kmr_dma_buf_sync_flags syncFlags;
};


/*
 * kmr_dma_buf_import_sync_file_create: Import a single file descriptor to a graphics API synchronization primitive
 *                                      into an array of DMA-BUF file descriptors with drmIoctl(DMA_BUF_IOCTL_IMPORT_SYNC_FILE).
 *
 * parameters:
 * @importSyncFileInfo - pointer to a struct kmr_dma_buf_import_sync_file_create_info
 * returns:
 *	on success 0
 *	on failure -1
 */
int
kmr_dma_buf_import_sync_file_create (struct kmr_dma_buf_import_sync_file_create_info *importSyncFileInfo);


/*
 * struct kmr_dma_buf_export_sync_file (kmsroots DMA Buffer Export Synchronization File Descriptor)
 *
 * members:
 * @syncFileFdsCount - Array size of @syncFileFds
 * @syncFileFds      - Pointer to an array of file descriptors used for synchronization
 *                     of size @syncFileFdsCount. These file descriptors may be imported
 *                     to a graphics API primitive. In Vulkan you can imported
 *                     via (VkImportSemaphoreFdInfoKHR -> vkImportSemaphoreFdKHR) or
 *                     by making a call to kmr_vk_sync_obj_import_external_sync_fd()
 */
struct kmr_dma_buf_export_sync_file {
	uint8_t syncFileFdsCount;
	int     *syncFileFds;
};


/*
 * struct kmr_dma_buf_export_sync_file_create_info (kmsroots DMA Buffer Export Synchronization File Descriptor Create Info)
 *
 * members:
 * @dmaBufferFdsCount - Array size of @dmaBufferFds
 * @dmaBufferFds      - Pointer to an array of file descriptors. These file descriptors point to
 *                      DMA-BUF's of size @dmaBufferFdsCount.
 * @syncFlags         - Flags used to determine permission allowed by file descriptor after export
 */
struct kmr_dma_buf_export_sync_file_create_info {
	uint8_t                dmaBufferFdsCount;
	int                    *dmaBufferFds;
	kmr_dma_buf_sync_flags syncFlags;
};


/*
 * kmr_dma_buf_export_sync_file_create: Exports an array of synchronization file descriptors from an array of
 *                                      DMA-BUF file descriptors with drmIoctl(DMA_BUF_IOCTL_EXPORT_SYNC_FILE).
 *
 * parameters:
 * @exportSyncFileInfo - Pointer to a struct kmr_dma_buf_export_sync_file_create_info
 * returns:
 *	on success Pointer to a struct kmr_dma_buf_export_sync_file
 *	on failure NULL
 */
struct kmr_dma_buf_export_sync_file *
kmr_dma_buf_export_sync_file_create (struct kmr_dma_buf_export_sync_file_create_info *exportSyncFileInfo);


/*
 * kmr_dma_buf_export_sync_file_destroy: Frees any allocated memory and closes FD's (if open) created after
 *                                       kmr_dma_buf_export_sync_file_create() call.
 *
 * parameters:
 * @exportSyncFile - Pointer to a valid struct kmr_dma_buf_export_sync_file
 *
 *                   Free'd members with fd's closed
 *                   struct kmr_dma_buf_export_sync_file {
 *          	         int *syncFileFds;
 *                   }
 */
void
kmr_dma_buf_export_sync_file_destroy (struct kmr_dma_buf_export_sync_file *exportSyncFile);


#endif /* KMR_DMA_BUF_H */
