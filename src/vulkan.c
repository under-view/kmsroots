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
  create_info.pNext = (uvrvk->enabledLayerNames) ? &features : NULL;
  create_info.flags = 0;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledLayerCount = uvrvk->enabledLayerCount;
  create_info.ppEnabledLayerNames = uvrvk->enabledLayerNames;
  create_info.enabledExtensionCount = uvrvk->enabledExtensionCount;
  create_info.ppEnabledExtensionNames = uvrvk->enabledExtensionNames;

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

  if (!uvrvk->instance) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_surface_create: VkInstance not instantiated");
    uvr_utils_log(UVR_DANGER, "[x] Make a call to uvr_vk_create_instance");
    return VK_NULL_HANDLE;
  }

  if (uvrvk->surfaceType != UVR_WAYLAND_CLIENT_SURFACE && uvrvk->surfaceType != UVR_XCB_CLIENT_SURFACE) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_surface_create: Must specify correct enum uvrvk_surface_type");
    return VK_NULL_HANDLE;
  }

#ifdef INCLUDE_WAYLAND
  if (uvrvk->surfaceType == UVR_WAYLAND_CLIENT_SURFACE) {
    VkWaylandSurfaceCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.display = uvrvk->display;
    create_info.surface = uvrvk->surface;

    res = vkCreateWaylandSurfaceKHR(uvrvk->instance, &create_info, NULL, &surface);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkCreateWaylandSurfaceKHR: %s", vkres_msg(res));
      return VK_NULL_HANDLE;
    }
  }
#endif

#ifdef INCLUDE_XCB
  if (uvrvk->surfaceType == UVR_XCB_CLIENT_SURFACE) {
    VkXcbSurfaceCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.connection = uvrvk->display;
    create_info.window = uvrvk->window;

    res = vkCreateXcbSurfaceKHR(uvrvk->instance, &create_info, NULL, &surface);
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


struct uvr_vk_phdev uvr_vk_phdev_create(struct uvr_vk_phdev_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkPhysicalDevice device = VK_NULL_HANDLE;
  uint32_t device_count = 0;
  VkPhysicalDevice *devices = NULL;

  if (!uvrvk->instance) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_phdev: VkInstance not instantiated");
    uvr_utils_log(UVR_DANGER, "[x] Make a call to uvr_vk_create_instance");
    goto exit_error_vk_phdev_create;
  }

  res = vkEnumeratePhysicalDevices(uvrvk->instance, &device_count, NULL);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkEnumeratePhysicalDevices: %s", vkres_msg(res));
    goto exit_error_vk_phdev_create;
  }

  if (device_count == 0) {
    uvr_utils_log(UVR_DANGER, "[x] failed to find GPU with Vulkan support!!!");
    goto exit_error_vk_phdev_create;
  }

  devices = (VkPhysicalDevice *) alloca((device_count * sizeof(VkPhysicalDevice)) + 1);

  res = vkEnumeratePhysicalDevices(uvrvk->instance, &device_count, devices);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkEnumeratePhysicalDevices: %s", vkres_msg(res));
    goto exit_error_vk_phdev_create;
  }

#ifdef INCLUDE_KMS
  /* Get KMS fd stats */
  struct stat drm_stat = {0};
  if (uvrvk->kmsfd != -1) {
    if (fstat(uvrvk->kmsfd, &drm_stat) == -1) {
      uvr_utils_log(UVR_DANGER, "[x] fstat('%d'): %s", uvrvk->kmsfd, strerror(errno));
      goto exit_error_vk_phdev_create;
    }
  }

  /* Taken from wlroots: https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/render/vulkan/vulkan.c#L349 */
  VkPhysicalDeviceProperties2 phdevProperties2;
  VkPhysicalDeviceDrmPropertiesEXT phdevDrmProperties;
  memset(&phdevProperties2, 0, sizeof(phdevProperties2));
  memset(&phdevDrmProperties, 0, sizeof(phdevDrmProperties));

  phdevDrmProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT;
  phdevProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
#endif

  int enter = 0;
  VkPhysicalDeviceFeatures phdevFeatures;
  VkPhysicalDeviceProperties phdevProperties;

  /*
   * get a physical device that is suitable
   * to do the graphics related task that we need
   */
  for (uint32_t i = 0; i < device_count; i++) {
    vkGetPhysicalDeviceFeatures(devices[i], &phdevFeatures); /* Query device features */
    vkGetPhysicalDeviceProperties(devices[i], &phdevProperties); /* Query device properties */

#ifdef INCLUDE_KMS
    if (uvrvk->kmsfd) {
      /* Taken from wlroots: https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/render/vulkan/vulkan.c#L311 */
      phdevProperties2.pNext = &phdevDrmProperties;
      vkGetPhysicalDeviceProperties2(devices[i], &phdevProperties2);

      phdevDrmProperties.pNext = phdevProperties2.pNext;
      dev_t primary_devid = makedev(phdevDrmProperties.primaryMajor, phdevDrmProperties.primaryMinor);
      dev_t render_devid = makedev(phdevDrmProperties.renderMajor, phdevDrmProperties.renderMinor);

      /*
       * Enter will be one if condition succeeds
       * Enter will be zero if condition fails
       * Need to make sure even if we have a valid DRI device node fd.
       * That the customer choosen device type is the same as the DRI device node.
       */
      enter |= ((primary_devid == drm_stat.st_rdev || render_devid == drm_stat.st_rdev) && phdevProperties.deviceType == uvrvk->deviceType);
    }
#endif

    /* Enter will be one if condition succeeds */
    enter |= (phdevProperties.deviceType == uvrvk->deviceType);
    if (enter) {
      memmove(&device, &devices[i], sizeof(devices[i]));
      uvr_utils_log(UVR_SUCCESS, "Suitable GPU Found: %s, api version: %u", phdevProperties.deviceName, phdevProperties.apiVersion);
      break;
    }
  }

  if (device == VK_NULL_HANDLE) {
    uvr_utils_log(UVR_DANGER, "[x] GPU that meets requirement not found!");
    goto exit_error_vk_phdev_create;
  }

  return (struct uvr_vk_phdev) { .instance = uvrvk->instance, .physDevice = device, .physDeviceProperties = phdevProperties, .physDeviceFeatures = phdevFeatures,
#ifdef INCLUDE_KMS
  .kmsfd = uvrvk->kmsfd, .physDeviceDrmProperties = phdevDrmProperties
#endif
  };

