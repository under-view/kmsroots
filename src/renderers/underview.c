#include <stdlib.h>
#include <stdbool.h>
#include <wlr/render/interface.h>

#include "vulkan.h"
#include "wserver.h"


static bool renderer_underview_bind_buffer(struct wlr_renderer UNUSED *renderer, struct wlr_buffer UNUSED *buffer)
{
	return true;
}


static bool renderer_underview_begin(struct wlr_renderer UNUSED *renderer, uint32_t UNUSED width, uint32_t UNUSED height)
{
	return true;
}


static void renderer_underview_end(struct wlr_renderer UNUSED *renderer)
{
	return;
}


static void renderer_underview_clear(struct wlr_renderer UNUSED *renderer, const float UNUSED color[4])
{
	return;
}


static void renderer_underview_scissor(struct wlr_renderer UNUSED *renderer, struct wlr_box UNUSED *box)
{
	return;
}


static bool renderer_underview_render_subtexture_with_matrix(struct wlr_renderer UNUSED *renderer,
                                                             struct wlr_texture UNUSED *texture,
                                                             const struct wlr_fbox UNUSED *box,
                                                             const float UNUSED matrix[9],
                                                             float UNUSED alpha)
{
	return true;
}


static void renderer_underview_render_quad_with_matrix(struct wlr_renderer UNUSED *renderer,
                                                       const float UNUSED color[4],
                                                       const float UNUSED matrix[9])
{
	return;
}


static const uint32_t *renderer_underview_get_shm_texture_formats(struct wlr_renderer UNUSED *renderer, size_t UNUSED *len)
{
	return NULL;
}


static const struct wlr_drm_format_set *renderer_underview_get_dmabuf_texture_formats(struct wlr_renderer UNUSED *renderer)
{
	return NULL;
}


static const struct wlr_drm_format_set *renderer_underview_get_render_formats(struct wlr_renderer UNUSED *renderer)
{
	return NULL;
}


static uint32_t renderer_underview_preferred_read_format(struct wlr_renderer UNUSED *renderer)
{
	return 0;
}


static bool renderer_underview_read_pixels(struct wlr_renderer UNUSED *renderer,
                                           uint32_t UNUSED fmt,
                                           uint32_t UNUSED stride,
                                           uint32_t UNUSED width,
                                           uint32_t UNUSED height,
                                           uint32_t UNUSED src_x,
                                           uint32_t UNUSED src_y,
                                           uint32_t UNUSED dst_x,
                                           uint32_t UNUSED dst_y,
                                           void UNUSED *data)
{
	return true;
}


static void renderer_underview_destroy(struct wlr_renderer UNUSED *renderer)
{
	return;
}


static int renderer_underview_get_drm_fd(struct wlr_renderer UNUSED *renderer)
{
	return 0;
}


static uint32_t renderer_underview_get_render_buffer_caps(struct wlr_renderer UNUSED *renderer)
{
	return 0;
}


static struct wlr_texture *renderer_underview_texture_from_buffer(struct wlr_renderer UNUSED *renderer,
                                                                  struct wlr_buffer UNUSED *buffer)
{
	return NULL;
}


const struct wlr_renderer_impl UNUSED renderer_impl = {
	.bind_buffer = renderer_underview_bind_buffer,
	.begin = renderer_underview_begin,
	.end = renderer_underview_end,
	.clear = renderer_underview_clear,
	.scissor = renderer_underview_scissor,
	.render_subtexture_with_matrix = renderer_underview_render_subtexture_with_matrix,
	.render_quad_with_matrix = renderer_underview_render_quad_with_matrix,
	.get_shm_texture_formats = renderer_underview_get_shm_texture_formats,
	.get_dmabuf_texture_formats = renderer_underview_get_dmabuf_texture_formats,
	.get_render_formats = renderer_underview_get_render_formats,
	.preferred_read_format = renderer_underview_preferred_read_format,
	.read_pixels = renderer_underview_read_pixels,
	.destroy = renderer_underview_destroy,
	.get_drm_fd = renderer_underview_get_drm_fd,
	.get_render_buffer_caps = renderer_underview_get_render_buffer_caps,
	.texture_from_buffer= renderer_underview_texture_from_buffer
};

