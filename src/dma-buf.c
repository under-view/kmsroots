#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <linux/dma-buf.h>
#include <linux/version.h>
#include <sys/utsname.h>
#include <xf86drm.h>

#include <cando/cando.h>

#include "dma-buf.h"


/*
 * Largely just a copy of what's in wlroots
 * https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/render/dmabuf_linux.c
 */

#define SYNC_FDS_MAX 25

/*
 * @brief Structure defining kmsroots DMA Buffer instance.
 *
 * @member err            - Stores information about the error that occured
 *                          for the given instance and may later be retrieved
 *                          by caller.
 * @member free           - If structure allocated with calloc(3) member will be
 *                          set to true so that, we know to call free(3) when
 *                          destroying the instance.
 * @member sync_fds_count - Array size of @sync_fds
 * @member sync_fds       - Pointer to an array of file descriptors used for synchronization
 *                          of size @sync_fds_count. These file descriptors may be imported
 *                          to a graphics API primitive. In Vulkan you can imported
 *                          via (VkImportSemaphoreFdInfoKHR -> vkImportSemaphoreFdKHR) or
 *                          by making a call to kmr_vk_sync_obj_import_external_sync_fd()
 */
struct kmr_dma_buf
{
	struct cando_log_error_struct err;
	bool                          free;
	unsigned int                  sync_fds_count;
	int                           sync_fds[SYNC_FDS_MAX];
};


/************************************
 * Start of static global functions *
 ************************************/

/*
 * Check whether DMA-BUF import/export from/to sync_file is available.
 *
 * If this function returns true, dmabuf_import_sync_file() is supported.
 */
static int
dmabuf_check_sync_file_import_export (struct kmr_dma_buf *buffer)
{
	/*
	 * Unfortunately there's no better way to check the availability of the
	 * IOCTL than to check the kernel version. See the discussion at:
	 * https://lore.kernel.org/dri-devel/20220601161303.64797-1-contact@emersion.fr/
	 */
	char ch;
	size_t i;

	char *rel =  NULL;

	struct utsname utsname = {0};

	int major = 0, minor = 0, patch = 0, ret = -1;

	ret = uname(&utsname);
	if (ret == -1) {
		cando_log_set_error(buffer, errno, "uname: %s", strerror(errno));
		return -2;
	}

	ret = strncmp(utsname.sysname, "Linux", 8);
	if (ret != 0) {
		cando_log_set_error(buffer, CANDO_LOG_ERR_UNCOMMON,
		                  "strcmp: operating system name incorrect");
		return -2;
	}

	/* Trim release suffix if any, e.g. "-arch1-1" */
	for (i = 0; utsname.release[i] != '\0'; i++) {
		ch = utsname.release[i];
		if ((ch < '0' || ch > '9') && ch != '.') {
			utsname.release[i] = '\0';
			break;
		}
	}

	rel = strtok(utsname.release, ".");
	major = atoi(rel);

	rel = strtok(NULL, ".");
	if (rel != NULL) {
		minor = atoi(rel);
	}

	rel = strtok(NULL, ".");
	if (rel != NULL) {
		patch = atoi(rel);
	}

	return KERNEL_VERSION(major, minor, patch) >= KERNEL_VERSION(5, 20, 0) ? 0 : -1;
}


#if !defined(DMA_BUF_IOCTL_IMPORT_SYNC_FILE)

struct dma_buf_import_sync_file
{
	__u32 flags;
	__s32 fd;
};

#define DMA_BUF_IOCTL_IMPORT_SYNC_FILE _IOW(DMA_BUF_BASE, 3, struct dma_buf_import_sync_file)

#endif


#if !defined(DMA_BUF_IOCTL_EXPORT_SYNC_FILE)

struct dma_buf_export_sync_file
{
	__u32 flags;
	__s32 fd;
};

#define DMA_BUF_IOCTL_EXPORT_SYNC_FILE _IOWR(DMA_BUF_BASE, 2, struct dma_buf_export_sync_file)

#endif

/**********************************
 * End of static global functions *
 **********************************/


/*****************************************
 * Start of kmr_dma_buf_create functions *
 *****************************************/

struct kmr_dma_buf *
kmr_dma_buf_create (struct kmr_dma_buf *p_buffer)
{
	struct kmr_dma_buf *buffer = p_buffer;

	if (!buffer) {
		buffer = calloc(1, sizeof(struct kmr_dma_buf));
		if (!buffer) {
			cando_log_error("calloc: %s\n", strerror(errno));
			return NULL;
		}
	}

	return buffer;
}

/***************************************
 * End of kmr_dma_buf_create functions *
 ***************************************/