exit_error_vk_phdev_create:
  memset(&phdevFeatures, 0, sizeof(phdevFeatures));
  memset(&phdevProperties, 0, sizeof(phdevProperties));

#ifdef INCLUDE_KMS
  memset(&phdevDrmProperties, 0, sizeof(phdevDrmProperties));
#endif

  return (struct uvr_vk_phdev) { .instance = VK_NULL_HANDLE, .physDevice = VK_NULL_HANDLE, .physDeviceProperties = phdevProperties, .physDeviceFeatures = phdevFeatures,
#ifdef INCLUDE_KMS
  .kmsfd = -1, .physDeviceDrmProperties = phdevDrmProperties
#endif
  };
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
  vkGetPhysicalDeviceQueueFamilyProperties(uvrvk->physDevice, &queue_count, NULL);
  queue_families = (VkQueueFamilyProperties *) alloca(queue_count * sizeof(VkQueueFamilyProperties));
  vkGetPhysicalDeviceQueueFamilyProperties(uvrvk->physDevice, &queue_count, queue_families);

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
  return (struct uvr_vk_queue) { .name[0] = '\0', .queue = VK_NULL_HANDLE, .familyIndex = -1, .queueCount = -1 };
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
  create_info.ppEnabledExtensionNames = uvrvk->enabledExtensionNames;
  create_info.pEnabledFeatures = uvrvk->enabledFeatures;

  /* Create logic device */
  res = vkCreateDevice(uvrvk->physDevice, &create_info, NULL, &device);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateDevice: %s", vkres_msg(res));
    goto err_vk_lgdev_free_pQueueCreateInfo;
  }

  for (uint32_t qc = 0; qc < uvrvk->queueCount; qc++) {
    vkGetDeviceQueue(device, uvrvk->queues[qc].familyIndex, 0, &uvrvk->queues[qc].queue);
    if (!uvrvk->queues[qc].queue)  {
      uvr_utils_log(UVR_DANGER, "[x] vkGetDeviceQueue: Failed to get %s queue handle", uvrvk->queues[qc].name);
      goto err_vk_lgdev_destroy;
    }

    uvr_utils_log(UVR_SUCCESS, "uvr_vk_lgdev_create: '%s' VkQueue successfully created retval(%p)", uvrvk->queues[qc].name, uvrvk->queues[qc].queue);
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_lgdev_create: VkDevice created retval(%p)", device);

  free(pQueueCreateInfo);
  return (struct uvr_vk_lgdev) { .logicalDevice = device, .queueCount = uvrvk->queueCount, .queues = uvrvk->queues };

err_vk_lgdev_destroy:
  if (device)
    vkDestroyDevice(device, NULL);
err_vk_lgdev_free_pQueueCreateInfo:
  free(pQueueCreateInfo);
err_vk_lgdev_create:
  return (struct uvr_vk_lgdev) { .logicalDevice = VK_NULL_HANDLE, .queueCount = -1, .queues = NULL };
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
  create_info.surface = uvrvk->surface;
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
  create_info.pQueueFamilyIndices = uvrvk->queueFamilyIndices;
  create_info.preTransform = uvrvk->surfaceCapabilities.currentTransform;
  create_info.compositeAlpha = uvrvk->compositeAlpha;
  create_info.presentMode = uvrvk->presentMode;
  create_info.clipped = uvrvk->clipped;
  create_info.oldSwapchain = uvrvk->oldSwapChain;

  res = vkCreateSwapchainKHR(uvrvk->logicalDevice, &create_info, NULL, &swapchain);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateSwapchainKHR: %s", vkres_msg(res));
    goto exit_vk_swapchain;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_swapchain_create: VkSwapchainKHR successfully created retval(%p)", swapchain);

  return (struct uvr_vk_swapchain) { .logicalDevice = uvrvk->logicalDevice, .swapChain = swapchain };

exit_vk_swapchain:
  return (struct uvr_vk_swapchain) { .logicalDevice = VK_NULL_HANDLE, .swapChain = VK_NULL_HANDLE };
}


