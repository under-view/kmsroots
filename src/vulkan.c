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
  app_info.pApplicationName = uvrvk->appName;
  app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
  app_info.pEngineName = uvrvk->engineName;
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
    uvr_utils_log(UVR_DANGER, "[x] vkCreateInstance: %s", vkres_msg(res));
    return VK_NULL_HANDLE;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_instance_create: VkInstance created retval(%p)", instance);

  return instance;
}


VkSurfaceKHR uvr_vk_surface_create(struct uvr_vk_surface_create_info *uvrvk) {
  VkResult UNUSED res = VK_RESULT_MAX_ENUM;
  VkSurfaceKHR surface = VK_NULL_HANDLE;

  if (!uvrvk->vkInst) {
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

    res = vkCreateWaylandSurfaceKHR(uvrvk->vkInst, &create_info, NULL, &surface);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkCreateWaylandSurfaceKHR: %s", vkres_msg(res));
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

    res = vkCreateXcbSurfaceKHR(uvrvk->vkInst, &create_info, NULL, &surface);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkCreateXcbSurfaceKHR: %s", vkres_msg(res));
      return VK_NULL_HANDLE;
    }
  }
#endif

#if defined(INCLUDE_WAYLAND) || defined(INCLUDE_XCB)
  uvr_utils_log(UVR_SUCCESS, "uvr_vk_surface_create: VkSurfaceKHR created retval(%p)", surface);
#endif

  return surface;
}


VkPhysicalDevice uvr_vk_phdev_create(struct uvr_vk_phdev_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkPhysicalDevice device = VK_NULL_HANDLE;
  uint32_t device_count = 0;
  VkPhysicalDevice *devices = NULL;

  if (!uvrvk->vkInst) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_phdev: VkInstance not instantiated");
    uvr_utils_log(UVR_DANGER, "[x] Make a call to uvr_vk_create_instance");
    return VK_NULL_HANDLE;
  }

  res = vkEnumeratePhysicalDevices(uvrvk->vkInst, &device_count, NULL);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkEnumeratePhysicalDevices: %s", vkres_msg(res));
    return VK_NULL_HANDLE;
  }

  if (device_count == 0) {
    uvr_utils_log(UVR_DANGER, "[x] failed to find GPU with Vulkan support!!!");
    return VK_NULL_HANDLE;
  }

  devices = (VkPhysicalDevice *) alloca((device_count * sizeof(VkPhysicalDevice)) + 1);

  res = vkEnumeratePhysicalDevices(uvrvk->vkInst, &device_count, devices);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkEnumeratePhysicalDevices: %s", vkres_msg(res));
    return VK_NULL_HANDLE;
  }

