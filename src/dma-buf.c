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


/**
 * Check whether DMA-BUF import/export from/to sync_file is available.
 *
 * If this function returns true, dmabuf_import_sync_file() is supported.
 */
static bool dmabuf_check_sync_file_import_export(void)
{
	/* Unfortunately there's no better way to check the availability of the
	 * IOCTL than to check the kernel version. See the discussion at:
	 * https://lore.kernel.org/dri-devel/20220601161303.64797-1-contact@emersion.fr/
	 */

	struct utsname utsname = {0};
	if (uname(&utsname) == -1) {
		kmr_utils_log(KMR_DANGER, "[x] uname: %s", strerror(errno));
		return false;
	}

	if (strncmp(utsname.sysname, "Linux", 8) != 0) {
		kmr_utils_log(KMR_DANGER, "[x] strcmp: operating system name incorrect");
		return false;
	}

	// Trim release suffix if any, e.g. "-arch1-1"
	for (size_t i = 0; utsname.release[i] != '\0'; i++) {
		char ch = utsname.release[i];
		if ((ch < '0' || ch > '9') && ch != '.') {
			utsname.release[i] = '\0';
			break;
		}
	}

	char *rel = strtok(utsname.release, ".");
	int major = atoi(rel);

	int minor = 0;
	rel = strtok(NULL, ".");
	if (rel != NULL) {
		minor = atoi(rel);
	}

	int patch = 0;
	rel = strtok(NULL, ".");
	if (rel != NULL) {
		patch = atoi(rel);
	}

	return KERNEL_VERSION(major, minor, patch) >= KERNEL_VERSION(5, 20, 0);
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


int kmr_dma_buf_import_sync_file_create(struct kmr_dma_buf_import_sync_file_create_info *kmrdma)
{
	struct dma_buf_import_sync_file data;

	if (dmabuf_check_sync_file_import_export() == false) {
		kmr_utils_log(KMR_DANGER, "[x] kmr_dma_buf_import_sync_file_create: importing external fd used in synchronization to DMA-BUF fds not supported.");
		return -1;
	}

	data.flags = kmrdma->syncFlags;
	data.fd = kmrdma->syncFileFd;

	for (uint8_t i = 0; i < kmrdma->dmaBufferFdsCount; i++) {
		if (drmIoctl(kmrdma->dmaBufferFds[i], DMA_BUF_IOCTL_IMPORT_SYNC_FILE, &data) != 0) {
			kmr_utils_log(KMR_DANGER, "[x] drmIoctl(DMA_BUF_IOCTL_IMPORT_SYNC_FILE)[dmaBufferFds[%u]]: %s", i, strerror(errno));
			return -1;
		}
	}

	close(kmrdma->syncFileFd);

	return 0;
}


struct kmr_dma_buf_export_sync_file kmr_dma_buf_export_sync_file_create(struct kmr_dma_buf_export_sync_file_create_info *kmrdma)
{
	int *syncFileFds = NULL;
	uint8_t syncFileFdsCount, i;
	struct dma_buf_export_sync_file data;

	if (dmabuf_check_sync_file_import_export() == false) {
		kmr_utils_log(KMR_DANGER, "[x] kmr_dma_buf_export_sync_file_create: exporting fds used for synchronization from DMA-BUF fds not supported.");
		goto exit_error_kmr_dma_buf_export_sync_file;
	}

	syncFileFdsCount = kmrdma->dmaBufferFdsCount;

	syncFileFds = calloc(syncFileFdsCount, sizeof(int));
	if (!syncFileFds) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_dma_buf_export_sync_file;
	}

	data.flags = kmrdma->syncFlags;
	for (i = 0; i < syncFileFdsCount; i++) {
		data.fd = -1;

		if (drmIoctl(kmrdma->dmaBufferFds[i], DMA_BUF_IOCTL_EXPORT_SYNC_FILE, &data) != 0) {
			kmr_utils_log(KMR_DANGER, "[x] drmIoctl(DMA_BUF_IOCTL_EXPORT_SYNC_FILE)[dmaBufferFds[%u]]: %s", i, strerror(errno));
			goto exit_error_kmr_dma_buf_export_sync_file_free_syncFileFds;
		}

		syncFileFds[i] = data.fd;
	}

	return (struct kmr_dma_buf_export_sync_file) { .syncFileFds = syncFileFds, .syncFileFdsCount = syncFileFdsCount };

exit_error_kmr_dma_buf_export_sync_file_free_syncFileFds:
	for (i = 0; i < syncFileFdsCount; i++) {
		if (syncFileFds[i])
			close(syncFileFds[i]);
	}
	free(syncFileFds);
exit_error_kmr_dma_buf_export_sync_file:
	return (struct kmr_dma_buf_export_sync_file) { .syncFileFds = NULL, .syncFileFdsCount = 0 };
}


void kmr_dma_buf_destroy(struct kmr_dma_buf_destroy *kmrdma)
{
	uint32_t i, j;

	for (i = 0; i < kmrdma->kmr_dma_buf_export_sync_file_cnt; i++) {
		for (j = 0; j < kmrdma->kmr_dma_buf_export_sync_file[i].syncFileFdsCount; j++) {
			if (kmrdma->kmr_dma_buf_export_sync_file[i].syncFileFds[j])
				close(kmrdma->kmr_dma_buf_export_sync_file[i].syncFileFds[j]);
			free(kmrdma->kmr_dma_buf_export_sync_file[i].syncFileFds);
		}
	}
}
