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
 * Due to Vulkan not directly exposing functions for all platforms.
 * Dynamically (at runtime) retrieve or acquire the address of a VkInstance func.
 * Via token concatenation and String-izing Tokens
 */
#define UVR_VK_INSTANCE_PROC_ADDR(inst, var, func) \
  do { \
    var = (PFN_vk##func) vkGetInstanceProcAddr(inst, "vk" #func); \
  } while(0);


/*
 * Due to Vulkan not directly exposing functions for all platforms.
 * Dynamically (at runtime) retrieve or acquire the address of a VkDevice (logical device) func.
 * Via token concatenation and String-izing Tokens
 */
#define UVR_VK_DEVICE_PROC_ADDR(dev, var, func) \
  do { \
    var = (PFN_vk##func) vkGetDeviceProcAddr(dev, "vk" #func); \
  } while(0);


/*
 * struct uvr_vk_instance_create_info (Underview Renderer Vulkan Instance Create Information)
 *
 * members:
 * @app_name                - A member of the VkApplicationInfo structure reserved for
 *                            the name of the application.
 * @engine_name             - A member of the VkApplicationInfo structure reserved for
 *                            the name of the engine used to create a given application.
 * @enabledLayerCount       - A member of the VkInstanceCreateInfo structure used to pass
 *                            the number of Vulkan Validation Layers one wants to enable.
 * @ppEnabledLayerNames     - A member of the VkInstanceCreateInfo structure used to pass
 *                            the actual Vulkan Validation Layers one wants to enable.
 * @enabledExtensionCount   - A member of the VkInstanceCreateInfo structure used to pass
 *                            the the number of vulkan instance extensions one wants to enable.
 * @ppEnabledExtensionNames - A member of the VkInstanceCreateInfo structure used to pass
 *                            the actual vulkan instance extensions one wants to enable.
 */
struct uvr_vk_instance_create_info {
  const char *app_name;
  const char *engine_name;
  uint32_t enabledLayerCount;
  const char **ppEnabledLayerNames;
  uint32_t enabledExtensionCount;
  const char **ppEnabledExtensionNames;
};


/*
 * uvr_vk_create_instance: Creates a VkInstance object and establishes a connection to the Vulkan API.
 *                         It also acts as an easy wrapper that allows one to define instance extensions.
 *                         Instance extensions basically allow developers to define what an app is setup to do.
 *                         So, if I want the application to work with wayland or X etc…​ One should enable those
 *                         extensions inorder to gain access to those particular capabilities.
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_instance_create_info
 * return:
 *    on success VkInstance handle
 *    on failure VK_NULL_HANDLE
 */
VkInstance uvr_vk_instance_create(struct uvr_vk_instance_create_info *uvrvk);


/*
 * enum uvr_vk_surface_type (Underview Renderer Surface Type)
 *
 * Display server protocol options used by uvr_vk_surface_create
 * to create a VkSurfaceKHR object based upon platform specific information
 */
typedef enum uvr_vk_surface_type {
  WAYLAND_CLIENT_SURFACE,
  XCB_CLIENT_SURFACE,
} uvr_vk_surface_type;


/*
 * struct uvr_vk_surface_create_info (Underview Renderer Vulkan Surface Create Information)
 *
 * members:
 * @sType    - Must pass a valid enum uvrvk_surface_type value. Used in determine what vkCreate*SurfaceKHR
 *             function and associated structs to use.
 * @vkinst   - Must pass a valid VkInstance handle to create/associate surfaces for an application
 * @surface  - Must pass a pointer to a wl_surface interface
 * @display  - Must pass either a pointer to wl_display interface or a pointer to an xcb_connection_t
 * @window   - Must pass an xcb_window_t window or an unsigned int representing XID
 */
struct uvr_vk_surface_create_info {
  uvr_vk_surface_type sType;
  VkInstance vkinst;
  void *surface;
  void *display;
  unsigned int window;
};


/*
 * uvr_vk_surface_create: Creates a VkSurfaceKHR object based upon platform specific
 *                        information about the given surface
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_surface_create_info
 * return:
 *    on success VkSurfaceKHR handle
 *    on failure VK_NULL_HANDLE
 */
VkSurfaceKHR uvr_vk_surface_create(struct uvr_vk_surface_create_info *uvrvk);


/*
 * struct uvr_vk_phdev_create_info (Underview Renderer Vulkan Physical Device Create Information)
 *
 * members:
 * @vkinst   - Must pass a valid VkInstance handle to create/associate surfaces for an application
 * @vkpdtype - Must pass a valid enum uvr_vk_surface_type value.
 * @kmsfd    - Must pass a valid kms file descriptor for which a VkPhysicalDevice will be created
 *             if corresponding properties match
 */
struct uvr_vk_phdev_create_info {
  VkInstance vkinst;
  VkPhysicalDeviceType vkpdtype;
#ifdef INCLUDE_KMS
  int kmsfd;
#endif
};


/*
 * uvr_vk_phdev_create: Creates a VkPhysicalDevice handle if certain characteristics of
 *                      a physical device are meet
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_phdev_create_info
 * return:
 *    on success VkPhysicalDevice handle
 *    on failure VK_NULL_HANDLE
 */
VkPhysicalDevice uvr_vk_phdev_create(struct uvr_vk_phdev_create_info *uvrvk);


/*
 * uvr_vk_get_phdev_features: Populates the VkPhysicalDeviceFeatures struct with features
 *                            supported by a given VkPhysicalDevice
 *
 * args:
 * @phdev - Must pass a valid VkPhysicalDevice handle
 * return:
 *    populated VkPhysicalDeviceFeatures
 */
VkPhysicalDeviceFeatures uvr_vk_get_phdev_features(VkPhysicalDevice phdev);


/*
 * struct uvr_vk_queue (Underview Renderer Vulkan Queue)
 *
 * members:
 * @name       - Name of the given queue
 * @queue      - VkQueue handle used when submitting command buffers to physical device. Handle assigned
 *               in uvr_vk_lgdev_create after VkDevice handle creation.
 * @famindex   - Queue family index
 * @queueCount - Count of queues in a given queue family
 */
struct uvr_vk_queue {
  char name[20];
  VkQueue queue;
  int famindex;
  int queueCount;
};


/*
 * struct uvr_vk_queue_create_info (Underview Renderer Vulkan Queue Create Information)
 *
 * members:
 * @phdev      - Must pass a valid VkPhysicalDevice handle
 * @queueFlags - Must pass a valid VkQueueFlagBits. https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkQueueFlagBits.html
 */
struct uvr_vk_queue_create_info {
  VkPhysicalDevice phdev;
  VkQueueFlags queueFlag;
};


/*
 * uvr_vk_queue_create: Retrieves queue family index and queue count at said index given a single VkQueueFlags.
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_queue_create_info. Contains information on which queue family
 *          we are trying to find and the physical device that supports said queue family.
 * return:
 *    on success struct uvr_vk_queue
 *    on failure struct uvr_vk_queue { with members nulled, int's set to -1 }
 */
struct uvr_vk_queue uvr_vk_queue_create(struct uvr_vk_queue_create_info *uvrvk);


/*
 * struct uvr_vk_lgdev (Underview Renderer Vulkan Logical Device)
 *
 * members:
 * @device    - Returned VkDevice handle which represents vulkams access to physical device
 * @queue_cnt - Elements in array of struct uvr_vk_queue. This information gets populated with the
 *              data pass through struct uvr_vk_lgdev_create_info { member: numqueues }
 * @queues    - Array of struct uvr_vk_queue. This information gets populated with the
 *              data pass through struct uvr_vk_lgdev_create_info { member: queues }
 */
struct uvr_vk_lgdev {
  VkDevice device;

  // Strictly for extra information storage
  int queue_cnt;
  struct uvr_vk_queue *queues;
};


/*
 * struct uvr_vk_lgdev_create_info (Underview Renderer Vulkan Logical Device Create Information)
 *
 * members:
 * @vkinst                  - Must pass a valid VkInstance handle to create/associate surfaces for an application
 * @phdev                   - Must pass a valid VkPhysicalDevice handle
 * @pEnabledFeatures        - Must pass a valid pointer to a VkPhysicalDeviceFeatures with X features enabled
 * @enabledExtensionCount   - Must pass the amount of Vulkan Device extensions to enable
 * @ppEnabledExtensionNames - Must pass an array of Vulkan Device extension to enable
 * @numqueues               - Must pass the amount of struct uvr_vk_queue (VkQueue,VkQueueFamily indicies) to
 *                            create along with a given logical device
 * @queues                  - Must pass an array of struct uvr_vk_queue (VkQueue,VkQueueFamily indicies) to
 *                            create along with a given logical device
 */
struct uvr_vk_lgdev_create_info {
  VkInstance vkinst;
  VkPhysicalDevice phdev;
  VkDeviceCreateFlags lgdevflags;
  VkPhysicalDeviceFeatures *pEnabledFeatures;
  uint32_t enabledExtensionCount;
  const char *const *ppEnabledExtensionNames;
  uint32_t numqueues;
  struct uvr_vk_queue *queues;
};


/*
 * uvr_vk_lgdev_create: Creates a VkDevice object and allows a connection to a given physical device.
 *                      The VkDevice object is more of a local object its state and operations are local
 *                      to it and are not seen by other logical devices. Function also acts as an easy wrapper
 *                      that allows one to define device extensions. Device extensions basically allow developers
 *                      to define what operations a given logical device is capable of doing. So, if one wants the
 *                      device to be capable of utilizing a swap chain, etc…​ One should enable those extensions
 *                      inorder to gain access to those particular capabilities. Allows for creation of multiple
 *                      VkQueue's although the only one needed is the Graphics queue.
 *
 *                      struct uvr_vk_queue: member 'VkQueue queue' handle is assigned in this function as vkGetDeviceQueue
 *                      requires a logical device.
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_lgdev_create_info
 * return:
 *    on success struct uvr_vk_lgdev
 *    on failure struct uvr_vk_lgdev { with members nulled, int's set to -1 }
 */
struct uvr_vk_lgdev uvr_vk_lgdev_create(struct uvr_vk_lgdev_create_info *uvrvk);


/*
 * uvr_vk_get_surface_capabilities: Populates the VkSurfaceCapabilitiesKHR struct with surface
 *                                  information supported by a given VkPhysicalDevice. Needed in order
 *                                  to create a swapchain.
 *
 * args:
 * @phdev   - Must pass a valid VkPhysicalDevice handle
 * @surface - Must pass a valid VkSurfaceKHR handle
 * return:
 *    populated VkSurfaceCapabilitiesKHR
 */
VkSurfaceCapabilitiesKHR uvr_vk_get_surface_capabilities(VkPhysicalDevice phdev, VkSurfaceKHR surface);


/*
 * uvr_vk_get_surface_formats: Returns an array of supported color formats a given
 *                             physical device and surface supports.
 *
 * args:
 * @phdev   - Must pass a valid VkPhysicalDevice handle
 * @surface - Must pass a valid VkSurfaceKHR handle
 * return:
 *    on success pointer to a VkSurfaceFormatKHR
 *    on failure NULL
 */
VkSurfaceFormatKHR *uvr_vk_get_surface_formats(VkPhysicalDevice phdev, VkSurfaceKHR surface);


/*
 * struct uvr_vk_destroy (Underview Renderer Vulkan Destroy)
 *
 * members:
 * @vkinst        - Must pass a valid VkInstance handle
 * @vksurf        - Must pass a valid VkSurfaceKHR handle
 * @vklgdevs_cnt  - Must pass the amount of VkDevice handles allocated in app
 * @vklgdevs      - Must pass an array of valid VkDevice handle
 * @vksurfformats - Must pass an array of valied VkSurfaceFormatKHR handles
 */
struct uvr_vk_destroy {
  VkInstance vkinst;
  VkSurfaceKHR vksurf;

  uint32_t vklgdevs_cnt;
  struct uvr_vk_lgdev *vklgdevs;

  VkSurfaceFormatKHR *vksurfformats;
};


/*
 * uvr_vk_destory: frees any allocated memory defined by user
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_destroy contains all objects created during
 *          application lifetime in need freeing
 */
void uvr_vk_destory(struct uvr_vk_destroy *uvrvk);

#endif