struct uvr_vk_image uvr_vk_image_create(struct uvr_vk_image_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  struct uvr_vk_image_handle *images = NULL;
  struct uvr_vk_image_view_handle *views = NULL;

  uint32_t icount = 0, i;
  VkImage *vkimages = NULL;
  VkDeviceMemory *vkDeviceMemories = NULL;

  if (uvrvk->swapChain) {
    res = vkGetSwapchainImagesKHR(uvrvk->logicalDevice, uvrvk->swapChain, &icount, NULL);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkGetSwapchainImagesKHR: %s", vkres_msg(res));
      goto exit_vk_image;
    }

    vkimages = alloca(icount * sizeof(VkImage));

    res = vkGetSwapchainImagesKHR(uvrvk->logicalDevice, uvrvk->swapChain, &icount, vkimages);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkGetSwapchainImagesKHR: %s", vkres_msg(res));
      goto exit_vk_image;
    }

    uvr_utils_log(UVR_INFO, "uvr_vk_image_create: Total images in swapchain %u", icount);
  } else {

    VkImageCreateInfo image_create_info;
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = NULL;
    image_create_info.flags = uvrvk->imageflags;
    image_create_info.imageType = uvrvk->imageType;
    image_create_info.format = uvrvk->imageViewFormat;
    image_create_info.extent = uvrvk->imageExtent3D;
    image_create_info.mipLevels = uvrvk->imageMipLevels;
    image_create_info.arrayLayers = uvrvk->imageArrayLayers;
    image_create_info.samples = uvrvk->imageSamples;
    image_create_info.tiling = uvrvk->imageTiling;
    image_create_info.usage = uvrvk->imageUsage;
    image_create_info.sharingMode = uvrvk->imageSharingMode;
    image_create_info.queueFamilyIndexCount = uvrvk->imageQueueFamilyIndexCount;
    image_create_info.pQueueFamilyIndices = uvrvk->imageQueueFamilyIndices;
    image_create_info.initialLayout = uvrvk->imageInitialLayout;

    icount = uvrvk->imageCount;
    vkimages = alloca(icount * sizeof(VkImage));
    vkDeviceMemories = alloca(icount * sizeof(VkDeviceMemory));

    memset(vkimages, 0, icount * sizeof(VkImage));
    memset(vkDeviceMemories, 0, icount * sizeof(VkDeviceMemory));

    for (i = 0; i < icount; i++) {
      res = vkCreateImage(uvrvk->logicalDevice, &image_create_info, NULL, &vkimages[i]);
      if (res) {
        uvr_utils_log(UVR_DANGER, "[x] vkCreateImage: %s", vkres_msg(res));
        goto exit_vk_image_free_images;
      }

      VkMemoryRequirements mem_reqs;
      vkGetImageMemoryRequirements(uvrvk->logicalDevice, vkimages[i], &mem_reqs);

      VkMemoryAllocateInfo alloc_info = {};
      alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
      alloc_info.allocationSize = mem_reqs.size;
      alloc_info.memoryTypeIndex = retrieve_memory_type_index(uvrvk->physDevice, mem_reqs.memoryTypeBits, uvrvk->memPropertyFlags);

      res = vkAllocateMemory(uvrvk->logicalDevice, &alloc_info, NULL, &vkDeviceMemories[i]);
      if (res) {
        uvr_utils_log(UVR_DANGER, "[x] vkAllocateMemory: %s", vkres_msg(res));
        goto exit_vk_image_free_images;
      }

      vkBindImageMemory(uvrvk->logicalDevice, vkimages[i], vkDeviceMemories[i], 0);
    }
  }

  images = calloc(icount, sizeof(struct uvr_vk_image_handle));
  if (!images) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_vk_image_free_images;
  }

  views = calloc(icount, sizeof(struct uvr_vk_image_view_handle));
  if (!views) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_vk_image_free_images;
  }

  VkImageViewCreateInfo image_view_create_info;
  image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  image_view_create_info.pNext = NULL;
  image_view_create_info.flags = uvrvk->imageViewflags;
  image_view_create_info.viewType = uvrvk->imageViewType;
  image_view_create_info.format = uvrvk->imageViewFormat;
  image_view_create_info.components = uvrvk->imageViewComponents;
  image_view_create_info.subresourceRange = uvrvk->imageViewSubresourceRange;

  for (i = 0; i < icount; i++) {
    image_view_create_info.image = images[i].image = vkimages[i];
    images[i].deviceMemory = VK_NULL_HANDLE;

    if (vkDeviceMemories) {
      if (vkDeviceMemories[i])
        images[i].deviceMemory = vkDeviceMemories[i];
    }

    res = vkCreateImageView(uvrvk->logicalDevice, &image_view_create_info, NULL, &views[i].view);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkCreateImageView: %s", vkres_msg(res));
      goto exit_vk_image_free_image_views;
    }

    uvr_utils_log(UVR_WARNING, "uvr_vk_image_create: VkImage (%p) associate with VkImageView", images[i].image);
    uvr_utils_log(UVR_SUCCESS, "uvr_vk_image_create: VkImageView successfully created retval(%p)", views[i].view);
  }

  return (struct uvr_vk_image) { .logicalDevice = uvrvk->logicalDevice, .imageCount = icount, .imageHandles = images,
                                 .imageViewHandles = views, .swapChain = uvrvk->swapChain };

exit_vk_image_free_image_views:
  if (views) {
    for (i = 0; i < icount; i++) {
      if (views[i].view)
        vkDestroyImageView(uvrvk->logicalDevice, views[i].view, NULL);
    }
  }
  free(views);
exit_vk_image_free_images:
  if (vkimages && vkDeviceMemories && !uvrvk->swapChain) {
    for (i = 0; i < icount; i++) {
      if (vkimages[i])
        vkDestroyImage(uvrvk->logicalDevice, vkimages[i], NULL);
      if (vkDeviceMemories[i])
        vkFreeMemory(uvrvk->logicalDevice, vkDeviceMemories[i], NULL);
    }
  }
  free(images);
exit_vk_image:
  return (struct uvr_vk_image) { .logicalDevice = VK_NULL_HANDLE, .imageCount = 0, .imageHandles = NULL,
                                 .imageViewHandles = NULL, .swapChain = VK_NULL_HANDLE };
}


struct uvr_vk_shader_module uvr_vk_shader_module_create(struct uvr_vk_shader_module_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkShaderModule shaderModule = VK_NULL_HANDLE;

  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.codeSize = uvrvk->sprivByteSize;
  create_info.pCode = (const uint32_t *) uvrvk->sprivBytes;

  res = vkCreateShaderModule(uvrvk->logicalDevice, &create_info, NULL, &shaderModule);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateShaderModule: %s", vkres_msg(res));
    goto exit_vk_shader_module;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_shader_module_create: '%s' shader VkShaderModule successfully created retval(%p)", uvrvk->shaderName, shaderModule);

  return (struct uvr_vk_shader_module) { .logicalDevice = uvrvk->logicalDevice, .shaderModule = shaderModule, .shaderName = uvrvk->shaderName };

exit_vk_shader_module:
  return (struct uvr_vk_shader_module) { .logicalDevice = VK_NULL_HANDLE, .shaderModule = VK_NULL_HANDLE, .shaderName = NULL };
}


struct uvr_vk_pipeline_layout uvr_vk_pipeline_layout_create(struct uvr_vk_pipeline_layout_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkPipelineLayout playout = VK_NULL_HANDLE;

  VkPipelineLayoutCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.setLayoutCount = uvrvk->descriptorSetLayoutCount;
  create_info.pSetLayouts = uvrvk->descriptorSetLayouts;
  create_info.pushConstantRangeCount = uvrvk->pushConstantRangeCount;
  create_info.pPushConstantRanges = uvrvk->pushConstantRanges;

  res = vkCreatePipelineLayout(uvrvk->logicalDevice, &create_info, NULL, &playout);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreatePipelineLayout: %s", vkres_msg(res));
    goto exit_vk_pipeline_layout;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_pipeline_layout_create: VkPipelineLayout successfully created retval(%p)", playout);

  return (struct uvr_vk_pipeline_layout) { .logicalDevice = uvrvk->logicalDevice, .pipelineLayout = playout };

exit_vk_pipeline_layout:
  return (struct uvr_vk_pipeline_layout) { .logicalDevice = VK_NULL_HANDLE, .pipelineLayout = VK_NULL_HANDLE };
}


