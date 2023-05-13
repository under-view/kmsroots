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


struct kmr_buffer kmr_buffer_create(struct kmr_buffer_create_info *kmsbuff)
{
	struct gbm_device *gbmDevice = NULL;
	struct kmr_buffer_object *bufferObjects = NULL;

	uint32_t currentBuffer, currentPlane;

	if (kmsbuff->bufferType == KMR_BUFFER_GBM_BUFFER || kmsbuff->bufferType == KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS) {
		gbmDevice = gbm_create_device(kmsbuff->kmsfd);
		if (!gbmDevice) goto exit_error_buffer_null_struct;

		bufferObjects = calloc(kmsbuff->bufferCount, sizeof(struct kmr_buffer_object));
		if (!bufferObjects) {
			kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
			goto exit_error_buffer_gbmdev_destroy;
		}
	}

	if (kmsbuff->bufferType == KMR_BUFFER_GBM_BUFFER) {
		for (unsigned b = 0; b < kmsbuff->bufferCount; b++) {
			bufferObjects[b].kmsfd = kmsbuff->kmsfd;
			bufferObjects[b].bo = gbm_bo_create(gbmDevice, kmsbuff->width, kmsbuff->height, kmsbuff->pixelFormat, kmsbuff->gbmBoFlags);
			if (!bufferObjects[b].bo) {
				kmr_utils_log(KMR_DANGER, "[x] gbm_bo_create: failed to create gbm_bo with res %u x %u", kmsbuff->width, kmsbuff->height);
				goto exit_error_buffer_gbm_bo_detroy;
			}
		}
	}

	if (kmsbuff->bufferType == KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS) {
		for (unsigned b = 0; b < kmsbuff->bufferCount; b++) {
			bufferObjects[b].kmsfd = kmsbuff->kmsfd;
			bufferObjects[b].bo = gbm_bo_create_with_modifiers2(gbmDevice, kmsbuff->width, kmsbuff->height, kmsbuff->pixelFormat, kmsbuff->modifiers, kmsbuff->modifierCount, kmsbuff->gbmBoFlags);
			if (!bufferObjects[b].bo) {
				kmr_utils_log(KMR_DANGER, "[x] gbm_bo_create_with_modifiers: failed to create gbm_bo with res %u x %u", kmsbuff->width, kmsbuff->height);
				goto exit_error_buffer_gbm_bo_detroy;
			}
		}
	}

