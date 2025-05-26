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
 * @brief Creates struct kmr_dma_buf instance.
 *
 * @param buffer - May be NULL or a pointer to a struct kmr_dma_buf.
 *                 If NULL memory will be allocated and return to
 *                 caller. If not NULL address passed will be used
 *                 to store the newly created struct kmr_dma_buf
 *                 instance.
 *
 * @returns
 * 	on success: Pointer to a struct kmr_dma_buf
 * 	on failure: NULL
 */
struct kmr_dma_buf *
kmr_dma_buf_create (struct kmr_dma_buf *buffer);


/*
 * @brief Enumeration defining kmsroots DMA Buffer Synchronization Flags
 */
enum kmr_dma_buf_sync_flags
{
	KMR_DMA_BUF_SYNC_READ  = (1 << 0),
	KMR_DMA_BUF_SYNC_WRITE = (2 << 0),
	KMR_DMA_BUF_SYNC_RW    = (KMR_DMA_BUF_SYNC_READ | KMR_DMA_BUF_SYNC_WRITE)
};


/*
 * @brief Structure defining kmsroots DMA Buffer
 *        Import Synchronization File Descriptor Info.
 *
 * @member dma_buf_fds_count - Array size of @dma_buf_fds.
 * @member dma_buf_fds       - Pointer to an array of file descriptors to
 *                             DMA-BUF's of size @dma_buf_fds_count.
 * @member sync_fd           - File descriptor to a graphics API synchronization
 *                             primitive. May be acquired in Vulkan via
 *                             (VkSemaphoreGetFdInfoKHR -> vkGetSemaphoreFdKHR) or
 *                             by making a call to kmr_vk_sync_obj_export_external_sync_fd().
 *                             Will be closed on success or failure.
 * @member sync_flags        - Flags used to determine permission allowed after import.
 */
struct kmr_dma_buf_import_sync_fd_info
{
	unsigned int                dma_buf_fds_count;
	int                         *dma_buf_fds;
	int                         sync_fd;
	enum kmr_dma_buf_sync_flags sync_flags;
};


/*
 * @brief Import a single file descriptor to a
 *        graphics API synchronization primitive
 *        into an array of DMA-BUF file descriptors
 *        with drmIoctl(DMA_BUF_IOCTL_IMPORT_SYNC_FILE).
 *
 * @param buffer           - Pointer to a struct kmr_dma_buf.
 * @param import_sync_info - Pointer to a struct kmr_dma_buf_import_sync_fd_info.
 *                           The use of pointer to a void is to limit amount
 *                           of columns required to define a function.
 *
 * @returns
 *	on success: 0
 *	on failure: -1
 */
int
kmr_dma_buf_import_sync_fd (struct kmr_dma_buf *buffer,
                            const void *import_sync_info);


/*
 * @brief Structure defining kmsroots DMA Buffer
 *        Export Synchronization File Descriptor Info.
 *
 * @member dma_buf_fds_count - Array size of @dma_buf_fds.
 * @member dma_buf_fds       - Pointer to an array of file descriptors.
 *                             These file descriptors point to DMA-BUF's
 *                             of size @dma_buf_fds_count.
 * @member sync_flags        - Flags used to determine permission allowed
 *                             by file descriptor after export.
 */
struct kmr_dma_buf_export_sync_fd_info
{
	uint8_t                     dma_buf_fds_count;
	int                         *dma_buf_fds;
	enum kmr_dma_buf_sync_flags sync_flags;
};


/*
 * @brief Exports an array of synchronization
 *        file descriptors from an array of
 *        DMA-BUF file descriptors with
 *        drmIoctl(DMA_BUF_IOCTL_EXPORT_SYNC_FILE).
 *
 * @param buffer           - Pointer to a struct kmr_dma_buf.
 * @param export_sync_info - Pointer to a struct kmr_dma_buf_export_sync_fd_info.
 *                           The use of pointer to a void is to limit amount
 *                           of columns required to define a function.
 *
 * @returns
 *	on success: 0
 *	on failure: -1
 */
int
kmr_dma_buf_export_sync_fd (struct kmr_dma_buf *buffer,
                            const void *export_sync_info);


/*
 * @brief Frees any allocated memory and closes FD's (if open) created after
 *        kmr_dma_buf_create() call.
 *
 * @param bufffer - Pointer to a valid struct kmr_dma_buf.
 */
void
kmr_dma_buf_destroy (struct kmr_dma_buf *buffer);


/*
 * @brief Returns size of the internal structure. So,
 *        if caller decides to allocate memory outside
 *        of API interface they know the exact amount
 *        of bytes.
 *
 * @return
 *	on success: sizeof(struct kmr_dma_buf)
 *	on failure: sizeof(struct kmr_dma_buf)
 */
int
kmr_dma_buf_get_sizeof (void);

#endif /* KMR_DMA_BUF_H */