struct uvr_vk_render_pass uvr_vk_render_pass_create(struct uvr_vk_render_pass_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkRenderPass renderpass = VK_NULL_HANDLE;

  VkRenderPassCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.attachmentCount = uvrvk->attachmentDescriptionCount;
  create_info.pAttachments = uvrvk->attachmentDescriptions;
  create_info.subpassCount = uvrvk->subpassDescriptionCount;
  create_info.pSubpasses = uvrvk->subpassDescriptions;
  create_info.dependencyCount = uvrvk->subpassDependencyCount;
  create_info.pDependencies = uvrvk->subpassDependencies;

  res = vkCreateRenderPass(uvrvk->logicalDevice, &create_info, NULL, &renderpass);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateRenderPass: %s", vkres_msg(res));
    goto exit_vk_render_pass;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_render_pass_create: VkRenderPass successfully created retval(%p)", renderpass);

  return (struct uvr_vk_render_pass) { .logicalDevice = uvrvk->logicalDevice, .renderPass = renderpass };

exit_vk_render_pass:
  return (struct uvr_vk_render_pass) { .logicalDevice = VK_NULL_HANDLE, .renderPass = VK_NULL_HANDLE };
}


struct uvr_vk_graphics_pipeline uvr_vk_graphics_pipeline_create(struct uvr_vk_graphics_pipeline_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkPipeline pipeline = VK_NULL_HANDLE;

  VkGraphicsPipelineCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  create_info.pNext = NULL;
  create_info.flags = 0;
  create_info.stageCount = uvrvk->shaderStageCount;
  create_info.pStages = uvrvk->shaderStages;
  create_info.pVertexInputState = uvrvk->vertexInputState;
  create_info.pInputAssemblyState = uvrvk->inputAssemblyState;
  create_info.pTessellationState = uvrvk->tessellationState;
  create_info.pViewportState = uvrvk->viewportState;
  create_info.pRasterizationState = uvrvk->rasterizationState;
  create_info.pMultisampleState = uvrvk->multisampleState;
  create_info.pDepthStencilState = uvrvk->depthStencilState;
  create_info.pColorBlendState = uvrvk->colorBlendState;
  create_info.pDynamicState = uvrvk->dynamicState;
  create_info.layout = uvrvk->pipelineLayout;
  create_info.renderPass = uvrvk->renderPass;
  create_info.subpass = uvrvk->subpass;
  // Won't be supporting
  create_info.basePipelineHandle = VK_NULL_HANDLE;
  create_info.basePipelineIndex = -1;

  res = vkCreateGraphicsPipelines(uvrvk->logicalDevice, NULL, 1, &create_info, NULL, &pipeline);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateGraphicsPipelines: %s", vkres_msg(res));
    goto exit_vk_graphics_pipeline;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_graphics_pipeline_create: VkPipeline successfully created retval(%p)", pipeline);

  return (struct uvr_vk_graphics_pipeline) { .logicalDevice = uvrvk->logicalDevice, .graphicsPipeline = pipeline };

exit_vk_graphics_pipeline:
  return (struct uvr_vk_graphics_pipeline) { .logicalDevice = VK_NULL_HANDLE, .graphicsPipeline = VK_NULL_HANDLE };
}


struct uvr_vk_framebuffer uvr_vk_framebuffer_create(struct uvr_vk_framebuffer_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  struct uvr_vk_framebuffer_handle *vkfbs = NULL;
  uint32_t fbc;

  vkfbs = (struct uvr_vk_framebuffer_handle *) calloc(uvrvk->frameBufferCount, sizeof(struct uvr_vk_framebuffer_handle));
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
    create_info.pAttachments = &uvrvk->imageViewHandles[fbc].view;

    res = vkCreateFramebuffer(uvrvk->logicalDevice, &create_info, NULL, &vkfbs[fbc].frameBuffer);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkCreateFramebuffer: %s", vkres_msg(res));
      goto exit_vk_framebuffer_vk_framebuffer_destroy;
    }

    uvr_utils_log(UVR_SUCCESS, "uvr_vk_framebuffer_create: VkFramebuffer successfully created retval(%p)", vkfbs[fbc].frameBuffer);
  }

  return (struct uvr_vk_framebuffer) { .logicalDevice = uvrvk->logicalDevice, .frameBufferCount = uvrvk->frameBufferCount, .frameBufferHandles = vkfbs };

exit_vk_framebuffer_vk_framebuffer_destroy:
  for (fbc = 0; fbc < uvrvk->frameBufferCount; fbc++) {
    if (vkfbs[fbc].frameBuffer)
      vkDestroyFramebuffer(uvrvk->logicalDevice, vkfbs[fbc].frameBuffer, NULL);
  }
//exit_vk_framebuffer_free_vkfbs:
  free(vkfbs);
exit_vk_framebuffer:
  return (struct uvr_vk_framebuffer) { .logicalDevice = VK_NULL_HANDLE, .frameBufferCount = 0, .frameBufferHandles = NULL };
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

  res = vkCreateCommandPool(uvrvk->logicalDevice, &create_info, NULL, &cmdpool);
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

  res = vkAllocateCommandBuffers(uvrvk->logicalDevice, &alloc_info, cmdbuffs);
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
    cbuffs[i].commandBuffer = cmdbuffs[i];
    uvr_utils_log(UVR_WARNING, "uvr_vk_command_buffer_create: VkCommandBuffer successfully created retval(%p)", cbuffs[i].commandBuffer);
  }

  return (struct uvr_vk_command_buffer) { .logicalDevice = uvrvk->logicalDevice, .commandPool = cmdpool,
                                          .commandBufferCount = uvrvk->commandBufferCount,
                                          .commandBuffers = cbuffs };

exit_vk_command_buffer_destroy_cmd_pool:
  vkDestroyCommandPool(uvrvk->logicalDevice, cmdpool, NULL);
exit_vk_command_buffer:
  return (struct uvr_vk_command_buffer) { .logicalDevice = VK_NULL_HANDLE, .commandPool = VK_NULL_HANDLE,
                                          .commandBufferCount = 0, .commandBuffers = NULL };
}


