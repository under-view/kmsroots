
#ifndef UVC_VULKAN_H
#define UVC_VULKAN_H

#include "common.h"

#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

typedef struct _uvcvk {
  VkInstance instance;
} uvcvk;

VkInstance uvc_vk_create_instance(const char *app_name,
                                  const char *engine_name,
                                  uint32_t enabledLayerCount,
                                  const char *ppEnabledLayerNames[],
                                  uint32_t enabledExtensionCount,
                                  const char *ppEnabledExtensionNames[]);

void uvc_vk_destory(uvcvk *app);

#endif
