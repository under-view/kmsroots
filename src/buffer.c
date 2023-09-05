#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <drm.h>
#include <drm_fourcc.h>
#include <drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "buffer.h"


struct kmr_buffer kmr_buffer_create(struct kmr_buffer_create_info *kmrbuff)
{
	struct gbm_device *gbmDevice = NULL;
	struct kmr_buffer_object *bufferObjects = NULL;

	uint32_t currentBuffer, currentPlane;

	if (kmrbuff->bufferType == KMR_BUFFER_GBM_BUFFER || kmrbuff->bufferType == KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS) {
		gbmDevice = gbm_create_device(kmrbuff->kmsfd);
		if (!gbmDevice) goto exit_error_buffer_null_struct;

		bufferObjects = calloc(kmrbuff->bufferCount, sizeof(struct kmr_buffer_object));
		if (!bufferObjects) {
			kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
			goto exit_error_buffer_gbmdev_destroy;
		}
	}

	if (kmrbuff->bufferType == KMR_BUFFER_GBM_BUFFER) {
		for (unsigned b = 0; b < kmrbuff->bufferCount; b++) {
			bufferObjects[b].kmsfd = kmrbuff->kmsfd;
			bufferObjects[b].bo = gbm_bo_create(gbmDevice, kmrbuff->width, kmrbuff->height, kmrbuff->pixelFormat, kmrbuff->gbmBoFlags);
			if (!bufferObjects[b].bo) {
				kmr_utils_log(KMR_DANGER, "[x] gbm_bo_create: failed to create gbm_bo with res %u x %u", kmrbuff->width, kmrbuff->height);
				goto exit_error_buffer_gbm_bo_detroy;
			}
		}
	}

	if (kmrbuff->bufferType == KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS) {
		for (unsigned b = 0; b < kmrbuff->bufferCount; b++) {
			bufferObjects[b].kmsfd = kmrbuff->kmsfd;
			bufferObjects[b].bo = gbm_bo_create_with_modifiers2(gbmDevice, kmrbuff->width, kmrbuff->height, kmrbuff->pixelFormat, kmrbuff->modifiers, kmrbuff->modifierCount, kmrbuff->gbmBoFlags);
			if (!bufferObjects[b].bo) {
				kmr_utils_log(KMR_DANGER, "[x] gbm_bo_create_with_modifiers: failed to create gbm_bo with res %u x %u", kmrbuff->width, kmrbuff->height);
				goto exit_error_buffer_gbm_bo_detroy;
			}
		}
	}