int uvr_vk_command_buffer_record_begin(struct uvr_vk_command_buffer_record_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;

  VkCommandBufferBeginInfo begin_info = {};
  begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  begin_info.pNext = NULL;
  begin_info.flags = uvrvk->commandBufferUsageflags;
  // We don't use secondary command buffers in API so set to null
  begin_info.pInheritanceInfo = NULL;

  for (uint32_t i = 0; i < uvrvk->commandBufferCount; i++) {
    res = vkBeginCommandBuffer(uvrvk->commandBuffers[i].commandBuffer, &begin_info);
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
    res = vkEndCommandBuffer(uvrvk->commandBuffers[i].commandBuffer);
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

  vkFences = calloc(uvrvk->fenceCount, sizeof(struct uvr_vk_fence_handle));
  if (!vkFences) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_vk_sync_obj;
  }

  vkSemaphores = calloc(uvrvk->semaphoreCount, sizeof(struct uvr_vk_semaphore_handle));
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
    res = vkCreateFence(uvrvk->logicalDevice, &fence_create_info, NULL, &vkFences[s].fence);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkCreateFence: %s", vkres_msg(res));
      goto exit_vk_sync_obj_destroy_vk_fence;
    }
  }

  for (s = 0; s < uvrvk->semaphoreCount; s++) {
    res = vkCreateSemaphore(uvrvk->logicalDevice, &semphore_create_info, NULL, &vkSemaphores[s].semaphore);
    if (res) {
      uvr_utils_log(UVR_DANGER, "[x] vkCreateSemaphore: %s", vkres_msg(res));
      goto exit_vk_sync_obj_destroy_vk_semaphore;
    }
  }

  return (struct uvr_vk_sync_obj) { .logicalDevice = uvrvk->logicalDevice, .fenceCount = uvrvk->fenceCount, .fenceHandles = vkFences,
                                    .semaphoreCount = uvrvk->semaphoreCount, .semaphoreHandles = vkSemaphores };


exit_vk_sync_obj_destroy_vk_semaphore:
  for (s = 0; s < uvrvk->semaphoreCount; s++)
    if (vkSemaphores[s].semaphore)
      vkDestroySemaphore(uvrvk->logicalDevice, vkSemaphores[s].semaphore, NULL);
exit_vk_sync_obj_destroy_vk_fence:
  for (s = 0; s < uvrvk->fenceCount; s++)
    if (vkFences[s].fence)
      vkDestroyFence(uvrvk->logicalDevice, vkFences[s].fence, NULL);
//exit_vk_sync_obj_free_vk_semaphore:
  free(vkSemaphores);
exit_vk_sync_obj_free_vk_fence:
  free(vkFences);
exit_vk_sync_obj:
  return (struct uvr_vk_sync_obj) { .logicalDevice = VK_NULL_HANDLE, .fenceCount = 0, .fenceHandles = NULL, .semaphoreCount = 0, .semaphoreHandles = NULL };
};


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
  res = vkCreateBuffer(uvrvk->logicalDevice, &buffer_create_info, NULL, &buffer);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateBuffer: %s", vkres_msg(res));
    goto exit_vk_buffer;
  }

  VkMemoryRequirements mem_reqs;
  vkGetBufferMemoryRequirements(uvrvk->logicalDevice, buffer, &mem_reqs);

  VkMemoryAllocateInfo alloc_info = {};
  alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  alloc_info.allocationSize = mem_reqs.size;
  alloc_info.memoryTypeIndex = retrieve_memory_type_index(uvrvk->physDevice, mem_reqs.memoryTypeBits, uvrvk->memPropertyFlags);

  res = vkAllocateMemory(uvrvk->logicalDevice, &alloc_info, NULL, &memory);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkAllocateMemory: %s", vkres_msg(res));
    goto exit_vk_buffer;
  }

  vkBindBufferMemory(uvrvk->logicalDevice, buffer, memory, 0);

  return (struct uvr_vk_buffer) { .logicalDevice = uvrvk->logicalDevice, .buffer = buffer, .deviceMemory = memory };

exit_vk_buffer:
  return (struct uvr_vk_buffer) { .logicalDevice = VK_NULL_HANDLE, .buffer = VK_NULL_HANDLE, .deviceMemory = VK_NULL_HANDLE };
}


struct uvr_vk_descriptor_set_layout uvr_vk_descriptor_set_layout_create(struct uvr_vk_descriptor_set_layout_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;
  VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

  VkDescriptorSetLayoutCreateInfo desc_layout_create_info;
  desc_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  desc_layout_create_info.pNext = NULL;
  desc_layout_create_info.flags = uvrvk->descriptorSetLayoutCreateflags;
  desc_layout_create_info.bindingCount = uvrvk->descriptorSetLayoutBindingCount;
  desc_layout_create_info.pBindings = uvrvk->descriptorSetLayoutBindings;

  res = vkCreateDescriptorSetLayout(uvrvk->logicalDevice, &desc_layout_create_info, VK_NULL_HANDLE, &descriptorSetLayout);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateDescriptorSetLayout: %s", vkres_msg(res));
    goto exit_vk_descriptor_set_layout;
  }

  return (struct uvr_vk_descriptor_set_layout) { .logicalDevice = uvrvk->logicalDevice, .descriptorSetLayout = descriptorSetLayout };

exit_vk_descriptor_set_layout:
  return (struct uvr_vk_descriptor_set_layout) { .logicalDevice = VK_NULL_HANDLE, .descriptorSetLayout = VK_NULL_HANDLE };
}


struct uvr_vk_descriptor_set uvr_vk_descriptor_set_create(struct uvr_vk_descriptor_set_create_info *uvrvk) {
  VkResult res = VK_RESULT_MAX_ENUM;

  struct uvr_vk_descriptor_set_handle *descriptorSets = NULL;
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
  VkDescriptorSet *descriptorsets = VK_NULL_HANDLE;

  /*
   * Per my understanding allocate VkDescriptorPool given a certain amount of information. Like the amount of sets,
   * the layout for a given set, and descriptors. No descriptor is assigned to a set when the pool is created.
   * Given a descriptor set layout the actual assignment of descriptor to descriptor set happens in the
   * vkAllocateDescriptorSets function.
   */
  VkDescriptorPoolCreateInfo desc_pool_create_info;
  desc_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  desc_pool_create_info.pNext = NULL;
  desc_pool_create_info.flags = uvrvk->descriptorPoolCreateflags;
  desc_pool_create_info.maxSets = uvrvk->descriptorSetLayoutCount;
  desc_pool_create_info.poolSizeCount = uvrvk->descriptorPoolInfoCount;
  desc_pool_create_info.pPoolSizes = uvrvk->descriptorPoolInfos;

