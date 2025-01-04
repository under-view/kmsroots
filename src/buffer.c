#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <drm.h>
#include <drm_fourcc.h>
#include <drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <gbm.h>

#include <cando/cando.h>

#include "buffer.h"

#define MAX_BUFFER_COUNT 5
#define MAX_PLANE_COUNT 4

/*
 * @brief struct kmr_buffer_object (kmsroots Buffer Object)
 *
 * @member bo           - Handle to some GEM allocated buffer. Used to get GEM handles,
 *                        DMA buffer fds (fd associate with GEM buffer), pitches, and
 *                        offsets for the buffer used by DRI device (GPU).
 * @member fbid         - Framebuffer ID
 * @member format       - The format of an image details how each pixel color channels
 *                        is laid out in memory: (i.e. RAM, VRAM, etc...). So, basically
 *                        the width in bits, type, and ordering of each pixels color channels.
 * @member modifier     - The modifier details information on how pixels should be within a
 *                        buffer for different types of operations such as scan out or rendering.
 *                        (i.e linear, tiled, compressed, etc...)
 *                        https://01.org/linuxgraphics/Linux-Window-Systems-with-DRM
 * @member planeCount   - Number of Planar Formats. The number of @dmaBufferFds, @offsets,
 *                        @pitches retrieved per plane. More information may be found
 *                        https://en.wikipedia.org/wiki/Planar_(computer_graphics).
 * @member pitches      - Width in bytes for each plane.
 * @member offsets      - Offset of each plane. More information can be found
 *                        https://gitlab.freedesktop.org/mesa/drm/-/blob/main/include/drm/drm_mode.h#L589
 * @member dmaBufferFds - (PRIME fd) Stores file descriptors to buffers that can be shared across hardware.
 * @member kmsfd        - File descriptor to open DRI device
 */
struct kmr_buffer_object
{
	struct gbm_bo *bo;
	int           fbid;
	unsigned int  format;
	uint64_t      modifier;
	unsigned int  planeCount;
	unsigned int  pitches[MAX_PLANE_COUNT];
	unsigned int  offsets[MAX_PLANE_COUNT];
	int           dmaBufferFds[MAX_PLANE_COUNT];
	int           kmsfd;
};


/*
 * @brief struct kmr_buffer (kmsroots Buffer)
 *
 * @member err           - Stores information about the error that occured
 *                         for the given instance and may later be retrieved
 *                         by caller.
 * @member gbmDevice     - A handle used to allocate gbm buffers & surfaces
 * @member bufferCount   - Array size of @bufferObjects
 * @member bufferObjects - Stores an array of gbm_bo's and corresponding
 *                         information about the individual buffer.
 */
struct kmr_buffer
{
	struct cando_log_error_struct err;
	struct gbm_device             *gbmDevice;
	unsigned int                  bufferCount;
	struct kmr_buffer_object      bufferObjects[MAX_BUFFER_COUNT];
};


/****************************************
 * Start of kmr_buffer_create functions *
 ****************************************/

/*
 * Type defines to shorten code blocks.
 * Hopefully makes it more readable in the future.
 */
typedef struct gbm_bo * \
(*gbm_func_impl) (struct gbm_device *,
                  const struct kmr_buffer_create_info *);


typedef int \
(*framebuffer_func_impl) (struct kmr_buffer *,
                          const struct kmr_buffer_create_info *,
                          const unsigned int,
                          const unsigned int *);


static struct gbm_bo *
gbm_buffer_create_impl (struct gbm_device *gbmDevice,
                        const struct kmr_buffer_create_info *bufferInfo)
{
	return gbm_bo_create(gbmDevice,
	                     bufferInfo->width,
	                     bufferInfo->height,
	                     bufferInfo->pixelFormat,
	                     bufferInfo->gbmBoFlags);
}


