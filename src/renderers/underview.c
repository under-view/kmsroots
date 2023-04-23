#include <stdlib.h>
#include <stdbool.h>
#include <wlr/render/interface.h>

#include "vulkan.h"
#include "wserver.h"
#include "renderers/underview.h"

bool uvr_renderers_underview_bind_buffer(struct wlr_renderer UNUSED *renderer, struct wlr_buffer UNUSED *buffer)
{
	return true;
}


bool uvr_renderers_underview_begin(struct wlr_renderer UNUSED *renderer, uint32_t UNUSED width, uint32_t UNUSED height)
{
	return true;
}


void uvr_renderers_underview_end(struct wlr_renderer UNUSED *renderer)
{
	return;
}


void uvr_renderers_underview_clear(struct wlr_renderer UNUSED *renderer, const float UNUSED color[4])
{
	return;
}


void uvr_renderers_underview_scissor(struct wlr_renderer UNUSED *renderer, struct wlr_box UNUSED *box)
{
	return;
}


bool uvr_renderers_underview_render_subtexture_with_matrix(struct wlr_renderer UNUSED *renderer,
                                                           struct wlr_texture UNUSED *texture,
                                                           const struct wlr_fbox UNUSED *box,
                                                           const float UNUSED matrix[9],
                                                           float UNUSED alpha)
{
	return true;
}


void uvr_renderers_underview_render_quad_with_matrix(struct wlr_renderer UNUSED *renderer,
                                                     const float UNUSED color[4],
                                                     const float UNUSED matrix[9])
{
	return;
}


const uint32_t *uvr_renderers_underview_get_shm_texture_formats(struct wlr_renderer UNUSED *renderer, size_t UNUSED *length)
{
	return NULL;
}


const struct wlr_drm_format_set *uvr_renderers_underview_get_dmabuf_texture_formats(struct wlr_renderer UNUSED *renderer)
{
	return NULL;
}


const struct wlr_drm_format_set *uvr_renderers_underview_get_render_formats(struct wlr_renderer UNUSED *renderer)
{
	return NULL;
}


uint32_t uvr_renderers_underview_preferred_read_format(struct wlr_renderer UNUSED *renderer)
{
	return 0;
}


bool uvr_renderers_underview_read_pixels(struct wlr_renderer UNUSED *renderer,
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


void uvr_renderers_underview_destroy(struct wlr_renderer UNUSED *renderer)
{
	return;
}


int uvr_renderers_underview_get_drm_fd(struct wlr_renderer UNUSED *renderer)
{
	return 0;
}


uint32_t uvr_renderers_underview_get_render_buffer_caps(struct wlr_renderer UNUSED *renderer)
{
	return WLR_BUFFER_CAP_DMABUF;
}


struct wlr_texture *uvr_renderers_underview_texture_from_buffer(struct wlr_renderer UNUSED *renderer,
                                                                struct wlr_buffer UNUSED *buffer)
{
	return NULL;
}
