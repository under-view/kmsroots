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
 * @brief Structure defining kmsroots Buffer Object
 *
 * @member bo           - Handle to some GEM allocated buffer. Used to get GEM handles,
 *                        DMA buffer fds (fd associate with GEM buffer), pitches, and
 *                        offsets for the buffer used by DRI device (GPU).
 * @member fbid         - Framebuffer ID.
 * @member format       - The format of an image details how each pixel color channels
 *                        is laid out in memory: (i.e. RAM, VRAM, etc...). So, basically
 *                        the width in bits, type, and ordering of each pixels color channels.
 * @member modifier     - The modifier details information on how pixels should be within a
 *                        buffer for different types of operations such as scan out or rendering
 *                        (i.e linear, tiled, compressed, etc...).
 *                        https://01.org/linuxgraphics/Linux-Window-Systems-with-DRM
 * @member plane_count  - Number of Planar Formats. The number of @dma_buff_fds, @offsets,
 *                        @pitches retrieved per plane. More information may be found
 *                        https://en.wikipedia.org/wiki/Planar_(computer_graphics).
 * @member pitches      - Width in bytes for each plane.
 * @member offsets      - Offset of each plane. More information can be found
 *                        https://gitlab.freedesktop.org/mesa/drm/-/blob/main/include/drm/drm_mode.h#L589.
 * @member dma_buff_fds - (PRIME fd) Stores file descriptors to buffers that can be shared across hardware.
 */
struct kmr_buffer_object
{
	struct gbm_bo *bo;
	int           fbid;
	unsigned int  format;
	uint64_t      modifier;
	unsigned int  plane_count;
	unsigned int  pitches[MAX_PLANE_COUNT];
	unsigned int  offsets[MAX_PLANE_COUNT];
	int           dma_buff_fds[MAX_PLANE_COUNT];
};


/*
 * @brief struct kmr_buffer (kmsroots Buffer)
 *
 * @member err          - Stores information about the error that occured
 *                        for the given instance and may later be retrieved
 *                        by caller.
 * @member free         - If structure allocated with calloc(3) member will be
 *                        set to true so that, we know to call free(3) when
 *                        destroying the instance.
 * @member kms_fd       - File descriptor to open DRI device.
 * @member gbm_device   - A handle used to allocate gbm buffers & surfaces.
 * @member buffer_count - Array size of @buff_objs.
 * @member buff_objs    - Stores an array of gbm_bo's and corresponding
 *                        information about the individual buffer.
 */