	// Stores GEM handles per plane used to retrieve an FD to a DMA-BUF
	// and retrieve a KMS framebuffer ID for modesetting purposes.
	unsigned gemHandles[4];
	for (currentBuffer = 0; currentBuffer < kmrbuff->bufferCount; currentBuffer++) {
		bufferObjects[currentBuffer].planeCount = gbm_bo_get_plane_count(bufferObjects[currentBuffer].bo);
		bufferObjects[currentBuffer].modifier = gbm_bo_get_modifier(bufferObjects[currentBuffer].bo);
		bufferObjects[currentBuffer].format = gbm_bo_get_format(bufferObjects[currentBuffer].bo);
		memset(gemHandles, 0, sizeof(gemHandles));

		for (currentPlane = 0; currentPlane < bufferObjects[currentBuffer].planeCount; currentPlane++) {
			union gbm_bo_handle boHandle;
			memset(&boHandle,0,sizeof(boHandle));

			boHandle = gbm_bo_get_handle_for_plane(bufferObjects[currentBuffer].bo, currentPlane);
			if (!boHandle.u32 || boHandle.s32 == -1) {
				kmr_utils_log(KMR_DANGER, "[x] failed to get BO plane %d gem handle (modifier 0x%" PRIx64 ")", currentPlane, bufferObjects[currentBuffer].modifier);
				goto exit_error_buffer_gbm_bo_detroy;
			}

			gemHandles[currentPlane] = boHandle.u32;

			bufferObjects[currentBuffer].pitches[currentPlane] = gbm_bo_get_stride_for_plane(bufferObjects[currentBuffer].bo, currentPlane);
			if (!bufferObjects[currentBuffer].pitches[currentPlane]) {
				kmr_utils_log(KMR_DANGER, "[x] failed to get stride/pitch for BO plane %d (modifier 0x%" PRIx64 ")", currentPlane, bufferObjects[currentBuffer].modifier);
				goto exit_error_buffer_gbm_bo_detroy;
			}

			bufferObjects[currentBuffer].offsets[currentPlane] = gbm_bo_get_offset(bufferObjects[currentBuffer].bo, currentPlane);

			struct drm_prime_handle drmPrimeRequest;
			drmPrimeRequest.handle = gemHandles[currentPlane];
			drmPrimeRequest.flags  = DRM_RDWR;
			drmPrimeRequest.fd     = -1;

			/*
			 * Retrieve a DMA-BUF fd (PRIME fd) for a given GEM buffer via the GEM handle.
			 * This fd can be passed along to other processes
			 */
			if (ioctl(bufferObjects[currentBuffer].kmsfd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &drmPrimeRequest) == -1)  {
				kmr_utils_log(KMR_DANGER, "[x] ioctl(DRM_IOCTL_PRIME_HANDLE_TO_FD): %s", strerror(errno));
				goto exit_error_buffer_gbm_bo_detroy;
			}

			bufferObjects[currentBuffer].dmaBufferFds[currentPlane] = drmPrimeRequest.fd;
		}

		/*
		 * TAKEN from Daniel Stone kms-quads
		 * Wrap our GEM buffer in a KMS framebuffer, so we can then attach it
		 * to a plane.
		 *
		 * drmModeAddFB2(struct drm_mode_fb_cmd) accepts multiple image planes (not to be confused with
		 * the KMS plane objects!), for images which have multiple buffers.
		 * For example, YUV images may have the luma (Y) components in a
		 * separate buffer to the chroma (UV) components.
		 *
		 * When using modifiers (which we do not for dumb buffers), we can also
		 * have multiple planes even for RGB images, as image compression often
		 * uses an auxiliary buffer to store compression metadata.
		 *
		 * Dump buffers are always strictly single-planar, so we do not need
		 * the extra planes nor the offset field.
		 *
		 * drmModeAddFB2WithModifiers(struct drm_mode_fb_cmd2) takes a list of modifiers per plane, however
		 * the kernel enforces that they must be the same for each plane
		 * which is there, and 0 for everything else.
		 */
		if (kmrbuff->bufferType == KMR_BUFFER_GBM_BUFFER) {
			struct drm_mode_fb_cmd framebuffer;
			memset(&framebuffer,0,sizeof(struct drm_mode_fb_cmd));

			framebuffer.bpp    = kmrbuff->bitsPerPixel;
			framebuffer.depth  = kmrbuff->bitDepth;
			framebuffer.width  = kmrbuff->width;
			framebuffer.height = kmrbuff->height;
			framebuffer.pitch  = bufferObjects[currentBuffer].pitches[0];
			framebuffer.handle = gemHandles[0];
			bufferObjects[currentBuffer].fbid = 0;

			if (ioctl(bufferObjects[currentBuffer].kmsfd, DRM_IOCTL_MODE_ADDFB, &framebuffer) == -1) {
				kmr_utils_log(KMR_DANGER, "[x] ioctl(DRM_IOCTL_MODE_ADDFB): %s", strerror(errno));
				goto exit_error_buffer_gbm_bo_detroy;
			}

			bufferObjects[currentBuffer].fbid = framebuffer.fb_id;
		}

		if (kmrbuff->bufferType == KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS) {
			struct drm_mode_fb_cmd2 framebuffer;
			memset(&framebuffer,0,sizeof(struct drm_mode_fb_cmd2));

			framebuffer.width  = kmrbuff->width;
			framebuffer.height = kmrbuff->height;
			framebuffer.pixel_format = bufferObjects[currentBuffer].format;
			framebuffer.flags = DRM_MODE_FB_MODIFIERS;

			memcpy(framebuffer.handles, gemHandles, sizeof(framebuffer.handles));
			memcpy(framebuffer.pitches, bufferObjects[currentBuffer].pitches, sizeof(framebuffer.pitches));
			memcpy(framebuffer.offsets, bufferObjects[currentBuffer].offsets, sizeof(framebuffer.offsets));
			memcpy(framebuffer.modifier, kmrbuff->modifiers, sizeof(framebuffer.modifier));
			bufferObjects[currentBuffer].fbid = 0;

			if (ioctl(bufferObjects[currentBuffer].kmsfd, DRM_IOCTL_MODE_ADDFB2, &framebuffer) == -1) {
				kmr_utils_log(KMR_DANGER, "[x] ioctl(DRM_IOCTL_MODE_ADDFB2): %s", strerror(errno));
				goto exit_error_buffer_gbm_bo_detroy;
			}

			bufferObjects[currentBuffer].fbid = framebuffer.fb_id;
		}
	}

