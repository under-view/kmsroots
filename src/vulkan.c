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


VkInstance uvr_vk_instance_create(struct uvrvk_instance *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkInstance instance = VK_NULL_HANDLE;

  /* initialize the VkApplicationInfo structure */
  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pNext = NULL;
  app_info.pApplicationName = uvrvk->app_name;
  app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
  app_info.pEngineName = uvrvk->engine_name;
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
  create_info.pNext = (uvrvk->ppEnabledLayerNames) ? &features : NULL;
  create_info.flags = 0;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledLayerCount = uvrvk->enabledLayerCount;
  create_info.ppEnabledLayerNames = uvrvk->ppEnabledLayerNames;
  create_info.enabledExtensionCount = uvrvk->enabledExtensionCount;
  create_info.ppEnabledExtensionNames = uvrvk->ppEnabledExtensionNames;

  /* Create the instance */
  res = vkCreateInstance(&create_info, NULL, &instance);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_instance_create(vkCreateInstance): %s", vkres_msg(res));
    return VK_NULL_HANDLE;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_instance_create: VkInstance created retval(%p)", instance);

  return instance;
}


VkSurfaceKHR uvr_vk_surface_create(struct uvrvk_surface *uvrvk) {
  VkResult UNUSED res = VK_RESULT_MAX_ENUM;
  VkSurfaceKHR surface = VK_NULL_HANDLE;

  if (!uvrvk->vkinst) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_surface_create: VkInstance not instantiated");
    uvr_utils_log(UVR_DANGER, "[x] Make a call to uvr_vk_create_instance");
    return VK_NULL_HANDLE;
  }

  if (uvrvk->sType != WAYLAND_CLIENT_SURFACE && uvrvk->sType != XCB_CLIENT_SURFACE) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_surface_create: Must specify correct enum uvrvk_surface_type");
    return VK_NULL_HANDLE;
  }

#ifdef INCLUDE_WAYLAND
  if (uvrvk->sType == WAYLAND_CLIENT_SURFACE) {
    VkWaylandSurfaceCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.display = uvrvk->display;
    create_info.surface = uvrvk->surface;

    res = vkCreateWaylandSurfaceKHR(uvrvk->vkinst, &create_info, NULL, &surface);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_surfaceKHR(vkCreateWaylandSurfaceKHR): %s", vkres_msg(res));
      return VK_NULL_HANDLE;
    }
  }
#endif


#ifdef INCLUDE_XCB
  if (uvrvk->sType == XCB_CLIENT_SURFACE) {
    VkXcbSurfaceCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.connection = uvrvk->display;
    create_info.window = uvrvk->window;

    res = vkCreateXcbSurfaceKHR(uvrvk->vkinst, &create_info, NULL, &surface);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_surfaceKHR(vkCreateXcbSurfaceKHR): %s", vkres_msg(res));
      return VK_NULL_HANDLE;
    }
  }
#endif

#if defined(INCLUDE_WAYLAND) || defined(INCLUDE_XCB)
  uvr_utils_log(UVR_SUCCESS, "uvr_vk_create_surfaceKHR: VkSurfaceKHR created retval(%p)", surface);
#endif

  return surface;
}


VkPhysicalDevice uvr_vk_phdev_create(struct uvrvk_phdev *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkPhysicalDevice device = VK_NULL_HANDLE;
  uint32_t device_count = 0;
  VkPhysicalDevice *devices = NULL;

  if (!uvrvk->vkinst) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_phdev: VkInstance not instantiated");
    uvr_utils_log(UVR_DANGER, "[x] Make a call to uvr_vk_create_instance");
    return VK_NULL_HANDLE;
  }

  res = vkEnumeratePhysicalDevices(uvrvk->vkinst, &device_count, NULL);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_phdev(vkEnumeratePhysicalDevices): %s", vkres_msg(res));
    return VK_NULL_HANDLE;
  }

  if (device_count == 0) {
    uvr_utils_log(UVR_DANGER, "[x] failed to find GPU with Vulkan support!!!");
    return VK_NULL_HANDLE;
  }

  devices = (VkPhysicalDevice *) alloca(device_count * sizeof(VkPhysicalDevice));

  res = vkEnumeratePhysicalDevices(uvrvk->vkinst, &device_count, devices);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_phdev(vkEnumeratePhysicalDevices): %s", vkres_msg(res));
    return VK_NULL_HANDLE;
  }

#ifdef INCLUDE_KMS
  /* Get KMS fd stats */
  struct stat drm_stat = {0};
  if (uvrvk->drmfd != -1) {
    if (fstat(uvrvk->drmfd, &drm_stat) == -1) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_phdev(fstat): %s", strerror(errno));
      return VK_NULL_HANDLE;
    }
  }
#endif

  VkPhysicalDeviceProperties devprops;
  VkPhysicalDeviceFeatures devfeats;

  /*
   * get a physical device that is suitable
   * to do the graphics related task that we need
   */
  for (uint32_t i = 0; i < device_count; i++) {
    vkGetPhysicalDeviceProperties(devices[i], &devprops); /* Query device properties */
    vkGetPhysicalDeviceFeatures(devices[i], &devfeats); /* Query device features */
    if (devprops.deviceType == uvrvk->vkpdtype) {
      memmove(&device, &devices[i], sizeof(devices[i]));
      uvr_utils_log(UVR_SUCCESS, "Suitable GPU Found: %s", devprops.deviceName);
      break;
    }
  }

  return device;
}


void uvr_vk_destory(struct uvrvk_destroy *uvrvk) {
  if (uvrvk->vklgdev) {
    vkDeviceWaitIdle(uvrvk->vklgdev);
    vkDestroyDevice(uvrvk->vklgdev, NULL);
  }

#if defined(INCLUDE_WAYLAND) || defined(INCLUDE_XCB)
  if (uvrvk->vksurf)
    vkDestroySurfaceKHR(uvrvk->vkinst, uvrvk->vksurf, NULL);
#endif
  if (uvrvk->vkinst)
    vkDestroyInstance(uvrvk->vkinst, NULL);
}
