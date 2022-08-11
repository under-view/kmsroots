#ifndef UVR_VULKAN_H
#define UVR_VULKAN_H

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
 * @appName                 - A member of the VkApplicationInfo structure reserved for the name of the application.
 * @engineName              - A member of the VkApplicationInfo structure reserved for the name of the engine name
 *                            (if any) used to create application.
 * @enabledLayerCount       - A member of the VkInstanceCreateInfo structure used to pass the number of Vulkan
 *                            Validation Layers a client wants to enable.
 * @ppEnabledLayerNames     - A member of the VkInstanceCreateInfo structure used to pass a pointer to an array
 *                            of strings containing the name of the Vulkan Validation Layers one wants to enable.
 * @enabledExtensionCount   - A member of the VkInstanceCreateInfo structure used to pass the the number of vulkan
 *                            instance extensions a client wants to enable.
 * @ppEnabledExtensionNames - A member of the VkInstanceCreateInfo structure used to pass a pointer to an array the actual
 *                            of strings containing the name of the Vulkan Instance Extensions one wants to enable.
 */
struct uvr_vk_instance_create_info {
  const char *appName;
  const char *engineName;
  uint32_t   enabledLayerCount;
  const char **ppEnabledLayerNames;
  uint32_t   enabledExtensionCount;
  const char **ppEnabledExtensionNames;
};


/*
 * uvr_vk_create_instance: Creates a VkInstance object and establishes a connection to the Vulkan API.
 *                         It also acts as an easy wrapper that allows one to define instance extensions.
 *                         Instance extensions basically allow developers to define what an app is setup to do.
 *                         So, if a client wants the application to work with wayland surface or X surface etc…​
 *                         Client should enable those extensions inorder to gain access to those particular capabilities.
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
 * Display server protocol options. ENUM Used by uvr_vk_surface_create
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
 * @sType    - Must pass a valid enum uvr_vk_surface_type value. Used in determine what vkCreate*SurfaceKHR
 *             function and associated structs to utilize when creating the VkSurfaceKHR object.
 * @vkInst   - Must pass a valid VkInstance handle to create/associate surfaces for an application
 * @surface  - Must pass a pointer to a struct wl_surface object
 * @display  - Must pass either a pointer to struct wl_display object or a pointer to an xcb_connection_t
 * @window   - Must pass an xcb_window_t window id or an unsigned int representing XID
 */
struct uvr_vk_surface_create_info {
  uvr_vk_surface_type sType;
  VkInstance          vkInst;
  void                *surface;
  void                *display;
  unsigned int        window;
};


/*
 * uvr_vk_surface_create: Creates a VkSurfaceKHR object based upon platform specific information about the given surface.
 *                        VkSurfaceKHR are the interface between the window and Vulkan defined images in a given swapchain
 *                        if vulkan swapchain exists.
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
 * @vkInst      - Must pass a valid VkInstance handle which to find VkPhysicalDevice with
 * @vkPhdevType - Must pass one of the supported VkPhysicalDeviceType's.
 *                https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceType.html
 * @kmsFd       - Must pass a valid kms file descriptor for which a VkPhysicalDevice will be created
 *                if corresponding DRM properties match.
 */
struct uvr_vk_phdev_create_info {
  VkInstance           vkInst;
  VkPhysicalDeviceType vkPhdevType;
#ifdef INCLUDE_KMS
  int                  kmsFd;
#endif
};


/*
 * uvr_vk_phdev_create: Retrieves a VkPhysicalDevice handle if certain characteristics of a physical device are meet
 *                      Characteristics include @vkPhdevType and @kmsFd.
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_phdev_create_info
 * return:
 *    on success VkPhysicalDevice handle
 *    on failure VK_NULL_HANDLE
 */
VkPhysicalDevice uvr_vk_phdev_create(struct uvr_vk_phdev_create_info *uvrvk);


/*
 * uvr_vk_get_phdev_features: Populates the VkPhysicalDeviceFeatures struct with features supported by a given VkPhysicalDevice
 *
 * args:
 * @phdev - Must pass a valid VkPhysicalDevice handle
 * return:
 *    populated VkPhysicalDeviceFeatures struct
 */
VkPhysicalDeviceFeatures uvr_vk_get_phdev_features(VkPhysicalDevice phdev);


/*
 * struct uvr_vk_queue (Underview Renderer Vulkan Queue)
 *
 * members:
 * @name        - Name of the given queue. This is strictly to give a name to the queue. Not required by API.
 * @queue       - VkQueue handle used when submitting command buffers to physical device. Handle assigned
 *                in uvr_vk_lgdev_create after VkDevice handle creation.
 * @familyIndex - VkQueue family index
 * @queueCount  - Count of queues in a given VkQueue family
 */
struct uvr_vk_queue {
  char    name[20];
  VkQueue vkQueue;
  int     familyIndex;
  int     queueCount;
};


/*
 * struct uvr_vk_queue_create_info (Underview Renderer Vulkan Queue Create Information)
 *
 * members:
 * @vkPhdev   - Must pass a valid VkPhysicalDevice handle to query queues associate with phsyical device
 * @queueFlag - Must pass one VkQueueFlagBits, if multiple flags are or'd function will fail to return VkQueue family index (struct uvr_vk_queue).
 *              https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkQueueFlagBits.html
 */
struct uvr_vk_queue_create_info {
  VkPhysicalDevice vkPhdev;
  VkQueueFlags     queueFlag;
};


