#ifndef UVR_RENDERERS_UNDERVIEW_H
#define UVR_RENDERERS_UNDERVIEW_H

#include <stdbool.h>


/*
 * uvr_renderers_underview_bind_buffer:
 *
 * args:
 * @renderer -
 * @buffer   -
 * return:
 *	on success true
 *	on failure false
 */
bool uvr_renderers_underview_bind_buffer(struct wlr_renderer *renderer, struct wlr_buffer *buffer);


/*
 * uvr_renderers_underview_begin:
 *
 * args:
 * @renderer -
 * @width    -
 * @height   -
 * return:
 *	on success true
 *	on failure false
 */
bool uvr_renderers_underview_begin(struct wlr_renderer *renderer, uint32_t width, uint32_t height);


/*
 * uvr_renderers_underview_end:
 *
 * args:
 * @renderer -
 */
void uvr_renderers_underview_end(struct wlr_renderer *renderer);


/*
 * uvr_renderers_underview_clear:
 *
 * args:
 * @renderer -
 * @color    -
 */
void uvr_renderers_underview_clear(struct wlr_renderer *renderer, const float color[4]);


/*
 * uvr_renderers_underview_scissor:
 *
 * args:
 * @renderer -
 * @box      -
 */
void uvr_renderers_underview_scissor(struct wlr_renderer *renderer, struct wlr_box *box);


/*
 * uvr_renderers_underview_render_subtexture_with_matrix:
 *
 * args:
 * @renderer -
 * @texture  -
 * @box      -
 * @matrix   -
 * @alpha    -
 * return:
 *	on success true
 *	on failure false
 */
bool uvr_renderers_underview_render_subtexture_with_matrix(struct wlr_renderer *renderer,
                                                           struct wlr_texture *texture,
                                                           const struct wlr_fbox *box,
                                                           const float matrix[9],
                                                           float alpha);


/*
 * uvr_renderers_underview_render_quad_with_matrix:
 *
 * args:
 * @renderer -
 * @color    -
 * @matrix   -
 */
void uvr_renderers_underview_render_quad_with_matrix(struct wlr_renderer *renderer,
                                                     const float color[4],
                                                     const float matrix[9]);


/*
 * uvr_renderers_underview_get_shm_texture_formats:
 *
 * args:
 * @renderer -
 * @length   -
 * return:
 *	on success pointer to an unsigned 32 bit integer
 *	on failure NULL
 */
const uint32_t *uvr_renderers_underview_get_shm_texture_formats(struct wlr_renderer *renderer, size_t *length);


/*
 * uvr_renderers_underview_get_dmabuf_texture_formats:
 *
 * args:
 * @renderer -
 * return:
 *	on success pointer to a struct wlr_drm_format_set
 *	on failure NULL
 */
const struct wlr_drm_format_set *uvr_renderers_underview_get_dmabuf_texture_formats(struct wlr_renderer *renderer);


/*
 * uvr_renderers_underview_get_render_formats:
 *
 * args:
 * @renderer -
 * return:
 *	on success pointer to a struct wlr_drm_format_set
 *	on failure NULL
 */
const struct wlr_drm_format_set *uvr_renderers_underview_get_render_formats(struct wlr_renderer *renderer);


/*
 * uvr_renderers_underview_preferred_read_format:
 *
 * args:
 * @renderer -
 * return:
 *	on success unsigned 32 bit integer
 *	on failure unsigned 32 bit integer max 0xffffffff
 */
uint32_t uvr_renderers_underview_preferred_read_format(struct wlr_renderer *renderer);


/*
 * uvr_renderers_underview_read_pixels:
 *
 * args:
 * @renderer -
 * @fmt      -
 * @stride   -
 * @width    -
 * @height   -
 * @src_x    -
 * @src_y    -
 * @dst_x    -
 * @dst_y    -
 * @data     -
 * return:
 *	on success true
 *	on failure false
 */
bool uvr_renderers_underview_read_pixels(struct wlr_renderer *renderer,
                                         uint32_t fmt,
                                         uint32_t stride,
                                         uint32_t width,
                                         uint32_t height,
                                         uint32_t src_x,
                                         uint32_t src_y,
                                         uint32_t dst_x,
                                         uint32_t dst_y,
                                         void *data);

/*
 * uvr_renderers_underview_destroy:
 *
 * args:
 * @renderer -
 */
void uvr_renderers_underview_destroy(struct wlr_renderer *renderer);


/*
 * uvr_renderers_underview_get_drm_fd:
 *
 * args:
 * @renderer -
 * return:
 *	on success valid kms/drm file descriptor to open DRI node
 *	on failure -1
 */
int uvr_renderers_underview_get_drm_fd(struct wlr_renderer *renderer);


/*
 * uvr_renderers_underview_get_render_buffer_caps:
 *
 * args:
 * @renderer -
 * return:
 * 	WLR_BUFFER_CAP_DMABUF
 */
uint32_t uvr_renderers_underview_get_render_buffer_caps(struct wlr_renderer *renderer);


/*
 * uvr_renderers_underview_texture_from_buffer:
 *
 * args:
 * @renderer -
 * return:
 *	on success pointer to a struct wlr_texture
 *	on failure NULL
 */
struct wlr_texture *uvr_renderers_underview_texture_from_buffer(struct wlr_renderer UNUSED *renderer,
                                                                struct wlr_buffer UNUSED *buffer);

#endif
