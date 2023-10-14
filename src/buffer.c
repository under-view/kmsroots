#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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


/*****************************************
 * START OF create_gbm_buffers FUNCTIONS *
 *****************************************/

/*
 * Type defines to shorten code blocks.
 * Hopefully makes it more readable in the future.
 */
typedef struct gbm_bo * (*gbm_func_impl)(struct gbm_device *,
                                         struct kmr_buffer_create_info *);

typedef int (*framebuffer_func_impl)(struct kmr_buffer *,
                                     struct kmr_buffer_create_info *,
                                     uint32_t,
                                     unsigned *);

static struct gbm_bo *
gbm_buffer_create_impl (struct gbm_device *gbmDevice,
                        struct kmr_buffer_create_info *bufferInfo)
{
	return gbm_bo_create(gbmDevice,
	                     bufferInfo->width,
	                     bufferInfo->height,
	                     bufferInfo->pixelFormat,
	                     bufferInfo->gbmBoFlags);
}


static int
gbm_framebuffer_create_impl (struct kmr_buffer *buffer,
                             struct kmr_buffer_create_info *bufferInfo,
                             uint32_t currentBuffer,
                             unsigned *gemHandles)
{
	struct drm_mode_fb_cmd framebuffer;
	memset(&framebuffer,0,sizeof(struct drm_mode_fb_cmd));

	framebuffer.bpp    = bufferInfo->bitsPerPixel;
	framebuffer.depth  = bufferInfo->bitDepth;
	framebuffer.width  = bufferInfo->width;
	framebuffer.height = bufferInfo->height;
	framebuffer.pitch  = buffer->bufferObjects[currentBuffer].pitches[0];
	framebuffer.handle = gemHandles[0];

	if (ioctl(buffer->bufferObjects[currentBuffer].kmsfd, DRM_IOCTL_MODE_ADDFB, &framebuffer) == -1) {
		kmr_utils_log(KMR_DANGER, "[x] ioctl(DRM_IOCTL_MODE_ADDFB): %s", strerror(errno));
		return -1;
	}

	return framebuffer.fb_id;
}


static struct gbm_bo *
gbm_buffer_create_with_modifiers_impl (struct gbm_device *gbmDevice,
                                       struct kmr_buffer_create_info *bufferInfo)
{
	return gbm_bo_create_with_modifiers2(gbmDevice,
	                                     bufferInfo->width,
	                                     bufferInfo->height,
	                                     bufferInfo->pixelFormat,
	                                     bufferInfo->modifiers,
	                                     bufferInfo->modifierCount,
	                                     bufferInfo->gbmBoFlags);
}


static int
gbm_framebuffer_create_with_modifiers_impl (struct kmr_buffer *buffer,
                                            struct kmr_buffer_create_info *bufferInfo,
                                            uint32_t currentBuffer,
                                            unsigned *gemHandles)
{
	struct drm_mode_fb_cmd2 framebuffer;
	memset(&framebuffer,0,sizeof(struct drm_mode_fb_cmd2));

	framebuffer.width  = bufferInfo->width;
	framebuffer.height = bufferInfo->height;
	framebuffer.pixel_format = buffer->bufferObjects[currentBuffer].format;
	framebuffer.flags = DRM_MODE_FB_MODIFIERS;

	memcpy(framebuffer.handles, gemHandles, sizeof(framebuffer.handles));
	memcpy(framebuffer.pitches, buffer->bufferObjects[currentBuffer].pitches, sizeof(framebuffer.pitches));
	memcpy(framebuffer.offsets, buffer->bufferObjects[currentBuffer].offsets, sizeof(framebuffer.offsets));
	memcpy(framebuffer.modifier, bufferInfo->modifiers, sizeof(framebuffer.modifier));

	if (ioctl(buffer->bufferObjects[currentBuffer].kmsfd, DRM_IOCTL_MODE_ADDFB2, &framebuffer) == -1) {
		kmr_utils_log(KMR_DANGER, "[x] ioctl(DRM_IOCTL_MODE_ADDFB2): %s", strerror(errno));
		return -1;
	}

	return framebuffer.fb_id;
}


