#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include "vulkan.h"


/*
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
    case VK_ERROR_OUT_OF_POOL_MEMORY:
      return "a pool memory allocation has failed";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
      return "an external handle is not a valid handle of the specified type";
    case VK_ERROR_FRAGMENTATION:
      return "a descriptor pool creation has failed due to fragmentation";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
      return "a buffer creation or memory allocation failed because the requested address is not available";
    case VK_PIPELINE_COMPILE_REQUIRED:
      return "A requested pipeline creation would have required compilation, but the application requested" \
             "compilation to not be performed.";
    case VK_ERROR_SURFACE_LOST_KHR:
      return "a surface is no longer available";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
      return "the requested window is already in use by Vulkan or another API in a manner which " \
             "prevents it from being used again";
    case VK_SUBOPTIMAL_KHR:
      return "A swapchain no longer matches the surface properties exactly, but can still be used to present" \
             "to the surface successfully.";
    case VK_ERROR_OUT_OF_DATE_KHR:
      return "a surface has changed in such a way that it is no longer compatible with the swapchain, " \
             "any further presentation requests using the swapchain will fail";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
      return "The display used by a swapchain does not use the same presentable image layout, or is " \
             "incompatible in a way that prevents sharing an image";
    case VK_ERROR_VALIDATION_FAILED_EXT:
      return "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV:
      return "one or more shaders failed to compile or link";
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
      return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    case VK_ERROR_NOT_PERMITTED_KHR:
      return "VK_ERROR_NOT_PERMITTED_KHR";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
      return "an operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as " \
             "it did not have exlusive full-screen access";
    case VK_THREAD_IDLE_KHR:
      return "A deferred operation is not complete but there is currently no work for this thread to do at the time of this call.";
    case VK_THREAD_DONE_KHR:
      return "A deferred operation is not complete but there is no work remaining to assign to additional threads.";
    case VK_OPERATION_DEFERRED_KHR:
      return "A deferred operation was requested and at least some of the work was deferred.";
    case VK_OPERATION_NOT_DEFERRED_KHR:
      return "A deferred operation was requested and no operations were deferred.";
    case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
      return "An image creation failed because internal resources required for compression are exhausted." \
             "This must only be returned when fixed-rate compression is requested.";
  }

  // VK_ERROR_UNKNOWN
  return "An unknown error has occurred. Either the application has provided invalid input, or an implementation failure has occurred";
}


VkInstance uvr_vk_instance_create(struct uvr_vk_instance_create_info *uvrvk) {
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


VkSurfaceKHR uvr_vk_surface_create(struct uvr_vk_surface_create_info *uvrvk) {
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


VkPhysicalDevice uvr_vk_phdev_create(struct uvr_vk_phdev_create_info *uvrvk) {
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

  devices = (VkPhysicalDevice *) alloca((device_count * sizeof(VkPhysicalDevice)) + 1);

  res = vkEnumeratePhysicalDevices(uvrvk->vkinst, &device_count, devices);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_phdev(vkEnumeratePhysicalDevices): %s", vkres_msg(res));
    return VK_NULL_HANDLE;
  }

#ifdef INCLUDE_KMS
  /* Get KMS fd stats */
  struct stat drm_stat = {0};
  if (uvrvk->kmsfd != -1) {
    if (fstat(uvrvk->kmsfd, &drm_stat) == -1) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_phdev(fstat): %s", strerror(errno));
      return VK_NULL_HANDLE;
    }
  }

  VkPhysicalDeviceProperties2 devprops2;
  VkPhysicalDeviceDrmPropertiesEXT drm_props;
  memset(&devprops2, 0, sizeof(devprops2));
  memset(&drm_props, 0, sizeof(drm_props));

  drm_props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT;
  devprops2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