	kmr_utils_log(KMR_SUCCESS, "Successfully create GBM buffers");

	return (struct kmr_buffer) { .gbmDevice = gbmDevice, .bufferCount = kmrbuff->bufferCount, .bufferObjects = bufferObjects };

exit_error_buffer_gbm_bo_detroy:
	for (currentBuffer = 0; currentBuffer < kmrbuff->bufferCount; currentBuffer++) {
		if (bufferObjects[currentBuffer].fbid)
			ioctl(bufferObjects[currentBuffer].kmsfd, DRM_IOCTL_MODE_RMFB, &bufferObjects[currentBuffer].fbid);
		if (bufferObjects[currentBuffer].bo)
			gbm_bo_destroy(bufferObjects[currentBuffer].bo);
		for (currentPlane = 0; currentPlane < bufferObjects[currentBuffer].planeCount; currentPlane++) {
			if (bufferObjects[currentBuffer].dmaBufferFds[currentPlane] != -1) {
				drmCloseBufferHandle(bufferObjects[currentBuffer].kmsfd, bufferObjects[currentBuffer].dmaBufferFds[currentPlane]);
				bufferObjects[currentBuffer].dmaBufferFds[currentPlane] = -1;
			}
		}
	}
	free(bufferObjects);
exit_error_buffer_gbmdev_destroy:
	if (gbmDevice)
		gbm_device_destroy(gbmDevice);
exit_error_buffer_null_struct:
	return (struct kmr_buffer) { .gbmDevice = NULL, .bufferCount = 0, .bufferObjects = NULL };
}


void kmr_buffer_destroy(struct kmr_buffer *kmrbuff, unsigned count)
{
	unsigned int currentIndex, i, j;

	for (currentIndex = 0; currentIndex < count; currentIndex++) {
		for (i = 0; i < kmrbuff[currentIndex].bufferCount; i++) {
			if (kmrbuff[currentIndex].bufferObjects[i].fbid)
				ioctl(kmrbuff[currentIndex].bufferObjects[i].kmsfd, DRM_IOCTL_MODE_RMFB, &kmrbuff[currentIndex].bufferObjects[i].fbid);
			if (kmrbuff[currentIndex].bufferObjects[i].bo)
				gbm_bo_destroy(kmrbuff[currentIndex].bufferObjects[i].bo);

			for (j = 0; j < kmrbuff[currentIndex].bufferObjects[i].planeCount; j++)  {
				if (kmrbuff[currentIndex].bufferObjects[i].dmaBufferFds[j] != -1)
					drmCloseBufferHandle(kmrbuff[currentIndex].bufferObjects[i].kmsfd, kmrbuff[currentIndex].bufferObjects[i].dmaBufferFds[j]);
			}
		}
		free(kmrbuff[currentIndex].bufferObjects);
		if (kmrbuff[currentIndex].gbmDevice)
			gbm_device_destroy(kmrbuff[currentIndex].gbmDevice);
	}
}
