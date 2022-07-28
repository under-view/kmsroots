#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "wclient.h"

#define WIDTH 1920
#define HEIGHT 1080


struct uvr_wc {
  struct uvr_wc_core_interface wcinterfaces;
  struct uvr_wc_surface wcsurf;
  struct uvr_wc_buffer wcbuffs;
};


/* https://github.com/dvdhrm/docs/blob/master/drm-howto/modeset-atomic.c#L825 */
static uint8_t next_color(bool *up, uint8_t cur, unsigned int mod) {
  uint8_t next;

  next = cur + (*up ? 1 : -1) * (rand() % mod);
  if ((*up && next < cur) || (!*up && next > cur)) {
    *up = !*up;
    next = cur;
  }

  return next;
}


void render(bool UNUSED *running, uint32_t *cbuf, void *data) {
  struct uvr_wc *wc = data;

  unsigned int bytes_per_pixel = 4, offset = 0;
  unsigned int stride = WIDTH * bytes_per_pixel;
  bool r_up = true, g_up = true, b_up = true;

  srand(time(NULL));
  uint8_t r = next_color(&r_up, rand() % 0xff, 20);
  uint8_t g = next_color(&g_up, rand() % 0xff, 10);
  uint8_t b = next_color(&b_up, rand() % 0xff, 5);

  for (unsigned int x = 0; x < WIDTH; x++) {
    for (unsigned int y = 0; y < HEIGHT; y++) {
      offset = (x * bytes_per_pixel) + (y * stride);
      if (wc->wcbuffs.uvrWcShmBuffers[*cbuf].shmPoolSize <= offset) break;
      *(uint32_t *) &wc->wcbuffs.uvrWcShmBuffers[*cbuf].shmPoolData[offset] = (r << 16) | (g << 8) | b;
    }
  }
}


/*
 * Example code demonstrating how to wayland shm buffers
 */
int main(void) {
  struct uvr_wc wc;
  struct uvr_wc_destroy wcd;
  memset(&wcd, 0, sizeof(wcd));
  memset(&wc, 0, sizeof(wc));

  struct uvr_wc_core_interface_create_info wc_interfaces_info;
  wc_interfaces_info.wlDisplayName = NULL;
  wc_interfaces_info.iType = UVR_WC_WL_COMPOSITOR_INTERFACE | UVR_WC_XDG_WM_BASE_INTERFACE |
                             UVR_WC_WL_SHM_INTERFACE        | UVR_WC_WL_SEAT_INTERFACE     |
                             UVR_WC_ZWP_FULLSCREEN_SHELL_V1;

  wc.wcinterfaces = uvr_wc_core_interface_create(&wc_interfaces_info);
  if (!wc.wcinterfaces.wlDisplay || !wc.wcinterfaces.wlRegistry || !wc.wcinterfaces.wlCompositor)
    goto exit_error;

  struct uvr_wc_buffer_create_info wc_buffer_info;
  wc_buffer_info.uvrWcCore = &wc.wcinterfaces;
  wc_buffer_info.bufferCount = 2;
  wc_buffer_info.width = WIDTH;
  wc_buffer_info.height = HEIGHT;
  wc_buffer_info.bytesPerPixel = 4;
  wc_buffer_info.wlBufferPixelFormat = WL_SHM_FORMAT_XRGB8888;

  wc.wcbuffs = uvr_wc_buffer_create(&wc_buffer_info);
  if (!wc.wcbuffs.uvrWcShmBuffers || !wc.wcbuffs.uvrWcWlBuffers)
    goto exit_error;

  static uint32_t cbuf = 0;
  static bool running = true;

  struct uvr_wc_surface_create_info wc_surface_info;
  wc_surface_info.uvrWcCore = &wc.wcinterfaces;
  wc_surface_info.uvrWcBuffer = &wc.wcbuffs;
  wc_surface_info.appName = "WL SHM Example Window";
  wc_surface_info.fullscreen = true;
  wc_surface_info.renderer = &render;
  wc_surface_info.rendererData = &wc;
  wc_surface_info.rendererCbuf = &cbuf;
  wc_surface_info.rendererRuning = &running;

  wc.wcsurf = uvr_wc_surface_create(&wc_surface_info);
  if (!wc.wcsurf.wlSurface)
    goto exit_error;

  while (wl_display_dispatch(wc.wcinterfaces.wlDisplay) != -1 && running) {
    // Leave blank
  }

exit_error:
  wcd.uvr_wc_core_interface = wc.wcinterfaces;
  wcd.uvr_wc_buffer = wc.wcbuffs;
  wcd.uvr_wc_surface = wc.wcsurf;
  uvr_wc_destroy(&wcd);
  return 0;
}
