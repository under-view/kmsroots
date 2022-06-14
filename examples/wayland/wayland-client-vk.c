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
  app.sformats = uvr_vk_get_surface_formats(app.phdev, app.surface);
  app.spmodes = uvr_vk_get_surface_present_modes(app.phdev, app.surface);

  /* choose swap chain surface format & present mode */
  VkSurfaceFormatKHR sformat;
  VkPresentModeKHR presmode;
  VkExtent2D extent2D = {3840, 2160};

  for (uint32_t s = 0; s < app.sformats.fcount; s++) {
    if (app.sformats.formats[s].format == VK_FORMAT_B8G8R8A8_SRGB && app.sformats.formats[s].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      sformat = app.sformats.formats[s];
    }
  }

  for (uint32_t p = 0; p < app.spmodes.mcount; p++) {
    if (app.spmodes.modes[p] == VK_PRESENT_MODE_MAILBOX_KHR) {
      presmode = app.spmodes.modes[p];
    }
  }

  free(app.sformats.formats);
  free(app.spmodes.modes);

  if (app.surfcap.currentExtent.width != UINT32_MAX) {
    extent2D = app.surfcap.currentExtent;
  } else {
    extent2D.width = fmax(app.surfcap.minImageExtent.width, fmin(app.surfcap.maxImageExtent.width, extent2D.width));
    extent2D.height = fmax(app.surfcap.minImageExtent.height, fmin(app.surfcap.maxImageExtent.height, extent2D.height));
  }

  struct uvr_vk_swapchain_create_info sc_create_info = {
    .lgdev = app.lgdev.device, .surfaceKHR = app.surface, .surfcap = app.surfcap, .surfaceFormat = sformat,
    .extent2D = extent2D, .imageArrayLayers = 1, .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE, .queueFamilyIndexCount = 0, .pQueueFamilyIndices = NULL,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, .presentMode = presmode, .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE
  };

  app.schain = uvr_vk_swapchain_create(&sc_create_info);
  if (!app.schain.swapchain)
    goto exit_error;

exit_error:
  /*
   * Let the api know of what addresses to free and fd's to close
   */
  appd.vkinst = app.instance;
  appd.vksurf = app.surface;
  appd.vklgdevs_cnt = 1;
  appd.vklgdevs = &app.lgdev;
  appd.vkswapchain_cnt = 1;
  appd.vkswapchains = &app.schain;
  uvr_vk_destory(&appd);

  wcd.wccinterface = wc.wcinterfaces;
  wcd.wcsurface = wc.wcsurf;
  uvr_wc_destory(&wcd);
  return 0;
}