#endif

  int enter = 0;
  VkPhysicalDeviceFeatures devfeats;
  VkPhysicalDeviceProperties devprops;

  /*
   * get a physical device that is suitable
   * to do the graphics related task that we need
   */
  for (uint32_t i = 0; i < device_count; i++) {
    vkGetPhysicalDeviceFeatures(devices[i], &devfeats); /* Query device features */
    vkGetPhysicalDeviceProperties(devices[i], &devprops); /* Query device properties */

#ifdef INCLUDE_KMS
    if (uvrvk->kmsfd) {
      devprops2.pNext = &drm_props;
      vkGetPhysicalDeviceProperties2(devices[i], &devprops2);

      drm_props.pNext = devprops2.pNext;
      dev_t primary_devid = makedev(drm_props.primaryMajor, drm_props.primaryMinor);
      dev_t render_devid = makedev(drm_props.renderMajor, drm_props.renderMinor);

      /*
       * Enter will be one if condition succeeds
       * Enter will be zero if condition fails
       * Need to make sure even if we have a valid DRI device node fd.
       * That the customer choosen device type is the same as the DRI device node.
       */
      enter |= ((primary_devid == drm_stat.st_rdev || render_devid == drm_stat.st_rdev) && devprops.deviceType == uvrvk->vkpdtype);
    }
#endif

    /* Enter will be one if condition succeeds */
    enter |= (devprops.deviceType == uvrvk->vkpdtype);
    if (enter) {
      memmove(&device, &devices[i], sizeof(devices[i]));
      uvr_utils_log(UVR_SUCCESS, "Suitable GPU Found: %s, api version: %u", devprops.deviceName, devprops.apiVersion);
      break;
    }
  }

  if (device == VK_NULL_HANDLE)
    uvr_utils_log(UVR_DANGER, "Suitable GPU not found!");

  return device;
}


VkPhysicalDeviceFeatures uvr_vk_get_phdev_features(VkPhysicalDevice phdev) {
  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(phdev, &features);
  return features;
}


struct uvr_vk_queue uvr_vk_queue_create(struct uvr_vk_queue_create_info *uvrvk) {
  uint32_t queue_count = 0, flagcnt = 0;
  VkQueueFamilyProperties *queue_families = NULL;
  struct uvr_vk_queue queue;

  flagcnt += (uvrvk->queueFlag & VK_QUEUE_GRAPHICS_BIT) ? 1 : 0;
  flagcnt += (uvrvk->queueFlag & VK_QUEUE_COMPUTE_BIT) ? 1 : 0;
  flagcnt += (uvrvk->queueFlag & VK_QUEUE_TRANSFER_BIT) ? 1 : 0;
  flagcnt += (uvrvk->queueFlag & VK_QUEUE_SPARSE_BINDING_BIT) ? 1 : 0;
  flagcnt += (uvrvk->queueFlag & VK_QUEUE_PROTECTED_BIT) ? 1 : 0;

  if (flagcnt != 1) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_queue_create: Multiple VkQueueFlags specified, only one allowed per queue");
    goto err_vk_queue_create;
  }

  /*
   * Almost every operation in Vulkan, requires commands to be submitted to a queue
   * Find queue family index for a given queue
   */
  vkGetPhysicalDeviceQueueFamilyProperties(uvrvk->phdev, &queue_count, NULL);
  queue_families = (VkQueueFamilyProperties *) alloca(queue_count * sizeof(VkQueueFamilyProperties));
  vkGetPhysicalDeviceQueueFamilyProperties(uvrvk->phdev, &queue_count, queue_families);

  for (uint32_t i = 0; i < queue_count; i++) {
    queue.queueCount = queue_families[i].queueCount;
    if (queue_families[i].queueFlags & uvrvk->queueFlag & VK_QUEUE_GRAPHICS_BIT) {
      strncpy(queue.name, "graphics", sizeof(queue.name));
      queue.famindex = i; break;
    }

    if (queue_families[i].queueFlags & uvrvk->queueFlag & VK_QUEUE_COMPUTE_BIT) {
      strncpy(queue.name, "compute", sizeof(queue.name));
      queue.famindex = i; break;
    }

    if (queue_families[i].queueFlags & uvrvk->queueFlag & VK_QUEUE_TRANSFER_BIT) {
      strncpy(queue.name, "transfer", sizeof(queue.name));
      queue.famindex = i; break;
    }

    if (queue_families[i].queueFlags & uvrvk->queueFlag & VK_QUEUE_SPARSE_BINDING_BIT) {
      strncpy(queue.name, "sparse_binding", sizeof(queue.name));
      queue.famindex = i; break;
    }

    if (queue_families[i].queueFlags & uvrvk->queueFlag & VK_QUEUE_PROTECTED_BIT) {
      strncpy(queue.name, "protected", sizeof(queue.name));
      queue.famindex = i; break;
    }
  }

  return queue;