struct kmr_buffer
{
	struct cando_log_error_struct err;
	bool                          free;
	int                           kms_fd;
	struct gbm_device             *gbm_device;
	unsigned int                  buffer_count;
	struct kmr_buffer_object      buff_objs[MAX_BUFFER_COUNT];
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
(*frame_buffer_func_impl) (struct kmr_buffer *,
                           const struct kmr_buffer_create_info *,
                           const unsigned int,
                           const unsigned int *);


static struct gbm_bo *
gbm_buffer_create_impl (struct gbm_device *gbm_device,
                        const struct kmr_buffer_create_info *buff_info)
{
	return gbm_bo_create(gbm_device,
	                     buff_info->width,
	                     buff_info->height,
	                     buff_info->pixel_format,
	                     buff_info->gbm_bo_flags);
}


static int
gbm_frame_buffer_create_impl (struct kmr_buffer *buffer,
                              const struct kmr_buffer_create_info *buff_info,
                              const unsigned int cur_buff,
                              const unsigned int *gem_handles)
{
	int err = -1;

	struct drm_mode_fb_cmd frame_buffer;

	struct kmr_buffer_object *buff_obj = NULL;

	memset(&frame_buffer,0,sizeof(struct drm_mode_fb_cmd));

	buff_obj = &(buffer->buff_objs[cur_buff]);

	frame_buffer.bpp    = buff_info->bits_per_pixel;
	frame_buffer.depth  = buff_info->bit_depth;
	frame_buffer.width  = buff_info->width;
	frame_buffer.height = buff_info->height;
	frame_buffer.pitch  = buff_obj->pitches[0];
	frame_buffer.handle = *gem_handles;

	err = ioctl(buffer->kms_fd, DRM_IOCTL_MODE_ADDFB, &frame_buffer);
	if (err == -1) {
		cando_log_set_error(buffer, errno,
		                    "ioctl(DRM_IOCTL_MODE_ADDFB): %s",
		                    strerror(errno));
		return -1;
	}

	return frame_buffer.fb_id;
}


static struct gbm_bo *
gbm_buffer_create_with_modifiers_impl (struct gbm_device *gbm_device,
                                       const struct kmr_buffer_create_info *buff_info)
{
	return gbm_bo_create_with_modifiers2(gbm_device,
	                                     buff_info->width,
	                                     buff_info->height,
	                                     buff_info->pixel_format,
	                                     buff_info->modifiers,
	                                     buff_info->modifier_count,
	                                     buff_info->gbm_bo_flags);
}


static int
gbm_frame_buffer_create_with_modifiers_impl (struct kmr_buffer *buffer,
                                             const struct kmr_buffer_create_info *buff_info,
                                             const unsigned int cur_buff,
                                             const unsigned int *gem_handles)
{
	int err = -1;

	struct drm_mode_fb_cmd2 frame_buffer;

	struct kmr_buffer_object *buff_obj = NULL;

	buff_obj = &(buffer->buff_objs[cur_buff]);

	memset(&frame_buffer,0,sizeof(struct drm_mode_fb_cmd2));

	frame_buffer.width  = buff_info->width;
	frame_buffer.height = buff_info->height;
	frame_buffer.pixel_format = buff_obj->format;
	frame_buffer.flags = DRM_MODE_FB_MODIFIERS;

	memcpy(frame_buffer.handles, gem_handles, sizeof(frame_buffer.handles));
	memcpy(frame_buffer.pitches, buff_obj->pitches, sizeof(frame_buffer.pitches));
	memcpy(frame_buffer.offsets, buff_obj->offsets, sizeof(frame_buffer.offsets));
	memcpy(frame_buffer.modifier, buff_info->modifiers, sizeof(frame_buffer.modifier));

	err = ioctl(buffer->kms_fd, DRM_IOCTL_MODE_ADDFB2, &frame_buffer);
	if (err == -1) {
		cando_log_set_error(buffer, errno,
		                    "ioctl(DRM_IOCTL_MODE_ADDFB2): %s",
		                    strerror(errno));
		return -1;
	}

	return frame_buffer.fb_id;
}


struct gbm_func_impl
{
	gbm_func_impl          gbm_bo_create;
	frame_buffer_func_impl get_frame_buffer_id;
};


struct gbm_func_impl
gbm_funcs[KMR_BUFFER_MAX_TYPE] = \
{
	[KMR_BUFFER_GBM_BUFFER] = \
	{
		.gbm_bo_create = gbm_buffer_create_impl,
		.get_frame_buffer_id = gbm_frame_buffer_create_impl,
	},

	[KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS] = \
	{
		.gbm_bo_create = gbm_buffer_create_with_modifiers_impl,
		.get_frame_buffer_id = gbm_frame_buffer_create_with_modifiers_impl,
	}
};


static int
create_planes (struct kmr_buffer *buffer,
               unsigned int cur_buff,
               unsigned int *gem_handles)
{
	int ret = -1;

	uint32_t cur_plane;

	union gbm_bo_handle bo_handle;

	struct drm_prime_handle drm_prime_req;

	struct kmr_buffer_object *buff_obj = &(buffer->buff_objs[cur_buff]);

	for (cur_plane = 0; cur_plane < buff_obj->plane_count; cur_plane++) {
		memset(&bo_handle,0,sizeof(bo_handle));

		bo_handle = gbm_bo_get_handle_for_plane(buff_obj->bo, cur_plane);
		if (!(bo_handle.u32) || bo_handle.s32 == -1) {
			cando_log_set_error(buffer, CANDO_LOG_ERR_UNCOMMON,
					    "failed to get BO plane %d gem handle (modifier 0x%" PRIx64 ")",
					    cur_plane, buff_obj->modifier);
			return -1;
		}

		buff_obj->pitches[cur_plane] = gbm_bo_get_stride_for_plane(buff_obj->bo, cur_plane);
		buff_obj->offsets[cur_plane] = gbm_bo_get_offset(buff_obj->bo, cur_plane);

		gem_handles[cur_plane] = bo_handle.u32;
		drm_prime_req.handle   = gem_handles[cur_plane];
		drm_prime_req.flags    = DRM_RDWR;
		drm_prime_req.fd       = -1;

		/*
		 * Retrieve a DMA-BUF fd (PRIME fd) for a given GEM buffer via the GEM handle.
		 * This fd can be passed along to other processes
		 */
		ret = ioctl(buffer->kms_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &drm_prime_req);
		if (ret == -1)  {
			cando_log_set_error(buffer, errno,
					    "ioctl(DRM_IOCTL_PRIME_HANDLE_TO_FD): %s",
					    strerror(errno));
			return -1;
		}

		buff_obj->dma_buff_fds[cur_plane] = drm_prime_req.fd;
	}

	return 0;
}


static int
create_gbm_buffers (struct kmr_buffer *buffer,
                    const struct kmr_buffer_create_info *buff_info)
{
	int ret = -1;

	unsigned int cur_buff;

	/*
	 * Stores GEM handles per plane used to retrieve an FD to a DMA-BUF
	 * and retrieve a KMS frame_buffer ID for modesetting purposes.
	 */
	unsigned int gem_handles[4];

	struct kmr_buffer_object *buff_obj = NULL;

	buffer->kms_fd = buff_info->kms_fd;
	buffer->gbm_device = gbm_create_device(buffer->kms_fd);
	if (!(buffer->gbm_device)) {
		cando_log_set_error(buffer, CANDO_LOG_ERR_UNCOMMON,
		                    "Failed to create gbm device context.");
		return -1;
	}

	for (cur_buff = 0; cur_buff < buff_info->buffer_count; cur_buff++) {

		buff_obj = &(buffer->buff_objs[cur_buff]);

		buff_obj->bo = gbm_funcs[buff_info->buffer_type].gbm_bo_create(buffer->gbm_device, buff_info);
		if (!(buff_obj->bo)) {
			cando_log_set_error(buffer, CANDO_LOG_ERR_UNCOMMON,
			                    "%s: failed to create gbm_bo with res %u x %u",
			                    buff_info->buffer_type == KMR_BUFFER_GBM_BUFFER ? \
			                    "gbm_bo_create" : "gbm_bo_create_with_modifiers2",
			                    buff_info->width, buff_info->height);
			return -1;
		}

		buff_obj->plane_count = gbm_bo_get_plane_count(buff_obj->bo);
		buff_obj->modifier = gbm_bo_get_modifier(buff_obj->bo);
		buff_obj->format = gbm_bo_get_format(buff_obj->bo);

		memset(gem_handles,0,sizeof(gem_handles));

		ret = create_planes(buffer, cur_buff, &(gem_handles[0]));
		if (ret == -1)
			return -1;

		/*
		 * TAKEN from Daniel Stone kms-quads
		 *
		 * Wrap our GEM buffer in a KMS frame_buffer, so we can then attach it
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
		buff_obj->fbid = \
		gbm_funcs[buff_info->buffer_type].get_frame_buffer_id(buffer,
	                                                              buff_info,
		                                                      cur_buff,
		                                                      &(gem_handles[0]));
		if (buff_obj->fbid == -1)
			return -1;
	}

	cando_log(CANDO_LOG_SUCCESS, "Successfully create GBM buffers\n");

	return 0;
}


struct kmr_buffer *
kmr_buffer_create (struct kmr_buffer *p_buffer,
                   const void *p_buff_info)
{
	int err = -1;

	struct kmr_buffer *buffer = p_buffer;

	const struct kmr_buffer_create_info *buff_info = p_buff_info;

	if (!buff_info || \
	    buff_info->buffer_count >= MAX_BUFFER_COUNT)
	{
		cando_log_error("Incorrect data passed\n");
		return NULL;
	}

	if (!buffer) {
		buffer = calloc(1, sizeof(struct kmr_buffer));
		if (!buffer) {
			cando_log_error("calloc: %s\n", strerror(errno));
			return NULL;
		}

		buffer->free = true;
	}

	buffer->buffer_count = buff_info->buffer_count;

	switch (buff_info->buffer_type) {
		case KMR_BUFFER_GBM_BUFFER:
		case KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS:
			err = create_gbm_buffers(buffer, buff_info);
			if (err == -1) {
				cando_log_error("%s\n", cando_log_get_error(buffer));
				kmr_buffer_destroy(buffer);
				return NULL;
			}

			break;

		case KMR_BUFFER_DUMP_BUFFER:
			cando_log_error("Dump buffer creation not supported\n");
			kmr_buffer_destroy(buffer);
			return NULL;

		default:
			cando_log_error("Passed incorrect enum kmr_buffer_type\n");
			kmr_buffer_destroy(buffer);
			return NULL;
	}

	err = CANDO_PAGE_SET_READ(buffer, sizeof(struct kmr_buffer));
	if (err == -1) {
		cando_log_error("mprotect: %s\n", strerror(errno));
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
                  const unsigned int buff_index,
                  const void *data,
                  const size_t size)
{
	int ret = -1;

	if (!buffer || \
	    !data || \
	    buff_index >= buffer->buffer_count || \
	    size <= 0)
	{
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	if (buffer->buff_objs[buff_index].bo) {
		ret = gbm_bo_write(buffer->buff_objs[buff_index].bo, data, size);
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
kmr_buffer_get_buffer_count (struct kmr_buffer *buffer)
{
	if (!buffer)
	{
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->buffer_count;
}


int
kmr_buffer_get_kms_fd (struct kmr_buffer *buffer)
{
	if (!buffer)
	{
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->kms_fd;
}


const void *
kmr_buffer_get_gbm_bo (struct kmr_buffer *buffer,
                       const unsigned int buff_index)
{
	if (!buffer || \
	    buff_index >= buffer->buffer_count)
	{
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return NULL;
	}

	return buffer->buff_objs[buff_index].bo;
}


int
kmr_buffer_get_frame_buffer_id (struct kmr_buffer *buffer,
                                const unsigned int buff_index)
{
	if (!buffer || \
	    buff_index >= buffer->buffer_count)
	{
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->buff_objs[buff_index].fbid;
}


int
kmr_buffer_get_pixel_format (struct kmr_buffer *buffer,
                             const unsigned int buff_index)
{
	if (!buffer || \
	    buff_index >= buffer->buffer_count)
	{
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->buff_objs[buff_index].format;
}


int
kmr_buffer_get_format_modifier (struct kmr_buffer *buffer,
                                const unsigned int buff_index)
{
	if (!buffer || \
	    buff_index >= buffer->buffer_count)
	{
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->buff_objs[buff_index].modifier;
}


int
kmr_buffer_get_plane_count (struct kmr_buffer *buffer,
                            const unsigned int buff_index)
{
	if (!buffer || \
	    buff_index >= buffer->buffer_count)
	{
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->buff_objs[buff_index].plane_count;
}


int *
kmr_buffer_get_dma_buf_fds (struct kmr_buffer *buffer,
                            const unsigned int buff_index)
{
	if (!buffer || \
	    buff_index >= buffer->buffer_count)
	{
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return NULL;
	}

	return &(buffer->buff_objs[buff_index].dma_buff_fds[0]);
}


int
kmr_buffer_get_plane_pitch (struct kmr_buffer *buffer,
                            const unsigned int buff_index,
                            const unsigned int plane_index)
{
	if (!buffer || \
	    buff_index >= buffer->buffer_count || \
            plane_index >= MAX_PLANE_COUNT)
	{
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->buff_objs[buff_index].pitches[plane_index];
}


int
kmr_buffer_get_plane_offset (struct kmr_buffer *buffer,
                             const unsigned int buff_index,
                             const unsigned int plane_index)
{
	if (!buffer || \
	    buff_index >= buffer->buffer_count || \
            plane_index >= MAX_PLANE_COUNT)
	{
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->buff_objs[buff_index].offsets[plane_index];
}


int
kmr_buffer_get_dma_buf_fd (struct kmr_buffer *buffer,
                           const unsigned int buff_index,
                           const unsigned int dma_buff_index)
{
	if (!buffer || \
	    buff_index >= buffer->buffer_count || \
	    dma_buff_index >= MAX_PLANE_COUNT)
	{
		cando_log_set_error(buffer, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return buffer->buff_objs[buff_index].dma_buff_fds[dma_buff_index];
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

	struct kmr_buffer_object *buff_obj = NULL;

	if (!buffer)
		return;

	for (b = 0; b < buffer->buffer_count; b++) {
		buff_obj = &(buffer->buff_objs[b]);

		if (buff_obj->fbid) {
			fsync(buff_obj->fbid);
			ioctl(buffer->kms_fd, DRM_IOCTL_MODE_RMFB, &(buff_obj->fbid));
		}

		if (buff_obj->bo)
			gbm_bo_destroy(buff_obj->bo);

		for (p = 0; p < buff_obj->plane_count; p++)
			drmCloseBufferHandle(buffer->kms_fd, buff_obj->dma_buff_fds[p]);
	}

	if (buffer->gbm_device)
		gbm_device_destroy(buffer->gbm_device);

	if (buffer->free) {
		free(buffer);
	} else {
		memset(buffer, 0, sizeof(struct kmr_buffer));
	}
}

/***************************************
 * End of kmr_buffer_destroy functions *
 ***************************************/


/**************************************************
 * Start of non struct kmr_buffer param functions *
 **************************************************/

int
kmr_buffer_get_sizeof (void)
{
	return sizeof(struct kmr_buffer);
}

/************************************************
 * End of non struct kmr_buffer param functions *
 ************************************************/