  res = vkCreateDescriptorPool(uvrvk->logicalDevice, &desc_pool_create_info, VK_NULL_HANDLE, &descriptorPool);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateDescriptorSetLayout: %s", vkres_msg(res));
    goto exit_vk_descriptor_set;
  }

  descriptorsets = alloca(uvrvk->descriptorSetLayoutCount * sizeof(VkDescriptorSet));

  VkDescriptorSetAllocateInfo desc_set_alloc_info;
  desc_set_alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  desc_set_alloc_info.pNext = NULL;
  desc_set_alloc_info.descriptorPool = descriptorPool;
  desc_set_alloc_info.descriptorSetCount = uvrvk->descriptorSetLayoutCount;
  desc_set_alloc_info.pSetLayouts = uvrvk->descriptorSetLayouts;

  res = vkAllocateDescriptorSets(uvrvk->logicalDevice, &desc_set_alloc_info, descriptorsets);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkAllocateDescriptorSets: %s", vkres_msg(res));
    goto exit_vk_descriptor_set_destroy_pool;
  }

  descriptorSets = calloc(uvrvk->descriptorSetLayoutCount, sizeof(struct uvr_vk_descriptor_set_handle));
  if (!descriptorSets) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_vk_descriptor_set_destroy_pool;
  }

  for (uint32_t i = 0; i < uvrvk->descriptorSetLayoutCount; i++) {
    descriptorSets[i].descriptorSet = descriptorsets[i];
    uvr_utils_log(UVR_WARNING, "uvr_vk_descriptor_set_create: VkDescriptorSet successfully created retval(%p)", descriptorSets[i].descriptorSet);
  }

  return (struct uvr_vk_descriptor_set) { .logicalDevice = uvrvk->logicalDevice, .descriptorPool = descriptorPool, .descriptorSetHandles = descriptorSets, .descriptorSetsCount = uvrvk->descriptorSetLayoutCount };

exit_vk_descriptor_set_destroy_pool:
  if (descriptorPool)
    vkDestroyDescriptorPool(uvrvk->logicalDevice, descriptorPool, NULL);
exit_vk_descriptor_set:
  return (struct uvr_vk_descriptor_set) { .logicalDevice = VK_NULL_HANDLE, .descriptorPool = VK_NULL_HANDLE, .descriptorSetHandles = VK_NULL_HANDLE, .descriptorSetsCount = 0 };
}


struct uvr_vk_sampler uvr_vk_sampler_create(struct uvr_vk_sampler_create_info *uvrvk) {
  VkSampler vkSampler = VK_NULL_HANDLE;
  VkResult res = VK_RESULT_MAX_ENUM;

  VkSamplerCreateInfo sampler_create_info = {};
  sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  sampler_create_info.pNext = NULL;
  sampler_create_info.flags = uvrvk->samplerFlags;
  sampler_create_info.magFilter = uvrvk->samplerMagFilter;
  sampler_create_info.minFilter = uvrvk->samplerMinFilter;
  sampler_create_info.mipmapMode = uvrvk->samplerMipmapMode;
  sampler_create_info.addressModeU = uvrvk->samplerAddressModeU;
  sampler_create_info.addressModeV = uvrvk->samplerAddressModeV;
  sampler_create_info.addressModeW = uvrvk->samplerAddressModeW;
  sampler_create_info.mipLodBias = uvrvk->samplerMipLodBias;
  sampler_create_info.anisotropyEnable = uvrvk->samplerAnisotropyEnable;
  sampler_create_info.maxAnisotropy = uvrvk->samplerMaxAnisotropy;
  sampler_create_info.compareEnable = uvrvk->samplerCompareEnable;
  sampler_create_info.compareOp = uvrvk->samplerCompareOp;
  sampler_create_info.minLod = uvrvk->samplerMinLod;
  sampler_create_info.maxLod = uvrvk->samplerMaxLod;
  sampler_create_info.borderColor = uvrvk->samplerBorderColor;
  sampler_create_info.unnormalizedCoordinates = uvrvk->samplerUnnormalizedCoordinates;

  res = vkCreateSampler(uvrvk->logicalDevice, &sampler_create_info, NULL, &vkSampler);
  if (res) {
    uvr_utils_log(UVR_DANGER, "[x] vkCreateSampler: %s", vkres_msg(res));
    goto exit_vk_sampler;
  }

  uvr_utils_log(UVR_SUCCESS, "uvr_vk_sampler_create: VkSampler created retval(%p)", vkSampler);

  return (struct uvr_vk_sampler) { .logicalDevice = uvrvk->logicalDevice, .sampler = vkSampler };

exit_vk_sampler:
  return (struct uvr_vk_sampler) { .logicalDevice = VK_NULL_HANDLE, .sampler = VK_NULL_HANDLE };
}


