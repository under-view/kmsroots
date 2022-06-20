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
 *                                  information supported by a given VkSurfaceKHR. Needed in order
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
 * struct uvr_vk_surface_format (Underview Renderer Vulkan Surface Format)
 *
 * members:
 * @fcount  - Amount of color formats a given surface supports
 * @formats - Pointer to a VkSurfaceFormatKHR which stores color space and pixel format
 */
struct uvr_vk_surface_format {
  uint32_t fcount;
  VkSurfaceFormatKHR *formats;
};


/*
 * uvr_vk_get_surface_formats: Creates block of memory with all supported color space's and pixel formats a given surface supports
 *                             Application must free struct uvr_vk_surface_format { member: formats }
 *
 * args:
 * @phdev   - Must pass a valid VkPhysicalDevice handle
 * @surface - Must pass a valid VkSurfaceKHR handle
 * return:
 *    on success struct uvr_vk_surface_format
 *    on failure struct uvr_vk_surface_format { with member nulled }
 */
struct uvr_vk_surface_format uvr_vk_get_surface_formats(VkPhysicalDevice phdev, VkSurfaceKHR surface);


/*
 * struct uvr_vk_surface_present_mode (Underview Renderer Vulkan Surface Present Mode)
 *
 * members:
 * @mcount - Amount of present modes a given surface supports
 * @modes  - Pointer to a VkPresentModeKHR which stores values of potential surface present modes
 */
struct uvr_vk_surface_present_mode {
  uint32_t mcount;
  VkPresentModeKHR *modes;
};


/*
 * uvr_vk_get_surface_present_modes: Creates block of memory with all supported presentation modes for a surface
 *                                   Application must free struct uvr_vk_surface_present_mode { member: modes }
 *                                   More information on presentation modes can be found here:
 *                                   https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html
 *
 * args:
 * @phdev   - Must pass a valid VkPhysicalDevice handle
 * @surface - Must pass a valid VkSurfaceKHR handle
 * return:
 *    on success struct uvr_vk_surface_present_mode
 *    on failure struct uvr_vk_surface_present_mode { with member nulled }
 */
struct uvr_vk_surface_present_mode uvr_vk_get_surface_present_modes(VkPhysicalDevice phdev, VkSurfaceKHR surface);


/*
 * struct uvr_vk_swapchain (Underview Renderer Vulkan Swapchain)
 *
 * members:
 * @lgdev     - Logical device used when swapchain was created
 * @swapchain - Vulkan handle/object representing the swapchain itself
 */
struct uvr_vk_swapchain {
  VkDevice lgdev;
  VkSwapchainKHR swapchain;
};


/*
 * struct uvr_vk_swapchain_create_info (Underview Renderer Vulkan Swapchain Create Information)
 *
 * members:
 * @lgdev                 - Must pass a valid VkDevice handle (Logical Device)
 * @surfaceKHR            - Must pass a valid VkSurfaceKHR handle. From uvr_vk_surface_create(3)
 * @surfcap               - Passed the queried surface capabilities. From uvr_vk_get_surface_capabilities(3)
 * @surfaceFormat         - Pass colorSpace & pixel format of choice. Recommend querrying first via uvr_vk_get_surface_formats(3)
 *                          then check if pixel format and colorSpace you want is supported by a given surface.
 * See: https://khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkSwapchainCreateInfoKHR.html for bellow members
 * @extent2D
 * @imageArrayLayers
 * @imageUsage
 * @imageSharingMode
 * @queueFamilyIndexCount
 * @pQueueFamilyIndices
 * @compositeAlpha
 * @presentMode
 * @clipped
 * @oldSwapchain
 */
struct uvr_vk_swapchain_create_info {
  VkDevice                         lgdev;
  VkSurfaceKHR                     surfaceKHR;
  VkSurfaceCapabilitiesKHR         surfcap;
  VkSurfaceFormatKHR               surfaceFormat;
  VkExtent2D                       extent2D;
  uint32_t                         imageArrayLayers;
  VkImageUsageFlags                imageUsage;
  VkSharingMode                    imageSharingMode;
  uint32_t                         queueFamilyIndexCount;
  const uint32_t*                  pQueueFamilyIndices;
  VkCompositeAlphaFlagBitsKHR      compositeAlpha;
  VkPresentModeKHR                 presentMode;
  VkBool32                         clipped;
  VkSwapchainKHR                   oldSwapchain;
};


/*
 * uvr_vk_swapchain_create: Creates block of memory with all supported presentation modes for a surface
 *                          Minimum image count is equal to VkSurfaceCapabilitiesKHR.minImageCount + 1
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_swapchain_create_info
 * return:
 *    on success struct uvr_vk_swapchain
 *    on failure struct uvr_vk_swapchain { with member nulled }
 */
struct uvr_vk_swapchain uvr_vk_swapchain_create(struct uvr_vk_swapchain_create_info *uvrvk);