#ifdef INCLUDE_KMS
  /* Get KMS fd stats */
  struct stat drm_stat = {0};
  if (uvrvk->kmsFd != -1) {
    if (fstat(uvrvk->kmsFd, &drm_stat) == -1) {
      uvr_utils_log(UVR_DANGER, "[x] fstat('%d'): %s", uvrvk->kmsFd, strerror(errno));
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
    if (uvrvk->kmsFd) {
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
      enter |= ((primary_devid == drm_stat.st_rdev || render_devid == drm_stat.st_rdev) && devprops.deviceType == uvrvk->vkPhdevType);
    }
#endif

    /* Enter will be one if condition succeeds */
    enter |= (devprops.deviceType == uvrvk->vkPhdevType);
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
  vkGetPhysicalDeviceQueueFamilyProperties(uvrvk->vkPhdev, &queue_count, NULL);
  queue_families = (VkQueueFamilyProperties *) alloca(queue_count * sizeof(VkQueueFamilyProperties));
  vkGetPhysicalDeviceQueueFamilyProperties(uvrvk->vkPhdev, &queue_count, queue_families);

  for (uint32_t i = 0; i < queue_count; i++) {
    queue.queueCount = queue_families[i].queueCount;
    if (queue_families[i].queueFlags & uvrvk->queueFlag & VK_QUEUE_GRAPHICS_BIT) {
      strncpy(queue.name, "graphics", sizeof(queue.name));
      queue.familyIndex = i; break;
    }

    if (queue_families[i].queueFlags & uvrvk->queueFlag & VK_QUEUE_COMPUTE_BIT) {
      strncpy(queue.name, "compute", sizeof(queue.name));
      queue.familyIndex = i; break;
    }

    if (queue_families[i].queueFlags & uvrvk->queueFlag & VK_QUEUE_TRANSFER_BIT) {
      strncpy(queue.name, "transfer", sizeof(queue.name));
      queue.familyIndex = i; break;
    }

    if (queue_families[i].queueFlags & uvrvk->queueFlag & VK_QUEUE_SPARSE_BINDING_BIT) {
      strncpy(queue.name, "sparse_binding", sizeof(queue.name));
      queue.familyIndex = i; break;
    }

    if (queue_families[i].queueFlags & uvrvk->queueFlag & VK_QUEUE_PROTECTED_BIT) {
      strncpy(queue.name, "protected", sizeof(queue.name));
      queue.familyIndex = i; break;
    }
  }

  return queue;

err_vk_queue_create:
  return (struct uvr_vk_queue) { .name[0] = '\0', .vkQueue = VK_NULL_HANDLE, .familyIndex = -1, .queueCount = -1 };
}


struct uvr_vk_lgdev uvr_vk_lgdev_create(struct uvr_vk_lgdev_create_info *uvrvk) {
  VkDevice device = VK_NULL_HANDLE;
  VkResult res = VK_RESULT_MAX_ENUM;

  VkDeviceQueueCreateInfo *pQueueCreateInfo = (VkDeviceQueueCreateInfo *) calloc(uvrvk->queueCount, sizeof(VkDeviceQueueCreateInfo));
  if (!pQueueCreateInfo) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto err_vk_lgdev_create;
  }

  /*
   * https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/vkspec.html#devsandqueues-priority
   * set the default priority of all queues to be the highest
   */
  const float pQueuePriorities = 1.f;
  for (uint32_t qc = 0; qc < uvrvk->queueCount; qc++) {
    pQueueCreateInfo[qc].flags = 0;
    pQueueCreateInfo[qc].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    pQueueCreateInfo[qc].queueFamilyIndex = uvrvk->queues[qc].familyIndex;
    pQueueCreateInfo[qc].queueCount = uvrvk->queues[qc].queueCount;
    pQueueCreateInfo[qc].pQueuePriorities = &pQueuePriorities;
  }

  VkDeviceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.queueCreateInfoCount = uvrvk->queueCount;
  create_info.pQueueCreateInfos = pQueueCreateInfo;
  create_info.enabledLayerCount = 0; // Deprecated and ignored
  create_info.ppEnabledLayerNames = NULL; // Deprecated and ignored
  create_info.enabledExtensionCount = uvrvk->enabledExtensionCount;
  create_info.ppEnabledExtensionNames = uvrvk->ppEnabledExtensionNames;
  create_info.pEnabledFeatures = uvrvk->pEnabledFeatures;

  /* Create logic device */
  res = vkCreateDevice(uvrvk->vkPhdev, &create_info, NULL, &device);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateDevice: %s", vkres_msg(res));
    goto err_vk_lgdev_free_pQueueCreateInfo;
  }

  for (uint32_t qc = 0; qc < uvrvk->queueCount; qc++) {
    vkGetDeviceQueue(device, uvrvk->queues[qc].familyIndex, 0, &uvrvk->queues[qc].vkQueue);
    if (!uvrvk->queues[qc].vkQueue)  {
      uvr_utils_log(UVR_DANGER, "[x] vkGetDeviceQueue: Failed to get %s queue handle", uvrvk->queues[qc].name);
      goto err_vk_lgdev_destroy;
    }

    uvr_utils_log(UVR_SUCCESS, "uvr_vk_lgdev_create: '%s' VkQueue successfully created retval(%p)", uvrvk->queues[qc].name, uvrvk->queues[qc].vkQueue);
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_lgdev_create: VkDevice created retval(%p)", device);

  free(pQueueCreateInfo);
  return (struct uvr_vk_lgdev) { .vkDevice = device, .queueCount = uvrvk->queueCount, .queues = uvrvk->queues };

err_vk_lgdev_destroy:
  if (device)
    vkDestroyDevice(device, NULL);
err_vk_lgdev_free_pQueueCreateInfo:
  free(pQueueCreateInfo);
err_vk_lgdev_create:
  return (struct uvr_vk_lgdev) { .vkDevice = VK_NULL_HANDLE, .queueCount = -1, .queues = NULL };
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
    uvr_utils_log(UVR_DANGER, "[x] vkGetPhysicalDeviceSurfaceFormatsKHR: %s", vkres_msg(res));
    goto exit_vk_surface_formats;
  }

  formats = (VkSurfaceFormatKHR *) calloc(fcount, sizeof(formats));
  if (!formats) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_vk_surface_formats;
  }

  res = vkGetPhysicalDeviceSurfaceFormatsKHR(phdev, surface, &fcount, formats);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkGetPhysicalDeviceSurfaceFormatsKHR: %s", vkres_msg(res));
    goto exit_vk_surface_formats_free;
  }

  return (struct uvr_vk_surface_format) { .surfaceFormatCount = fcount, .surfaceFormats = formats };

