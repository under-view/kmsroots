#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "wclient-common.h"

static uint8_t next_color(bool *up, uint8_t cur, unsigned int mod) {
  uint8_t next;

  next = cur + (*up ? 1 : -1) * (rand() % mod);
  if ((*up && next < cur) || (!*up && next > cur)) {
    *up = !*up;
    next = cur;
  }

  return next;
}


void render(int *cbuf, void *data) {
  struct uvr_wc *wc = data;

  unsigned int width = 3840, height = 2160, bytes_per_pixel = 4;
  unsigned int UNUSED stride = width * bytes_per_pixel;
  bool r_up = true, g_up = true, b_up = true;

  srand(time(NULL));
  uint8_t r = next_color(&r_up, rand() % 0xff, 20);
  uint8_t g = next_color(&g_up, rand() % 0xff, 10);
  uint8_t b = next_color(&b_up, rand() % 0xff, 5);

  for (unsigned int x = 0; x < width; x++)
    for (unsigned int y = 0; y < height; y++)
      *(uint32_t *) &wc->wcbuffs.uvrwcshmbufs[*cbuf].shm_pool_data[stride * x + y * bytes_per_pixel] = (r << 16) | (g << 8) | b;
}


/*
 * Example code demonstrating how to wayland shm buffers
 */
int main(void) {
  struct uvr_wc wc;
  struct uvr_wc_destory wcd;
  memset(&wcd, 0, sizeof(wcd));
  memset(&wc, 0, sizeof(wc));

  struct uvr_wc_core_interface_create_info wcinterfaces_info = {
    .wl_display_name = NULL,
    .iType = UVR_WC_WL_COMPOSITOR_INTERFACE | UVR_WC_XDG_WM_BASE_INTERFACE | UVR_WC_WL_SHM_INTERFACE | UVR_WC_WL_SEAT_INTERFACE
  };

  wc.wcinterfaces = uvr_wc_core_interface_create(&wcinterfaces_info);
  if (!wc.wcinterfaces.display || !wc.wcinterfaces.registry || !wc.wcinterfaces.compositor) goto exit_error;

  struct uvr_wc_buffer_create_info uvrwcbuff_info = {
    .uvrwccore = &wc.wcinterfaces, .buffer_count = 2,
    .width = 3840, .height = 2160, .bytes_per_pixel = 4,
    .wl_pix_format = WL_SHM_FORMAT_XRGB8888
  };

  wc.wcbuffs = uvr_wc_buffer_create(&uvrwcbuff_info);
  if (!wc.wcbuffs.uvrwcwlbufs || !wc.wcbuffs.uvrwcwlbufs)
    goto exit_error;

  static int cbuf = 0;
  struct uvr_wc_surface_create_info uvrwcsurf_info = {
    .uvrwccore = &wc.wcinterfaces,
    .uvrwcbuff = &wc.wcbuffs,
    .appname = "Example Window",
    .fullscreen = true,
    .renderer = &render,
    .rendererdata = &wc,
    .renderercbuf = &cbuf
  };

  wc.wcsurf = uvr_wc_surface_create(&uvrwcsurf_info);
  if (!wc.wcsurf.surface) goto exit_error;

  while (wl_display_dispatch(wc.wcinterfaces.display) != -1 && wc.wcsurf.running) {
    // Leave blank
  }

exit_error:
  wcd.uvr_wc_core_interface = wc.wcinterfaces;
  wcd.uvr_wc_buffer = wc.wcbuffs;
  wcd.uvr_wc_surface = wc.wcsurf;
  uvr_wc_destory(&wcd);
  return 0;
}
