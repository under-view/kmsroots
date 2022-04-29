#include "vulkan.h"


/**
* Taken from lunarG vulkan API
* https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkResult.html
*/
static const char *vkres_msg(int err) {
  switch (err) {
    case VK_ERROR_OUT_OF_HOST_MEMORY:
      return "host memory allocation has failed";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
      return "device memory allocation has failed";
    case VK_ERROR_INITIALIZATION_FAILED:
      return "initialization of an object could not be completed for implementation-specific reasons";
    case VK_ERROR_DEVICE_LOST:
      return "the logical or physical device has been lost";
    case VK_ERROR_MEMORY_MAP_FAILED:
      return "mapping of a memory object has failed";
    case VK_ERROR_LAYER_NOT_PRESENT:
      return "a requested layer is not present or could not be loaded";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
      return "a requested extension is not supported";
    case VK_ERROR_FEATURE_NOT_PRESENT:
      return "a requested feature is not supported";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
      return "the requested version of Vulkan is not supported by the driver or is otherwise "  \
             "incompatible for implementation-specific reasons";
    case VK_ERROR_TOO_MANY_OBJECTS:
      return "too many objects of the type have already been created";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
      return "a requested format is not supported on this device";
    case VK_ERROR_FRAGMENTED_POOL:
      return "a pool allocation has failed due to fragmentation of the pool's memory";
    case VK_ERROR_SURFACE_LOST_KHR:
      return "a surface is no longer available";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
      return "the requested window is already in use by Vulkan or another API in a manner which " \
             "prevents it from being used again";
    case VK_ERROR_OUT_OF_DATE_KHR:
      return "a surface has changed in such a way that it is no longer compatible with the swapchain, " \
             "any further presentation requests using the swapchain will fail";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
      return "The display used by a swapchain does not use the same presentable image layout, or is " \
             "incompatible in a way that prevents sharing an image";
    case VK_ERROR_INVALID_SHADER_NV:
      return "one or more shaders failed to compile or link";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
      return "a pool memory allocation has failed";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
      return "an external handle is not a valid handle of the specified type";
    case VK_ERROR_FRAGMENTATION:
      return "a descriptor pool creation has failed due to fragmentation";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
      return "a buffer creation or memory allocation failed because the requested address is not available";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
      return "an operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as " \
             "it did not have exlusive full-screen access";
  }

  return "An unknown error has occurred. Either the application has provided invalid input, or an implementation failure has occurred";
}


int uvr_vk_create_instance(struct uvrvk *app,
                           const char *app_name,
                           const char *engine_name,
                           uint32_t enabledLayerCount,
                           const char **ppEnabledLayerNames,
                           uint32_t enabledExtensionCount,
                           const char **ppEnabledExtensionNames)
{

  VkResult res = VK_RESULT_MAX_ENUM;

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
  res = vkCreateInstance(&create_info, NULL, &app->instance);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_instance(vkCreateInstance): %s", vkres_msg(res));
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
   * Purposely marked as UNUSED as
   * INCLUDE_WAYLAND/XCB may not be defined
   */
  VkResult UNUSED res = VK_RESULT_MAX_ENUM;

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
      uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_surfaceKHR(vkCreateWaylandSurfaceKHR): %s", vkres_msg(res));
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
      uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_surfaceKHR(vkCreateXcbSurfaceKHR): %s", vkres_msg(res));
      return -1;
    }
  }
#endif

#if defined(INCLUDE_WAYLAND) || defined(INCLUDE_XCB)
  uvr_utils_log(UVR_SUCCESS, "uvr_vk_create_surfaceKHR: VkSurfaceKHR created retval(%p)", app->surface);
#endif

  return 0;
}


int uvr_vk_create_phdev(struct uvrvk *app, VkPhysicalDeviceType vkpdtype, char *fmt, ...) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkPhysicalDevice *devices = VK_NULL_HANDLE;
  uint32_t device_count = 0;

  if (!app->instance) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_phdev: VkInstance not instantiated");
    uvr_utils_log(UVR_DANGER, "[x] Make a call to uvr_vk_create_instance");
    return -1;
  }

  res = vkEnumeratePhysicalDevices(app->instance, &device_count, NULL);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_phdev(vkEnumeratePhysicalDevices): %s", vkres_msg(res));
    return -1;
  }

  if (device_count == 0) {
    uvr_utils_log(UVR_DANGER, "[x] failed to find GPU with Vulkan support!!!");
    return -1;
  }

  devices = (VkPhysicalDevice *) alloca(device_count * sizeof(VkPhysicalDevice));

  res = vkEnumeratePhysicalDevices(app->instance, &device_count, devices);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_phdev(vkEnumeratePhysicalDevices): %s", vkres_msg(res));
    return -1;
  }

  /* Get KMS fd stats */
  int drmfd=-1; va_list ap;
  if (fmt) {
    va_start(ap, fmt);
    drmfd = va_arg(ap, int);
    va_end(ap);
  }

  struct stat drm_stat = {0};
  if (drmfd != -1) {
    if (fstat(drmfd, &drm_stat) == -1) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_phdev(fstat): %s", strerror(errno));
      return -1;
    }
  }

  VkPhysicalDeviceProperties devprops;
  VkPhysicalDeviceFeatures devfeats;

  /*
   * get a physical device that is suitable
   * to do the graphics related task that we need
   */
  for (uint32_t i = 0; i < device_count; i++) {
    vkGetPhysicalDeviceProperties(devices[i], &devprops); /* Query device properties */
    vkGetPhysicalDeviceFeatures(devices[i], &devfeats); /* Query device features */
    if (devprops.deviceType == vkpdtype) {
      memmove(&app->phdev, &devices[i], sizeof(devices[i]));
      uvr_utils_log(UVR_SUCCESS, "Suitable GPU Found: %s", devprops.deviceName);
      break;
    }
  }

  return 0;
}


void uvr_vk_destory(struct uvrvk *app) {
  if (app->lgdev)
    vkDestroyDevice(app->lgdev, NULL);
#if defined(INCLUDE_WAYLAND) || defined(INCLUDE_XCB)
  if (app->surface)
    vkDestroySurfaceKHR(app->instance, app->surface, NULL);
#endif
  if (app->instance)
    vkDestroyInstance(app->instance, NULL);
}