exit_vk_surface_formats_free:
  free(formats);
exit_vk_surface_formats:
  return (struct uvr_vk_surface_format) { .surfaceFormatCount = 0, .surfaceFormats = NULL };
}


struct uvr_vk_surface_present_mode uvr_vk_get_surface_present_modes(VkPhysicalDevice phdev, VkSurfaceKHR surface) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkPresentModeKHR *modes = NULL;
  uint32_t mcount = 0;

  res = vkGetPhysicalDeviceSurfacePresentModesKHR(phdev, surface, &mcount, NULL);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkGetPhysicalDeviceSurfacePresentModesKHR: %s", vkres_msg(res));
    goto exit_vk_surface_present_modes;
  }

  modes = (VkPresentModeKHR *) calloc(mcount, sizeof(modes));
  if (!modes) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_vk_surface_present_modes;
  }

  res = vkGetPhysicalDeviceSurfacePresentModesKHR(phdev, surface, &mcount, modes);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkGetPhysicalDeviceSurfacePresentModesKHR: %s", vkres_msg(res));
    goto exit_vk_surface_present_modes_free;
  }

  return (struct uvr_vk_surface_present_mode) { .presentModeCount = mcount, .presentModes = modes };

exit_vk_surface_present_modes_free:
  free(modes);
exit_vk_surface_present_modes:
  return (struct uvr_vk_surface_present_mode) { .presentModeCount = 0, .presentModes = NULL };
}


struct uvr_vk_swapchain uvr_vk_swapchain_create(struct uvr_vk_swapchain_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;

  if (uvrvk->surfaceCapabilities.currentExtent.width != UINT32_MAX) {
    uvrvk->extent2D = uvrvk->surfaceCapabilities.currentExtent;
  } else {
    uvrvk->extent2D.width = fmax(uvrvk->surfaceCapabilities.minImageExtent.width, fmin(uvrvk->surfaceCapabilities.maxImageExtent.width, uvrvk->extent2D.width));
    uvrvk->extent2D.height = fmax(uvrvk->surfaceCapabilities.minImageExtent.height, fmin(uvrvk->surfaceCapabilities.maxImageExtent.height, uvrvk->extent2D.height));
  }

  VkSwapchainCreateInfoKHR create_info;
  create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.surface = uvrvk->vkSurface;
  if (uvrvk->surfaceCapabilities.maxImageCount > 0 && (uvrvk->surfaceCapabilities.minImageCount + 1) < uvrvk->surfaceCapabilities.maxImageCount)
    create_info.minImageCount = uvrvk->surfaceCapabilities.maxImageCount;
  else
    create_info.minImageCount = uvrvk->surfaceCapabilities.minImageCount + 1;
  create_info.imageFormat = uvrvk->surfaceFormat.format;
  create_info.imageColorSpace = uvrvk->surfaceFormat.colorSpace;
  create_info.imageExtent = uvrvk->extent2D;
  create_info.imageArrayLayers = uvrvk->imageArrayLayers;
  create_info.imageUsage = uvrvk->imageUsage;
  create_info.imageSharingMode = uvrvk->imageSharingMode;
  create_info.queueFamilyIndexCount = uvrvk->queueFamilyIndexCount;
  create_info.pQueueFamilyIndices = uvrvk->pQueueFamilyIndices;
  create_info.preTransform = uvrvk->surfaceCapabilities.currentTransform;
  create_info.compositeAlpha = uvrvk->compositeAlpha;
  create_info.presentMode = uvrvk->presentMode;
  create_info.clipped = uvrvk->clipped;
  create_info.oldSwapchain = uvrvk->oldSwapchain;

  res = vkCreateSwapchainKHR(uvrvk->vkDevice, &create_info, NULL, &swapchain);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateSwapchainKHR: %s", vkres_msg(res));
    goto exit_vk_swapchain;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_swapchain_create: VkSwapchainKHR successfully created retval(%p)", swapchain);

  return (struct uvr_vk_swapchain) { .vkDevice = uvrvk->vkDevice, .vkSwapchain = swapchain };

exit_vk_swapchain:
  return (struct uvr_vk_swapchain) { .vkDevice = VK_NULL_HANDLE, .vkSwapchain = VK_NULL_HANDLE };
}