	for (currentBuffer = 0; currentBuffer < kmsbuff->bufferCount; currentBuffer++) {
		bufferObjects[currentBuffer].planeCount = gbm_bo_get_plane_count(bufferObjects[currentBuffer].bo);
		bufferObjects[currentBuffer].modifier = gbm_bo_get_modifier(bufferObjects[currentBuffer].bo);
		bufferObjects[currentBuffer].format = gbm_bo_get_format(bufferObjects[currentBuffer].bo);

		for (currentPlane = 0; currentPlane < bufferObjects[currentBuffer].planeCount; currentPlane++) {
			union gbm_bo_handle boHandle;
			memset(&boHandle,0,sizeof(boHandle));

			boHandle = gbm_bo_get_handle_for_plane(bufferObjects[currentBuffer].bo, currentPlane);
			if (!boHandle.u32 || boHandle.s32 == -1) {
				kmr_utils_log(KMR_DANGER, "[x] failed to get BO plane %d gem handle (modifier 0x%" PRIx64 ")", currentPlane, bufferObjects[currentBuffer].modifier);
				goto exit_error_buffer_gbm_bo_detroy;
			}

			bufferObjects[currentBuffer].gemHandles[currentPlane] = boHandle.u32;

			bufferObjects[currentBuffer].pitches[currentPlane] = gbm_bo_get_stride_for_plane(bufferObjects[currentBuffer].bo, currentPlane);
			if (!bufferObjects[currentBuffer].pitches[currentPlane]) {
				kmr_utils_log(KMR_DANGER, "[x] failed to get stride/pitch for BO plane %d (modifier 0x%" PRIx64 ")", currentPlane, bufferObjects[currentBuffer].modifier);
				goto exit_error_buffer_gbm_bo_detroy;
			}

			bufferObjects[currentBuffer].offsets[currentPlane] = gbm_bo_get_offset(bufferObjects[currentBuffer].bo, currentPlane);

			struct drm_prime_handle drmPrimeRequest = {
				.handle = bufferObjects[currentBuffer].gemHandles[currentPlane],
				.flags  = DRM_RDWR,
				.fd     = -1
			};

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
		if (kmsbuff->bufferType == KMR_BUFFER_GBM_BUFFER) {
			struct drm_mode_fb_cmd framebuffer;
			memset(&framebuffer,0,sizeof(struct drm_mode_fb_cmd));

			framebuffer.bpp    = kmsbuff->bitsPerPixel;
			framebuffer.depth  = kmsbuff->bitDepth;
			framebuffer.width  = kmsbuff->width;
			framebuffer.height = kmsbuff->height;
			framebuffer.pitch  = bufferObjects[currentBuffer].pitches[0];
			framebuffer.handle = bufferObjects[currentBuffer].gemHandles[0];
			bufferObjects[currentBuffer].fbid = 0;

			if (ioctl(bufferObjects[currentBuffer].kmsfd, DRM_IOCTL_MODE_ADDFB, &framebuffer) == -1) {
				kmr_utils_log(KMR_DANGER, "[x] ioctl(DRM_IOCTL_MODE_ADDFB): %s", strerror(errno));
				goto exit_error_buffer_gbm_bo_detroy;
			}

			bufferObjects[currentBuffer].fbid = framebuffer.fb_id;
		}

		if (kmsbuff->bufferType == KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS) {
			struct drm_mode_fb_cmd2 framebuffer;
			memset(&framebuffer,0,sizeof(struct drm_mode_fb_cmd2));

			framebuffer.width  = kmsbuff->width;
			framebuffer.height = kmsbuff->height;
			framebuffer.pixel_format = bufferObjects[currentBuffer].format;
			framebuffer.flags = DRM_MODE_FB_MODIFIERS;

			memcpy(framebuffer.handles, bufferObjects[currentBuffer].gemHandles, sizeof(framebuffer.handles));
			memcpy(framebuffer.pitches, bufferObjects[currentBuffer].pitches, sizeof(framebuffer.pitches));
			memcpy(framebuffer.offsets, bufferObjects[currentBuffer].offsets, sizeof(framebuffer.offsets));
			memcpy(framebuffer.modifier, kmsbuff->modifiers, sizeof(framebuffer.modifier));
			bufferObjects[currentBuffer].fbid = 0;

			if (ioctl(bufferObjects[currentBuffer].kmsfd, DRM_IOCTL_MODE_ADDFB2, &framebuffer) == -1) {
				kmr_utils_log(KMR_DANGER, "[x] ioctl(DRM_IOCTL_MODE_ADDFB2): %s", strerror(errno));
				goto exit_error_buffer_gbm_bo_detroy;
			}

			bufferObjects[currentBuffer].fbid = framebuffer.fb_id;
		}
	}

	kmr_utils_log(KMR_SUCCESS, "Successfully create GBM buffers");

	return (struct kmr_buffer) { .gbmDevice = gbmDevice, .bufferCount = kmsbuff->bufferCount, .bufferObjects = bufferObjects };

exit_error_buffer_gbm_bo_detroy:
	for (currentBuffer = 0; currentBuffer < kmsbuff->bufferCount; currentBuffer++) {
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


void kmr_buffer_destroy(struct kmr_buffer_destroy *kmsbuff)
{
	unsigned int currentIndex, i, j;

	for (currentIndex = 0; currentIndex < kmsbuff->kmr_buffer_cnt; currentIndex++) {
		for (i = 0; i < kmsbuff->kmr_buffer[currentIndex].bufferCount; i++) {
			if (kmsbuff->kmr_buffer[currentIndex].bufferObjects[i].fbid)
				ioctl(kmsbuff->kmr_buffer[currentIndex].bufferObjects[i].kmsfd, DRM_IOCTL_MODE_RMFB, &kmsbuff->kmr_buffer[currentIndex].bufferObjects[i].fbid);
			if (kmsbuff->kmr_buffer[currentIndex].bufferObjects[i].bo)
				gbm_bo_destroy(kmsbuff->kmr_buffer[currentIndex].bufferObjects[i].bo);
		for (j = 0; j < kmsbuff->kmr_buffer[currentIndex].bufferObjects[i].planeCount; j++)  {
			if (kmsbuff->kmr_buffer[currentIndex].bufferObjects[i].dmaBufferFds[j] != -1)
				drmCloseBufferHandle(kmsbuff->kmr_buffer[currentIndex].bufferObjects[i].kmsfd, kmsbuff->kmr_buffer[currentIndex].bufferObjects[i].dmaBufferFds[j]);
		}
	}
	free(kmsbuff->kmr_buffer[currentIndex].bufferObjects);
	if (kmsbuff->kmr_buffer[currentIndex].gbmDevice)
		gbm_device_destroy(kmsbuff->kmr_buffer[currentIndex].gbmDevice);
	}
}