err_vk_queue_create:
  return (struct uvr_vk_queue) { .name[0] = '\0', .queue = VK_NULL_HANDLE, .famindex = -1, .queueCount = -1 };
}


struct uvr_vk_lgdev uvr_vk_lgdev_create(struct uvr_vk_lgdev_create_info *uvrvk) {
  VkDevice device = VK_NULL_HANDLE;
  VkResult res = VK_RESULT_MAX_ENUM;

  VkDeviceQueueCreateInfo *pQueueCreateInfo = (VkDeviceQueueCreateInfo *) calloc(uvrvk->numqueues, sizeof(VkDeviceQueueCreateInfo));
  if (!pQueueCreateInfo) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_lgdev_create(calloc): %s", strerror(errno));
    goto err_vk_lgdev_create;
  }

  /*
   * https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/vkspec.html#devsandqueues-priority
   * set the default priority of all queues to be the highest
   */
  const float pQueuePriorities = 1.f;
  for (uint32_t qc = 0; qc < uvrvk->numqueues; qc++) {
    pQueueCreateInfo[qc].flags = 0;
    pQueueCreateInfo[qc].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    pQueueCreateInfo[qc].queueFamilyIndex = uvrvk->queues[qc].famindex;
    pQueueCreateInfo[qc].queueCount = uvrvk->queues[qc].queueCount;
    pQueueCreateInfo[qc].pQueuePriorities = &pQueuePriorities;
  }

  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.queueCreateInfoCount = uvrvk->numqueues;
  create_info.pQueueCreateInfos = pQueueCreateInfo;
  create_info.enabledLayerCount = 0; // Deprecated and ignored
  create_info.ppEnabledLayerNames = NULL; // Deprecated and ignored
  create_info.enabledExtensionCount = uvrvk->enabledExtensionCount;
  create_info.ppEnabledExtensionNames = uvrvk->ppEnabledExtensionNames;
  create_info.pEnabledFeatures = uvrvk->pEnabledFeatures;

  /* Create logic device */
  res = vkCreateDevice(uvrvk->phdev, &create_info, NULL, &device);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_lgdev_create(vkCreateDevice): %s", vkres_msg(res));
    goto err_vk_lgdev_free_pQueueCreateInfo;
  }

  for (uint32_t qc = 0; qc < uvrvk->numqueues; qc++) {
    vkGetDeviceQueue(device, uvrvk->queues[qc].famindex, 0, &uvrvk->queues[qc].queue);
    if (!uvrvk->queues[qc].queue)  {
      uvr_utils_log(UVR_DANGER, "[x] uvr_vk_lgdev_create(vkGetDeviceQueue): Failed to get %s queue handle", uvrvk->queues[qc].name);
      goto err_vk_lgdev_destroy;
    }

    uvr_utils_log(UVR_SUCCESS, "uvr_vk_lgdev_create: '%s' VkQueue successfully created retval(%p)", uvrvk->queues[qc].name, uvrvk->queues[qc].queue);
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_lgdev_create: VkDevice created retval(%p)", device);

  free(pQueueCreateInfo);
  return (struct uvr_vk_lgdev) { .device = device, .queue_cnt = uvrvk->numqueues, .queues = uvrvk->queues };

err_vk_lgdev_destroy:
  if (device)
    vkDestroyDevice(device, NULL);
err_vk_lgdev_free_pQueueCreateInfo:
  free(pQueueCreateInfo);
err_vk_lgdev_create:
  return (struct uvr_vk_lgdev) { .device = VK_NULL_HANDLE, .queue_cnt = -1, .queues = NULL };
}


VkSurfaceCapabilitiesKHR uvr_vk_get_surface_capabilities(VkPhysicalDevice phdev, VkSurfaceKHR surface) {
  VkSurfaceCapabilitiesKHR cap;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phdev, surface, &cap);
  return cap;
}