static int
gbm_framebuffer_create_impl (struct kmr_buffer *buffer,
                             const struct kmr_buffer_create_info *bufferInfo,
                             const unsigned int currentBuffer,
                             const unsigned int *gemHandles)
{
	int ret = -1;

	struct drm_mode_fb_cmd framebuffer;

	struct kmr_buffer_object *bufferObject = NULL;

	memset(&framebuffer,0,sizeof(struct drm_mode_fb_cmd));

	bufferObject = &(buffer->bufferObjects[currentBuffer]);

	framebuffer.bpp    = bufferInfo->bitsPerPixel;
	framebuffer.depth  = bufferInfo->bitDepth;
	framebuffer.width  = bufferInfo->width;
	framebuffer.height = bufferInfo->height;
	framebuffer.pitch  = bufferObject->pitches[0];
	framebuffer.handle = *gemHandles;

	ret = ioctl(bufferObject->kmsfd, DRM_IOCTL_MODE_ADDFB, &framebuffer);
	if (ret == -1) {
		cando_log_set_err(buffer, errno,
		                  "ioctl(DRM_IOCTL_MODE_ADDFB): %s",
		                  strerror(errno));
		return -1;
	}

	return framebuffer.fb_id;
}


static struct gbm_bo *
gbm_buffer_create_with_modifiers_impl (struct gbm_device *gbmDevice,
                                       const struct kmr_buffer_create_info *bufferInfo)
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
                                            const struct kmr_buffer_create_info *bufferInfo,
                                            const unsigned int currentBuffer,
                                            const unsigned int *gemHandles)
{
	int ret = -1;

	struct drm_mode_fb_cmd2 framebuffer;

	struct kmr_buffer_object *bufferObject = NULL;

	bufferObject = &(buffer->bufferObjects[currentBuffer]);

	memset(&framebuffer,0,sizeof(struct drm_mode_fb_cmd2));

	framebuffer.width  = bufferInfo->width;
	framebuffer.height = bufferInfo->height;
	framebuffer.pixel_format = bufferObject->format;
	framebuffer.flags = DRM_MODE_FB_MODIFIERS;

	memcpy(framebuffer.handles, gemHandles, sizeof(framebuffer.handles));
	memcpy(framebuffer.pitches, bufferObject->pitches, sizeof(framebuffer.pitches));
	memcpy(framebuffer.offsets, bufferObject->offsets, sizeof(framebuffer.offsets));
	memcpy(framebuffer.modifier, bufferInfo->modifiers, sizeof(framebuffer.modifier));

	ret = ioctl(bufferObject->kmsfd, DRM_IOCTL_MODE_ADDFB2, &framebuffer);
	if (ret == -1) {
		cando_log_set_err(buffer, errno,
		                  "ioctl(DRM_IOCTL_MODE_ADDFB2): %s",
		                  strerror(errno));
		return -1;
	}

	return framebuffer.fb_id;
}


struct gbm_func_impl
{
	gbm_func_impl         gbm_bo_create;
	framebuffer_func_impl get_framebuffer_id;
};


struct gbm_func_impl gbmFuncs[KMR_BUFFER_MAX_TYPE] = \
{
	[KMR_BUFFER_GBM_BUFFER] = \
	{
		.gbm_bo_create = gbm_buffer_create_impl,
		.get_framebuffer_id = gbm_framebuffer_create_impl,
	},

	[KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS] = \
	{
		.gbm_bo_create = gbm_buffer_create_with_modifiers_impl,
		.get_framebuffer_id = gbm_framebuffer_create_with_modifiers_impl,
	}
};


