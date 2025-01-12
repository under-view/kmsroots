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
 * @brief struct kmr_dma_buf (kmsroots DMA Buffer)
 *
 * @member err          - Stores information about the error that occured
 *                        for the given instance and may later be retrieved
 *                        by caller.
 * @member syncFdsCount - Array size of @syncFds
 * @member syncFds      - Pointer to an array of file descriptors used for synchronization
 *                        of size @syncFdsCount. These file descriptors may be imported
 *                        to a graphics API primitive. In Vulkan you can imported
 *                        via (VkImportSemaphoreFdInfoKHR -> vkImportSemaphoreFdKHR) or
 *                        by making a call to kmr_vk_sync_obj_import_external_sync_fd()
 */
struct kmr_dma_buf
{
	struct cando_log_error_struct err;
	unsigned int                  syncFdsCount;
	int                           syncFds[SYNC_FDS_MAX];
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
kmr_dma_buf_create (void)
{
	struct kmr_dma_buf *buffer = NULL;

	buffer = mmap(NULL,
	              sizeof(struct kmr_dma_buf),
	              PROT_READ,
	              MAP_PRIVATE|MAP_ANONYMOUS,
	              -1, 0);
	if (buffer == (void*)-1) {
		cando_log_error("mmap: %s\n", strerror(errno));
		return NULL;
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
                            const void *_importSyncInfo)
{
	int ret;

	uint8_t i;

	struct dma_buf_import_sync_file data;

	const struct kmr_dma_buf_import_sync_fd_info *importSyncInfo = _importSyncInfo;

	if (!buffer || \
	    !importSyncInfo)
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

	data.flags = importSyncInfo->syncFlags;
	data.fd = importSyncInfo->syncFileFd;

	for (i = 0; i < importSyncInfo->dmaBufferFdsCount; i++) {
		ret = drmIoctl(importSyncInfo->dmaBufferFds[i],
		               DMA_BUF_IOCTL_IMPORT_SYNC_FILE, &data);
		if (ret != 0) {
			cando_log_set_error(buffer, errno,
			                  "drmIoctl(DMA_BUF_IOCTL_IMPORT_SYNC_FILE)[dmaBufferFds[%u]]: %s",
			                  i, strerror(errno));
			close(importSyncInfo->syncFileFd);
			return -1;
		}
	}

	close(importSyncInfo->syncFileFd);

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
                            const void *_exportSyncInfo)
{
	uint8_t i;

	int ret = -1;

	struct dma_buf_export_sync_file data;

	const struct kmr_dma_buf_export_sync_fd_info *exportSyncInfo = _exportSyncInfo;

	if (!buffer || \
	    !exportSyncInfo)
	{
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	ret = dmabuf_check_sync_file_import_export(buffer);
	if (ret == -1) {
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA,
		                  "Exporting fds used for synchronization " \
		                  "from DMA-BUF fds not supported. " \
		                  "Must use kernel version >=5.20.0");
		return -1;
	} else if (ret == -2) {
		return -1;
	}

	ret = CANDO_PAGE_SET_WRITE(buffer, sizeof(struct kmr_dma_buf));
	if (ret == -1) {
		cando_log_set_error(buffer, errno, "mprotect: %s", strerror(errno));
		return -1;
	}

	buffer->syncFdsCount = exportSyncInfo->dmaBufferFdsCount;
	data.flags = exportSyncInfo->syncFlags;

	for (i = 0; i < buffer->syncFdsCount; i++) {
		data.fd = -1;

		ret = drmIoctl(exportSyncInfo->dmaBufferFds[i], DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &data);
		if (ret != 0) {
			cando_log_set_error(buffer, errno,
			                  "drmIoctl(DMA_BUF_IOCTL_EXPORT_SYNC_FILE)[dmaBufferFds[%u]]: %s",
			                  i, strerror(errno));
			return -1;
		}

		buffer->syncFds[i] = data.fd;
	}

	ret = CANDO_PAGE_SET_READ(buffer, sizeof(struct kmr_dma_buf));
	if (ret == -1) {
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

	for (b = 0; b < buffer->syncFdsCount; b++)
		close(buffer->syncFds[b]);

	munmap(buffer, sizeof(struct kmr_dma_buf));
}

/****************************************
 * End of kmr_dma_buf_destroy functions *
 ****************************************/