struct uvr_vk_surface_format uvr_vk_get_surface_formats(VkPhysicalDevice phdev, VkSurfaceKHR surface) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkSurfaceFormatKHR *formats = NULL;
  uint32_t fcount = 0;

  res = vkGetPhysicalDeviceSurfaceFormatsKHR(phdev, surface, &fcount, NULL);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_get_surface_formats(vkGetPhysicalDeviceSurfaceFormatsKHR): %s", vkres_msg(res));
    goto exit_vk_surface_formats;
  }

  formats = (VkSurfaceFormatKHR *) calloc(fcount, sizeof(formats));
  if (!formats) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_get_surface_formats(calloc): %s", strerror(errno));
    goto exit_vk_surface_formats;
  }

  res = vkGetPhysicalDeviceSurfaceFormatsKHR(phdev, surface, &fcount, formats);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_get_surface_formats(vkGetPhysicalDeviceSurfaceFormatsKHR): %s", vkres_msg(res));
    goto exit_vk_surface_formats_free;
  }

  return (struct uvr_vk_surface_format) { .fcount = fcount, .formats = formats };

exit_vk_surface_formats_free:
  free(formats);
exit_vk_surface_formats:
  return (struct uvr_vk_surface_format) { .fcount = 0, .formats = NULL };
}


struct uvr_vk_surface_present_mode uvr_vk_get_surface_present_modes(VkPhysicalDevice phdev, VkSurfaceKHR surface) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkPresentModeKHR *modes = NULL;
  uint32_t mcount = 0;

  res = vkGetPhysicalDeviceSurfacePresentModesKHR(phdev, surface, &mcount, NULL);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_get_surface_present_modes(vkGetPhysicalDeviceSurfacePresentModesKHR): %s", vkres_msg(res));
    goto exit_vk_surface_present_modes;
  }

  modes = (VkPresentModeKHR *) calloc(mcount, sizeof(modes));
  if (!modes) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_get_surface_formats(calloc): %s", strerror(errno));
    goto exit_vk_surface_present_modes;
  }

  res = vkGetPhysicalDeviceSurfacePresentModesKHR(phdev, surface, &mcount, modes);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_get_surface_present_modes(vkGetPhysicalDeviceSurfacePresentModesKHR): %s", vkres_msg(res));
    goto exit_vk_surface_present_modes_free;
  }

  return (struct uvr_vk_surface_present_mode) { .mcount = mcount, .modes = modes };

exit_vk_surface_present_modes_free:
  free(modes);
exit_vk_surface_present_modes:
  return (struct uvr_vk_surface_present_mode) { .mcount = 0, .modes = NULL };
}


struct uvr_vk_swapchain uvr_vk_swapchain_create(struct uvr_vk_swapchain_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;

  if (uvrvk->surfcap.currentExtent.width != UINT32_MAX) {
    uvrvk->extent2D = uvrvk->surfcap.currentExtent;
  } else {
    uvrvk->extent2D.width = fmax(uvrvk->surfcap.minImageExtent.width, fmin(uvrvk->surfcap.maxImageExtent.width, uvrvk->extent2D.width));
    uvrvk->extent2D.height = fmax(uvrvk->surfcap.minImageExtent.height, fmin(uvrvk->surfcap.maxImageExtent.height, uvrvk->extent2D.height));
  }

  VkSwapchainCreateInfoKHR create_info;
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.surface = uvrvk->surfaceKHR;
  if (uvrvk->surfcap.maxImageCount > 0 && (uvrvk->surfcap.minImageCount + 1) < uvrvk->surfcap.maxImageCount)
    create_info.minImageCount = uvrvk->surfcap.maxImageCount;
  else
    create_info.minImageCount = uvrvk->surfcap.minImageCount + 1;
  create_info.imageFormat = uvrvk->surfaceFormat.format;
  create_info.imageColorSpace = uvrvk->surfaceFormat.colorSpace;
  create_info.imageExtent = uvrvk->extent2D;
  create_info.imageArrayLayers = uvrvk->imageArrayLayers;
  create_info.imageUsage = uvrvk->imageUsage;
  create_info.imageSharingMode = uvrvk->imageSharingMode;
  create_info.queueFamilyIndexCount = uvrvk->queueFamilyIndexCount;
  create_info.pQueueFamilyIndices = uvrvk->pQueueFamilyIndices;
  create_info.preTransform = uvrvk->surfcap.currentTransform;
  create_info.compositeAlpha = uvrvk->compositeAlpha;
  create_info.presentMode = uvrvk->presentMode;
  create_info.clipped = uvrvk->clipped;
  create_info.oldSwapchain = uvrvk->oldSwapchain;

  res = vkCreateSwapchainKHR(uvrvk->lgdev, &create_info, NULL, &swapchain);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_swapchain_create(vkCreateSwapchainKHR): %s", vkres_msg(res));
    goto exit_vk_swapchain;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_swapchain_create: VkSwapchainKHR successfully created retval(%p)", swapchain);

  return (struct uvr_vk_swapchain) { .lgdev = uvrvk->lgdev, .swapchain = swapchain };

exit_vk_swapchain:
  return (struct uvr_vk_swapchain) { .lgdev = VK_NULL_HANDLE, .swapchain = VK_NULL_HANDLE };
}