/*
 * uvr_vk_queue_create: Queries the queues a given physical device contains. Then returns a queue
 *                      family index and the queue count at said index given a single VkQueueFlagBits.
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
 * @vkDevice   - Returned VkDevice handle which represents vulkans access to physical device
 * @queueCount - Amount of elements in pointer to array of struct uvr_vk_queue. This information
 *               gets populated with the data pass through struct uvr_vk_lgdev_create_info { member: numqueues }
 * @queues     - Pointer to an array of struct uvr_vk_queue. This information gets populated with the
 *               data pass through struct uvr_vk_lgdev_create_info { member: queues }
 *
 *               @queueCount & @queues are strictly for struct uvr_vk_lgdev to have extra information amount VkQueue's
 */
struct uvr_vk_lgdev {
  VkDevice            vkDevice;
  uint32_t            queueCount;
  struct uvr_vk_queue *queues;
};


/*
 * struct uvr_vk_lgdev_create_info (Underview Renderer Vulkan Logical Device Create Information)
 *
 * members:
 * @vkInst                  - Must pass a valid VkInstance handle to create VkDevice handle from.
 * @vkPhdev                 - Must pass a valid VkPhysicalDevice handle to associate VkDevice handle with.
 * @pEnabledFeatures        - Must pass a valid pointer to a VkPhysicalDeviceFeatures with X features enabled
 * @enabledExtensionCount   - Must pass the amount of Vulkan Device extensions to enable.
 * @ppEnabledExtensionNames - Must pass an array of strings containing Vulkan Device extension to enable.
 * @queueCount              - Must pass the amount of struct uvr_vk_queue (VkQueue,VkQueueFamily indicies) to
 *                            create along with a given logical device
 * @queues                  - Must pass a pointer to an array of struct uvr_vk_queue (VkQueue,VkQueueFamily indicies) to
 *                            create along with a given logical device
 */
struct uvr_vk_lgdev_create_info {
  VkInstance               vkInst;
  VkPhysicalDevice         vkPhdev;
  VkPhysicalDeviceFeatures *pEnabledFeatures;
  uint32_t                 enabledExtensionCount;
  const char *const        *ppEnabledExtensionNames;
  uint32_t                 queueCount;
  struct uvr_vk_queue      *queues;
};


/*
 * uvr_vk_lgdev_create: Creates a VkDevice object and allows vulkan to have a connection to a given physical device.
 *                      The VkDevice object is more of a local object its state and operations are local
 *                      to it and are not seen by other logical devices. Function also acts as an easy wrapper
 *                      that allows client to define device extensions. Device extensions basically allow developers
 *                      to define what operations a given logical device is capable of doing. So, if one wants the
 *                      device to be capable of utilizing a swap chain, etc…​ One should enable those extensions
 *                      inorder to gain access to those particular capabilities. Allows for creation of multiple
 *                      VkQueue's although the only one needed is the Graphics queue.
 *
 *                      struct uvr_vk_queue: member 'VkQueue queue' handle is assigned in this function as vkGetDeviceQueue
 *                      requires a logical device handle.
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_lgdev_create_info
 * return:
 *    on success struct uvr_vk_lgdev
 *    on failure struct uvr_vk_lgdev { with members nulled, int's set to -1 }
 */
struct uvr_vk_lgdev uvr_vk_lgdev_create(struct uvr_vk_lgdev_create_info *uvrvk);


/*
 * uvr_vk_get_surface_capabilities: Populates the VkSurfaceCapabilitiesKHR struct with supported GPU device surface capabilities.
 *                                  Queries what a physical device is capable of supporting for any given surface.
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
 * @surfaceFormatCount - Amount of color formats a given surface supports
 * @surfaceFormats     - Pointer to a array of VkSurfaceFormatKHR which stores color space and pixel format
 */
struct uvr_vk_surface_format {
  uint32_t           surfaceFormatCount;
  VkSurfaceFormatKHR *surfaceFormats;
};


/*
 * uvr_vk_get_surface_formats: Creates block of memory with all supported color space's and pixel formats a given physical device
 *                             supports for any given surface. Application must free struct uvr_vk_surface_format { member: surfaceFormats }
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
 * @presentModeCount - Amount of present modes a given surface supports
 * @presentModes     - Pointer to an array of VkPresentModeKHR which stores values of potential surface present modes
 */
struct uvr_vk_surface_present_mode {
  uint32_t         presentModeCount;
  VkPresentModeKHR *presentModes;
};


/*
 * uvr_vk_get_surface_present_modes: Creates block of memory with all supported presentation modes for a surface
 *                                   Application must free struct uvr_vk_surface_present_mode { member: presentModes }
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
 * @vkDevice    - VkDevice handle (Logical Device) associated with swapchain
 * @vkSwapchain - Vulkan handle/object representing the swapchain itself
 */
struct uvr_vk_swapchain {
  VkDevice       vkDevice;
  VkSwapchainKHR vkSwapchain;
};