int uvr_vk_resource_copy(struct uvr_vk_resource_copy_info *uvrvk) {
  struct uvr_vk_command_buffer_handle command_buffer_handle;
  command_buffer_handle.commandBuffer = uvrvk->commandBuffer;

  struct uvr_vk_command_buffer_record_info command_buffer_record_info;
  command_buffer_record_info.commandBufferCount = 1;
  command_buffer_record_info.commandBuffers = &command_buffer_handle;
  command_buffer_record_info.commandBufferUsageflags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (uvr_vk_command_buffer_record_begin(&command_buffer_record_info) == -1)
    return -1;

  VkCommandBuffer cmdBuffer = command_buffer_handle.commandBuffer;

  switch (uvrvk->resourceCopyType) {
    case UVR_VK_COPY_VK_BUFFER_TO_VK_BUFFER:
    {
      vkCmdCopyBuffer(cmdBuffer, (VkBuffer) uvrvk->srcResource, (VkBuffer) uvrvk->dstResource, 1, uvrvk->bufferCopyInfo);
      break;
    }
    case UVR_VK_COPY_VK_BUFFER_TO_VK_IMAGE:
    {
      vkCmdCopyBufferToImage(cmdBuffer, (VkBuffer) uvrvk->srcResource, (VkImage) uvrvk->dstResource, uvrvk->imageLayout, 1, uvrvk->bufferImageCopyInfo);
      break;
    }
    case UVR_VK_COPY_VK_IMAGE_TO_VK_BUFFER:
    {
      vkCmdCopyImageToBuffer(cmdBuffer, (VkImage) uvrvk->srcResource, uvrvk->imageLayout, (VkBuffer) uvrvk->dstResource,  1, uvrvk->bufferImageCopyInfo);
      break;
    }
    default:
      uvr_utils_log(UVR_DANGER, "[x] uvr_vk_buffer_copy: Must pass a valid uvr_vk_buffer_copy_type value");
      return -1;
  }

  if (uvr_vk_command_buffer_record_end(&command_buffer_record_info) == -1)
    return -1;

  VkSubmitInfo submit_info;
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = NULL;
  submit_info.waitSemaphoreCount = 0;
  submit_info.pWaitSemaphores = NULL;
  submit_info.pWaitDstStageMask = NULL;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &cmdBuffer;
  submit_info.signalSemaphoreCount = 0;
  submit_info.pSignalSemaphores = NULL;

  vkQueueSubmit(uvrvk->queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(uvrvk->queue);

  return 0;
}


int uvr_vk_resource_pipeline_barrier(struct uvr_vk_resource_pipeline_barrier_info *uvrvk) {
  struct uvr_vk_command_buffer_handle command_buffer_handle;
  command_buffer_handle.commandBuffer = uvrvk->commandBuffer;

  struct uvr_vk_command_buffer_record_info command_buffer_record_info;
  command_buffer_record_info.commandBufferCount = 1;
  command_buffer_record_info.commandBuffers = &command_buffer_handle;
  command_buffer_record_info.commandBufferUsageflags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  if (uvr_vk_command_buffer_record_begin(&command_buffer_record_info) == -1)
    return -1;

  vkCmdPipelineBarrier(command_buffer_handle.commandBuffer,
                       uvrvk->srcPipelineStage,
                       uvrvk->dstPipelineStage,
                       uvrvk->dependencyFlags,
                       (uvrvk->memoryBarrier      ) ? 1 : 0, uvrvk->memoryBarrier,
                       (uvrvk->bufferMemoryBarrier) ? 1 : 0, uvrvk->bufferMemoryBarrier,
                       (uvrvk->imageMemoryBarrier ) ? 1 : 0, uvrvk->imageMemoryBarrier);

  if (uvr_vk_command_buffer_record_end(&command_buffer_record_info) == -1)
    return -1;

  VkSubmitInfo submit_info;
  submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submit_info.pNext = NULL;
  submit_info.waitSemaphoreCount = 0;
  submit_info.pWaitSemaphores = NULL;
  submit_info.pWaitDstStageMask = NULL;
  submit_info.commandBufferCount = 1;
  submit_info.pCommandBuffers = &command_buffer_handle.commandBuffer;
  submit_info.signalSemaphoreCount = 0;
  submit_info.pSignalSemaphores = NULL;

  vkQueueSubmit(uvrvk->queue, 1, &submit_info, VK_NULL_HANDLE);
  vkQueueWaitIdle(uvrvk->queue);

  return 0;
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

  formats = (VkSurfaceFormatKHR *) calloc(fcount, sizeof(VkSurfaceFormatKHR));
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

  modes = (VkPresentModeKHR *) calloc(mcount, sizeof(VkPresentModeKHR));
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


struct uvr_vk_phdev_format_prop uvr_vk_get_phdev_format_properties(VkPhysicalDevice phdev, VkFormat *formats, uint32_t formatCount) {
  VkFormatProperties *formatProperties = NULL;

  formatProperties = (VkFormatProperties *) calloc(formatCount, sizeof(VkFormatProperties));
  if (!formatProperties) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto exit_get_phdev_format_properties;
  }

  for (uint32_t f = 0; f < formatCount; f++) {
    vkGetPhysicalDeviceFormatProperties(phdev, formats[f], &formatProperties[f]);
  }

  return (struct uvr_vk_phdev_format_prop) { .formatProperties = formatProperties, .formatPropertyCount = formatCount };

exit_get_phdev_format_properties:
  free(formatProperties);
  return (struct uvr_vk_phdev_format_prop) { .formatProperties = NULL, .formatPropertyCount = 0 };
}


void uvr_vk_destory(struct uvr_vk_destroy *uvrvk) {
  uint32_t i, j;

  for (i = 0; i < uvrvk->uvr_vk_lgdev_cnt; i++) {
    if (uvrvk->uvr_vk_lgdev[i].logicalDevice) {
      vkDeviceWaitIdle(uvrvk->uvr_vk_lgdev[i].logicalDevice);
    }
  }

  if (uvrvk->uvr_vk_sync_obj) {
    for (i = 0; i < uvrvk->uvr_vk_sync_obj_cnt; i++) {
      for (j = 0; j < uvrvk->uvr_vk_sync_obj[i].fenceCount; j++) {
        if (uvrvk->uvr_vk_sync_obj[i].fenceHandles[j].fence) {
          vkDestroyFence(uvrvk->uvr_vk_sync_obj[i].logicalDevice, uvrvk->uvr_vk_sync_obj[i].fenceHandles[j].fence, NULL);
        }
      }
      for (j = 0; j < uvrvk->uvr_vk_sync_obj[i].semaphoreCount; j++) {
        if (uvrvk->uvr_vk_sync_obj[i].semaphoreHandles[j].semaphore) {
          vkDestroySemaphore(uvrvk->uvr_vk_sync_obj[i].logicalDevice, uvrvk->uvr_vk_sync_obj[i].semaphoreHandles[j].semaphore, NULL);
        }
      }
      free(uvrvk->uvr_vk_sync_obj[i].fenceHandles);
      free(uvrvk->uvr_vk_sync_obj[i].semaphoreHandles);
    }
  }

  if (uvrvk->uvr_vk_command_buffer) {
    for (i = 0; i < uvrvk->uvr_vk_command_buffer_cnt; i++) {
      if (uvrvk->uvr_vk_command_buffer[i].logicalDevice && uvrvk->uvr_vk_command_buffer[i].commandPool)
        vkDestroyCommandPool(uvrvk->uvr_vk_command_buffer[i].logicalDevice, uvrvk->uvr_vk_command_buffer[i].commandPool, NULL);
      free(uvrvk->uvr_vk_command_buffer[i].commandBuffers);
    }
  }

  if (uvrvk->uvr_vk_buffer) {
    for (i = 0; i < uvrvk->uvr_vk_buffer_cnt; i++) {
      if (uvrvk->uvr_vk_buffer[i].buffer)
        vkDestroyBuffer(uvrvk->uvr_vk_buffer[i].logicalDevice, uvrvk->uvr_vk_buffer[i].buffer, NULL);
      if (uvrvk->uvr_vk_buffer[i].deviceMemory)
        vkFreeMemory(uvrvk->uvr_vk_buffer[i].logicalDevice, uvrvk->uvr_vk_buffer[i].deviceMemory, NULL);
    }
  }

  if (uvrvk->uvr_vk_descriptor_set) {
    for (i = 0; i < uvrvk->uvr_vk_descriptor_set_cnt; i++) {
      if (uvrvk->uvr_vk_descriptor_set[i].logicalDevice && uvrvk->uvr_vk_descriptor_set[i].descriptorPool)
        vkDestroyDescriptorPool(uvrvk->uvr_vk_descriptor_set[i].logicalDevice, uvrvk->uvr_vk_descriptor_set[i].descriptorPool, NULL);
      free(uvrvk->uvr_vk_descriptor_set[i].descriptorSetHandles);
    }
  }

  if (uvrvk->uvr_vk_descriptor_set_layout) {
    for (i = 0; i < uvrvk->uvr_vk_descriptor_set_layout_cnt; i++) {
      if (uvrvk->uvr_vk_descriptor_set_layout[i].logicalDevice && uvrvk->uvr_vk_descriptor_set_layout[i].descriptorSetLayout)
        vkDestroyDescriptorSetLayout(uvrvk->uvr_vk_descriptor_set_layout[i].logicalDevice, uvrvk->uvr_vk_descriptor_set_layout[i].descriptorSetLayout, NULL);
    }
  }

  if (uvrvk->uvr_vk_framebuffer) {
    for (i = 0; i < uvrvk->uvr_vk_framebuffer_cnt; i++) {
      for (j = 0; j < uvrvk->uvr_vk_framebuffer[i].frameBufferCount; j++) {
        if (uvrvk->uvr_vk_framebuffer[i].logicalDevice && uvrvk->uvr_vk_framebuffer[i].frameBufferHandles[j].frameBuffer)
          vkDestroyFramebuffer(uvrvk->uvr_vk_framebuffer[i].logicalDevice, uvrvk->uvr_vk_framebuffer[i].frameBufferHandles[j].frameBuffer, NULL);
      }
      free(uvrvk->uvr_vk_framebuffer[i].frameBufferHandles);
    }
  }

  if (uvrvk->uvr_vk_graphics_pipeline) {
    for (i = 0; i < uvrvk->uvr_vk_graphics_pipeline_cnt; i++) {
      if (uvrvk->uvr_vk_graphics_pipeline[i].logicalDevice && uvrvk->uvr_vk_graphics_pipeline[i].graphicsPipeline)
        vkDestroyPipeline(uvrvk->uvr_vk_graphics_pipeline[i].logicalDevice, uvrvk->uvr_vk_graphics_pipeline[i].graphicsPipeline, NULL);
    }
  }

  if (uvrvk->uvr_vk_pipeline_layout) {
    for (i = 0; i < uvrvk->uvr_vk_pipeline_layout_cnt; i++) {
      if (uvrvk->uvr_vk_pipeline_layout[i].logicalDevice && uvrvk->uvr_vk_pipeline_layout[i].pipelineLayout)
        vkDestroyPipelineLayout(uvrvk->uvr_vk_pipeline_layout[i].logicalDevice, uvrvk->uvr_vk_pipeline_layout[i].pipelineLayout, NULL);
    }
  }

  if (uvrvk->uvr_vk_render_pass) {
    for (i = 0; i < uvrvk->uvr_vk_render_pass_cnt; i++) {
      if (uvrvk->uvr_vk_render_pass[i].logicalDevice && uvrvk->uvr_vk_render_pass[i].renderPass)
        vkDestroyRenderPass(uvrvk->uvr_vk_render_pass[i].logicalDevice, uvrvk->uvr_vk_render_pass[i].renderPass, NULL);
    }
  }

  if (uvrvk->uvr_vk_shader_module) {
    for (i = 0; i < uvrvk->uvr_vk_shader_module_cnt; i++) {
      if (uvrvk->uvr_vk_shader_module[i].logicalDevice && uvrvk->uvr_vk_shader_module[i].shaderModule)
        vkDestroyShaderModule(uvrvk->uvr_vk_shader_module[i].logicalDevice, uvrvk->uvr_vk_shader_module[i].shaderModule, NULL);
    }
  }

  if (uvrvk->uvr_vk_sampler) {
    for (i = 0; i < uvrvk->uvr_vk_sampler_cnt; i++) {
      if (uvrvk->uvr_vk_sampler[i].logicalDevice && uvrvk->uvr_vk_sampler[i].sampler)
        vkDestroySampler(uvrvk->uvr_vk_sampler[i].logicalDevice, uvrvk->uvr_vk_sampler[i].sampler, NULL);
    }
  }

  if (uvrvk->uvr_vk_image) {
    for (i = 0; i < uvrvk->uvr_vk_image_cnt; i++) {
      for (j = 0; j < uvrvk->uvr_vk_image[i].imageCount; j++) {
        if (uvrvk->uvr_vk_image[i].logicalDevice && uvrvk->uvr_vk_image[i].imageHandles[j].image && !uvrvk->uvr_vk_image[i].swapChain)
          vkDestroyImage(uvrvk->uvr_vk_image[i].logicalDevice, uvrvk->uvr_vk_image[i].imageHandles[j].image, NULL);
        if (uvrvk->uvr_vk_image[i].logicalDevice && uvrvk->uvr_vk_image[i].imageHandles[j].deviceMemory && !uvrvk->uvr_vk_image[i].swapChain)
          vkFreeMemory(uvrvk->uvr_vk_image[i].logicalDevice, uvrvk->uvr_vk_image[i].imageHandles[j].deviceMemory, NULL);
        if (uvrvk->uvr_vk_image[i].logicalDevice && uvrvk->uvr_vk_image[i].imageViewHandles[j].view)
          vkDestroyImageView(uvrvk->uvr_vk_image[i].logicalDevice, uvrvk->uvr_vk_image[i].imageViewHandles[j].view, NULL);
      }
      free(uvrvk->uvr_vk_image[i].imageHandles);
      free(uvrvk->uvr_vk_image[i].imageViewHandles);
    }
  }

  if (uvrvk->uvr_vk_swapchain) {
    for (i = 0; i < uvrvk->uvr_vk_swapchain_cnt; i++) {
      if (uvrvk->uvr_vk_swapchain[i].logicalDevice && uvrvk->uvr_vk_swapchain[i].swapChain)
        vkDestroySwapchainKHR(uvrvk->uvr_vk_swapchain[i].logicalDevice, uvrvk->uvr_vk_swapchain[i].swapChain, NULL);
    }
  }

  for (i = 0; i < uvrvk->uvr_vk_lgdev_cnt; i++) {
    if (uvrvk->uvr_vk_lgdev[i].logicalDevice) {
      vkDestroyDevice(uvrvk->uvr_vk_lgdev[i].logicalDevice, NULL);
    }
  }

  if (uvrvk->surface)
    vkDestroySurfaceKHR(uvrvk->instance, uvrvk->surface, NULL);
  if (uvrvk->instance)
    vkDestroyInstance(uvrvk->instance, NULL);
}
