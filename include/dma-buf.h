#ifndef KMR_DMA_BUF_H
#define KMR_DMA_BUF_H

/*
 * similar comments may be found in wlroots
 * https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/include/render/dmabuf.h
 */


/*
 * Stores information about the kmr_dma_buf instance.
 */
struct kmr_dma_buf;


/*
 * @brief Creates struct kmr_dma_buf context.
 *
 * @returns
 * 	on success: Pointer to a struct kmr_dma_buf
 * 	on failure: NULL
 */
struct kmr_dma_buf *
kmr_dma_buf_create (void);


/*
 * @brief enum kmr_dma_buf_sync_flags (kmsroots DMA Buffer Synchronization Flags)
 */
enum kmr_dma_buf_sync_flags
{
	KMR_DMA_BUF_SYNC_READ  = (1 << 0),
	KMR_DMA_BUF_SYNC_WRITE = (2 << 0),
	KMR_DMA_BUF_SYNC_RW    = (KMR_DMA_BUF_SYNC_READ | KMR_DMA_BUF_SYNC_WRITE)
};


/*
 * @brief struct kmr_dma_buf_import_sync_fd_info
 *        (kmsroots DMA Buffer Import Synchronization File Descriptor Info)
 *
 * @member dmaBufferFdsCount - Array size of @dmaBufferFds
 * @member dmaBufferFds      - Pointer to an array of file descriptors to
 *                             DMA-BUF's of size @dmaBufferFdsCount.
 * @member syncFileFd        - File descriptor to a graphics API synchronization
 *                             primitive. May be acquired in Vulkan via
 *                             (VkSemaphoreGetFdInfoKHR -> vkGetSemaphoreFdKHR) or
 *                             by making a call to kmr_vk_sync_obj_export_external_sync_fd().
 *                             Will be closed on success or failure.
 * @member syncFlags         - Flags used to determine permission allowed after import
 */
struct kmr_dma_buf_import_sync_fd_info
{
	uint8_t                     dmaBufferFdsCount;
	int                         *dmaBufferFds;
	int                         syncFileFd;
	enum kmr_dma_buf_sync_flags syncFlags;
};


/*
 * @brief Import a single file descriptor to a graphics API synchronization primitive
 *        into an array of DMA-BUF file descriptors with drmIoctl(DMA_BUF_IOCTL_IMPORT_SYNC_FILE).
 *
 * @param buffer         - Pointer to a struct kmr_dma_buf
 * @param importSyncInfo - Pointer to a struct kmr_dma_buf_import_sync_fd_info
 *
 * @returns
 *	on success: 0
 *	on failure: -1
 */
int
kmr_dma_buf_import_sync_fd (struct kmr_dma_buf *buffer,
                            const void *importSyncInfo);


/*
 * @brief struct kmr_dma_buf_export_sync_fd_info
 *        (kmsroots DMA Buffer Export Synchronization File Descriptor Info)
 *
 * @dmaBufferFdsCount - Array size of @dmaBufferFds
 * @dmaBufferFds      - Pointer to an array of file descriptors. These file descriptors point to
 *                      DMA-BUF's of size @dmaBufferFdsCount.
 * @syncFlags         - Flags used to determine permission allowed by file descriptor after export
 */
struct kmr_dma_buf_export_sync_fd_info
{
	uint8_t                     dmaBufferFdsCount;
	int                         *dmaBufferFds;
	enum kmr_dma_buf_sync_flags syncFlags;
};


/*
 * @brief Exports an array of synchronization file descriptors from an array of
 *        DMA-BUF file descriptors with drmIoctl(DMA_BUF_IOCTL_EXPORT_SYNC_FILE).
 *
 * @param buffer         - Pointer to a struct kmr_dma_buf
 * @param exportSyncInfo - Pointer to a struct kmr_dma_buf_export_sync_fd_info
 *
 * @returns
 *	on success: 0
 *	on failure: -1
 */
int
kmr_dma_buf_export_sync_fd (struct kmr_dma_buf *buffer,
                            const void *exportSyncInfo);


/*
 * @brief Frees any allocated memory and closes FD's (if open) created after
 *        kmr_dma_buf_create() call.
 *
 * @param bufffer - Pointer to a valid struct kmr_dma_buf
 */
void
kmr_dma_buf_destroy (struct kmr_dma_buf *buffer);

#endif /* KMR_DMA_BUF_H */