/*
 * struct uvr_vk_swapchain_create_info (Underview Renderer Vulkan Swapchain Create Information)
 *
 * members:
 * @vkDevice              - Must pass a valid VkDevice handle (Logical Device)
 * @vkSurface             - Must pass a valid VkSurfaceKHR handle. From uvr_vk_surface_create(3)
 * @surfaceCapabilities   - Passed the queried surface capabilities. From uvr_vk_get_surface_capabilities(3)
 * @surfaceFormat         - Pass colorSpace & pixel format of choice. Recommend querrying first via uvr_vk_get_surface_formats(3)
 *                          then check if pixel format and colorSpace you want is supported by a given physical device.
 * See: https://khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkSwapchainCreateInfoKHR.html for more info on bellow members
 * @extent2D              - The width and height of the images in the swapchain in amount of pixels.
 * @imageArrayLayers
 * @imageUsage            - Intended use of images in swapchain
 * @imageSharingMode      - Sets whether images can only be accessed by a single queue or multiple queues
 * @queueFamilyIndexCount - Amount of queue families that may have access to the swapchain images. Only set if @imageSharingMode is
 *                          not set to VK_SHARING_MODE_EXCLUSIVE.
 * @pQueueFamilyIndices   - Pointer to an array of queue family indicies
 * @compositeAlpha        - How to blend images with external graphics
 * @presentMode           - How images are queued and presented internally by the swapchain (FIFO, MAIL_BOX are the only ones known not to lead to tearing)
 * @clipped               - Allow vulkan to clip images not in view. (i.e clip part of the image if it's behind a window)
 * @oldSwapchain          - If a swapchain is still in use while a window is resized passing pointer to old swapchain may aid in
 *                          resource reuse as the application is allowed to present images already acquired from swapchain. Thus,
 *                          no need to recreate them.
 */
struct uvr_vk_swapchain_create_info {
  VkDevice                    vkDevice;
  VkSurfaceKHR                vkSurface;
  VkSurfaceCapabilitiesKHR    surfaceCapabilities;
  VkSurfaceFormatKHR          surfaceFormat;
  VkExtent2D                  extent2D;
  uint32_t                    imageArrayLayers;
  VkImageUsageFlags           imageUsage;
  VkSharingMode               imageSharingMode;
  uint32_t                    queueFamilyIndexCount;
  const uint32_t              *pQueueFamilyIndices;
  VkCompositeAlphaFlagBitsKHR compositeAlpha;
  VkPresentModeKHR            presentMode;
  VkBool32                    clipped;
  VkSwapchainKHR              oldSwapchain;
};


/*
 * uvr_vk_swapchain_create: Creates VkSwapchainKHR object that provides ability to present renderered results to a given VkSurfaceKHR
 *                          Minimum image count is equal to VkSurfaceCapabilitiesKHR.minImageCount + 1.
 *                          The swapchain can be defined as a set of images that can be drawn to and presented to a surface.
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_swapchain_create_info
 * return:
 *    on success struct uvr_vk_swapchain
 *    on failure struct uvr_vk_swapchain { with member nulled }
 */
struct uvr_vk_swapchain uvr_vk_swapchain_create(struct uvr_vk_swapchain_create_info *uvrvk);


/*
 * struct uvr_vk_image_handle (Underview Renderer Vulkan Image Handle)
 *
 * @image - Represents actual image itself. May be a texture, etc...
 */
struct uvr_vk_image_handle {
  VkImage image;
};


/*
 * struct uvr_vk_image_view_handle (Underview Renderer Vulkan Image View Handle)
 *
 * @view - Represents a way to interface with the actual VkImage itself. Describes to
 *         vulkan how to interface with a given VkImage. How to read the given image and
 *         what in the image to read (color channels, etc...)
 */
struct uvr_vk_image_view_handle {
  VkImageView view;
};


/*
 * struct uvr_vk_image (Underview Renderer Vulkan Image)
 *
 * members:
 * @vkDevice     - VkDevice handle (Logical Device) associated with VkImageView objects
 * @imageCount   - Amount of VkImage's created. If VkSwapchainKHR reference is passed value would
 *                 be the amount of images in the given swapchain.
 * @vkImages     - Pointer to an array of VkImage handles
 * @vkImageViews - Pointer to an array of VkImageView handles
 * @vkSwapchain  - Member not required, but used for storage purposes. A valid VkSwapchainKHR
 *                 reference to the VkSwapchainKHR passed to uvr_vk_image_create. Represents
 *                 the swapchain that created VkImage's.
 */
struct uvr_vk_image {
  VkDevice                         vkDevice;
  uint32_t                         imageCount;
  struct uvr_vk_image_handle       *vkImages;
  struct uvr_vk_image_view_handle  *vkImageViews;
  VkSwapchainKHR                   vkSwapchain;
};


/*
 * struct uvr_vk_image_create_info (Underview Renderer Vulkan Image Create Information)
 *
 * members:
 * @vkDevice         - Must pass a valid VkDevice handle (Logical Device)
 * @vkSwapchain      - Must pass a valid VkSwapchainKHR handle. Used when retrieving references to underlying VkImage
 *                     If VkSwapchainKHR reference is not passed value set amount of VkImage/VkImageViews view
 * @viewcount        - Must pass amount of VkImage/VkImageView's to create
 * See: https://khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkImageViewCreateInfo.html for more info on bellow members
 * @flags            - Specifies additional prameters associated with VkImageView. Normally set to zero.
 * @viewType         - Specifies what the image view type is. From what I'm seeting it means the dimensions allowed by a
 *                     given image view. 2D image requires 2D image view 3D image require 3D image view.
 * @format           - Image Format (Bits per color channel, the color channel ordering, etc...)
 * @components       - Makes it so that we can select what value goes to what color channel. Basically if we want to assign
 *                     red channel value to green channel. Or set all (RGBA) color channel values to the value at B channel
 *                     this is how we achieve that.
 * @subresourceRange - Gates an image so that only a part of an image is allowed to be viewable.
 */
struct uvr_vk_image_create_info {
  VkDevice                vkDevice;
  VkSwapchainKHR          vkSwapchain;
  uint32_t                viewCount;
  VkImageViewCreateFlags  flags;
  VkImageViewType         viewType;
  VkFormat                format;
  VkComponentMapping      components;
  VkImageSubresourceRange subresourceRange;
};


