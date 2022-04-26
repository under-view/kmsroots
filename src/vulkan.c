#include "vulkan.h"


int uvr_vk_create_instance(struct uvrvk *app,
                           const char *app_name,
                           const char *engine_name,
                           uint32_t enabledLayerCount,
                           const char **ppEnabledLayerNames,
                           uint32_t enabledExtensionCount,
                           const char **ppEnabledExtensionNames)
{

  /* initialize the VkApplicationInfo structure */
  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pNext = NULL;
  app_info.pApplicationName = app_name;
  app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
  app_info.pEngineName = engine_name;
  app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
  app_info.apiVersion = VK_MAKE_VERSION(1, 3, 0);

  /* enable validation layers best practice */
  VkValidationFeatureEnableEXT enables[] = {VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};
  VkValidationFeaturesEXT features = {};
  features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
  features.enabledValidationFeatureCount = ARRAY_LEN(enables);
  features.pEnabledValidationFeatures = enables;

  /*
   * Tells Vulkan which instance extensions
   * and validation layers we want to use
   */
  VkInstanceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pNext = (ppEnabledLayerNames) ? &features : NULL;
  create_info.flags = 0;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledLayerCount = enabledLayerCount;
  create_info.ppEnabledLayerNames = ppEnabledLayerNames;
  create_info.enabledExtensionCount = enabledExtensionCount;
  create_info.ppEnabledExtensionNames = ppEnabledExtensionNames;

  /* Create the instance */
  VkResult res = vkCreateInstance(&create_info, NULL, &app->instance);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_instance: vkCreateInstance Failed");
    return -1;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_create_instance: VkInstance created retval(%p)", app->instance);

  return 0;
}


int uvr_vk_create_surfaceKHR(struct uvrvk *app, enum uvrvk_surface_type st, char *fmt, ...){

  if (!app->instance) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_surfaceKHR: VkInstance not instantiated");
    uvr_utils_log(UVR_DANGER, "[x] Make a call to uvr_vk_create_instance");
    return -1;
  }

  if (st != WAYLAND_CLIENT_SURFACE && st != XCB_CLIENT_SURFACE) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_surfaceKHR: Must specify correct enum uvrvk_surface_type");
    return -1;
  }

  /*
   * Purposely marked as unused as INCLUDE_WAYLAND/INCLUDE_XCB
   * may not be defined
   */
  VkResult UNUSED res;

  struct platform_surface_info {
    void *surface;
    void *connection_to_display;
    unsigned int window;
  } UNUSED pinfo;

  va_list ap;
  va_start(ap, fmt);
  while (*fmt) {
    switch (*fmt++) {
      case 'c':
        pinfo.connection_to_display = (void *) va_arg(ap, void *);
        break;
      case 's':
        pinfo.surface = (void *) va_arg(ap, void *);
        break;
      case 'w':
        pinfo.window = (unsigned int) va_arg(ap, unsigned int);
        break;
      default: break;
    }
  }
  va_end(ap);

#ifdef INCLUDE_WAYLAND
  if (st == WAYLAND_CLIENT_SURFACE) {
    VkWaylandSurfaceCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.display = pinfo.connection_to_display;
    create_info.surface = pinfo.surface;

    res = vkCreateWaylandSurfaceKHR(app->instance, &create_info, NULL, &app->surface);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_surfaceKHR: vkCreateWaylandSurfaceKHR Failed");
      return -1;
    }
  }
#endif


#ifdef INCLUDE_XCB
  if (st == XCB_CLIENT_SURFACE) {
    VkXcbSurfaceCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.connection = pinfo.connection_to_display;
    create_info.window = pinfo.window;

    res = vkCreateXcbSurfaceKHR(app->instance, &create_info, NULL, &app->surface);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_surfaceKHR: vkCreateXcbSurfaceKHR Failed");
      return -1;
    }
  }
#endif

#if defined(INCLUDE_WAYLAND) || defined(INCLUDE_XCB)
  uvr_utils_log(UVR_SUCCESS, "uvr_vk_create_surfaceKHR: VkSurfaceKHR created retval(%p)", app->surface);
#endif

  return 0;
}


void uvr_vk_destory(struct uvrvk *app) {
#if defined(INCLUDE_WAYLAND) || defined(INCLUDE_XCB)
  if (app->surface)
    vkDestroySurfaceKHR(app->instance, app->surface, NULL);
#endif
  if (app->instance)
    vkDestroyInstance(app->instance, NULL);
}