struct uvr_vk_image uvr_vk_image_create(struct uvr_vk_image_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  struct uvrvkimage *images = NULL;
  struct uvrvkview *views = NULL;

  uint32_t icount = 0, i;
  VkImage *vkimages = NULL;

  res = vkGetSwapchainImagesKHR(uvrvk->lgdev, uvrvk->swapchain, &icount, NULL);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_image_create(vkGetSwapchainImagesKHR): %s", vkres_msg(res));
    goto exit_vk_image;
  }

  vkimages = alloca(icount * sizeof(VkImage));

  res = vkGetSwapchainImagesKHR(uvrvk->lgdev, uvrvk->swapchain, &icount, vkimages);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_image_create(vkGetSwapchainImagesKHR): %s", vkres_msg(res));
    goto exit_vk_image;
  }

  uvr_utils_log(UVR_INFO, "uvr_vk_image_create: Total images in swapchain %u", icount);

  images = calloc(icount, sizeof(struct uvrvkimage));
  if (!images) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_image_create(calloc): %s", strerror(errno));
    goto exit_vk_image;
  }

  views = calloc(icount, sizeof(struct uvrvkview));
  if (!views) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_image_create(calloc): %s", strerror(errno));
    goto exit_vk_image_free_images;
  }

  VkImageViewCreateInfo create_info;
  create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = uvrvk->flags;
  create_info.viewType = uvrvk->viewType;
  create_info.format = uvrvk->format;
  create_info.components = uvrvk->components;
  create_info.subresourceRange = uvrvk->subresourceRange;

  for (i = 0; i < icount; i++) {
    create_info.image = images[i].image = vkimages[i];

    res = vkCreateImageView(uvrvk->lgdev, &create_info, NULL, &views[i].view);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_vk_image_create(vkCreateImageView): %s", vkres_msg(res));
      goto exit_vk_image_free_image_view;
    }

    uvr_utils_log(UVR_WARNING, "uvr_vk_image_create: VkImage (%p) associate with VkImageView", images[i].image);
    uvr_utils_log(UVR_SUCCESS, "uvr_vk_image_create: VkImageView successfully created retval(%p)", views[i].view);
  }

  return (struct uvr_vk_image) { .lgdev = uvrvk->lgdev, .icount = icount, .images = images, .vcount = icount, .views = views, .swapchain = uvrvk->swapchain };


exit_vk_image_free_image_view:
  if (views) {
    for (i = 0; i < icount; i++) {
      if (views[i].view)
        vkDestroyImageView(uvrvk->lgdev, views[i].view, NULL);
    }
  }
//exit_vk_image_free_image_views:
  free(views);
exit_vk_image_free_images:
  free(images);
exit_vk_image:
  return (struct uvr_vk_image) { .lgdev = VK_NULL_HANDLE, .icount = 0, .images = NULL, .vcount = 0, .views = NULL, .swapchain = VK_NULL_HANDLE };
}


