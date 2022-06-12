#include "vulkan-common.h"
#include "wclient-common.h"

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


/*
 * Example code demonstrating how use Vulkan with Wayland
 */
int main(void) {
  struct uvr_vk app;
  struct uvr_vk_destroy appd;
  memset(&app, 0, sizeof(app));
  memset(&appd, 0, sizeof(appd));

  struct uvr_wc wc;
  struct uvr_wc_destory wcd;
  memset(&wcd, 0, sizeof(wcd));
  memset(&wc, 0, sizeof(wc));


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
    .iType = UVR_WC_WL_COMPOSITOR_INTERFACE | UVR_WC_XDG_WM_BASE_INTERFACE | UVR_WC_WL_SEAT_INTERFACE
  };

  wc.wcinterfaces = uvr_wc_core_interface_create(&wcinterfaces_info);
  if (!wc.wcinterfaces.display || !wc.wcinterfaces.registry || !wc.wcinterfaces.compositor) goto exit_error;

  struct uvr_wc_surface_create_info uvrwcsurf_info = {
    .uvrwccore = &wc.wcinterfaces,
    .uvrwcbuff = NULL,
    .appname = "Example Window",
    .fullscreen = true
  };

  wc.wcsurf = uvr_wc_surface_create(&uvrwcsurf_info);
  if (!wc.wcsurf.surface) goto exit_error;

  /*
   * Create Vulkan Surface
   */
  struct uvr_vk_surface_create_info vksurf = {
    .vkinst = app.instance, .sType = WAYLAND_CLIENT_SURFACE,
    .display = wc.wcinterfaces.display, .surface = wc.wcsurf.surface
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
  app.graphics_queue = uvr_vk_queue_create(&vkqueueinfo);
  if (app.graphics_queue.famindex == -1)
    goto exit_error;

  VkPhysicalDeviceFeatures phdevfeats = uvr_vk_get_phdev_features(app.phdev);
  struct uvr_vk_lgdev_create_info vklgdev_info = {
    .vkinst = app.instance, .phdev = app.phdev,
    .pEnabledFeatures = &phdevfeats,
    .enabledExtensionCount = ARRAY_LEN(device_extensions),
    .ppEnabledExtensionNames = device_extensions,
    .numqueues = 1,
    .queues = &app.graphics_queue
  };

  app.lgdev = uvr_vk_lgdev_create(&vklgdev_info);
  if (!app.lgdev.device)
    goto exit_error;


  app.surfcap = uvr_vk_get_surface_capabilities(app.phdev, app.surface);
  app.formats = uvr_vk_get_surface_formats(app.phdev, app.surface);

  sleep(5);

exit_error:
  /*
   * Let the api know of what addresses to free and fd's to close
   */
  appd.vkinst = app.instance;
  appd.vksurf = app.surface;
  appd.vksurfformats = app.formats;
  appd.vklgdevs_cnt = 1;
  appd.vklgdevs = &app.lgdev;
  uvr_vk_destory(&appd);

  wcd.wccinterface = wc.wcinterfaces;
  wcd.wcsurface = wc.wcsurf;
  uvr_wc_destory(&wcd);
  return 0;
}