struct uvr_vk_image uvr_vk_image_create(struct uvr_vk_image_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  struct uvr_vk_image_handle *images = NULL;
  struct uvr_vk_image_view_handle *views = NULL;

  uint32_t icount = 0, i;
  VkImage *vkimages = NULL;

  if (uvrvk->vkSwapchain) {
    res = vkGetSwapchainImagesKHR(uvrvk->vkDevice, uvrvk->vkSwapchain, &icount, NULL);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkGetSwapchainImagesKHR: %s", vkres_msg(res));
      goto exit_vk_image;
    }

    vkimages = alloca(icount * sizeof(VkImage));

    res = vkGetSwapchainImagesKHR(uvrvk->vkDevice, uvrvk->vkSwapchain, &icount, vkimages);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkGetSwapchainImagesKHR: %s", vkres_msg(res));
      goto exit_vk_image;
    }

    uvr_utils_log(UVR_INFO, "uvr_vk_image_create: Total images in swapchain %u", icount);
  }

  images = calloc(icount, sizeof(images));
  if (!images) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_vk_image;
  }

  views = calloc(icount, sizeof(views));
  if (!views) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
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

    res = vkCreateImageView(uvrvk->vkDevice, &create_info, NULL, &views[i].view);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkCreateImageView: %s", vkres_msg(res));
      goto exit_vk_image_free_image_view;
    }

    uvr_utils_log(UVR_WARNING, "uvr_vk_image_create: VkImage (%p) associate with VkImageView", images[i].image);
    uvr_utils_log(UVR_SUCCESS, "uvr_vk_image_create: VkImageView successfully created retval(%p)", views[i].view);
  }

  return (struct uvr_vk_image) { .vkDevice = uvrvk->vkDevice, .imageCount = icount, .vkImages = images,
                                 .vkImageViews = views, .vkSwapchain = uvrvk->vkSwapchain };


exit_vk_image_free_image_view:
  if (views) {
    for (i = 0; i < icount; i++) {
      if (views[i].view)
        vkDestroyImageView(uvrvk->vkDevice, views[i].view, NULL);
    }
  }
//exit_vk_image_free_image_views:
  free(views);
exit_vk_image_free_images:
  free(images);
exit_vk_image:
  return (struct uvr_vk_image) { .vkDevice = VK_NULL_HANDLE, .imageCount = 0, .vkImages = NULL,
                                 .vkImageViews = NULL, .vkSwapchain = VK_NULL_HANDLE };
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

  res = vkCreateShaderModule(uvrvk->vkDevice, &create_info, NULL, &shader);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateShaderModule: %s", vkres_msg(res));
    goto exit_vk_shader_module;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_shader_module_create: '%s' shader VkShaderModule successfully created retval(%p)", uvrvk->name, shader);

  return (struct uvr_vk_shader_module) { .vkDevice = uvrvk->vkDevice, .shader = shader, .name = uvrvk->name };

exit_vk_shader_module:
  return (struct uvr_vk_shader_module) { .vkDevice = VK_NULL_HANDLE, .shader = VK_NULL_HANDLE, .name = NULL };
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

  res = vkCreatePipelineLayout(uvrvk->vkDevice, &create_info, NULL, &playout);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreatePipelineLayout: %s", vkres_msg(res));
    goto exit_vk_pipeline_layout;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_pipeline_layout_create: VkPipelineLayout successfully created retval(%p)", playout);

  return (struct uvr_vk_pipeline_layout) { .vkDevice = uvrvk->vkDevice, .vkPipelineLayout = playout };

exit_vk_pipeline_layout:
  return (struct uvr_vk_pipeline_layout) { .vkDevice = VK_NULL_HANDLE, .vkPipelineLayout = VK_NULL_HANDLE };
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

  res = vkCreateRenderPass(uvrvk->vkDevice, &create_info, NULL, &renderpass);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateRenderPass: %s", vkres_msg(res));
    goto exit_vk_render_pass;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_render_pass_create: VkRenderPass successfully created retval(%p)", renderpass);

  return (struct uvr_vk_render_pass) { .vkDevice = uvrvk->vkDevice, .renderPass = renderpass };

exit_vk_render_pass:
  return (struct uvr_vk_render_pass) { .vkDevice = VK_NULL_HANDLE, .renderPass = VK_NULL_HANDLE };
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
  create_info.layout = uvrvk->vkPipelineLayout;
  create_info.renderPass = uvrvk->renderPass;
  create_info.subpass = uvrvk->subpass;
  // Won't be supporting
  create_info.basePipelineHandle = VK_NULL_HANDLE;
  create_info.basePipelineIndex = -1;

  res = vkCreateGraphicsPipelines(uvrvk->vkDevice, NULL, 1, &create_info, NULL, &pipeline);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateGraphicsPipelines: %s", vkres_msg(res));
    goto exit_vk_graphics_pipeline;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_graphics_pipeline_create: VkPipeline successfully created retval(%p)", pipeline);

  return (struct uvr_vk_graphics_pipeline) { .vkDevice = uvrvk->vkDevice, .graphicsPipeline = pipeline };