struct uvr_vk_shader_module uvr_vk_shader_module_create(struct uvr_vk_shader_module_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkShaderModule shader = VK_NULL_HANDLE;

  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.codeSize = uvrvk->codeSize;
  create_info.pCode = (const uint32_t *) uvrvk->pCode;

  res = vkCreateShaderModule(uvrvk->lgdev, &create_info, NULL, &shader);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_shader_module_create(vkCreateShaderModule): %s", vkres_msg(res));
    goto exit_vk_shader_module;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_shader_module_create: '%s' shader VkShaderModule successfully created retval(%p)", uvrvk->name, shader);

  return (struct uvr_vk_shader_module) { .lgdev = uvrvk->lgdev, .shader = shader, .name = uvrvk->name };

exit_vk_shader_module:
  return (struct uvr_vk_shader_module) { .lgdev = VK_NULL_HANDLE, .shader = VK_NULL_HANDLE, .name = NULL };
}


struct uvr_vk_pipeline_layout uvr_vk_pipeline_layout_create(struct uvr_vk_pipeline_layout_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkPipelineLayout playout = VK_NULL_HANDLE;

  VkPipelineLayoutCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.setLayoutCount = uvrvk->setLayoutCount;
  create_info.pSetLayouts = uvrvk->pSetLayouts;
  create_info.pushConstantRangeCount = uvrvk->pushConstantRangeCount;
  create_info.pPushConstantRanges = uvrvk->pPushConstantRanges;

  res = vkCreatePipelineLayout(uvrvk->lgdev, &create_info, NULL, &playout);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_pipeline_layout_create(vkCreatePipelineLayout): %s", vkres_msg(res));
    goto exit_vk_pipeline_layout;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_pipeline_layout_create: VkPipelineLayout successfully created retval(%p)", playout);

  return (struct uvr_vk_pipeline_layout) { .lgdev = uvrvk->lgdev, .playout = playout };

exit_vk_pipeline_layout:
  return (struct uvr_vk_pipeline_layout) { .lgdev = VK_NULL_HANDLE, .playout = VK_NULL_HANDLE };
}


struct uvr_vk_render_pass uvr_vk_render_pass_create(struct uvr_vk_render_pass_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkRenderPass renderpass = VK_NULL_HANDLE;

  VkRenderPassCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.attachmentCount = uvrvk->attachmentCount;
  create_info.pAttachments = uvrvk->pAttachments;
  create_info.subpassCount = uvrvk->subpassCount;
  create_info.pSubpasses = uvrvk->pSubpasses;
  create_info.dependencyCount = uvrvk->dependencyCount;
  create_info.pDependencies = uvrvk->pDependencies;

  res = vkCreateRenderPass(uvrvk->lgdev, &create_info, NULL, &renderpass);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_render_pass_create(vkCreateRenderPass): %s", vkres_msg(res));
    goto exit_vk_render_pass;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_render_pass_create: VkRenderPass successfully created retval(%p)", renderpass);

  return (struct uvr_vk_render_pass) { .lgdev = uvrvk->lgdev, .renderpass = renderpass };

exit_vk_render_pass:
  return (struct uvr_vk_render_pass) { .lgdev = VK_NULL_HANDLE, .renderpass = VK_NULL_HANDLE };
}


struct uvr_vk_graphics_pipeline uvr_vk_graphics_pipeline_create(struct uvr_vk_graphics_pipeline_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkPipeline pipeline = VK_NULL_HANDLE;

  VkGraphicsPipelineCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.stageCount = uvrvk->stageCount;
  create_info.pStages = uvrvk->pStages;
  create_info.pVertexInputState = uvrvk->pVertexInputState;
  create_info.pInputAssemblyState = uvrvk->pInputAssemblyState;
  create_info.pTessellationState = uvrvk->pTessellationState;
  create_info.pViewportState = uvrvk->pViewportState;
  create_info.pRasterizationState = uvrvk->pRasterizationState;
  create_info.pMultisampleState = uvrvk->pMultisampleState;
  create_info.pDepthStencilState = uvrvk->pDepthStencilState;
  create_info.pColorBlendState = uvrvk->pColorBlendState;
  create_info.pDynamicState = uvrvk->pDynamicState;
  create_info.layout = uvrvk->layout;
  create_info.renderPass = uvrvk->renderPass;
  create_info.subpass = uvrvk->subpass;
  // Won't be supporting
  create_info.basePipelineHandle = VK_NULL_HANDLE;
  create_info.basePipelineIndex = -1;

  res = vkCreateGraphicsPipelines(uvrvk->lgdev, NULL, 1, &create_info, NULL, &pipeline);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_graphics_pipeline_create(vkCreateGraphicsPipelines): %s", vkres_msg(res));
    goto exit_vk_graphics_pipeline;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_graphics_pipeline_create: VkPipeline successfully created retval(%p)", pipeline);

  return (struct uvr_vk_graphics_pipeline) { .lgdev = uvrvk->lgdev, .graphics_pipeline = pipeline };

exit_vk_graphics_pipeline:
  return (struct uvr_vk_graphics_pipeline) { .lgdev = VK_NULL_HANDLE, .graphics_pipeline = VK_NULL_HANDLE };
}