struct gbm_func_impl {
	gbm_func_impl         gbm_bo_create;
	framebuffer_func_impl get_framebuffer_id;
};


struct gbm_func_impl gbmFuncs[KMR_BUFFER_MAX_TYPE] = {
	[KMR_BUFFER_GBM_BUFFER] =  {
		.gbm_bo_create = gbm_buffer_create_impl,
		.get_framebuffer_id = gbm_framebuffer_create_impl,
	},
	[KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS] = {
		.gbm_bo_create = gbm_buffer_create_with_modifiers_impl,
		.get_framebuffer_id = gbm_framebuffer_create_with_modifiers_impl,
	}
};


static int
create_gbm_buffers (struct kmr_buffer *buffer,
                    struct kmr_buffer_create_info *bufferInfo)
{
	// Stores GEM handles per plane used to retrieve an FD to a DMA-BUF
	// and retrieve a KMS framebuffer ID for modesetting purposes.
	unsigned gemHandles[4];

	uint32_t currentBuffer, currentPlane;
	union gbm_bo_handle boHandle;
	struct kmr_buffer_object *bufferObjects = NULL;
	struct drm_prime_handle drmPrimeRequest;

	bufferObjects = buffer->bufferObjects;

	buffer->gbmDevice = gbm_create_device(bufferInfo->kmsfd);
	if (!buffer->gbmDevice)
		return -1;

	for (currentBuffer = 0; currentBuffer < bufferInfo->bufferCount; currentBuffer++) {
		bufferObjects[currentBuffer].kmsfd = bufferInfo->kmsfd;
		bufferObjects[currentBuffer].bo = gbmFuncs[bufferInfo->bufferType].gbm_bo_create(buffer->gbmDevice, bufferInfo);
		if (!bufferObjects[currentBuffer].bo) {
			kmr_utils_log(KMR_DANGER,
				      "[x] %s: failed to create gbm_bo with res %u x %u",
				      bufferInfo->bufferType == KMR_BUFFER_GBM_BUFFER ? "gbm_bo_create" : "gbm_bo_create_with_modifiers2",
				      bufferInfo->width, bufferInfo->height);
			return -1;
		}

		bufferObjects[currentBuffer].planeCount = gbm_bo_get_plane_count(bufferObjects[currentBuffer].bo);
		bufferObjects[currentBuffer].modifier = gbm_bo_get_modifier(bufferObjects[currentBuffer].bo);
		bufferObjects[currentBuffer].format = gbm_bo_get_format(bufferObjects[currentBuffer].bo);
		memset(gemHandles,0,sizeof(gemHandles));

		for (currentPlane = 0; currentPlane < bufferObjects[currentBuffer].planeCount; currentPlane++) {
			memset(&boHandle,0,sizeof(boHandle));

			boHandle = gbm_bo_get_handle_for_plane(bufferObjects[currentBuffer].bo, currentPlane);
			if (!boHandle.u32 || boHandle.s32 == -1) {
				kmr_utils_log(KMR_DANGER,
				              "[x] failed to get BO plane %d gem handle (modifier 0x%" PRIx64 ")",
					      currentPlane, bufferObjects[currentBuffer].modifier);
				return -1;
			}

			gemHandles[currentPlane] = boHandle.u32;

			bufferObjects[currentBuffer].pitches[currentPlane] = gbm_bo_get_stride_for_plane(bufferObjects[currentBuffer].bo, currentPlane);
			if (!bufferObjects[currentBuffer].pitches[currentPlane]) {
				kmr_utils_log(KMR_DANGER,
				              "[x] failed to get stride/pitch for BO plane %d (modifier 0x%" PRIx64 ")",
					      currentPlane, bufferObjects[currentBuffer].modifier);
				return -1;
			}

			bufferObjects[currentBuffer].offsets[currentPlane] = gbm_bo_get_offset(bufferObjects[currentBuffer].bo, currentPlane);

			drmPrimeRequest.handle = gemHandles[currentPlane];
			drmPrimeRequest.flags  = DRM_RDWR;
			drmPrimeRequest.fd     = -1;

			/*
			 * Retrieve a DMA-BUF fd (PRIME fd) for a given GEM buffer via the GEM handle.
			 * This fd can be passed along to other processes
			 */
			if (ioctl(bufferObjects[currentBuffer].kmsfd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &drmPrimeRequest) == -1)  {
				kmr_utils_log(KMR_DANGER, "[x] ioctl(DRM_IOCTL_PRIME_HANDLE_TO_FD): %s", strerror(errno));
				return -1;
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
		bufferObjects[currentBuffer].fbid = \
			gbmFuncs[bufferInfo->bufferType].get_framebuffer_id(buffer,
		                                                            bufferInfo,
			                                                    currentBuffer,
			                                                    &gemHandles[0]);
		if (bufferObjects[currentBuffer].fbid == -1)
			return -1;
	}

	kmr_utils_log(KMR_SUCCESS, "Successfully create GBM buffers");

	return 0;
}

/***************************************
 * END OF create_gbm_buffers FUNCTIONS *
 ***************************************/

/**************************************************
 * START OF kmr_buffer_{create,destroy} FUNCTIONS *
 **************************************************/

struct kmr_buffer *
kmr_buffer_create (struct kmr_buffer_create_info *bufferInfo)
{
	int ret = -1;
	struct kmr_buffer *buffer = NULL;

	buffer = calloc(1, sizeof(struct kmr_buffer));
	if (!buffer) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_buffer_create;
	}

	buffer->bufferObjects = calloc(bufferInfo->bufferCount, sizeof(struct kmr_buffer_object));
	if (!buffer->bufferObjects) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_buffer_create;
	}