exit_vk_graphics_pipeline:
  return (struct uvr_vk_graphics_pipeline) { .vkDevice = VK_NULL_HANDLE, .graphicsPipeline = VK_NULL_HANDLE };
}


struct uvr_vk_framebuffer uvr_vk_framebuffer_create(struct uvr_vk_framebuffer_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  struct uvr_vk_framebuffer_handle *vkfbs = NULL;
  uint32_t fbc;

  vkfbs = (struct uvr_vk_framebuffer_handle *) calloc(uvrvk->frameBufferCount, sizeof(vkfbs));
  if (!vkfbs) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_vk_framebuffer;
  }

  VkFramebufferCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.renderPass = uvrvk->renderPass;
  create_info.width = uvrvk->width;
  create_info.height = uvrvk->height;
  create_info.layers = uvrvk->layers;

  for (fbc = 0; fbc < uvrvk->frameBufferCount; fbc++) {
    create_info.attachmentCount = 1;
    create_info.pAttachments = &uvrvk->vkImageViews[fbc].view;

    res = vkCreateFramebuffer(uvrvk->vkDevice, &create_info, NULL, &vkfbs[fbc].fb);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkCreateFramebuffer: %s", vkres_msg(res));
      goto exit_vk_framebuffer_vk_framebuffer_destroy;
    }

    uvr_utils_log(UVR_SUCCESS, "uvr_vk_framebuffer_create: VkFramebuffer successfully created retval(%p)", vkfbs[fbc].fb);
  }

  return (struct uvr_vk_framebuffer) { .vkDevice = uvrvk->vkDevice, .frameBufferCount = uvrvk->frameBufferCount, .vkFrameBuffers = vkfbs };

exit_vk_framebuffer_vk_framebuffer_destroy:
  for (fbc = 0; fbc < uvrvk->frameBufferCount; fbc++) {
    if (vkfbs[fbc].fb)
      vkDestroyFramebuffer(uvrvk->vkDevice, vkfbs[fbc].fb, NULL);
  }
//exit_vk_framebuffer_free_vkfbs:
  free(vkfbs);
exit_vk_framebuffer:
  return (struct uvr_vk_framebuffer) { .vkDevice = VK_NULL_HANDLE, .frameBufferCount = 0, .vkFrameBuffers = NULL };
}


struct uvr_vk_command_buffer uvr_vk_command_buffer_create(struct uvr_vk_command_buffer_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkCommandPool cmdpool = VK_NULL_HANDLE;
  VkCommandBuffer *cmdbuffs = VK_NULL_HANDLE;
  struct uvr_vk_command_buffer_handle *cbuffs = NULL;

  VkCommandPoolCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  create_info.queueFamilyIndex = uvrvk->queueFamilyIndex;

  res = vkCreateCommandPool(uvrvk->vkDevice, &create_info, NULL, &cmdpool);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateCommandPool: %s", vkres_msg(res));
    goto exit_vk_command_buffer;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_command_buffer_create: VkCommandPool successfully created retval(%p)", cmdpool);

  cmdbuffs = alloca(uvrvk->commandBufferCount * sizeof(VkCommandBuffer));

  VkCommandBufferAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  alloc_info.commandPool = cmdpool;
  alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  alloc_info.commandBufferCount = uvrvk->commandBufferCount;

  res = vkAllocateCommandBuffers(uvrvk->vkDevice, &alloc_info, cmdbuffs);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkAllocateCommandBuffers: %s", vkres_msg(res));
    goto exit_vk_command_buffer_destroy_cmd_pool;
  }

  cbuffs = calloc(uvrvk->commandBufferCount, sizeof(struct uvr_vk_command_buffer_handle));
  if (!cbuffs) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_vk_command_buffer_destroy_cmd_pool;
  }

  for (uint32_t i = 0; i < uvrvk->commandBufferCount; i++) {
    cbuffs[i].buffer = cmdbuffs[i];
    uvr_utils_log(UVR_WARNING, "uvr_vk_command_buffer_create: VkCommandBuffer successfully created retval(%p)", cbuffs[i].buffer);
  }

  return (struct uvr_vk_command_buffer) { .vkDevice = uvrvk->vkDevice, .vkCommandPool = cmdpool,
                                          .commandBufferCount = uvrvk->commandBufferCount, .vkCommandbuffers = cbuffs };

exit_vk_command_buffer_destroy_cmd_pool:
  vkDestroyCommandPool(uvrvk->vkDevice, cmdpool, NULL);
