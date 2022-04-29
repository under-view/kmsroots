
#ifndef UVR_VULKAN_H
#define UVR_VULKAN_H

#include "common.h"
#include "utils.h"

#ifdef INCLUDE_WAYLAND
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif

#ifdef INCLUDE_XCB
#define VK_USE_PLATFORM_XCB_KHR
#endif

#include <vulkan/vulkan.h>


/*
 * Dynamically retrieve or acquire the address of a VkInstance func.
 * Via token concatenation and String-izing Tokens
 */
#define UVR_VK_INSTANCE_PROC_ADDR(inst, var, func) \
  do { \
    var = (PFN_vk##func) vkGetInstanceProcAddr(inst, "vk" #func); \
  } while(0);


/*
 * Dynamically retrieve or acquire the address of a VkDevice (logical device) func.
 * Via token concatenation and String-izing Tokens
 */
#define UVR_VK_DEVICE_PROC_ADDR(dev, var, func) \
  do { \
    var = (PFN_vk##func) vkGetDeviceProcAddr(dev, "vk" #func); \
  } while(0);


/*
 * struct uvrvk (Underview Renderer Vulkan)
 *
 * @instance - Keeps track of all application state. The connection to the vulkan loader.
 * @surface  - Containing the platform-specific information about the surface
 * @phdev    - Physical device handle representing actual connection to the physical device (GPU, CPU, etc...)
 * @lgdev    -
 */
struct uvrvk {
  VkInstance instance;
  VkPhysicalDevice phdev;
  VkDevice lgdev;

#if defined(INCLUDE_WAYLAND) || defined(INCLUDE_XCB)
  VkSurfaceKHR surface;
#endif
};


/*
 * enum uvrvk_surface_type (Underview Renderer Surface Type)
 *
 * Display server protocol options used by uvr_vk_create_surfaceKHR
 * to create a VkSurfaceKHR object based upon platform specific information
 */
enum uvrvk_surface_type {
  WAYLAND_CLIENT_SURFACE,
  XCB_CLIENT_SURFACE,
};


/*
 * uvr_vk_create_instance: Creates a VkInstance object and establishes a connection to the Vulkan API.
 *                         It also acts as an easy wrapper that allows one to define instance extensions.
 *                         Instance extensions basically allow developers to define what an app is setup to do.
 *                         So, if I want the application to work with wayland or X etc…​ One should enable those
 *                         extensions inorder to gain access to those particular capabilities.
 *
 * args:
 * @app                     - pointer to a struct uvrwc contains all objects
 *                            necessary to run a vulkan application.
 *
 *                            Members that recieve addresses and values:
 *                            @instance
 * @app_name                - A member of the VkApplicationInfo structure reserved for
 *                            the name of the application.
 * @engine_name             - A member of the VkApplicationInfo structure reserved for
 *                            the name of the engine used to create a given application.
 * @enabledLayerCount       - A member of the VkInstanceCreateInfo structure used to pass
 *                            the number of Vulkan Layers one wants to enable.
 * @ppEnabledLayerNames     - A member of the VkInstanceCreateInfo structure used to pass
 *                            the actual Vulkan Layers one wants to enable.
 * @enabledExtensionCount   - A member of the VkInstanceCreateInfo structure used to pass
 *                            the the number of instance extensions one wants to enable.
 * @ppEnabledExtensionNames - A member of the VkInstanceCreateInfo structure used to pass
 *                            the actual Vulkan instance extensions one wants to enable.
 * return:
 *    0 on success
 *   -1 on failure
 */
int uvr_vk_create_instance(struct uvrvk *app,
                           const char *app_name,
                           const char *engine_name,
                           uint32_t enabledLayerCount,
                           const char **ppEnabledLayerNames,
                           uint32_t enabledExtensionCount,
                           const char **ppEnabledExtensionNames);


/*
 * uvr_vk_create_surfaceKHR: Creates a VkSurfaceKHR object based upon platform specific
 *                           information about the surface
 *
 * args:
 * @app - pointer to a struct uvrwc contains all objects
 *        necessary to run a vulkan application.
 *
 *        Members that recieve addresses and values:
 *        @surface
 * @st  - an enum above to help select
 * @fmt - Used to describe what argument is passed and in what order
 *        Character Codes:
 *            * c - Pass the address that gives reference to the underlying display protocol.
 *                  Usually will be (struct wl_display *), (xcb_connection_t *conn)
 *            * s - Pass struct wl_surface interface
 *            * w - Pass xcb_window_t window or an unsigned int representing XID
 * return:
 *    0 on success
 *   -1 on failure
 */
int uvr_vk_create_surfaceKHR(struct uvrvk *app, enum uvrvk_surface_type st, char *fmt, ...);


/*
 * uvr_vk_create_phdev: Creates a VkSurfaceKHR object based upon platform specific
 *                       information about the surface
 *
 * args:
 * @app - pointer to a struct uvrwc contains all objects
 *        necessary to run a vulkan application.
 *
 *        Members that recieve addresses and values:
 *        @phdev, @lgdev
 * @vkpdtype  - an enum above to help select
 * @fmt - Used to describe what argument is passed and in what order
 *        Character Codes:
 *            * d - Pass the address that gives reference to the underlying display protocol.
 *                  Usually will be (struct wl_display *), (xcb_connection_t *conn)
 *            * NULL - If drm file descriptor doesn't exist
 * return:
 *    0 on success
 *   -1 on failure
 */
int uvr_vk_create_phdev(struct uvrvk *app, VkPhysicalDeviceType vkpdtype, char *fmt, ...);


/*
 * uvr_vk_destory: frees any remaining allocated memory contained in struct uvrwc
 *
 * args:
 * @app - pointer to a struct uvrwc contains all objects
 *        necessary to run a vulkan application.
 */
void uvr_vk_destory(struct uvrvk *app);

#endif
