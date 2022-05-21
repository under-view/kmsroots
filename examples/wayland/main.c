#include "vulkan-common.h"
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
 * Example code demonstrating how use Vulkan with X11
 */
int main(void) {
  struct uvr_vk app;
  struct uvr_vk_destroy appd;
  memset(&app, 0, sizeof(app));
  memset(&appd, 0, sizeof(appd));

  struct uvr_wc wclient;
  memset(&wclient, 0, sizeof(wclient));

  /*
   * Create Vulkan Instance
   */
  struct uvr_vk_instance_create_info vkinst = {
    .app_name = "Example App",
    .engine_name = "No Engine",
    .enabledLayerCount = ARRAY_LEN(validation_layers),
    .ppEnabledLayerNames = validation_layers,
    .enabledExtensionCount = ARRAY_LEN(instance_extensions),
    .ppEnabledExtensionNames = instance_extensions
  };

  app.instance = uvr_vk_instance_create(&vkinst);
  if (!app.instance)
    goto exit_error;

  /*
   * Create Vulkan Physical Device Handle
   */
  struct uvr_vk_phdev_create_info vkphdev = {
    .vkinst = app.instance,
    .vkpdtype = VK_PHYSICAL_DEVICE_TYPE
  };

  app.phdev = uvr_vk_phdev_create(&vkphdev);
  if (!app.phdev)
    goto exit_error;


  wclient.display = uvr_wc_display_connect(NULL);
  if (!wclient.display) goto exit_error;

  if (uvr_wc_alloc_interfaces(&wclient) == -1)
    goto exit_error;

  int buffer_count = 2, width = 3840, height = 2160, bytes_per_pixel = 4;
  if (uvr_wc_alloc_shm_buffers(&wclient, buffer_count, width, height,
                               bytes_per_pixel, WL_SHM_FORMAT_XRGB8888) == -1)
    goto exit_error;

  if (uvr_wc_window_create(&wclient, "Example Window", true))
    goto exit_error;


  /*
   * Create Vulkan Surface
   */
  struct uvr_vk_surface_create_info vksurf = {
    .vkinst = app.instance, .sType = WAYLAND_CLIENT_SURFACE,
    .display = wclient.display, .surface = wclient.surface
  };

  app.surface = uvr_vk_surface_create(&vksurf);
  if (!app.surface)
    goto exit_error;

  int stride = width * bytes_per_pixel;
  bool r_up = true, g_up = true, b_up = true;

  srand(time(NULL));
  uint8_t r = next_color(&r_up, rand() % 0xff, 20);
  uint8_t g = next_color(&g_up, rand() % 0xff, 10);
  uint8_t b = next_color(&b_up, rand() % 0xff, 5);

  for (int x = 0; x < width; x++)
    for (int y = 0; y < height; y++)
      *(uint32_t *) &wclient.shm_pool_data[stride * x + y * bytes_per_pixel] = (r << 16) | (g << 8) | b;

  while (uvr_wc_process_events(&wclient)) {
    // Leave blank
  }

exit_error:
  /*
   * Let the api know of what addresses to free and fd's to close
   */
  appd.vkinst = app.instance;
  appd.vksurf = app.surface;
  uvr_vk_destory(&appd);
  uvr_wc_destory(&wclient);
  return 0;
}