/*************************************************
 * Start of kmr_dma_buf_import_sync_fd functions *
 *************************************************/

int
kmr_dma_buf_import_sync_fd (struct kmr_dma_buf *buffer,
                            const void *p_import_sync_info)
{
	int ret;

	uint8_t i;

	struct dma_buf_import_sync_file data;

	const struct kmr_dma_buf_import_sync_fd_info *import_sync_info = p_import_sync_info;

	if (!buffer || \
	    !import_sync_info)
	{
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	ret = dmabuf_check_sync_file_import_export(buffer);
	if (ret == -1) {
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA,
		                    "Importing external fd used in synchronization " \
		                    "to DMA-BUF fds not supported. " \
		                    "Must use kernel version >=5.20.0");
		return -1;
	} else if (ret == -2) {
		return -1;
	}

	data.flags = import_sync_info->sync_flags;
	data.fd = import_sync_info->sync_fd;

	for (i = 0; i < import_sync_info->dma_buf_fds_count; i++) {
		ret = drmIoctl(import_sync_info->dma_buf_fds[i],
		               DMA_BUF_IOCTL_IMPORT_SYNC_FILE, &data);
		if (ret != 0) {
			cando_log_set_error(buffer, errno,
				"drmIoctl(DMA_BUF_IOCTL_IMPORT_SYNC_FILE)[dma_buf_fds[%u]]: %s",
				i, strerror(errno));
			close(import_sync_info->sync_fd);
			return -1;
		}
	}

	close(import_sync_info->sync_fd);

	return 0;
}

/***********************************************
 * End of kmr_dma_buf_import_sync_fd functions *
 ***********************************************/


/*************************************************
 * Start of kmr_dma_buf_export_sync_fd functions *
 *************************************************/

int
kmr_dma_buf_export_sync_fd (struct kmr_dma_buf *buffer,
                            const void *p_export_sync_info)
{
	uint8_t i;

	int err = -1;

	struct dma_buf_export_sync_file data;

	const struct kmr_dma_buf_export_sync_fd_info *export_sync_info = p_export_sync_info;

	if (!buffer || \
	    !export_sync_info)
	{
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	err = dmabuf_check_sync_file_import_export(buffer);
	if (err == -1) {
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA,
		                    "Exporting fds used for synchronization " \
		                    "from DMA-BUF fds not supported. " \
		                    "Must use kernel version >=5.20.0");
		return -1;
	} else if (err == -2) {
		return -1;
	}

	err = CANDO_PAGE_SET_WRITE(buffer, sizeof(struct kmr_dma_buf));
	if (err == -1) {
		cando_log_set_error(buffer, errno, "mprotect: %s", strerror(errno));
		return -1;
	}

	buffer->sync_fds_count = export_sync_info->dma_buf_fds_count;
	data.flags = export_sync_info->sync_flags;

	for (i = 0; i < buffer->sync_fds_count; i++) {
		data.fd = -1;

		err = drmIoctl(export_sync_info->dma_buf_fds[i],
		               DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &data);
		if (err != 0) {
			cando_log_set_error(buffer, errno,
				"drmIoctl(DMA_BUF_IOCTL_EXPORT_SYNC_FILE)[dma_buf_fds[%u]]: %s",
				i, strerror(errno));
			return -1;
		}

		buffer->sync_fds[i] = data.fd;
	}

	err = CANDO_PAGE_SET_READ(buffer, sizeof(struct kmr_dma_buf));
	if (err == -1) {
		cando_log_set_error(buffer, errno, "mprotect: %s", strerror(errno));
		return -1;
	}

	return 0;
}

/***********************************************
 * End of kmr_dma_buf_export_sync_fd functions *
 ***********************************************/


/******************************************
 * Start of kmr_dma_buf_destroy functions *
 ******************************************/

void
kmr_dma_buf_destroy (struct kmr_dma_buf *buffer)
{
	unsigned int b;

	if (!buffer)
		return;

	for (b = 0; b < buffer->sync_fds_count; b++) {
		close(buffer->sync_fds[b]);
		buffer->sync_fds[b] = -1;
	}

	if (buffer->free) {
		free(buffer);
	} else {
		memset(buffer, 0, sizeof(struct kmr_dma_buf));
	}
}

/****************************************
 * End of kmr_dma_buf_destroy functions *
 ****************************************/


/***************************************************
 * Start of non struct kmr_dma_buf param functions *
 ***************************************************/

int
kmr_dma_buf_get_sizeof (void)
{
	return sizeof(struct kmr_dma_buf);
}

/*************************************************
 * End of non struct kmr_dma_buf param functions *
 *************************************************/