exit_vk_command_buffer:
  return (struct uvr_vk_command_buffer) { .vkDevice = VK_NULL_HANDLE, .vkCommandPool = VK_NULL_HANDLE,
                                          .commandBufferCount = 0, .vkCommandbuffers = NULL };
}


int uvr_vk_command_buffer_record_begin(struct uvr_vk_command_buffer_record_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.pNext = NULL;
  begin_info.flags = uvrvk->flags;
  // We don't use secondary command buffers in API so set to null
  begin_info.pInheritanceInfo = NULL;

  for (uint32_t i = 0; i < uvrvk->commandBufferCount; i++) {
    res = vkBeginCommandBuffer(uvrvk->vkCommandbuffers[i].buffer, &begin_info);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkBeginCommandBuffer: %s", vkres_msg(res));
      return -1;
    }
  }

  return 0;
}


int uvr_vk_command_buffer_record_end(struct uvr_vk_command_buffer_record_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;

  for (uint32_t i = 0; i < uvrvk->commandBufferCount; i++) {
    res = vkEndCommandBuffer(uvrvk->vkCommandbuffers[i].buffer);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkEndCommandBuffer: %s", vkres_msg(res));
      return -1;
    }
  }

  return 0;
}


struct uvr_vk_sync_obj uvr_vk_sync_obj_create(struct uvr_vk_sync_obj_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  struct uvr_vk_fence_handle *vkFences = NULL;
  struct uvr_vk_semaphore_handle *vkSemaphores = NULL;
  uint32_t s;

  vkFences = calloc(uvrvk->fenceCount, sizeof(vkFences));
  if (!vkFences) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_vk_sync_obj;
  }

  vkSemaphores = calloc(uvrvk->semaphoreCount, sizeof(vkSemaphores));
  if (!vkSemaphores) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_vk_sync_obj_free_vk_fence;
  }

  VkFenceCreateInfo fence_create_info = {};
  fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_create_info.pNext = NULL;
  fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  VkSemaphoreCreateInfo semphore_create_info = {};
  semphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  semphore_create_info.pNext = NULL;
  semphore_create_info.flags = 0;

  for (s = 0; s < uvrvk->fenceCount; s++) {
    res = vkCreateFence(uvrvk->vkDevice, &fence_create_info, NULL, &vkFences[s].fence);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkCreateFence: %s", vkres_msg(res));
      goto exit_vk_sync_obj_destroy_vk_fence;
    }
  }

  for (s = 0; s < uvrvk->semaphoreCount; s++) {
    res = vkCreateSemaphore(uvrvk->vkDevice, &semphore_create_info, NULL, &vkSemaphores[s].semaphore);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkCreateSemaphore: %s", vkres_msg(res));
      goto exit_vk_sync_obj_destroy_vk_semaphore;
    }
  }

  return (struct uvr_vk_sync_obj) { .vkDevice = uvrvk->vkDevice, .fenceCount = uvrvk->fenceCount, .vkFences = vkFences,
                                    .semaphoreCount = uvrvk->semaphoreCount, .vkSemaphores = vkSemaphores };


exit_vk_sync_obj_destroy_vk_semaphore:
  for (s = 0; s < uvrvk->semaphoreCount; s++)
    if (vkSemaphores[s].semaphore)
      vkDestroySemaphore(uvrvk->vkDevice, vkSemaphores[s].semaphore, NULL);
exit_vk_sync_obj_destroy_vk_fence:
  for (s = 0; s < uvrvk->fenceCount; s++)
    if (vkFences[s].fence)
      vkDestroyFence(uvrvk->vkDevice, vkFences[s].fence, NULL);
//exit_vk_sync_obj_free_vk_semaphore:
  free(vkSemaphores);
exit_vk_sync_obj_free_vk_fence:
  free(vkFences);
exit_vk_sync_obj:
  return (struct uvr_vk_sync_obj) { .vkDevice = VK_NULL_HANDLE, .fenceCount = 0, .vkFences = NULL, .semaphoreCount = 0, .vkSemaphores = NULL };
};


