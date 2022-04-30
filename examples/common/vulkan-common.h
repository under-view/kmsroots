#ifndef VULKAN_COMMON_H
#define VULKAN_COMMON_H

#include "vulkan.h"

/*
 * struct uvrvk (Underview Renderer Vulkan)
 *
 * @instance - Keeps track of all application state. The connection to the vulkan loader.
 * @surface  - Containing the platform-specific information about the surface
 * @phdev    - Physical device handle representing actual connection to the physical device (GPU, CPU, etc...)
 */
struct uvrvk {
  VkInstance instance;
  VkPhysicalDevice phdev;
  VkDevice lgdev;

#if defined(INCLUDE_WAYLAND) || defined(INCLUDE_XCB)
  VkSurfaceKHR surface;
#endif
};

#endif
