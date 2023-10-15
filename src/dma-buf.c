#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <linux/dma-buf.h>
#include <linux/version.h>
#include <sys/utsname.h>
#include <xf86drm.h>

#include "dma-buf.h"


/*
 * Largely just a copy of what's in wlroots
 * https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/render/dmabuf_linux.c
 */

/************************************
 * START OF STATIC GLOBAL FUNCTIONS *
 ************************************/

/*
 * Check whether DMA-BUF import/export from/to sync_file is available.
 *
 * If this function returns true, dmabuf_import_sync_file() is supported.
 */
static int
dmabuf_check_sync_file_import_export (void)
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
	int major = 0, minor = 0, patch = 0;

	if (uname(&utsname) == -1) {
		kmr_utils_log(KMR_DANGER, "[x] uname: %s", strerror(errno));
		return false;
	}

	if (strncmp(utsname.sysname, "Linux", 8) != 0) {
		kmr_utils_log(KMR_DANGER, "[x] strcmp: operating system name incorrect");
		return false;
	}

	// Trim release suffix if any, e.g. "-arch1-1"
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

struct dma_buf_import_sync_file {
	__u32 flags;
	__s32 fd;
};

#define DMA_BUF_IOCTL_IMPORT_SYNC_FILE _IOW(DMA_BUF_BASE, 3, struct dma_buf_import_sync_file)

#endif


#if !defined(DMA_BUF_IOCTL_EXPORT_SYNC_FILE)

struct dma_buf_export_sync_file {
	__u32 flags;
	__s32 fd;
};

#define DMA_BUF_IOCTL_EXPORT_SYNC_FILE _IOWR(DMA_BUF_BASE, 2, struct dma_buf_export_sync_file)

#endif

/**********************************
 * END OF STATIC GLOBAL FUNCTIONS *
 **********************************/

/**********************************************************
 * START OF kmr_dma_buf_import_sync_file_create FUNCTIONS *
 **********************************************************/

int
kmr_dma_buf_import_sync_file_create (struct kmr_dma_buf_import_sync_file_create_info *importSyncFileInfo)
{
	int ret;
	uint8_t i;

	struct dma_buf_import_sync_file data;

	ret = dmabuf_check_sync_file_import_export();
	if (ret == -1) {
		kmr_utils_log(KMR_DANGER, "[x] Importing external fd used in synchronization to DMA-BUF fds not supported.");
		kmr_utils_log(KMR_DANGER, "[x] Must use kernel version >=5.20.0");
		return -1;
	}

	data.flags = importSyncFileInfo->syncFlags;
	data.fd = importSyncFileInfo->syncFileFd;

	for (i = 0; i < importSyncFileInfo->dmaBufferFdsCount; i++) {
		ret = drmIoctl(importSyncFileInfo->dmaBufferFds[i], DMA_BUF_IOCTL_IMPORT_SYNC_FILE, &data);
		if (ret != 0) {
			kmr_utils_log(KMR_DANGER, "[x] drmIoctl(DMA_BUF_IOCTL_IMPORT_SYNC_FILE)[dmaBufferFds[%u]]: %s", i, strerror(errno));
			close(importSyncFileInfo->syncFileFd);
			return -1;
		}
	}

	close(importSyncFileInfo->syncFileFd);

	return 0;
}

/********************************************************
 * END OF kmr_dma_buf_import_sync_file_create FUNCTIONS *
 ********************************************************/

/**********************************************************
 * START OF kmr_dma_buf_export_sync_file_create FUNCTIONS *
 **********************************************************/

struct kmr_dma_buf_export_sync_file *
kmr_dma_buf_export_sync_file_create (struct kmr_dma_buf_export_sync_file_create_info *exportSyncFileInfo)
{
	uint8_t i;
	int ret = -1;

	struct dma_buf_export_sync_file data;
	struct kmr_dma_buf_export_sync_file *exportSyncFile = NULL;

	ret = dmabuf_check_sync_file_import_export();
	if (ret == -1) {
		kmr_utils_log(KMR_DANGER, "[x] Exporting fds used for synchronization from DMA-BUF fds not supported.");
		kmr_utils_log(KMR_DANGER, "[x] Must use kernel version >=5.20.0");
		goto exit_error_kmr_dma_buf_export_sync_file;
	}

	exportSyncFile = calloc(1, sizeof(struct kmr_dma_buf_export_sync_file_create_info));
	if (!exportSyncFile) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_dma_buf_export_sync_file;
	}

	exportSyncFile->syncFileFdsCount = exportSyncFileInfo->dmaBufferFdsCount;

	exportSyncFile->syncFileFds = calloc(exportSyncFile->syncFileFdsCount, sizeof(int));
	if (!exportSyncFile->syncFileFds) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_dma_buf_export_sync_file;
	}

	data.flags = exportSyncFileInfo->syncFlags;
	for (i = 0; i < exportSyncFile->syncFileFdsCount; i++) {
		data.fd = -1;

		ret = drmIoctl(exportSyncFileInfo->dmaBufferFds[i], DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &data);
		if (ret != 0) {
			kmr_utils_log(KMR_DANGER, "[x] drmIoctl(DMA_BUF_IOCTL_EXPORT_SYNC_FILE)[dmaBufferFds[%u]]: %s", i, strerror(errno));
			goto exit_error_kmr_dma_buf_export_sync_file;
		}

		exportSyncFile->syncFileFds[i] = data.fd;
	}

	return exportSyncFile;

exit_error_kmr_dma_buf_export_sync_file:
	kmr_dma_buf_export_sync_file_destroy(exportSyncFile);
	return NULL;
}


void
kmr_dma_buf_export_sync_file_destroy (struct kmr_dma_buf_export_sync_file *exportSyncFile)
{
	uint32_t b;

	if (!exportSyncFile)
		return;

	for (b = 0; b < exportSyncFile->syncFileFdsCount; b++) {
		if (exportSyncFile->syncFileFds[b] >= 0)
			close(exportSyncFile->syncFileFds[b]);
	}

	free(exportSyncFile->syncFileFds);
	free(exportSyncFile);
}

/******************************************************************
 * END OF kmr_dma_buf_export_sync_file_{create,destroy} FUNCTIONS *
 ******************************************************************/