static int
create_planes (struct kmr_buffer *buffer,
               unsigned int currentBuffer,
               unsigned int *gemHandles)
{
	int ret = -1;

	uint32_t currentPlane;

	union gbm_bo_handle boHandle;

	struct drm_prime_handle drmPrimeRequest;

	struct kmr_buffer_object *bufferObject = &(buffer->bufferObjects[currentBuffer]);

	for (currentPlane = 0; currentPlane < bufferObject->planeCount; currentPlane++) {
		memset(&boHandle,0,sizeof(boHandle));

		boHandle = gbm_bo_get_handle_for_plane(bufferObject->bo, currentPlane);
		if (!boHandle.u32 || boHandle.s32 == -1) {
			cando_log_set_err(buffer, CANDO_LOG_ERR_UNCOMMON,
					  "failed to get BO plane %d gem handle (modifier 0x%" PRIx64 ")",
					  currentPlane, bufferObject->modifier);
			return -1;
		}

		bufferObject->pitches[currentPlane] = gbm_bo_get_stride_for_plane(bufferObject->bo, currentPlane);
		bufferObject->offsets[currentPlane] = gbm_bo_get_offset(bufferObject->bo, currentPlane);

		gemHandles[currentPlane] = boHandle.u32;
		drmPrimeRequest.handle = gemHandles[currentPlane];
		drmPrimeRequest.flags  = DRM_RDWR;
		drmPrimeRequest.fd     = -1;

		/*
		 * Retrieve a DMA-BUF fd (PRIME fd) for a given GEM buffer via the GEM handle.
		 * This fd can be passed along to other processes
		 */
		ret = ioctl(bufferObject->kmsfd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &drmPrimeRequest);
		if (ret == -1)  {
			cando_log_set_err(buffer, errno,
					  "ioctl(DRM_IOCTL_PRIME_HANDLE_TO_FD): %s",
					  strerror(errno));
			return -1;
		}

		bufferObject->dmaBufferFds[currentPlane] = drmPrimeRequest.fd;
	}

	return 0;
}


static int
create_gbm_buffers (struct kmr_buffer *buffer,
                    const struct kmr_buffer_create_info *bufferInfo)
{
	int ret = -1;

	/*
	 * Stores GEM handles per plane used to retrieve an FD to a DMA-BUF
	 * and retrieve a KMS framebuffer ID for modesetting purposes.
	 */
	unsigned int gemHandles[4];

	unsigned int currentBuffer;

	struct kmr_buffer_object *bufferObject = NULL;

	buffer->gbmDevice = gbm_create_device(bufferInfo->kmsfd);
	if (!(buffer->gbmDevice)) {
		cando_log_set_err(buffer, CANDO_LOG_ERR_UNCOMMON,
		                  "Failed to create gbm device context.");
		return -1;
	}

	for (currentBuffer = 0; currentBuffer < bufferInfo->bufferCount; currentBuffer++) {

		bufferObject = &(buffer->bufferObjects[currentBuffer]);

		bufferObject->kmsfd = bufferInfo->kmsfd;
		bufferObject->bo = gbmFuncs[bufferInfo->bufferType].gbm_bo_create(buffer->gbmDevice, bufferInfo);
		if (!(bufferObject->bo)) {
			cando_log_set_err(buffer, CANDO_LOG_ERR_UNCOMMON,
			                  "%s: failed to create gbm_bo with res %u x %u",
			                  bufferInfo->bufferType == KMR_BUFFER_GBM_BUFFER ? \
			                  "gbm_bo_create" : "gbm_bo_create_with_modifiers2",
			                  bufferInfo->width, bufferInfo->height);
			return -1;
		}

		bufferObject->planeCount = gbm_bo_get_plane_count(bufferObject->bo);
		bufferObject->modifier = gbm_bo_get_modifier(bufferObject->bo);
		bufferObject->format = gbm_bo_get_format(bufferObject->bo);

		memset(gemHandles,0,sizeof(gemHandles));

		ret = create_planes(buffer, currentBuffer, &(gemHandles[0]));
		if (ret == -1)
			return -1;

		/*
		 * TAKEN from Daniel Stone kms-quads
		 *
		 * Wrap our GEM buffer in a KMS framebuffer, so we can then attach it
		 * to a plane.
		 *
		 * drmModeAddFB2(struct drm_mode_fb_cmd) accepts multiple image
		 * planes (not to be confused with the KMS plane objects!), for images
		 * which have multiple buffers.
		 *
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
		 * drmModeAddFB2WithModifiers(struct drm_mode_fb_cmd2) takes a list of
		 * modifiers per plane, however the kernel enforces that they must be
		 * the same for each plane which is there, and 0 for everything else.
		 */
		bufferObject->fbid = \
		gbmFuncs[bufferInfo->bufferType].get_framebuffer_id(buffer,
	                                                            bufferInfo,
		                                                    currentBuffer,
		                                                    &gemHandles[0]);
		if (bufferObject->fbid == -1)
			return -1;
	}

	cando_log(CANDO_LOG_SUCCESS, "Successfully create GBM buffers");

	return 0;
}