static uint32_t retrieve_memory_type_index(VkPhysicalDevice vkPhdev, uint32_t memoryType, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties mem_props;
  vkGetPhysicalDeviceMemoryProperties(vkPhdev, &mem_props);

  for (uint32_t i = 0; i < mem_props.memoryTypeCount; i++) {
    if (memoryType & (1 << i) && (mem_props.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  uvr_utils_log(UVR_DANGER, "[x] retrieve_memory_type: failed to find suitable memory type");

  return UINT32_MAX;
}


struct uvr_vk_buffer uvr_vk_buffer_create(struct uvr_vk_buffer_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory memory = VK_NULL_HANDLE;

  VkBufferCreateInfo buffer_create_info = {};
  buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_create_info.pNext = NULL;
  buffer_create_info.flags = uvrvk->bufferFlags;
  buffer_create_info.size  = uvrvk->bufferSize;
  buffer_create_info.usage = uvrvk->bufferUsage;
  buffer_create_info.sharingMode = uvrvk->bufferSharingMode;
  buffer_create_info.queueFamilyIndexCount = uvrvk->queueFamilyIndexCount;
  buffer_create_info.pQueueFamilyIndices = uvrvk->queueFamilyIndices;

  /* Creates underlying buffer header */
  res = vkCreateBuffer(uvrvk->vkDevice, &buffer_create_info, NULL, &buffer);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateBuffer: %s", vkres_msg(res));
    goto exit_vk_buffer;
  }

  VkMemoryRequirements mem_reqs;
  vkGetBufferMemoryRequirements(uvrvk->vkDevice, buffer, &mem_reqs);

  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = retrieve_memory_type_index(uvrvk->vkPhdev, mem_reqs.memoryTypeBits, uvrvk->memPropertyFlags);

  res = vkAllocateMemory(uvrvk->vkDevice, &alloc_info, NULL, &memory);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkAllocateMemory: %s", vkres_msg(res));
    goto exit_vk_buffer;
  }

  vkBindBufferMemory(uvrvk->vkDevice, buffer, memory, 0);

  return (struct uvr_vk_buffer) { .vkDevice = uvrvk->vkDevice, .vkBuffer = buffer, .vkDeviceMemory = memory };

exit_vk_buffer:
  return (struct uvr_vk_buffer) { .vkDevice = VK_NULL_HANDLE, .vkBuffer = VK_NULL_HANDLE, .vkDeviceMemory = VK_NULL_HANDLE };
}


void uvr_vk_destory(struct uvr_vk_destroy *uvrvk) {
  uint32_t i, j;

  if (uvrvk->uvr_vk_sync_obj) {
    for (i = 0; i < uvrvk->uvr_vk_sync_obj_cnt; i++) {
      for (j = 0; j < uvrvk->uvr_vk_sync_obj[i].fenceCount; j++) {
        if (uvrvk->uvr_vk_sync_obj[i].vkFences[j].fence) {
          vkWaitForFences(uvrvk->uvr_vk_sync_obj[i].vkDevice, 1, &uvrvk->uvr_vk_sync_obj[i].vkFences[j].fence, VK_TRUE, UINT64_MAX);
          vkDestroyFence(uvrvk->uvr_vk_sync_obj[i].vkDevice, uvrvk->uvr_vk_sync_obj[i].vkFences[j].fence, NULL);
        }
      }
      for (j = 0; j < uvrvk->uvr_vk_sync_obj[i].semaphoreCount; j++) {
        if (uvrvk->uvr_vk_sync_obj[i].vkSemaphores[j].semaphore) {
          vkDestroySemaphore(uvrvk->uvr_vk_sync_obj[i].vkDevice, uvrvk->uvr_vk_sync_obj[i].vkSemaphores[j].semaphore, NULL);
        }
      }
      free(uvrvk->uvr_vk_sync_obj[i].vkFences);
      free(uvrvk->uvr_vk_sync_obj[i].vkSemaphores);
    }
  }

  if (uvrvk->uvr_vk_command_buffer) {
    for (i = 0; i < uvrvk->uvr_vk_command_buffer_cnt; i++) {
      if (uvrvk->uvr_vk_command_buffer[i].vkDevice && uvrvk->uvr_vk_command_buffer[i].vkCommandPool)
        vkDestroyCommandPool(uvrvk->uvr_vk_command_buffer[i].vkDevice, uvrvk->uvr_vk_command_buffer[i].vkCommandPool, NULL);
      free(uvrvk->uvr_vk_command_buffer[i].vkCommandbuffers);
    }
  }

  if (uvrvk->uvr_vk_buffer) {
    for (i = 0; i < uvrvk->uvr_vk_buffer_cnt; i++) {
      if (uvrvk->uvr_vk_buffer[i].vkBuffer)
        vkDestroyBuffer(uvrvk->uvr_vk_buffer[i].vkDevice, uvrvk->uvr_vk_buffer[i].vkBuffer, NULL);
      if (uvrvk->uvr_vk_buffer[i].vkDeviceMemory)
        vkFreeMemory(uvrvk->uvr_vk_buffer[i].vkDevice, uvrvk->uvr_vk_buffer[i].vkDeviceMemory, NULL);
    }
  }

  if (uvrvk->uvr_vk_framebuffer) {
    for (i = 0; i < uvrvk->uvr_vk_framebuffer_cnt; i++) {
      for (j = 0; j < uvrvk->uvr_vk_framebuffer[i].frameBufferCount; j++) {
        if (uvrvk->uvr_vk_framebuffer[i].vkDevice && uvrvk->uvr_vk_framebuffer[i].vkFrameBuffers[j].fb)
          vkDestroyFramebuffer(uvrvk->uvr_vk_framebuffer[i].vkDevice, uvrvk->uvr_vk_framebuffer[i].vkFrameBuffers[j].fb, NULL);
      }
      free(uvrvk->uvr_vk_framebuffer[i].vkFrameBuffers);
    }
  }

  if (uvrvk->uvr_vk_graphics_pipeline) {
    for (i = 0; i < uvrvk->uvr_vk_graphics_pipeline_cnt; i++) {
      if (uvrvk->uvr_vk_graphics_pipeline[i].vkDevice && uvrvk->uvr_vk_graphics_pipeline[i].graphicsPipeline)
        vkDestroyPipeline(uvrvk->uvr_vk_graphics_pipeline[i].vkDevice, uvrvk->uvr_vk_graphics_pipeline[i].graphicsPipeline, NULL);
    }
  }

  if (uvrvk->uvr_vk_pipeline_layout) {
    for (i = 0; i < uvrvk->uvr_vk_pipeline_layout_cnt; i++) {
      if (uvrvk->uvr_vk_pipeline_layout[i].vkDevice && uvrvk->uvr_vk_pipeline_layout[i].vkPipelineLayout)
        vkDestroyPipelineLayout(uvrvk->uvr_vk_pipeline_layout[i].vkDevice, uvrvk->uvr_vk_pipeline_layout[i].vkPipelineLayout, NULL);
    }
  }

  if (uvrvk->uvr_vk_render_pass) {
    for (i = 0; i < uvrvk->uvr_vk_render_pass_cnt; i++) {
      if (uvrvk->uvr_vk_render_pass[i].vkDevice && uvrvk->uvr_vk_render_pass[i].renderPass)
        vkDestroyRenderPass(uvrvk->uvr_vk_render_pass[i].vkDevice, uvrvk->uvr_vk_render_pass[i].renderPass, NULL);
    }
  }

  if (uvrvk->uvr_vk_shader_module) {
    for (i = 0; i < uvrvk->uvr_vk_shader_module_cnt; i++) {
      if (uvrvk->uvr_vk_shader_module[i].vkDevice && uvrvk->uvr_vk_shader_module[i].shader)
        vkDestroyShaderModule(uvrvk->uvr_vk_shader_module[i].vkDevice, uvrvk->uvr_vk_shader_module[i].shader, NULL);
    }
  }

  if (uvrvk->uvr_vk_image) {
    for (i = 0; i < uvrvk->uvr_vk_image_cnt; i++) {
      for (j = 0; j < uvrvk->uvr_vk_image[i].imageCount; j++) {
        if (uvrvk->uvr_vk_image[i].vkDevice && uvrvk->uvr_vk_image[i].vkImageViews[j].view)
          vkDestroyImageView(uvrvk->uvr_vk_image[i].vkDevice, uvrvk->uvr_vk_image[i].vkImageViews[j].view, NULL);
      }
      free(uvrvk->uvr_vk_image[i].vkImages);
      free(uvrvk->uvr_vk_image[i].vkImageViews);
    }
  }

  if (uvrvk->uvr_vk_swapchain) {
    for (i = 0; i < uvrvk->uvr_vk_swapchain_cnt; i++) {
      if (uvrvk->uvr_vk_swapchain[i].vkDevice && uvrvk->uvr_vk_swapchain[i].vkSwapchain)
        vkDestroySwapchainKHR(uvrvk->uvr_vk_swapchain[i].vkDevice, uvrvk->uvr_vk_swapchain[i].vkSwapchain, NULL);
    }
  }

  for (i = 0; i < uvrvk->uvr_vk_lgdev_cnt; i++) {
    if (uvrvk->uvr_vk_lgdev[i].vkDevice) {
      vkDeviceWaitIdle(uvrvk->uvr_vk_lgdev[i].vkDevice);
      vkDestroyDevice(uvrvk->uvr_vk_lgdev[i].vkDevice, NULL);
    }
  }

  if (uvrvk->vksurf)
    vkDestroySurfaceKHR(uvrvk->vkinst, uvrvk->vksurf, NULL);
  if (uvrvk->vkinst)
    vkDestroyInstance(uvrvk->vkinst, NULL);
}