/*
 * uvr_vk_image_create: Function creates/retrieve VkImage's and associates VkImageView's with said images.
 *                      VkImageView's allows a VkImage to be accessed by a shader. If a VkSwapchainKHR
 *                      reference is passed function retrieves all images in the swapchain and uses that
 *                      to associate VkImageView objects.
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
 * @vkDevice - VkDevice handle (Logical Device) associated with VkShaderModule
 * @shader   - Contains shader code and one or more entry points.
 * @name     - Name given to shader module can be safely ignored not required by API.
 */
struct uvr_vk_shader_module {
  VkDevice       vkDevice;
  VkShaderModule shader;
  const char     *name;
};


/*
 * struct uvr_vk_shader_module_create_info (Underview Renderer Vulkan Shader Module Create Information)
 *
 * members:
 * @vkDevice - Must pass a valid VkDevice handle (Logical Device)
 * @codeSize - Sizeof SPIR-V byte code
 * @pCode    - SPIR-V byte code itself
 * @name     - Name given to shader module can be safely ignored not required by API.
 */
struct uvr_vk_shader_module_create_info {
  VkDevice   vkDevice;
  size_t     codeSize;
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
 * struct uvr_vk_pipeline_layout (Underview Renderer Vulkan Pipeline Layout)
 *
 * members:
 * @vkDevice         - VkDevice handle (Logical Device) associated with pipeline layout
 * @vkPipelineLayout - Represents what resources are needed to produce final image
 */
struct uvr_vk_pipeline_layout {
  VkDevice         vkDevice;
  VkPipelineLayout vkPipelineLayout;
};


/*
 * struct uvr_vk_pipeline_layout_create_info (Underview Renderer Vulkan Pipeline Layout Create Information)
 *
 * members:
 * @vkDevice                  - Must pass a valid VkDevice handle (Logical Device)
 * See: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPipelineLayoutCreateInfo.html for more info on bellow members
 * @setLayoutCount
 * @pSetLayouts
 * @pushConstantRangeCount
 * @pPushConstantRanges
 */
struct uvr_vk_pipeline_layout_create_info {
  VkDevice                    vkDevice;
  uint32_t                    setLayoutCount;
  const VkDescriptorSetLayout *pSetLayouts;
  uint32_t                    pushConstantRangeCount;
  const VkPushConstantRange   *pPushConstantRanges;
};


/*
 * uvr_vk_pipeline_layout_create: Function creates a VkPipelineLayout handle that is then later used by the graphics pipeline
 *                                itself so that is knows what resources are need to produce the final image and at what shader
 *                                stages. Resources include descriptor sets and push constants. In short defines the layout of
 *                                uniform inputs.
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_pipeline_layout_create_info
 * return:
 *    on success struct uvr_vk_pipeline_layout
 *    on failure struct uvr_vk_pipeline_layout { with member nulled }
 */
struct uvr_vk_pipeline_layout uvr_vk_pipeline_layout_create(struct uvr_vk_pipeline_layout_create_info *uvrvk);


/*
 * struct uvr_vk_render_pass (Underview Renderer Vulkan Render Pass)
 *
 * members:
 * @vkDevice   - VkDevice handle (Logical Device) associated with render pass instance
 * @renderpass - Represents a collection of attachments, subpasses, and dependencies between the subpasses
 */
struct uvr_vk_render_pass {
  VkDevice     vkDevice;
  VkRenderPass renderPass;
};


/*
 * struct uvr_vk_render_pass_create_info (Underview Renderer Vulkan Render Pass Create Information)
 *
 * members:
 * @vkDevice        - Must pass a valid VkDevice handle (Logical Device)
 * See: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkRenderPassCreateInfo.html for more info on bellow members
 * @attachmentCount
 * @pAttachments
 * @subpassCount
 * @pSubpasses
 * @dependencyCount - Amount of elements in VkSubpassDependency array
 * @pDependencies   - Pointer to an array of subpass dependencies that define stages in a pipeline where transitions need to occur
 */
struct uvr_vk_render_pass_create_info {
  VkDevice                      vkDevice;
  uint32_t                      attachmentCount;
  const VkAttachmentDescription *pAttachments;
  uint32_t                      subpassCount;
  const VkSubpassDescription    *pSubpasses;
  uint32_t                      dependencyCount;
  const VkSubpassDependency     *pDependencies;
};


/*
 * uvr_vk_render_pass_create: Function creates a VkRenderPass handle that is then later used by the graphics pipeline
 *                            itself so that is knows how many color and depth buffers there will be, how many samples
 *                            to use for each of them, and how their contents should be handled throughout rendering
 *                            operations. Subpasses within a render pass then references the attachments for every draw
 *                            operations and connects attachments to the graphics pipeline. In short the render pass
 *                            defines the entire render process and outputs from each pipeline to a framebuffer.
 *
 *                            Taken from: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Render_passes
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_render_pass_create_info
 * return:
 *    on success struct uvr_vk_render_pass
 *    on failure struct uvr_vk_render_pass { with member nulled }
 */
struct uvr_vk_render_pass uvr_vk_render_pass_create(struct uvr_vk_render_pass_create_info *uvrvk);


/*
 * struct uvr_vk_graphics_pipeline (Underview Renderer Vulkan Graphics Pipeline)
 *
 * members:
 * @vkDevice         - VkDevice handle (Logical Device) associated with VkPipeline (Graphics Pipeline)
 * @graphicsPipeline - Handle to a pipeline object
 */
struct uvr_vk_graphics_pipeline {
  VkDevice   vkDevice;
  VkPipeline graphicsPipeline;
};


/*
 * struct uvr_vk_graphics_pipeline_create_info (Underview Renderer Vulkan Graphics Pipeline Create Information)
 *
 * members:
 * @vkDevice            - Must pass a valid VkDevice handle (Logical Device)
 * See: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkGraphicsPipelineCreateInfo.html for more info on bellow members
 * @stageCount          - Amount of shaders being used by graphics pipeline
 * @pStages             - Defines shaders and at what shader stages this GP will utilize
 * @pVertexInputState   - Defines the layout and format of vertex input data. Provides details for loading vertex data.
 * @pInputAssemblyState - Defines how to assemble vertices to primitives (i.e. triangles or lines).
 * @pTessellationState  -
 * @pViewportState      - View port defines how to populated image with pixel data (i.e populate only the top half or bottom half). Scissor defines
 *                        how to crop an image. How much of image should be drawn (i.e draw whole image, right half, middle, etc...)
 * @pRasterizationState - Handles how raw vertex data turns into cordinates on screen and in a pixel buffer. Handle computation of fragments (pixels)
 *                        from primitives (i.e. triangles or lines)
 * @pMultisampleState   - If you want to do clever anti-aliasing through multisampling. Stores multisampling information.
 * @pDepthStencilState  - How to handle depth + stencil data. If one has to object we don't want it to draw the back object in the one
 *                        that's in front of it.
 * @pColorBlendState    - Defines how to blend fragments at the end of the pipeline.
 * @pDynamicState       - Graphics pipelines settings are static once set they can't change. To get new settings you'd have to create a whole new pipeline.
 *                        There are settings however that can be changed at runtime. We define which settings here.
 * @layout              - Pass VkPipelineLayout handle to define the resources (i.e. descriptor sets, push constants) given to the pipeline for a single draw operation
 * @renderPass          - Pass VkRenderPAss handle which Holds a pipeline and handles how it is execute. With final outputs being to a framebuffer.
 *                        One can have multiple smaller subpasses inside of it that each use a different pipeline.
 *                        Contains multiple attachments that go to all plausible pipeline outputs (i.e Depth, Color, etc..)
 * @subpass             - Pass the index of the subpass in the render pass
 */
struct uvr_vk_graphics_pipeline_create_info {
  VkDevice                                      vkDevice;
  uint32_t                                      stageCount;
  const VkPipelineShaderStageCreateInfo         *pStages;
  const VkPipelineVertexInputStateCreateInfo    *pVertexInputState;
  const VkPipelineInputAssemblyStateCreateInfo  *pInputAssemblyState;
  const VkPipelineTessellationStateCreateInfo   *pTessellationState;
  const VkPipelineViewportStateCreateInfo       *pViewportState;
  const VkPipelineRasterizationStateCreateInfo  *pRasterizationState;
  const VkPipelineMultisampleStateCreateInfo    *pMultisampleState;
  const VkPipelineDepthStencilStateCreateInfo   *pDepthStencilState;
  const VkPipelineColorBlendStateCreateInfo     *pColorBlendState;
  const VkPipelineDynamicStateCreateInfo        *pDynamicState;
  VkPipelineLayout                              vkPipelineLayout;
  VkRenderPass                                  renderPass;
  uint32_t                                      subpass;
};


/*
 * uvr_vk_graphics_pipeline_create: Function creates a VkPipeline handle. The graphics pipeline is a sequence of operations
 *                                  that takes the vertices and textures of your meshes all the way to the pixels in the render targets.
 *                                  Outputs it into a framebuffer
 *
 *                                  Taken from: https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Introduction
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_graphics_pipeline_create_info
 * return:
 *    on success struct uvr_vk_graphics_pipeline
 *    on failure struct uvr_vk_graphics_pipeline { with member nulled }
 */
struct uvr_vk_graphics_pipeline uvr_vk_graphics_pipeline_create(struct uvr_vk_graphics_pipeline_create_info *uvrvk);


/*
 * struct uvr_vk_framebuffer_handle (Underview Renderer Vulkan Framebuffer Handle)
 *
 * @fb - Framebuffers represent a collection of specific memory attachments that a render pass instance uses.
 *       Connection between an image (or images) and the render pass instance.
 */
struct uvr_vk_framebuffer_handle {
  VkFramebuffer fb;
};


/*
 * struct uvr_vk_framebuffer (Underview Renderer Vulkan Framebuffer)
 *
 * members:
 * @vkDevice         - VkDevice handle (Logical Device) associated with @frameBufferCount VkFramebuffer's
 * @frameBufferCount - Amount of VkFramebuffer handles created
 * @vkFrameBuffers   - Pointer to an array of VkFramebuffer handles
 */
struct uvr_vk_framebuffer {
  VkDevice                          vkDevice;
  uint32_t                          frameBufferCount;
  struct uvr_vk_framebuffer_handle  *vkFrameBuffers;
};


/*
 * struct uvr_vk_framebuffer_create_info (Underview Renderer Vulkan Framebuffer Create Information)
 *
 * members:
 * @vkDevice         - Must pass a valid VkDevice handle (Logical Device)
 * @frameBufferCount - Amount of VkFramebuffer handles to create
 * @vkImageViews     - Pointer to an array of VkImageView handles which will be used in a render pass instance.
 * @renderPass       - Defines the render pass a given framebuffer is compatible with
 * @width            - Framebuffer width in pixels
 * @height           - Framebuffer height in pixels
 * See: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkFramebufferCreateInfo.html for more info on bellow members
 * @layers
 */
struct uvr_vk_framebuffer_create_info {
  VkDevice                         vkDevice;
  uint32_t                         frameBufferCount;
  struct uvr_vk_image_view_handle  *vkImageViews;
  VkRenderPass                     renderPass;
  uint32_t                         width;
  uint32_t                         height;
  uint32_t                         layers;
};


/*
 * uvr_vk_framebuffer_create: Creates @frameBufferCount amount of VkFramebuffer handles. For simplicity each VkFramebuffer
 *                            handle created only has one VkImageView attached. Can think of this function as creating the
 *                            frames to hold the pictures in them, with each frame only containing one picture. Note framebuffer
 *                            images must match up one to one with attachments in the render pass.
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_framebuffer_create_info
 * return:
 *    on success struct uvr_vk_framebuffer
 *    on failure struct uvr_vk_framebuffer { with member nulled }
 */
struct uvr_vk_framebuffer uvr_vk_framebuffer_create(struct uvr_vk_framebuffer_create_info *uvrvk);


/*
 * struct uvr_vk_command_buffer_handle (Underview Renderer Vulkan Command Buffer Handle)
 *
 * @commandBuffer - Handle used to prerecord commands before they are submitted to a device queue and sent off to the GPU.
 */
struct uvr_vk_command_buffer_handle {
  VkCommandBuffer commandBuffer;
};


/*
 * struct uvr_vk_command_buffer (Underview Renderer Vulkan Command Buffer)
 *
 * members:
 * @vkDevice           - VkDevice handle (Logical Device) associated with VkCommandPool
 * @commandPool        - The command pool which the buffers where allocated from.
 * @commandBufferCount - Amount of VkCommandBuffer's alloocated
 * @commandBuffers     - Pointer to an array of VkCommandBuffer handles
 */
struct uvr_vk_command_buffer {
  VkDevice                            vkDevice;
  VkCommandPool                       commandPool;
  uint32_t                            commandBufferCount;
  struct uvr_vk_command_buffer_handle *commandBuffers;
};


/*
 * struct uvr_vk_command_buffer_create_info (Underview Renderer Vulkan Command Buffer Create Information)
 *
 * members:
 * @vkDevice           - Must pass a valid VkDevice handle (Logical Device)
 * @queueFamilyIndex   - Designates a queue family with VkCommandPool. All command buffers allocated from VkCommandPool
 *                       must used same queue.
 * @commandBufferCount - The amount of command buffers to allocate from a given pool
 */
struct uvr_vk_command_buffer_create_info {
  VkDevice vkDevice;
  uint32_t queueFamilyIndex;
  uint32_t commandBufferCount;
};


/*
 * uvr_vk_command_buffer_create: Function creates a VkCommandPool handle then allocates VkCommandBuffer handles from
 *                               that pool. The amount of VkCommandBuffer's allocated is based upon @commandBufferCount.
 *                               Function only allocates primary command buffers. VkCommandPool flags set
 *                               VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_command_buffer_create_info
 * return:
 *    on success struct uvr_vk_command_buffer
 *    on failure struct uvr_vk_command_buffer { with member nulled }
 */
struct uvr_vk_command_buffer uvr_vk_command_buffer_create(struct uvr_vk_command_buffer_create_info *uvrvk);


/*
 * struct uvr_vk_command_buffer_record_info (Underview Renderer Vulkan Command Buffer Record Information)
 *
 * members:
 * @commandBufferCount - Amount of VkCommandBuffer handles allocated
 * @commandBuffers     - Pointer to an array of struct uvr_vk_command_buffer_handle which contains your actual VkCommandBuffer handles to start writing commands to.
 * @flags              - https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkCommandBufferUsageFlagBits.html
 */
struct uvr_vk_command_buffer_record_info {
  uint32_t                            commandBufferCount;
  struct uvr_vk_command_buffer_handle *commandBuffers;
  VkCommandBufferUsageFlagBits        flags;
};


/*
 * uvr_vk_command_buffer_record_begin: Function starts command buffer recording for command buffers up to @commandBufferCount. Thus, allowing each
 *                                     command buffer to become writeable. Allowing for the application to write commands into it. Theses commands
 *                                     are later put into a queue to be sent off to the GPU.
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_command_buffer_record_info
 * return:
 *    on success 0
 *    on failure -1
 */
int uvr_vk_command_buffer_record_begin(struct uvr_vk_command_buffer_record_info *uvrvk);


/*
 * uvr_vk_command_buffer_record_end: Function stops command buffer to recording. Thus, ending each command buffers ability to accept commands.
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_command_buffer_record_info
 * return:
 *    on success 0
 *    on failure -1
 */
int uvr_vk_command_buffer_record_end(struct uvr_vk_command_buffer_record_info *uvrvk);


/*
 * struct uvr_vk_fence_handle (Underview Renderer Vulkan Fence Handle)
 *
 * @fence - May be used to insert a dependency from a queue to the host. Used to block host (CPU) operations
 *          until commands in a command buffer are finished. Handles CPU - GPU syncs. It is up to host to
 *          set VkFence to an unsignaled state after GPU set it to a signaled state when a resource becomes
 *          available. Host side we wait for that signal then conduct XYZ operations. This is how we block.
 */
struct uvr_vk_fence_handle {
  VkFence fence;
};


/*
 * struct uvr_vk_semaphore_handle (Underview Renderer Vulkan Semaphore Handle)
 *
 * @semaphore - May be used to insert a dependency between queue operations or between a queue operation
 *              and the host. Used to block queue operations until commands in a command buffer are finished.
 *              Handles GPU - GPU syncs. Solely utilized on the GPU itself. Thus, only the GPU can control
 *              the state of a semphore.
 */
struct uvr_vk_semaphore_handle {
  VkSemaphore semaphore;
};


/*
 * struct uvr_vk_sync_obj (Underview Renderer Vulkan Synchronization Objects)
 *
 * members:
 * @vkDevice       - VkDevice handle (Logical Device) associated with @fenceCount VkFence objects
 *                   and @semaphoreCount VkSemaphore objects
 * @fenceCount     - Amount of handles in @vkFence array
 * @vkFences       - Pointer to an array of VkFence handles
 * @semaphoreCount - Amount of handles in @vkSemaphore array
 * @vkSemaphores   - Pointer to an array of VkSemaphore handles
 */
struct uvr_vk_sync_obj {
  VkDevice vkDevice;
  uint32_t fenceCount;
  struct uvr_vk_fence_handle *vkFences;
  uint32_t semaphoreCount;
  struct uvr_vk_semaphore_handle *vkSemaphores;
};


/*
 * struct uvr_vk_sync_obj_create_info (Underview Renderer Vulkan Command Buffer Create Information)
 *
 * members:
 * @vkDevice       - Must pass a valid VkDevice handle (Logical Device)
 * @fenceCount     - Amount of VkFence objects to allocate.
 * @semaphoreCount - Amount of VkSemaphore objects to allocate.
 */
struct uvr_vk_sync_obj_create_info {
  VkDevice vkDevice;
  uint32_t fenceCount;
  uint32_t semaphoreCount;
};


/*
 * uvr_vk_sync_obj_create: Creates VkFence and VkSemaphore synchronization objects. Vulkan API calls that execute work
 *                         on the GPU happen asynchronously. Vulkan API function calls return before operations are fully finished.
 *                         So we need synchronization objects to make sure operations that require other operations to finish can
 *                         happen after.
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_sync_obj_create_info
 * return:
 *    on success struct uvr_vk_sync_obj
 *    on failure struct uvr_vk_sync_obj { with member nulled }
 */
struct uvr_vk_sync_obj uvr_vk_sync_obj_create(struct uvr_vk_sync_obj_create_info *uvrvk);


/*
 * struct uvr_vk_buffer (Underview Renderer Vulkan Buffer)
 *
 * members:
 * @vkDevice       - VkDevice handle (Logical Device) associated with VkBuffer
 * @vkBuffer       - Header of a given buffer that stores information about the buffer
 * @vkDeviceMemory - Actual memory whether CPU or GPU visible associate with VkBuffer header object
 */
struct uvr_vk_buffer {
  VkDevice vkDevice;
  VkBuffer vkBuffer;
  VkDeviceMemory vkDeviceMemory;
};


/*
 * struct uvr_vk_buffer_create_info (Underview Renderer Vulkan Buffer Create Information)
 *
 * members:
 * @vkDevice              - Must pass a valid VkDevice handle (Logical Device)
 * @vkPhdev               -
 * @bufferFlags           - Used when configuring sparse buffer memory
 * @bufferSize            - Size of underlying buffer to allocate
 * @bufferUsage           - Specifies type of buffer (i.e vertex, etc...). Multiple buffer types can be selected here
 *                          via bitwise or operations.
 * @bufferSharingMode     - Vulkan buffers may be owned by one device queue family or shared by multiple device queue families.
 * @queueFamilyIndexCount - Amount of queue families may own given vulkan buffer.
 * @queueFamilyIndices    - Pointer to an array of queue families to associate/own a given vulkan buffer.
 * @memPropertyFlags      - Used to determine the type of actual memory to allocated. Whether CPU (host) or GPU visible.
 */
struct uvr_vk_buffer_create_info {
  VkDevice                 vkDevice;
  VkPhysicalDevice         vkPhdev;
  VkBufferCreateFlagBits   bufferFlags;
  VkDeviceSize             bufferSize;
  VkBufferUsageFlags       bufferUsage;
  VkSharingMode            bufferSharingMode;
  uint32_t                 queueFamilyIndexCount;
  const uint32_t           *queueFamilyIndices;
  VkMemoryPropertyFlagBits memPropertyFlags;
};


/*
 * uvr_vk_buffer_create: Function creates vulkan buffer header and binds actual memory to said vulkan buffer headers. This
 *                       allows host visible data (i.e vertex data) to be given to the GPU.
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_buffer_create_info
 * return:
 *    on success struct uvr_vk_buffer
 *    on failure struct uvr_vk_buffer { with member nulled }
 */
struct uvr_vk_buffer uvr_vk_buffer_create(struct uvr_vk_buffer_create_info *uvrvk);


/*
 * struct uvr_vk_buffer_copy_info (Underview Renderer Vulkan Buffer Copy Information)
 *
 * members:
 * @commandBuffer - Command buffer used for recording. Best to utilize one already create via
 *                  uvr_vk_command_buffer_create(3). To save on unnecessary allocations.
 * @srcBuffer     - Source buffer containing data.
 * @dstBuffer     - Destination buffer to copy source buffer data to.
 * @bufferSize    - Amount of data to copy over in bytes.
 * @vkQueue       - The physical device queue (graphics or transfer) to submit the copy buffer command to.
 */
struct uvr_vk_buffer_copy_info {
  VkCommandBuffer commandBuffer;
  VkBuffer        srcBuffer;
  VkBuffer        dstBuffer;
  uint32_t        bufferSize;
  VkQueue         vkQueue;
};


/*
 * uvr_vk_buffer_copy: Function copies data from one VkBuffer to another. Best utilized when copying
 *                     data from CPU visible buffer over to GPU visible buffer. That way the GPU can
 *                     acquire data (vertex data) more quickly.
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_buffer_copy_info
 * return:
 *    on success 0
 *    on failure -1
 */
int uvr_vk_buffer_copy(struct uvr_vk_buffer_copy_info *uvrvk);


/*
 * struct uvr_vk_destroy (Underview Renderer Vulkan Destroy)
 *
 * members:
 * @vkinst                       - Must pass a valid VkInstance handle
 * @vksurf                       - Must pass a valid VkSurfaceKHR handle
 * @uvr_vk_lgdev_cnt             - Must pass the amount of elements in struct uvr_vk_lgdev array
 * @uvr_vk_lgdev                 - Must pass a pointer to an array of valid struct uvr_vk_lgdev { free'd  members: VkDevice handle }
 * @uvr_vk_swapchain_cnt         - Must pass the amount of elements in struct uvr_vk_swapchain array
 * @uvr_vk_swapchain             - Must pass a pointer to an array of valid struct uvr_vk_swapchain { free'd members: VkSwapchainKHR handle }
 * @uvr_vk_image_cnt             - Must pass the amount of elements in struct uvr_vk_image array
 * @uvr_vk_image                 - Must pass a pointer to an array of valid struct uvr_vk_image { free'd members: VkImageView handle, *views, *images }
 * @uvr_vk_shader_module_cnt     - Must pass the amount of elements in struct uvr_vk_shader_module array
 * @uvr_vk_shader_module         - Must pass a pointer to an array of valid struct uvr_vk_shader_module { free'd members: VkShaderModule handle }
 * @uvr_vk_render_pass_cnt       - Must pass the amount of elements in struct uvr_vk_render_pass array
 * @uvr_vk_render_pass           - Must pass a pointer to an array of valid struct uvr_vk_render_pass { free'd members: VkRenderPass handle }
 * @uvr_vk_pipeline_layout_cnt   - Must pass the amount of elements in struct uvr_vk_pipeline_layout array [VkPipelineLayout Count]
 * @uvr_vk_pipeline_layout       - Must pass a pointer to an array of valid struct uvr_vk_pipeline_layout { free'd members: VkPipelineLayout handle }
 * @uvr_vk_graphics_pipeline_cnt - Must pass the amount of elements in struct uvr_vk_graphics_pipeline array
 * @uvr_vk_graphics_pipeline     - Must pass a pointer to an array of valid struct uvr_vk_graphics_pipeline { free'd members: VkPipeline handle }
 * @uvr_vk_framebuffer_cnt       - Must pass the amount of elements in struct uvr_vk_framebuffer array
 * @uvr_vk_framebuffer           - Must pass a pointer to an array of valid struct uvr_vk_framebuffer { free'd members: VkFramebuffer handle, *vkfbs }
 * @uvr_vk_command_buffer_cnt    - Must pass the amount of elements in struct uvr_vk_command_buffer array
 * @uvr_vk_command_buffer        - Must pass a pointer to an array of valid struct uvr_vk_command_buffer { free'd members: VkCommandPool handle, *vkCommandbuffers }
 * @uvr_vk_sync_obj_cnt          - Must pass the amount of elements in struct uvr_vk_sync_obj array
 * @uvr_vk_sync_obj              - Must pass a pointer to an array of valid struct uvr_vk_sync_obj { free'd members: VkFence handle, VkSemaphore handle, *vkFences, *vkSemaphores }
 * @uvr_vk_buffer_cnt            -
 * @uvr_vk_buffer                -
 */
struct uvr_vk_destroy {
  VkInstance vkinst;
  VkSurfaceKHR vksurf;

