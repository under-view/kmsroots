#include "buffer.h"


struct uvrbuff uvr_buffer_create(struct uvrbuff_create_info *uvrbuff) {
  struct gbm_device *gbmdev = NULL;
  struct uvrbuff_object_info *bois = NULL;

  if (uvrbuff->bType == GBM_BUFFER || uvrbuff->bType == GBM_BUFFER_WITH_MODIFIERS) {
    gbmdev = gbm_create_device(uvrbuff->kmsfd);
    if (!gbmdev) goto exit_uvrbuff_null_struct;

    bois = calloc(uvrbuff->buff_cnt, sizeof(struct uvrbuff_object_info));
    if (!bois) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_buffer_create(calloc): failed to allocate space for struct uvrbuff_object_info *");
      goto exit_uvrbuff_gbmdev_destroy;
    }
  }

  if (uvrbuff->bType == GBM_BUFFER) {
    for (unsigned b = 0; b < uvrbuff->buff_cnt; b++) {
      bois[b].bo = gbm_bo_create(gbmdev, uvrbuff->width, uvrbuff->height, uvrbuff->gbm_bo_pixformat, uvrbuff->gbm_bo_flags);
      if (!bois[b].bo) {
        uvr_utils_log(UVR_DANGER, "[x] uvr_buffer_create(gbm_bo_create): failed to create gbm_bo with res %u x %u", uvrbuff->width, uvrbuff->height);
        goto exit_uvrbuff_gbm_bo_detroy;
      }
    }
  }

  if (uvrbuff->bType == GBM_BUFFER_WITH_MODIFIERS) {
    for (unsigned b = 0; b < uvrbuff->buff_cnt; b++) {
      bois[b].bo = gbm_bo_create_with_modifiers(gbmdev, uvrbuff->width, uvrbuff->height, uvrbuff->gbm_bo_pixformat, uvrbuff->modifiers, uvrbuff->modifiers_cnt);
      if (!bois[b].bo) {
        uvr_utils_log(UVR_DANGER, "[x] uvr_buffer_create(gbm_bo_create_with_modifiers): failed to create gbm_bo with res %u x %u", uvrbuff->width, uvrbuff->height);
        goto exit_uvrbuff_gbm_bo_detroy;
      }
    }
  }

  for (unsigned b = 0; b < uvrbuff->buff_cnt; b++) {
    bois[b].num_planes = gbm_bo_get_plane_count(bois[b].bo);
    bois[b].modifier = gbm_bo_get_modifier(bois[b].bo);
    bois[b].format = uvrbuff->gbm_bo_pixformat;

    for (unsigned p = 0; p < bois[b].num_planes; p++) {
      union gbm_bo_handle h;
      memset(&h,0,sizeof(h));

      h = gbm_bo_get_handle_for_plane(bois[b].bo, p);
      if (!h.u32 || h.s32 == -1) {
        uvr_utils_log(UVR_DANGER, "[x] failed to get BO plane %d gem handle (modifier 0x%" PRIx64 ")", p, bois[b].modifier);
        goto exit_uvrbuff_gbm_bo_detroy;
      }

      bois[b].gem_handles[p] = h.u32;

      bois[b].pitches[p] = gbm_bo_get_stride_for_plane(bois[b].bo, p);
      if (!bois[b].pitches[p]) {
        uvr_utils_log(UVR_DANGER, "[x] failed to get stride/pitch for BO plane %d (modifier 0x%" PRIx64 ")", p, bois[b].modifier);
        goto exit_uvrbuff_gbm_bo_detroy;
      }

      bois[b].offsets[p] = gbm_bo_get_offset(bois[b].bo, p);

      struct drm_prime_handle prime_request = {
        .handle = bois[b].gem_handles[p],
        .flags  = DRM_RDWR,
        .fd     = -1
      };

      /* Retrieve a DMA-BUF fd (PRIME fd) from the GEM handle/name to pass along to other processes */
      if (ioctl(uvrbuff->kmsfd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_request) == -1)  {
        uvr_utils_log(UVR_DANGER, "[x] ioctl: %s", strerror(errno));
        goto exit_uvrbuff_gbm_bo_detroy;
      }

      bois[b].dma_buf_fds[p] = prime_request.fd;
    }

    /* Create actual framebuffer */
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

      if (ioctl(uvrbuff->kmsfd, DRM_IOCTL_MODE_ADDFB, &f) == -1) {
        uvr_utils_log(UVR_DANGER, "[x] ioctl(DRM_IOCTL_MODE_ADDFB): %s", strerror(errno));
        goto exit_uvrbuff_gbm_bo_detroy;
      }

      bois[b].fbid = f.fb_id;
    }

    if (uvrbuff->bType == GBM_BUFFER_WITH_MODIFIERS) {
      struct drm_mode_fb_cmd2 f;
      memset(&f,0,sizeof(struct drm_mode_fb_cmd2));

      f.width  = uvrbuff->width;
      f.height = uvrbuff->height;
      f.pixel_format = uvrbuff->gbm_bo_pixformat;
      f.flags = 0;

      memcpy(f.handles, bois[b].gem_handles, 4 * sizeof(unsigned));
      memcpy(f.pitches, bois[b].pitches, 4 * sizeof(unsigned));
      memcpy(f.offsets, bois[b].offsets, 4 * sizeof(unsigned));
      memcpy(f.modifier, uvrbuff->modifiers, uvrbuff->modifiers_cnt * sizeof(unsigned));
      bois[b].fbid = 0;

      if (ioctl(uvrbuff->kmsfd, DRM_IOCTL_MODE_ADDFB2, &f) == -1) {
        uvr_utils_log(UVR_DANGER, "[x] ioctl(DRM_IOCTL_MODE_ADDFB2): %s", strerror(errno));
        goto exit_uvrbuff_gbm_bo_detroy;
      }

      bois[b].fbid = f.fb_id;
    }
  }

  return (struct uvrbuff) { .gbmdev = gbmdev, .info_buffers = bois };

exit_uvrbuff_gbm_bo_detroy:
  for (unsigned b = 0; b < uvrbuff->buff_cnt; b++)
    if (bois[b].bo)
      gbm_bo_destroy(bois[b].bo);
  free(bois);
exit_uvrbuff_gbmdev_destroy:
  if (gbmdev)
    gbm_device_destroy(gbmdev);
exit_uvrbuff_null_struct:
  return (struct uvrbuff) { .gbmdev = NULL, .info_buffers = NULL };
}


void uvr_buffer_destory(struct uvrbuff_destroy *uvrbuff) {
  for (unsigned b = 0; b < uvrbuff->buff_cnt; b++) {
    if (uvrbuff->info_buffers[b].bo)
      gbm_bo_destroy(uvrbuff->info_buffers[b].bo);
  }
  free(uvrbuff->info_buffers);
  if (uvrbuff->gbmdev)
    gbm_device_destroy(uvrbuff->gbmdev);
}