struct kmr_buffer *
kmr_buffer_create (const void *_bufferInfo)
{
	int ret = -1;

	struct kmr_buffer *buffer = NULL;

	const struct kmr_buffer_create_info *bufferInfo = _bufferInfo;

	if (!bufferInfo || \
	    bufferInfo->bufferCount >= MAX_BUFFER_COUNT)
	{
		cando_log_err("Incorrect data passed\n");
		return NULL;
	}

	buffer = mmap(NULL,
	              sizeof(struct kmr_buffer),
	              PROT_READ|PROT_WRITE,
	              MAP_PRIVATE|MAP_ANONYMOUS,
	              -1, 0);
	if (buffer == (void*)-1) {
		cando_log(CANDO_LOG_DANGER, "[x] mmap: %s", strerror(errno));
		return NULL;
	}

	buffer->bufferCount = bufferInfo->bufferCount;

	switch (bufferInfo->bufferType) {
		case KMR_BUFFER_GBM_BUFFER:
		case KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS:
			ret = create_gbm_buffers(buffer, bufferInfo);
			if (ret == -1) {
				cando_log_err("%s\n", cando_log_get_error(buffer));
				kmr_buffer_destroy(buffer);
				return NULL;
			}

			break;

		case KMR_BUFFER_DUMP_BUFFER:
			cando_log_err("Dump buffer creation not supported\n");
			kmr_buffer_destroy(buffer);
			return NULL;

		default:
			cando_log_err("Passed incorrect enum kmr_buffer_type\n");
			kmr_buffer_destroy(buffer);
			return NULL;
	}

	ret = CANDO_PAGE_SET_READ(buffer, sizeof(struct kmr_buffer));
	if (ret == -1) {
		cando_log_err("mprotect: %s\n", strerror(errno));
		kmr_buffer_destroy(buffer);
		return NULL;
	}

	return buffer;
}

/**************************************
 * End of kmr_buffer_create functions *
 **************************************/


/***************************************
 * Start of kmr_buffer_write functions *
 ***************************************/

