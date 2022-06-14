#ifndef VULKAN_COMMON_H
#define VULKAN_COMMON_H

#include "vulkan.h"

/*
 * struct uvr_vk (Underview Renderer Vulkan)
 *
 * @instance - Keeps track of all application state. The connection to the vulkan loader.
 * @surface  - Containing the platform-specific information about the surface
 * @phdev    - Physical device handle representing actual connection to the physical device (GPU, CPU, etc...)
 */
struct uvr_vk {
  VkInstance instance;
  VkPhysicalDevice phdev;
  struct uvr_vk_lgdev lgdev;
  struct uvr_vk_queue graphics_queue;

#if defined(INCLUDE_WAYLAND) || defined(INCLUDE_XCB)
  VkSurfaceKHR surface;
  VkSurfaceCapabilitiesKHR surfcap;
  struct uvr_vk_surface_format sformats;
  struct uvr_vk_surface_present_mode spmodes;
  struct uvr_vk_swapchain schain;
#endif
};

#endif
