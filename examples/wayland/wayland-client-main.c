#include "vulkan-common.h"
#include "wclient.h"

/*
 * "VK_LAYER_KHRONOS_validation"
 * All of the useful standard validation is
 * bundled into a layer included in the SDK
 */
const char *validation_layers[] = { "VK_LAYER_KHRONOS_validation" };

const char *instance_extensions[] = {
  "VK_KHR_wayland_surface",
  "VK_KHR_surface",
  "VK_KHR_display",
  "VK_EXT_debug_utils"
};

const char *device_extensions[] = {
  "VK_KHR_swapchain"
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

  struct uvr_wc_destory wcd;
  memset(&wcd, 0, sizeof(wcd));

  struct uvr_wc_core_interface wcinterfaces;
  memset(&wcinterfaces, 0, sizeof(struct uvr_wc_core_interface));

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

  struct uvr_wc_core_interface_create_info wcinterfaces_info = {
    .wl_display_name = NULL,
    .iType = UVR_WC_WL_COMPOSITOR_INTERFACE | UVR_WC_XDG_WM_BASE_INTERFACE | UVR_WC_WL_SHM_INTERFACE | UVR_WC_WL_SEAT_INTERFACE
  };

  wcinterfaces = uvr_wc_core_interface_create(&wcinterfaces_info);
  if (!wcinterfaces.display || !wcinterfaces.registry || !wcinterfaces.compositor) goto exit_error;

  int width = 3840, height = 2160, bytes_per_pixel = 4;
  struct uvr_wc_buffer_create_info uvrwcbuff_info = {
    .uvrwccore = &wcinterfaces, .buffer_count = 2,
    .width = width, .height = height, .bytes_per_pixel = bytes_per_pixel,
    .wl_pix_format = WL_SHM_FORMAT_XRGB8888
  };

  struct uvr_wc_buffer uvrwc_buffs = uvr_wc_buffer_create(&uvrwcbuff_info);
  if (!uvrwc_buffs.buffers)
    goto exit_error;

  struct uvr_wc_surface_create_info uvrwcsurf_info = {
    .uvrwccore = &wcinterfaces,
    .uvrwcbuff = &uvrwc_buffs,
    .appname = "Example Window",
    .fullscreen = true
  };

  struct uvr_wc_surface uvrwc_surf;
  memset(&uvrwc_surf, 0, sizeof(uvrwc_surf));

  uvrwc_surf = uvr_wc_surface_create(&uvrwcsurf_info);
  if (!uvrwc_surf.surface) goto exit_error;

  /*
   * Create Vulkan Surface
   */
  struct uvr_vk_surface_create_info vksurf = {
    .vkinst = app.instance, .sType = WAYLAND_CLIENT_SURFACE,
    .display = wcinterfaces.display, .surface = uvrwc_surf.surface
  };

  app.surface = uvr_vk_surface_create(&vksurf);
  if (!app.surface)
    goto exit_error;

  /*
   * Create Vulkan Physical Device Handle, After Window Surface
   * so that it doesn't effect VkPhysicalDevice selection
   */
  struct uvr_vk_phdev_create_info vkphdev = {
    .vkinst = app.instance,
    .vkpdtype = VK_PHYSICAL_DEVICE_TYPE
  };

  app.phdev = uvr_vk_phdev_create(&vkphdev);
  if (!app.phdev)
    goto exit_error;


  struct uvr_vk_queue_create_info vkqueueinfo = { .phdev = app.phdev, .queueFlag = VK_QUEUE_GRAPHICS_BIT };
  struct uvr_vk_queue queue = uvr_vk_queue_create(&vkqueueinfo);
  if (queue.famindex == -1)
    goto exit_error;

  VkPhysicalDeviceFeatures phdevfeats = uvr_vk_get_phdev_features(app.phdev);
  struct uvr_vk_lgdev_create_info vklgdev_info = {
    .vkinst = app.instance, .phdev = app.phdev,
    .pEnabledFeatures = &phdevfeats,
    .enabledExtensionCount = ARRAY_LEN(device_extensions),
    .ppEnabledExtensionNames = device_extensions,
    .numqueues = 1,
    .queues = &queue
  };

  app.lgdev = uvr_vk_lgdev_create(&vklgdev_info);
  if (!app.lgdev.device)
    goto exit_error;

  int stride = width * bytes_per_pixel, calls = 2;
  bool r_up = true, g_up = true, b_up = true;

  srand(time(NULL));
  uint8_t r = next_color(&r_up, rand() % 0xff, 20);
  uint8_t g = next_color(&g_up, rand() % 0xff, 10);
  uint8_t b = next_color(&b_up, rand() % 0xff, 5);

  for (int x = 0; x < width; x++)
    for (int y = 0; y < height; y++)
      *(uint32_t *) &uvrwc_buffs.shm_pool_data[stride * x + y * bytes_per_pixel] = (r << 16) | (g << 8) | b;

  for (int call = 0; call < calls; call++)
    uvr_wc_process_events(&wcinterfaces);

  sleep(5);

exit_error:
  /*
   * Let the api know of what addresses to free and fd's to close
   */
  appd.vkinst = app.instance;
  appd.vksurf = app.surface;
  appd.vklgdevs_cnt = 1;
  appd.vklgdevs = &app.lgdev;
  uvr_vk_destory(&appd);

  wcd.wccinterface = wcinterfaces;
  wcd.wcbuff = uvrwc_buffs;
  wcd.wcsurface = uvrwc_surf;
  uvr_wc_destory(&wcd);
  return 0;
}
