#include "vulkan.h"
#include "wclient.h"

/*
 * "VK_LAYER_KHRONOS_validation"
 * All of the useful standard validation is
 * bundled into a layer included in the SDK
 */
const char *validation_layers[] = { };

const char *instance_extensions[] = {
  "VK_KHR_wayland_surface",
  "VK_KHR_surface"
};

struct appwhole {
  struct uvrvk app;
  struct uvrwc wclient;
};

static void appwhole_destory(struct appwhole *whole) {
  uvr_vk_destory(&whole->app);
  uvr_wclient_destory(&whole->wclient);
}

static uint8_t next_color(bool *up, uint8_t cur, unsigned int mod) {
  uint8_t next;

  next = cur + (*up ? 1 : -1) * (rand() % mod);
  if ((*up && next < cur) || (!*up && next > cur)) {
    *up = !*up;
    next = cur;
  }

  return next;
}

/*
 * Example code demonstrating how to connect Vulkan to X11
 */
int main(void) {
  struct appwhole whole;
  memset(&whole, 0, sizeof(struct appwhole));

  whole.wclient.display = uvr_wclient_display_connect(NULL);
  if (!whole.wclient.display) goto exit_error;

  if (uvr_wclient_alloc_interfaces(&whole.wclient) == -1)
    goto exit_error;

  int buffer_count = 2, width = 3840, height = 2160, bytes_per_pixel = 4;
  if (uvr_wclient_alloc_shm_buffers(&whole.wclient, buffer_count, width, height,
                                    bytes_per_pixel, WL_SHM_FORMAT_XRGB8888) == -1)
    goto exit_error;

  if (uvr_wclient_create_window(&whole.wclient, "Example Window", true))
    goto exit_error;

  if (uvr_vk_create_instance(&whole.app, "Example App", "No Engine",
                             ARRAY_LEN(validation_layers), validation_layers,
                             ARRAY_LEN(instance_extensions), instance_extensions) == -1)
    goto exit_error;

  if (uvr_vk_create_surfaceKHR(&whole.app, WAYLAND_CLIENT_SURFACE, "c:s",
                               whole.wclient.display, whole.wclient.surface) == -1)
    goto exit_error;

  if (uvr_vk_create_phdev(&whole.app, VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, NULL) == -1)
    goto exit_error;

  int cbuff = 0, stride = width * bytes_per_pixel;
  bool r_up = true, g_up = true, b_up = true;

  srand(time(NULL));
  uint8_t r = rand() % 0xff;
  uint8_t g = rand() % 0xff;
  uint8_t b = rand() % 0xff;

  while (uvr_wclient_process_events(&whole.wclient)) {
    r = next_color(&r_up, r, 20);
    g = next_color(&g_up, g, 10);
    b = next_color(&b_up, b, 5);

    cbuff = (cbuff + 1) % whole.wclient.buffer_count;

    for (int x = 0; x < width; x++)
      for (int y = 0; y < height; y++)
        *(uint32_t *) &whole.wclient.shm_pool_data[stride * x + y * bytes_per_pixel * (cbuff+1)] = (r << 16) | (g << 8) | b;
  }

  appwhole_destory(&whole);
  return 0;

exit_error:
  appwhole_destory(&whole);
  return 1;
}