/*
 * struct uvr_vk_image (Underview Renderer Vulkan Image)
 *
 * members:
 * @lgdev     - Logical device used to associate a VkImageView with a VkImage
 * @icount    - Amount of VkImage's to create. If VkSwapchainKHR reference is passed value would
 *              be the amount of images in the given swapchain.
 * @images    - Array of VkImage's
 *            + @image - Represents actual image itself. May be a texture, etc...
 * @vcount    - Amount of VkImageView's to associate with VkImage's. If VkSwapchainKHR reference is passed
 *              value would be the amount of images in the given swapchain
 * @views     - Array of VkImageView's
 *            + @view - Represents a way to access the actual image itself
 * @swapchain - Member not required, but used for storage purposes. A valid VkSwapchainKHR
 *              reference to the VkSwapchainKHR passed to uvr_vk_image_create. Represents
 *              the swapchain that created VkImage's.
 */
struct uvr_vk_image {
  VkDevice lgdev;

  uint32_t icount;
  struct uvrvkimage {
    VkImage image;
  } *images;

  uint32_t vcount;
  struct uvrvkview {
    VkImageView view;
  } *views;

  VkSwapchainKHR swapchain;
};


/*
 * struct uvr_vk_image_create_info (Underview Renderer Vulkan Image Create Information)
 *
 * members:
 * @lgdev     - Must pass a valid VkDevice handle (Logical Device)
 * @swapchain - Must pass a valid VkSwapchainKHR handle. Used when retrieving references to underlying VkImage
 * See: https://khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkImageViewCreateInfo.html for bellow members
 * @flags
 * @viewType
 * @format
 * @components
 * @subresourceRange
 */
struct uvr_vk_image_create_info {
  VkDevice                lgdev;
  VkSwapchainKHR          swapchain;
  VkImageViewCreateFlags  flags;
  VkImageViewType         viewType;
  VkFormat                format;
  VkComponentMapping      components;
  VkImageSubresourceRange subresourceRange;
};


/*
 * uvr_vk_image_create: Function creates/retrieve VkImage's and associates VkImageView's with said images.
 *                      Allows image to be accessed by a shader. If VkSwapchainKHR reference is passed function
 *                      retrieves all images in the swapchain.
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_image_create_info
 * return:
 *    on success struct uvr_vk_image
 *    on failure struct uvr_vk_image { with member nulled }
 */
struct uvr_vk_image uvr_vk_image_create(struct uvr_vk_image_create_info *uvrvk);


/*
 * struct uvr_vk_shader_module (Underview Renderer Vulkan Shader Module)
 *
 * members:
 * @lgdev  - Logical device used when shader module was created
 * @shader - Contains shader code and one or more entry points.
 */
struct uvr_vk_shader_module {
  VkDevice lgdev;
  VkShaderModule shader;
  const char *name;
};


/*
 * struct uvr_vk_shader_module_create_info (Underview Renderer Vulkan Shader Module Create Information)
 *
 * members:
 * @lgdev    - Must pass a valid active logical device
 * @codeSize - Sizeof SPIR-V byte code
 * @pCode    - SPIR-V byte code itself
 */
struct uvr_vk_shader_module_create_info {
  VkDevice lgdev;
  size_t codeSize;
  const char *pCode;
  const char *name;
};


/*
 * uvr_vk_shader_module_create: Function creates VkShaderModule from passed SPIR-V byte code.
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_shader_module_create_info
 * return:
 *    on success struct uvr_vk_shader_module
 *    on failure struct uvr_vk_shader_module { with member nulled }
 */
struct uvr_vk_shader_module uvr_vk_shader_module_create(struct uvr_vk_shader_module_create_info *uvrvk);


/*
 * struct uvr_vk_destroy (Underview Renderer Vulkan Destroy)
 *
 * members:
 * @vkinst          - Must pass a valid VkInstance handle
 * @vksurf          - Must pass a valid VkSurfaceKHR handle
 * @vklgdevs_cnt    - Must pass the amount of elements in struct uvr_vk_lgdev array
 * @vklgdevs        - Must pass an array of valid struct uvr_vk_lgdev { free'd  members: VkDevice handle }
 * @vkswapchain_cnt - Must pass the amount of elements in struct uvr_vk_swapchain array
 * @vkswapchains    - Must pass an array of valid struct uvr_vk_swapchain { free'd members: VkSwapchainKHR handle }
 * @vkimage_cnt     - Must pass the amount of elements in struct uvr_vk_image array
 * @vkimages        - Must pass an array of valid struct uvr_vk_image { free'd members: VkImageView handle, *views, *images }
 * @vkshader_cnt    - Must pass the amount of elements in struct uvr_vk_shader_module array
 * @vkshaders       - Must pass an array of valid struct uvr_vk_shader_module { free'd members: VkShaderModule handle }
 */
struct uvr_vk_destroy {
  VkInstance vkinst;
  VkSurfaceKHR vksurf;

  uint32_t vklgdevs_cnt;
  struct uvr_vk_lgdev *vklgdevs;

  uint32_t vkswapchain_cnt;
  struct uvr_vk_swapchain *vkswapchains;

  uint32_t vkimage_cnt;
  struct uvr_vk_image *vkimages;

  uint32_t vkshader_cnt;
  struct uvr_vk_shader_module *vkshaders;
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