  uint32_t uvr_vk_lgdev_cnt;
  struct uvr_vk_lgdev *uvr_vk_lgdev;

  uint32_t uvr_vk_swapchain_cnt;
  struct uvr_vk_swapchain *uvr_vk_swapchain;

  uint32_t uvr_vk_image_cnt;
  struct uvr_vk_image *uvr_vk_image;

  uint32_t uvr_vk_shader_module_cnt;
  struct uvr_vk_shader_module *uvr_vk_shader_module;

  uint32_t uvr_vk_render_pass_cnt;
  struct uvr_vk_render_pass *uvr_vk_render_pass;

  uint32_t uvr_vk_pipeline_layout_cnt;
  struct uvr_vk_pipeline_layout *uvr_vk_pipeline_layout;

  uint32_t uvr_vk_graphics_pipeline_cnt;
  struct uvr_vk_graphics_pipeline *uvr_vk_graphics_pipeline;

  uint32_t uvr_vk_framebuffer_cnt;
  struct uvr_vk_framebuffer *uvr_vk_framebuffer;

  uint32_t uvr_vk_command_buffer_cnt;
  struct uvr_vk_command_buffer *uvr_vk_command_buffer;

  uint32_t uvr_vk_sync_obj_cnt;
  struct uvr_vk_sync_obj *uvr_vk_sync_obj;

  uint32_t uvr_vk_buffer_cnt;
  struct uvr_vk_buffer *uvr_vk_buffer;
};


/*
 * uvr_vk_destory: frees any allocated memory defined by customer
 *
 * args:
 * @uvrvk - pointer to a struct uvr_vk_destroy contains all objects created during
 *          application lifetime in need freeing
 */
void uvr_vk_destory(struct uvr_vk_destroy *uvrvk);

#endif