void uvr_vk_destory(struct uvr_vk_destroy *uvrvk) {
  uint32_t i, j;

  if (uvrvk->vkgraphics_pipelines) {
    for (i = 0; i < uvrvk->vkplayout_cnt; i++) {
      if (uvrvk->vkgraphics_pipelines[i].lgdev && uvrvk->vkgraphics_pipelines[i].graphics_pipeline)
        vkDestroyPipeline(uvrvk->vkgraphics_pipelines[i].lgdev, uvrvk->vkgraphics_pipelines[i].graphics_pipeline, NULL);
    }
  }

  if (uvrvk->vkplayouts) {
    for (i = 0; i < uvrvk->vkplayout_cnt; i++) {
      if (uvrvk->vkplayouts[i].lgdev && uvrvk->vkplayouts[i].playout)
        vkDestroyPipelineLayout(uvrvk->vkplayouts[i].lgdev, uvrvk->vkplayouts[i].playout, NULL);
    }
  }

  if (uvrvk->vkrenderpasses) {
    for (i = 0; i < uvrvk->vkrenderpass_cnt; i++) {
      if (uvrvk->vkrenderpasses[i].lgdev && uvrvk->vkrenderpasses[i].renderpass)
        vkDestroyRenderPass(uvrvk->vkrenderpasses[i].lgdev, uvrvk->vkrenderpasses[i].renderpass, NULL);
    }
  }

  if (uvrvk->vkshaders) {
    for (i = 0; i < uvrvk->vkshader_cnt; i++) {
      if (uvrvk->vkshaders[i].lgdev && uvrvk->vkshaders[i].shader)
        vkDestroyShaderModule(uvrvk->vkshaders[i].lgdev, uvrvk->vkshaders[i].shader, NULL);
    }
  }

  if (uvrvk->vkimages) {
    for (i = 0; i < uvrvk->vkimage_cnt; i++) {
      for (j = 0; j < uvrvk->vkimages[i].vcount; j++) {
        if (uvrvk->vkimages[i].lgdev && uvrvk->vkimages[i].views[j].view)
          vkDestroyImageView(uvrvk->vkimages[i].lgdev, uvrvk->vkimages[i].views[j].view, NULL);
      }
      free(uvrvk->vkimages[i].images);
      free(uvrvk->vkimages[i].views);
    }
  }

  if (uvrvk->vkswapchains) {
    for (i = 0; i < uvrvk->vkswapchain_cnt; i++) {
      if (uvrvk->vkswapchains[i].lgdev && uvrvk->vkswapchains[i].swapchain)
        vkDestroySwapchainKHR(uvrvk->vkswapchains[i].lgdev, uvrvk->vkswapchains[i].swapchain, NULL);
    }
  }

  for (i = 0; i < uvrvk->vklgdevs_cnt; i++) {
    if (uvrvk->vklgdevs[i].device) {
      vkDeviceWaitIdle(uvrvk->vklgdevs[i].device);
      vkDestroyDevice(uvrvk->vklgdevs[i].device, NULL);
    }
  }

  if (uvrvk->vksurf)
    vkDestroySurfaceKHR(uvrvk->vkinst, uvrvk->vksurf, NULL);
  if (uvrvk->vkinst)
    vkDestroyInstance(uvrvk->vkinst, NULL);
}
