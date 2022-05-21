#include "buffer.h"


struct uvr_buffer uvr_buffer_create(struct uvr_buffer_create_info *uvrbuff) {
  struct gbm_device *gbmdev = NULL;
  struct uvr_buffer_object_info *bois = NULL;

  if (uvrbuff->bType == GBM_BUFFER || uvrbuff->bType == GBM_BUFFER_WITH_MODIFIERS) {
    gbmdev = gbm_create_device(uvrbuff->kmsfd);
    if (!gbmdev) goto exit_uvr_buffer_null_struct;

    bois = calloc(uvrbuff->buff_cnt, sizeof(struct uvr_buffer_object_info));
    if (!bois) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_buffer_create(calloc): failed to allocate space for struct uvrbuff_object_info *");
      goto exit_uvr_buffer_gbmdev_destroy;
    }
  }

  if (uvrbuff->bType == GBM_BUFFER) {
    for (unsigned b = 0; b < uvrbuff->buff_cnt; b++) {
      bois[b].kmsfd = uvrbuff->kmsfd;
      bois[b].bo = gbm_bo_create(gbmdev, uvrbuff->width, uvrbuff->height, uvrbuff->pixformat, uvrbuff->gbm_bo_flags);
      if (!bois[b].bo) {
        uvr_utils_log(UVR_DANGER, "[x] uvr_buffer_create(gbm_bo_create): failed to create gbm_bo with res %u x %u", uvrbuff->width, uvrbuff->height);
        goto exit_uvr_buffer_gbm_bo_detroy;
      }
    }
  }

  if (uvrbuff->bType == GBM_BUFFER_WITH_MODIFIERS) {
    for (unsigned b = 0; b < uvrbuff->buff_cnt; b++) {
      bois[b].kmsfd = uvrbuff->kmsfd;
      bois[b].bo = gbm_bo_create_with_modifiers2(gbmdev, uvrbuff->width, uvrbuff->height, uvrbuff->pixformat, uvrbuff->modifiers, uvrbuff->modifiers_cnt, uvrbuff->gbm_bo_flags);
      if (!bois[b].bo) {
        uvr_utils_log(UVR_DANGER, "[x] uvr_buffer_create(gbm_bo_create_with_modifiers): failed to create gbm_bo with res %u x %u", uvrbuff->width, uvrbuff->height);
        goto exit_uvr_buffer_gbm_bo_detroy;
      }
    }
  }

  for (unsigned b = 0; b < uvrbuff->buff_cnt; b++) {
    bois[b].num_planes = gbm_bo_get_plane_count(bois[b].bo);
    bois[b].modifier = gbm_bo_get_modifier(bois[b].bo);
    bois[b].format = gbm_bo_get_format(bois[b].bo);

    for (unsigned p = 0; p < bois[b].num_planes; p++) {
      union gbm_bo_handle h;
      memset(&h,0,sizeof(h));

      h = gbm_bo_get_handle_for_plane(bois[b].bo, p);
      if (!h.u32 || h.s32 == -1) {
        uvr_utils_log(UVR_DANGER, "[x] failed to get BO plane %d gem handle (modifier 0x%" PRIx64 ")", p, bois[b].modifier);
        goto exit_uvr_buffer_gbm_bo_detroy;
      }

      bois[b].gem_handles[p] = h.u32;

      bois[b].pitches[p] = gbm_bo_get_stride_for_plane(bois[b].bo, p);
      if (!bois[b].pitches[p]) {
        uvr_utils_log(UVR_DANGER, "[x] failed to get stride/pitch for BO plane %d (modifier 0x%" PRIx64 ")", p, bois[b].modifier);
        goto exit_uvr_buffer_gbm_bo_detroy;
      }

      bois[b].offsets[p] = gbm_bo_get_offset(bois[b].bo, p);

      struct drm_prime_handle prime_request = {
        .handle = bois[b].gem_handles[p],
        .flags  = DRM_RDWR,
        .fd     = -1
      };

      /* Retrieve a DMA-BUF fd (PRIME fd) from the GEM handle/name to pass along to other processes */
      if (ioctl(bois[b].kmsfd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_request) == -1)  {
        uvr_utils_log(UVR_DANGER, "[x] uvr_buffer_create(ioctl): %s", strerror(errno));
        goto exit_uvr_buffer_gbm_bo_detroy;
      }

      bois[b].dma_buf_fds[p] = prime_request.fd;
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
     * Dumb buffers are always strictly single-planar, so we do not need
     * the extra planes nor the offset field.
     *
     * drmModeAddFB2WithModifiers(struct drm_mode_fb_cmd2) takes a list of modifiers per plane, however
     * the kernel enforces that they must be the same for each plane
     * which is there, and 0 for everything else.
     */
    if (uvrbuff->bType == GBM_BUFFER) {
      struct drm_mode_fb_cmd f;
      memset(&f,0,sizeof(struct drm_mode_fb_cmd));

      f.bpp    = uvrbuff->bpp;
      f.depth  = uvrbuff->bitdepth;
      f.width  = uvrbuff->width;
      f.height = uvrbuff->height;
      f.pitch  = bois[b].pitches[0];
      f.handle = bois[b].gem_handles[0];
      bois[b].fbid = 0;

      if (ioctl(bois[b].kmsfd, DRM_IOCTL_MODE_ADDFB, &f) == -1) {
        uvr_utils_log(UVR_DANGER, "[x] ioctl(DRM_IOCTL_MODE_ADDFB): %s", strerror(errno));
        goto exit_uvr_buffer_gbm_bo_detroy;
      }

      bois[b].fbid = f.fb_id;
    }

    if (uvrbuff->bType == GBM_BUFFER_WITH_MODIFIERS) {
      struct drm_mode_fb_cmd2 f;
      memset(&f,0,sizeof(struct drm_mode_fb_cmd2));

      f.width  = uvrbuff->width;
      f.height = uvrbuff->height;
      f.pixel_format = bois[b].format;
      f.flags = DRM_MODE_FB_MODIFIERS;

      memcpy(f.handles , bois[b].gem_handles, sizeof(f.handles));
      memcpy(f.pitches , bois[b].pitches    , sizeof(f.pitches));
      memcpy(f.offsets , bois[b].offsets    , sizeof(f.offsets));
      memcpy(f.modifier, uvrbuff->modifiers , sizeof(f.modifier));
      bois[b].fbid = 0;

      if (ioctl(bois[b].kmsfd, DRM_IOCTL_MODE_ADDFB2, &f) == -1) {
        uvr_utils_log(UVR_DANGER, "[x] ioctl(DRM_IOCTL_MODE_ADDFB2): %s", strerror(errno));
        goto exit_uvr_buffer_gbm_bo_detroy;
      }

      bois[b].fbid = f.fb_id;
    }
  }

  uvr_utils_log(UVR_SUCCESS, "Successfully create GBM buffers");

  return (struct uvr_buffer) { .gbmdev = gbmdev, .info_buffers = bois };

exit_uvr_buffer_gbm_bo_detroy:
  for (unsigned b = 0; b < uvrbuff->buff_cnt; b++) {
    if (bois[b].fbid)
      ioctl(bois[b].kmsfd, DRM_IOCTL_MODE_RMFB, &bois[b].fbid);
    if (bois[b].bo)
      gbm_bo_destroy(bois[b].bo);
    for (unsigned p = 0; p < bois[b].num_planes; p++) {
      if (bois[b].dma_buf_fds[p] != -1) {
        close(bois[b].dma_buf_fds[p]);
        bois[b].dma_buf_fds[p] = -1;
      }
    }
  }
  free(bois);
exit_uvr_buffer_gbmdev_destroy:
  if (gbmdev)
    gbm_device_destroy(gbmdev);
exit_uvr_buffer_null_struct:
  return (struct uvr_buffer) { .gbmdev = NULL, .info_buffers = NULL };
}


void uvr_buffer_destory(struct uvr_buffer_destroy *uvrbuff) {
  for (unsigned b = 0; b < uvrbuff->buff_cnt; b++) {
    if (uvrbuff->info_buffers[b].fbid)
      ioctl(uvrbuff->info_buffers[b].kmsfd, DRM_IOCTL_MODE_RMFB, &uvrbuff->info_buffers[b].fbid);
    if (uvrbuff->info_buffers[b].bo)
      gbm_bo_destroy(uvrbuff->info_buffers[b].bo);
    for (unsigned p = 0; p < uvrbuff->info_buffers[b].num_planes; p++)  {
      if (uvrbuff->info_buffers[b].dma_buf_fds[p] != -1)
        close(uvrbuff->info_buffers[b].dma_buf_fds[p]);
    }
  }
  free(uvrbuff->info_buffers);
  if (uvrbuff->gbmdev)
    gbm_device_destroy(uvrbuff->gbmdev);
}