	buffer->bufferCount = bufferInfo->bufferCount;

	switch (bufferInfo->bufferType) {
		case KMR_BUFFER_GBM_BUFFER:
		case KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS:
			ret = create_gbm_buffers(buffer, bufferInfo);
			if (ret == -1)
				goto exit_error_kmr_buffer_create;
			break;
		case KMR_BUFFER_DUMP_BUFFER:
			kmr_utils_log(KMR_WARNING, "Dump buffer creation not supported");
			goto exit_error_kmr_buffer_create;
		default:
			kmr_utils_log(KMR_DANGER, "[x] Passed incorrect enum kmr_buffer_type");
			goto exit_error_kmr_buffer_create;
	}

	return buffer;

exit_error_kmr_buffer_create:
	kmr_buffer_destroy(buffer);
	return NULL;
}


void
kmr_buffer_destroy (struct kmr_buffer *buffer)
{
	unsigned int i, j;

	if (!buffer)
		return;

	for (i = 0; i < buffer->bufferCount; i++) {
		if (buffer->bufferObjects[i].fbid) {
			fsync(buffer->bufferObjects[i].fbid);
			ioctl(buffer->bufferObjects[i].kmsfd, DRM_IOCTL_MODE_RMFB, &buffer->bufferObjects[i].fbid);
		}
		if (buffer->bufferObjects[i].bo)
			gbm_bo_destroy(buffer->bufferObjects[i].bo);
		for (j = 0; j < buffer->bufferObjects[i].planeCount; j++)
			drmCloseBufferHandle(buffer->bufferObjects[i].kmsfd, buffer->bufferObjects[i].dmaBufferFds[j]);
	}

	if (buffer->gbmDevice)
		gbm_device_destroy(buffer->gbmDevice);
	free(buffer->bufferObjects);
	free(buffer);
}

/************************************************
 * END OF kmr_buffer_{create,destroy} FUNCTIONS *
 ************************************************/