int
kmr_buffer_write (struct kmr_buffer *buffer,
                  const unsigned int bufferIndex,
                  const void *data,
                  const size_t dataSize)
{
	int ret = -1;

	if (!buffer || \
	    !data || \
	    bufferIndex >= buffer->bufferCount || \
	    dataSize <= 0)
	{
		cando_log_set_err(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	if (buffer->bufferObjects[bufferIndex].bo) {
		ret = gbm_bo_write(buffer->bufferObjects[bufferIndex].bo,
		                   data, dataSize);
	}

	return ret;
}

/*************************************
 * End of kmr_buffer_write functions *
 *************************************/


/*************************************
 * Start of kmr_buffer_get functions *
 *************************************/

int
kmr_buffer_get_kms_fd (struct kmr_buffer *buffer,
                       const unsigned int bufferIndex)
{
	if (!buffer || \
	    bufferIndex >= buffer->bufferCount)
	{
		cando_log_set_err(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->bufferObjects[bufferIndex].kmsfd;
}


int
kmr_buffer_get_buffer_count (struct kmr_buffer *buffer)
{
	if (!buffer)
	{
		cando_log_set_err(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->bufferCount;
}


const void *
kmr_buffer_get_gbm_bo (struct kmr_buffer *buffer,
                       const unsigned int bufferIndex)
{
	if (!buffer || \
	    bufferIndex >= buffer->bufferCount)
	{
		cando_log_set_err(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return NULL;
	}

	return buffer->bufferObjects[bufferIndex].bo;
}


int
kmr_buffer_get_framebuffer_id (struct kmr_buffer *buffer,
                               const unsigned int bufferIndex)
{
	if (!buffer || \
	    bufferIndex >= buffer->bufferCount)
	{
		cando_log_set_err(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->bufferObjects[bufferIndex].fbid;
}


int
kmr_buffer_get_pixel_format (struct kmr_buffer *buffer,
                             const unsigned int bufferIndex)
{
	if (!buffer || \
	    bufferIndex >= buffer->bufferCount)
	{
		cando_log_set_err(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->bufferObjects[bufferIndex].format;
}


int
kmr_buffer_get_format_modifier (struct kmr_buffer *buffer,
                                const unsigned int bufferIndex)
{
	if (!buffer || \
	    bufferIndex >= buffer->bufferCount)
	{
		cando_log_set_err(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->bufferObjects[bufferIndex].modifier;
}


int
kmr_buffer_get_plane_count (struct kmr_buffer *buffer,
                            const unsigned int bufferIndex)
{
	if (!buffer || \
	    bufferIndex >= buffer->bufferCount)
	{
		cando_log_set_err(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->bufferObjects[bufferIndex].planeCount;
}


int *
kmr_buffer_get_dma_buf_fds (struct kmr_buffer *buffer,
                            const unsigned int bufferIndex)
{
	if (!buffer || \
	    bufferIndex >= buffer->bufferCount)
	{
		cando_log_set_err(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return NULL;
	}

	return &(buffer->bufferObjects[bufferIndex].dmaBufferFds[0]);
}


int
kmr_buffer_get_plane_pitch (struct kmr_buffer *buffer,
                            const unsigned int bufferIndex,
                            const unsigned int planeIndex)
{
	if (!buffer || \
	    bufferIndex >= buffer->bufferCount || \
            planeIndex >= MAX_PLANE_COUNT)
	{
		cando_log_set_err(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->bufferObjects[bufferIndex].pitches[planeIndex];
}


int
kmr_buffer_get_plane_offset (struct kmr_buffer *buffer,
                             const unsigned int bufferIndex,
                             const unsigned int planeIndex)
{
	if (!buffer || \
	    bufferIndex >= buffer->bufferCount || \
            planeIndex >= MAX_PLANE_COUNT)
	{
		cando_log_set_err(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->bufferObjects[bufferIndex].offsets[planeIndex];
}


int
kmr_buffer_get_dma_buf_fd (struct kmr_buffer *buffer,
                           const unsigned int bufferIndex,
                           const unsigned int dmaBufIndex)
{
	if (!buffer || \
	    bufferIndex >= buffer->bufferCount || \
	    dmaBufIndex >= MAX_PLANE_COUNT)
	{
		cando_log_set_err(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->bufferObjects[bufferIndex].dmaBufferFds[dmaBufIndex];
}

/***********************************
 * End of kmr_buffer_get functions *
 ***********************************/


/*****************************************
 * Start of kmr_buffer_destroy functions *
 *****************************************/

void
kmr_buffer_destroy (struct kmr_buffer *buffer)
{
	unsigned int b, p;

	struct kmr_buffer_object *bufferObject = NULL;

	if (!buffer)
		return;

	for (b = 0; b < buffer->bufferCount; b++) {
		bufferObject = &(buffer->bufferObjects[b]);

		if (bufferObject->fbid) {
			fsync(bufferObject->fbid);
			ioctl(bufferObject->kmsfd, DRM_IOCTL_MODE_RMFB, &(bufferObject->fbid));
		}

		if (bufferObject->bo)
			gbm_bo_destroy(bufferObject->bo);

		for (p = 0; p < bufferObject->planeCount; p++)
			drmCloseBufferHandle(bufferObject->kmsfd, bufferObject->dmaBufferFds[p]);
	}

	if (buffer->gbmDevice)
		gbm_device_destroy(buffer->gbmDevice);

	munmap(buffer, sizeof(struct kmr_buffer));
}

/***************************************
 * End of kmr_buffer_destroy functions *
 ***************************************/
