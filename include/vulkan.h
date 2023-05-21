#ifndef KMR_VULKAN_H
#define KMR_VULKAN_H

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
#define KMR_VK_INSTANCE_PROC_ADDR(inst, var, func) \
	do { \
		var = (PFN_vk##func) vkGetInstanceProcAddr(inst, "vk" #func); \
	} while(0);


/*
 * Due to Vulkan not directly exposing functions for all platforms.
 * Dynamically (at runtime) retrieve or acquire the address of a VkDevice (logical device) func.
 * Via token concatenation and String-izing Tokens
 */
#define KMR_VK_DEVICE_PROC_ADDR(dev, var, func) \
	do { \
		var = (PFN_vk##func) vkGetDeviceProcAddr(dev, "vk" #func); \
	} while(0);


/*
 * struct kmr_vk_instance_create_info (kmsroots Vulkan Instance Create Information)
 *
 * members:
 * @appName                - A member of the VkApplicationInfo structure reserved for the name of the application.
 * @engineName             - A member of the VkApplicationInfo structure reserved for the name of the engine name
 *                           (if any) used to create application.
 * @enabledLayerCount      - A member of the VkInstanceCreateInfo structure used to pass the number of Vulkan
 *                           Validation Layers a client wants to enable.
 * @enabledLayerNames      - A member of the VkInstanceCreateInfo structure used to pass a pointer to an array
 *                           of strings containing the name of the Vulkan Validation Layers one wants to enable.
 * @enabledExtensionCount  - A member of the VkInstanceCreateInfo structure used to pass the the number of vulkan
 *                           instance extensions a client wants to enable.
 * @enabledExtensionNames  - A member of the VkInstanceCreateInfo structure used to pass a pointer to an array the actual
 *                           of strings containing the name of the Vulkan Instance Extensions one wants to enable.
 */
struct kmr_vk_instance_create_info {
	const char *appName;
	const char *engineName;
	uint32_t   enabledLayerCount;
	const char **enabledLayerNames;
	uint32_t   enabledExtensionCount;
	const char **enabledExtensionNames;
};


/*
 * kmr_vk_create_instance: Creates a VkInstance object and establishes a connection to the Vulkan API.
 *                         It also acts as an easy wrapper that allows one to define instance extensions.
 *                         Instance extensions basically allow developers to define what an app is setup to do.
 *                         So, if a client wants the application to work with wayland surface or X11 surface etc…​
 *                         Client should enable those extensions inorder to gain access to those particular capabilities.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_instance_create_info
 * return:
 *	on success VkInstance handle
 *	on failure VK_NULL_HANDLE
 */
VkInstance kmr_vk_instance_create(struct kmr_vk_instance_create_info *kmsvk);


/*
 * enum kmr_vk_surface_type (kmsroots Vulkan Surface Type)
 *
 * Display server protocol options. ENUM Used by kmr_vk_surface_create
 * to create a VkSurfaceKHR object based upon platform specific information
 */
typedef enum kmr_vk_surface_type {
	KMR_SURFACE_WAYLAND_CLIENT,
	KMR_SURFACE_XCB_CLIENT,
} kmr_vk_surface_type;


/*
 * struct kmr_vk_surface_create_info (kmsroots Vulkan Surface Create Information)
 *
 * members:
 * @surfaceType - Must pass a valid enum kmr_vk_surface_type value. Used in determine what vkCreate*SurfaceKHR
 *                function and associated structs to utilize when creating the VkSurfaceKHR object.
 * @instance    - Must pass a valid VkInstance handle to create/associate surfaces for an application
 * @surface     - Must pass a pointer to a struct wl_surface object
 * @display     - Must pass either a pointer to struct wl_display object or a pointer to an xcb_connection_t
 * @window      - Must pass an xcb_window_t window id or an unsigned int representing XID
 */
struct kmr_vk_surface_create_info {
	kmr_vk_surface_type surfaceType;
	VkInstance          instance;
	void                *surface;
	void                *display;
	unsigned int        window;
};


/*
 * kmr_vk_surface_create: Creates a VkSurfaceKHR object based upon platform specific information about the given surface.
 *                        VkSurfaceKHR are the interface between the window and Vulkan defined images in a given swapchain
 *                        if vulkan swapchain exists.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_surface_create_info
 * return:
 *	on success VkSurfaceKHR handle
 *	on failure VK_NULL_HANDLE
 */
VkSurfaceKHR kmr_vk_surface_create(struct kmr_vk_surface_create_info *kmsvk);


/*
 * struct kmr_vk_phdev_create_info (kmsroots Vulkan Physical Device Create Information)
 *
 * members:
 * @instance   - Must pass a valid VkInstance handle which to find VkPhysicalDevice with
 * @deviceType - Must pass one of the supported VkPhysicalDeviceType's.
 *               https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceType.html
 * @kmsfd      - Must pass a valid kms file descriptor for which a VkPhysicalDevice will be created
 *               if corresponding DRM properties match.
 */
struct kmr_vk_phdev_create_info {
	VkInstance           instance;
	VkPhysicalDeviceType deviceType;
#ifdef INCLUDE_KMS
	int                  kmsfd;
#endif
};


/*
 * struct kmr_vk_phdev (kmsroots Vulkan Physical Device)
 *
 * members:
 * @instance                - Must pass a valid VkInstance handle which to find and create a VkPhysicalDevice with
 * @physDevice              - Must pass one of the supported VkPhysicalDeviceType's.
 *                            https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceType.html
 * @physDeviceProperties    - Structure specifying physical device properties. Like allocation limits for Image Array Layers
 *                            or maximum resolution that the device supports.
 * @physDeviceFeatures      - Structure describing the features that can be supported by an physical device
 * @kmsfd                   - KMS device node file descriptor passed via struct kmr_vk_phdev_create_info
 * @physDeviceDrmProperties - Structure containing DRM information of a physical device. A VkPhysicalDeviceProperties2 structure
 *                            is utilzed to populate this member. Member information is then checked by the implementation to see
 *                            if passed KMS device node file descriptor struct kmr_vk_phdev_create_info { @kmsfd } is equal to the
 *                            physical device suggested by struct kmr_vk_phdev_create_info { @deviceType }. Contains data stored
 *                            after associate a DRM file descriptor with a vulkan physical device.
 */
struct kmr_vk_phdev {
	VkInstance                       instance;
	VkPhysicalDevice                 physDevice;
	VkPhysicalDeviceProperties       physDeviceProperties;
	VkPhysicalDeviceFeatures         physDeviceFeatures;
#ifdef INCLUDE_KMS
	int                              kmsfd;
	VkPhysicalDeviceDrmPropertiesEXT physDeviceDrmProperties;
#endif
};


/*
 * kmr_vk_phdev_create: Retrieves a VkPhysicalDevice handle if certain characteristics of a physical device are meet.
 *                      Also retrieves a given physical device properties and features to be later used by the application.
 *                      Characteristics include @vkPhdevType and @kmsFd.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_phdev_create_info
 * return:
 *	on success struct kmr_vk_phdev
 *	on failure struct kmr_vk_phdev { with members nulled, int's set to -1 }
 */
struct kmr_vk_phdev kmr_vk_phdev_create(struct kmr_vk_phdev_create_info *kmsvk);


/*
 * struct kmr_vk_queue (kmsroots Vulkan Queue)
 *
 * members:
 * @name        - Name of the given queue. This is strictly to give a name to the queue. Not required by API.
 * @queue       - VkQueue handle used when submitting command buffers to physical device. Handle assigned
 *                in kmr_vk_lgdev_create after VkDevice handle creation.
 * @familyIndex - VkQueue family index associate with struct kmr_vk_queue_create_info { @queueFlag } selected
 * @queueCount  - Count of queues in a given VkQueue family
 */
struct kmr_vk_queue {
	char    name[20];
	VkQueue queue;
	int     familyIndex;
	int     queueCount;
};


/*
 * struct kmr_vk_queue_create_info (kmsroots Vulkan Queue Create Information)
 *
 * members:
 * @physDevice - Must pass a valid VkPhysicalDevice handle to query queues associate with phsyical device
 * @queueFlag  - Must pass one VkQueueFlagBits, if multiple flags are or'd function will fail to return VkQueue family index (struct kmr_vk_queue).
 *               https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkQueueFlagBits.html
 */
struct kmr_vk_queue_create_info {
	VkPhysicalDevice physDevice;
	VkQueueFlags     queueFlag;
};


/*
 * kmr_vk_queue_create: Queries the queues a given physical device contains. Then returns a queue
 *                      family index and the queue count given a single VkQueueFlagBits. Queue
 *                      are used in vulkan to submit commands up to the GPU.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_queue_create_info. Contains information on which queue family
 *          we are trying to find and the physical device that supports said queue family.
 * return:
 *	on success struct kmr_vk_queue
 *	on failure struct kmr_vk_queue { with members nulled, int's set to -1 }
 */
struct kmr_vk_queue kmr_vk_queue_create(struct kmr_vk_queue_create_info *kmsvk);


/*
 * struct kmr_vk_lgdev (kmsroots Vulkan Logical Device)
 *
 * members:
 * @logicalDevice - Returned VkDevice handle which represents vulkans access to physical device
 * @queueCount    - Amount of elements in pointer to array of struct kmr_vk_queue. This information
 *                  gets populated with the data pass through struct kmr_vk_lgdev_create_info { member: numqueues }
 * @queues        - Pointer to an array of struct kmr_vk_queue. This information gets populated with the
 *                  data pass through struct kmr_vk_lgdev_create_info { member: queues }
 *
 *                  @queueCount & @queues are strictly for struct kmr_vk_lgdev to have extra information amount VkQueue's
 */
struct kmr_vk_lgdev {
	VkDevice            logicalDevice;
	uint32_t            queueCount;
	struct kmr_vk_queue *queues;
};


/*
 * struct kmr_vk_lgdev_create_info (kmsroots Vulkan Logical Device Create Information)
 *
 * members:
 * @instance              - Must pass a valid VkInstance handle to create VkDevice handle from.
 * @physDevice            - Must pass a valid VkPhysicalDevice handle to associate VkDevice handle with.
 * @enabledFeatures       - Must pass a valid pointer to a VkPhysicalDeviceFeatures with X features enabled
 * @enabledExtensionCount - Must pass the amount of Vulkan Device extensions to enable.
 * @enabledExtensionNames - Must pass an array of strings containing Vulkan Device extension to enable.
 * @queueCount            - Must pass the amount of struct kmr_vk_queue (VkQueue,VkQueueFamily indicies) to
 *                          create along with a given logical device
 * @queues                - Must pass a pointer to an array of struct kmr_vk_queue (VkQueue,VkQueueFamily indicies) to
 *                          create along with a given logical device
 */
struct kmr_vk_lgdev_create_info {
	VkInstance               instance;
	VkPhysicalDevice         physDevice;
	VkPhysicalDeviceFeatures *enabledFeatures;
	uint32_t                 enabledExtensionCount;
	const char *const        *enabledExtensionNames;
	uint32_t                 queueCount;
	struct kmr_vk_queue      *queues;
};


/*
 * kmr_vk_lgdev_create: Creates a VkDevice handle and allows vulkan to have a connection to a given physical device.
 *                      The VkDevice handle is more of a local object its state and operations are local
 *                      to it and are not seen by other logical devices. Function also acts as an easy wrapper
 *                      that allows client to define device extensions. Device extensions basically allow developers
 *                      to define what operations a given logical device is capable of doing. So, if one wants the
 *                      device to be capable of utilizing a swap chain, etc…​ One should enable those extensions
 *                      inorder to gain access to those particular capabilities. Allows for creation of multiple
 *                      VkQueue's although the only one needed is the Graphics queue.
 *
 *                      struct kmr_vk_queue: member 'VkQueue queue' handle is assigned in this function as vkGetDeviceQueue
 *                      requires a logical device handle.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_lgdev_create_info
 * return:
 *	on success struct kmr_vk_lgdev
 *	on failure struct kmr_vk_lgdev { with members nulled, int's set to -1 }
 */
struct kmr_vk_lgdev kmr_vk_lgdev_create(struct kmr_vk_lgdev_create_info *kmsvk);


/*
 * struct kmr_vk_swapchain (kmsroots Vulkan Swapchain)
 *
 * members:
 * @logicalDevice - VkDevice handle (Logical Device) associated with swapchain
 * @swapchain     - Vulkan handle/object representing the swapchain itself
 */
struct kmr_vk_swapchain {
	VkDevice       logicalDevice;
	VkSwapchainKHR swapchain;
};


/*
 * struct kmr_vk_swapchain_create_info (kmsroots Vulkan Swapchain Create Information)
 *
 * members:
 * @logicalDevice         - Must pass a valid VkDevice handle (Logical Device)
 * @surface               - Must pass a valid VkSurfaceKHR handle. From kmr_vk_surface_create(3)
 * @surfaceCapabilities   - Passed the queried surface capabilities. From kmr_vk_get_surface_capabilities(3)
 * @surfaceFormat         - Pass colorSpace & pixel format of choice. Recommend querrying first via kmr_vk_get_surface_formats(3)
 *                          then check if pixel format and colorSpace you want is supported by a given physical device.
 *
 * See: https://khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkSwapchainCreateInfoKHR.html for more info on bellow members
 *
 * @extent2D              - The width and height in pixels of the images in the swapchain.
 * @imageArrayLayers      - Number of views in a multiview/stereo surface
 * @imageUsage            - Intended use of images in swapchain
 * @imageSharingMode      - Sets whether images can only be accessed by a single queue or multiple queues
 * @queueFamilyIndexCount - Amount of queue families that may have access to the swapchain images. Only set if @imageSharingMode is
 *                          not set to VK_SHARING_MODE_EXCLUSIVE.
 * @queueFamilyIndices    - Pointer to an array of queue family indices that have access to images in the swapchain
 * @compositeAlpha        - How to blend images with external graphics
 * @presentMode           - How images are queued and presented internally by the swapchain (FIFO, MAIL_BOX are the only ones known not to lead to tearing)
 * @clipped               - Allow vulkan to clip images not in view. (i.e clip/display part of the image if it's behind a window)
 * @oldSwapChain          - If a swapchain is still in use while a window is resized passing pointer to old swapchain may aid in
 *                          resource reuse as the application is allowed to present images already acquired from swapchain. Thus,
 *                          no need to recreate them.
 */
struct kmr_vk_swapchain_create_info {
	VkDevice                    logicalDevice;
	VkSurfaceKHR                surface;
	VkSurfaceCapabilitiesKHR    surfaceCapabilities;
	VkSurfaceFormatKHR          surfaceFormat;
	VkExtent2D                  extent2D;
	uint32_t                    imageArrayLayers;
	VkImageUsageFlags           imageUsage;
	VkSharingMode               imageSharingMode;
	uint32_t                    queueFamilyIndexCount;
	const uint32_t              *queueFamilyIndices;
	VkCompositeAlphaFlagBitsKHR compositeAlpha;
	VkPresentModeKHR            presentMode;
	VkBool32                    clipped;
	VkSwapchainKHR              oldSwapChain;
};


/*
 * kmr_vk_swapchain_create: Creates VkSwapchainKHR object that provides ability to present renderered results to a given VkSurfaceKHR
 *                          Minimum image count is equal to VkSurfaceCapabilitiesKHR.minImageCount + 1.
 *                          The swapchain can be defined as a set of images that can be drawn to and presented to a surface.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_swapchain_create_info
 * return:
 *	on success struct kmr_vk_swapchain
 *	on failure struct kmr_vk_swapchain { with member nulled }
 */
struct kmr_vk_swapchain kmr_vk_swapchain_create(struct kmr_vk_swapchain_create_info *kmsvk);


/*
 * struct kmr_vk_image_handle (kmsroots Vulkan Image Handle)
 *
 * @image        - Represents actual image itself. May be a texture, etc...
 * @deviceMemory - Actual memory whether CPU or GPU visible associate with VkImage object
 */
struct kmr_vk_image_handle {
	VkImage        image;
	VkDeviceMemory deviceMemory;
};


/*
 * struct kmr_vk_image_view_handle (kmsroots Vulkan Image View Handle)
 *
 * @view - Represents a way to interface with the actual VkImage itself. Describes to
 *         vulkan how to interface with a given VkImage. How to read the given image and
 *         exactly what in the image to read (color channels, etc...)
 */
struct kmr_vk_image_view_handle {
	VkImageView view;
};


/*
 * struct kmr_vk_image (kmsroots Vulkan Image)
 *
 * members:
 * @logicalDevice    - VkDevice handle (Logical Device) associated with VkImageView objects
 * @imageCount       - Amount of VkImage's created. If VkSwapchainKHR reference is passed value would
 *                     be the amount of images in the given swapchain.
 * @imageHandles     - Pointer to an array of VkImage handles
 * @imageViewHandles - Pointer to an array of VkImageView handles
 * @swapchain        - Member not required, but used for storage purposes. A valid VkSwapchainKHR
 *                     reference to the VkSwapchainKHR passed to kmr_vk_image_create. Represents
 *                     the swapchain that created VkImage's.
 */
struct kmr_vk_image {
	VkDevice                         logicalDevice;
	uint32_t                         imageCount;
	struct kmr_vk_image_handle       *imageHandles;
	struct kmr_vk_image_view_handle  *imageViewHandles;
	VkSwapchainKHR                   swapchain;
};


/*
 * struct kmr_vk_image_view_create_info (kmsroots Vulkan Image View Create Information)
 *
 * See: https://khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkImageViewCreateInfo.html for more info on bellow members
 *
 * @imageViewflags             - Specifies additional prameters associated with VkImageView. Normally set to zero.
 * @imageViewType              - Specifies what the image view type is. Specifies coordinate system utilized by the image when
 *                               being addressed. Image view type must have compatible @imageType when @swapchain == VK_NULL_HANDLE.
 * @imageViewFormat            - Image Format (Bits per color channel, the color channel ordering, etc...).
 * @imageViewComponents        - Makes it so that we can select what value goes to what color channel. Basically if we want to assign
 *                               red channel value to green channel. Or set all (RGBA) color channel values to the value at B channel
 *                               this is how we achieve that.
 * @imageViewSubresourceRange  - Gates an image so that only a part of an image is allowed to be viewable.
 *
 */
struct kmr_vk_image_view_create_info {
	VkImageViewCreateFlags   imageViewflags;
	VkImageViewType          imageViewType;
	VkFormat                 imageViewFormat;
	VkComponentMapping       imageViewComponents;
	VkImageSubresourceRange  imageViewSubresourceRange;
};


/*
 * struct kmr_vk_vimage_create_info (kmsroots Vulkan VkImage Create Information)
 *
 * See: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageCreateInfo.html for more info on bellow members
 *
 * @imageflags                 - Bits used to specify additional parameters for a given VkImage.
 * @imageType                  - Coordinate system the pixels in the image will use when being addressed.
 * @imageFormat                - Image Format (Bits per color channel, the color channel ordering, etc...).
 * @imageExtent3D              - Dimension (i.e. width, height, and depth) of the given image(s).
 * @imageMipLevels             - The number of levels of detail available for minified sampling of the image.
 * @imageArrayLayers           - The number of layers in the image
 * @imageSamples               - Bitmask specifying sample counts supported for an image used for storage operations
 * @imageTiling                - Specifies the tiling arrangement (image layout) of data in an image (linear, optimal)
 * @imageUsage                 - Describes to vulkan the intended usage for the VkImage
 * @imageSharingMode           - Vulkan image may be owned by one device queue family or shared by multiple device queue families.
 *                               Sets whether images can only be accessed by a single queue or multiple queues
 * @imageQueueFamilyIndexCount - Array size of @imageQueueFamilyIndices. Amount of queue families may own given vulkan image.
 * @imageQueueFamilyIndices    - Pointer to an array of queue families to associate/own a given vulkan image.
 * @imageInitialLayout         - Set the inital memory layout of a VkImage
 */
struct kmr_vk_vimage_create_info {
	VkImageCreateFlags       imageflags;
	VkImageType              imageType;
	VkFormat                 imageFormat;
	VkExtent3D               imageExtent3D;
	uint32_t                 imageMipLevels;
	uint32_t                 imageArrayLayers;
	VkSampleCountFlagBits    imageSamples;
	VkImageTiling            imageTiling;
	VkImageUsageFlags        imageUsage;
	VkSharingMode            imageSharingMode;
	uint32_t                 imageQueueFamilyIndexCount;
	const uint32_t           *imageQueueFamilyIndices;
	VkImageLayout            imageInitialLayout;
};


/*
 * struct kmr_vk_image_create_info (kmsroots Vulkan Image Create Information)
 *
 * members:
 * @logicalDevice              - Must pass a valid VkDevice handle (Logical Device)
 * @swapchain                  - Must pass a valid VkSwapchainKHR handle. Used when retrieving references to underlying VkImage
 *                               If VkSwapchainKHR reference is not passed value set amount of VkImage/VkImageViews view
 * @imageCount                 - Must pass amount of VkImage's/VkImageView's to create. Set to 0 if @swapchain != VK_NULL_HANDLE
 *                               else set to number images to create.
 * @imageViewCreateInfos       - Pointer to an array of size @imageCount containing everything required to create an individual VkImageView.
 *                               If imageCount given array size must be @imageCount. If not given array size must equal 1.
 *
 * Bellow only required if @swapchain == VK_NULL_HANDLE
 * @imageCreateInfos           - Pointer to an array of size @imageCount containing everything required to create an individual VkImage
 *                               If imageCount given array size must be @imageCount. If not given array size must equal 1.
 * @physDevice                 - Must pass a valid VkPhysicalDevice handle as it is used to query memory properties.
 * @memPropertyFlags           - Used to determine the type of actual memory to allocated. Whether CPU (host) or GPU visible.
 */
struct kmr_vk_image_create_info {
	VkDevice                             logicalDevice;
	VkSwapchainKHR                       swapchain;
	uint32_t                             imageCount;
	struct kmr_vk_image_view_create_info *imageViewCreateInfos;
	struct kmr_vk_vimage_create_info     *imageCreateInfos;
	VkPhysicalDevice                     physDevice;
	VkMemoryPropertyFlagBits             memPropertyFlags;
};


/*
 * kmr_vk_image_create: Function creates/retrieve VkImage's and associates VkImageView's with said images.
 *                      VkImageView's allows a VkImage to be accessed by a shader. If a VkSwapchainKHR
 *                      reference is passed function retrieves all images in the swapchain and uses that
 *                      to associate VkImageView objects. If VkSwapchainKHR reference is not passed function
 *                      creates VkImage object's given passed data and associate VkDeviceMemory & VkImageView
 *                      objects with images. Amount of images created it based upon @imageCount
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_image_create_info
 * return:
 *	on success struct kmr_vk_image
 *	on failure struct kmr_vk_image { with member nulled }
 */
struct kmr_vk_image kmr_vk_image_create(struct kmr_vk_image_create_info *kmsvk);


/*
 * struct kmr_vk_shader_module (kmsroots Vulkan Shader Module)
 *
 * members:
 * @logicalDevice - VkDevice handle (Logical Device) associated with VkShaderModule
 * @shaderModule  - Contains shader code and one or more entry points.
 * @shaderName    - Name given to shader module can be safely ignored not required by API.
 */
struct kmr_vk_shader_module {
	VkDevice       logicalDevice;
	VkShaderModule shaderModule;
	const char     *shaderName;
};


/*
 * struct kmr_vk_shader_module_create_info (kmsroots Vulkan Shader Module Create Information)
 *
 * members:
 * @logicalDevice - Must pass a valid VkDevice handle (Logical Device)
 * @sprivByteSize - Sizeof SPIR-V byte code
 * @sprivBytes    - SPIR-V byte code itself
 * @shaderName    - Name given to shader module can be safely ignored not required by API.
 */
struct kmr_vk_shader_module_create_info {
	VkDevice            logicalDevice;
	size_t              sprivByteSize;
	const unsigned char *sprivBytes;
	const char          *shaderName;
};


/*
 * kmr_vk_shader_module_create: Function creates VkShaderModule from passed SPIR-V byte code.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_shader_module_create_info
 * return:
 *	on success struct kmr_vk_shader_module
 *	on failure struct kmr_vk_shader_module { with member nulled }
 */
struct kmr_vk_shader_module kmr_vk_shader_module_create(struct kmr_vk_shader_module_create_info *kmsvk);


/*
 * struct kmr_vk_pipeline_layout (kmsroots Vulkan Pipeline Layout)
 *
 * members:
 * @logicalDevice  - VkDevice handle (Logical Device) associated with pipeline layout
 * @pipelineLayout - Represents collection of what resources are needed to produce final image
 */
struct kmr_vk_pipeline_layout {
	VkDevice         logicalDevice;
	VkPipelineLayout pipelineLayout;
};


/*
 * struct kmr_vk_pipeline_layout_create_info (kmsroots Vulkan Pipeline Layout Create Information)
 *
 * members:
 * @logicalDevice            - Must pass a valid VkDevice handle (Logical Device)
 *
 * See: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPipelineLayoutCreateInfo.html for more info on bellow members
 *
 * @descriptorSetLayoutCount - Must pass the array size of @descriptorSetLayouts
 * @descriptorSetLayouts     - Must pass a pointer to an array of descriptor set layouts so a given graphics pipeline can know how a shader
 *                             can access a given resource.
 * @pushConstantRangeCount   - Must pass the array size of @descriptorSetLayouts
 * @pushConstantRanges       - Must pass a pointer to an array of push constant definitions that describe at what shader stage and the sizeof
 *                             the data being pushed to shader. If the shader needs to recieve smaller values quickly instead of creating
 *                             a dynamic uniform buffer and updating the value at memory address. Push constants allow for smaller data to
 *                             be more efficiently passed up to the GPU by passing values directly to the shader.
 */
struct kmr_vk_pipeline_layout_create_info {
	VkDevice                    logicalDevice;
	uint32_t                    descriptorSetLayoutCount;
	const VkDescriptorSetLayout *descriptorSetLayouts;
	uint32_t                    pushConstantRangeCount;
	const VkPushConstantRange   *pushConstantRanges;
};


/*
 * kmr_vk_pipeline_layout_create: Function creates a VkPipelineLayout handle that is then later used by the graphics pipeline
 *                                itself so that is knows what resources are need to produce the final image, at what shader
 *                                stages these resources will be accessed, and how to access them. Describes the layout of the
 *                                data that will be given to the pipeline for a single draw operation.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_pipeline_layout_create_info
 * return:
 *	on success struct kmr_vk_pipeline_layout
 *	on failure struct kmr_vk_pipeline_layout { with member nulled }
 */
struct kmr_vk_pipeline_layout kmr_vk_pipeline_layout_create(struct kmr_vk_pipeline_layout_create_info *kmsvk);


/*
 * struct kmr_vk_render_pass (kmsroots Vulkan Render Pass)
 *
 * members:
 * @logicalDevice - VkDevice handle (Logical Device) associated with render pass instance
 * @renderpass    - Represents a collection of attachments, subpasses, and dependencies between the subpasses
 */
struct kmr_vk_render_pass {
	VkDevice     logicalDevice;
	VkRenderPass renderPass;
};


/*
 * struct kmr_vk_render_pass_create_info (kmsroots Vulkan Render Pass Create Information)
 *
 * members:
 * @logicalDevice              - Must pass a valid VkDevice handle (Logical Device)
 *
 * See: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkRenderPassCreateInfo.html for more info on bellow members
 *
 * @attachmentDescriptionCount - Array size of @attachmentDescriptions
 * @attachmentDescriptions     - Describes the type of location to output fragment data to
 *                               Depth attachment outputs to a VkImage used for depth
 *                               Color attachment outputs to a VkImage used for coloring insides of a triangle
 * @subpassDescriptionCount    - Array size of @subpassDescriptions
 * @subpassDescriptions        - What type of pipeline attachments are bounded to (Graphics being the one we want) and the final layout
 *                               og the image before its presented on the screen.
 * @subpassDependenciesCount   - Amount of elements in @subpassDependencies array
 * @subpassDependencies        - Pointer to an array of subpass dependencies that define stages in a pipeline where image
 *                               transitions need to occur before sending output to framebuffer then later the viewport.
 */
struct kmr_vk_render_pass_create_info {
	VkDevice                      logicalDevice;
	uint32_t                      attachmentDescriptionCount;
	const VkAttachmentDescription *attachmentDescriptions;
	uint32_t                      subpassDescriptionCount;
	const VkSubpassDescription    *subpassDescriptions;
	uint32_t                      subpassDependencyCount;
	const VkSubpassDependency     *subpassDependencies;
};


/*
 * kmr_vk_render_pass_create: Function creates a VkRenderPass handle that is then later used by the graphics pipeline
 *                            itself so that is knows how many attachments (color, depth, etc...) there will be per VkFramebuffer,
 *                            how many samples an attachment has (samples to use for multisampling), and how their contents should
 *                            be handled throughout rendering operations. Subpasses within a render pass then references the attachments
 *                            for every draw operations and connects attachments (i.e. VkImage's connect to a VkFramebuffer) to the graphics
 *                            pipeline. In short the render pass is the intermediary step between your graphics pipeline and the framebuffer.
 *                            It describes how you want to render things to the viewport upon render time. Example at render time we wish to
 *                            color in the center of a triangle. We want to give the appears of depth to an image.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_render_pass_create_info
 * return:
 *	on success struct kmr_vk_render_pass
 *	on failure struct kmr_vk_render_pass { with member nulled }
 */
struct kmr_vk_render_pass kmr_vk_render_pass_create(struct kmr_vk_render_pass_create_info *kmsvk);


/*
 * struct kmr_vk_graphics_pipeline (kmsroots Vulkan Graphics Pipeline)
 *
 * members:
 * @logicalDevice    - VkDevice handle (Logical Device) associated with VkPipeline (Graphics Pipeline)
 * @graphicsPipeline - Handle to a pipeline object
 */
struct kmr_vk_graphics_pipeline {
	VkDevice   logicalDevice;
	VkPipeline graphicsPipeline;
};


/*
 * struct kmr_vk_graphics_pipeline_create_info (kmsroots Vulkan Graphics Pipeline Create Information)
 *
 * members:
 * @logicalDevice       - Must pass a valid VkDevice handle (Logical Device)
 *
 * See: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkGraphicsPipelineCreateInfo.html for more info on bellow members
 *
 * @shaderStageCount    - Amount of shaders being used by graphics pipeline
 * @pStages             - Defines shaders and at what shader stages this GP will utilize them
 * @vertexInputState    - Defines the layout and format of vertex input data. Provides details for loading vertex data.
 * @inputAssemblyState  - Defines how to assemble vertices to primitives (i.e. triangles or lines).
 *                        https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPrimitiveTopology.html
 * @tessellationState   -
 * @viewportState       - View port defines how to populated image with pixel data (i.e populate only the top half or bottom half). Scissor defines
 *                        how to crop an image. How much of image should be drawn (i.e draw whole image, right half, middle, etc...)
 * @rasterizationState  - Handles how raw vertex data turns into cordinates on screen and in a pixel buffer. Handle computation of fragments (pixels)
 *                        from primitives (i.e. triangles or lines)
 * @multisampleState    - If you want to do clever anti-aliasing through multisampling. Stores multisampling information.
 * @depthStencilState   - How to handle depth + stencil data. If one has to object we don't want it to draw the back object in the one
 *                        that's in front of it.
 * @colorBlendState     - Defines how to blend fragments at the end of the pipeline.
 * @dynamicState        - Graphics pipelines settings are static once set they can't change. To get new settings you'd have to create a whole new pipeline.
 *                        There are settings however that can be changed at runtime. We define which settings here.
 * @pipelineLayout      - Pass VkPipelineLayout handle to define the resources (i.e. descriptor sets, push constants) given to the pipeline for a single draw operation
 * @renderPass          - Pass VkRenderPass handle which holds a pipeline and handles how it is execute. With final outputs being to a framebuffer.
 *                        One can have multiple smaller subpasses inside of render pass. Used to bind a given render pass to the graphics pipeline.
 *                        Contains multiple attachments that go to all plausible pipeline outputs (i.e Depth, Color, etc..)
 * @subpass             - Pass the index of the subpass to use in the @renderPass instance
 */
struct kmr_vk_graphics_pipeline_create_info {
	VkDevice                                      logicalDevice;
	uint32_t                                      shaderStageCount;
	const VkPipelineShaderStageCreateInfo         *shaderStages;
	const VkPipelineVertexInputStateCreateInfo    *vertexInputState;
	const VkPipelineInputAssemblyStateCreateInfo  *inputAssemblyState;
	const VkPipelineTessellationStateCreateInfo   *tessellationState;
	const VkPipelineViewportStateCreateInfo       *viewportState;
	const VkPipelineRasterizationStateCreateInfo  *rasterizationState;
	const VkPipelineMultisampleStateCreateInfo    *multisampleState;
	const VkPipelineDepthStencilStateCreateInfo   *depthStencilState;
	const VkPipelineColorBlendStateCreateInfo     *colorBlendState;
	const VkPipelineDynamicStateCreateInfo        *dynamicState;
	VkPipelineLayout                              pipelineLayout;
	VkRenderPass                                  renderPass;
	uint32_t                                      subpass;
};


/*
 * kmr_vk_graphics_pipeline_create: Function creates a VkPipeline handle that references a sequence of operations that first takes
 *                                  raw vertices (points on a coordinate plane), textures coordinates, color coordinates, tangent,
 *                                  etc.. [NOTE: Data combinded is called a mesh]. Utilizes shaders to plot the points on a coordinate
 *                                  system then the rasterizer converts all plotted points and turns it into fragments/pixels for your
 *                                  fragment shader to then color in.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_graphics_pipeline_create_info
 * return:
 *	on success struct kmr_vk_graphics_pipeline
 *	on failure struct kmr_vk_graphics_pipeline { with member nulled }
 */
struct kmr_vk_graphics_pipeline kmr_vk_graphics_pipeline_create(struct kmr_vk_graphics_pipeline_create_info *kmsvk);


/*
 * struct kmr_vk_framebuffer_handle (kmsroots Vulkan Framebuffer Handle)
 *
 * @framebuffer - Framebuffers represent a collection of specific memory attachments that a render pass instance uses.
 *                Connection between an image (or images) and the render pass instance.
 */
struct kmr_vk_framebuffer_handle {
	VkFramebuffer framebuffer;
};


/*
 * struct kmr_vk_framebuffer (kmsroots Vulkan Framebuffer)
 *
 * members:
 * @logicalDevice      - VkDevice handle (Logical Device) associated with @framebufferCount VkFramebuffer's
 * @framebufferCount   - Amount of VkFramebuffer handles created
 * @framebufferHandles - Pointer to an array of VkFramebuffer handles
 */
struct kmr_vk_framebuffer {
	VkDevice                          logicalDevice;
	uint8_t                           framebufferCount;
	struct kmr_vk_framebuffer_handle  *framebufferHandles;
};


/*
 * struct kmr_vk_framebuffer_images (kmsroots Vulkan Framebuffer Images)
 *
 * @imageAttachments - Allow at most 6 attachments (VkImageView->VkImage) per VkFrameBbuffer.
 */
struct kmr_vk_framebuffer_images {
	VkImageView imageAttachments[6];
};


/*
 * struct kmr_vk_framebuffer_create_info (kmsroots Vulkan Framebuffer Create Information)
 *
 * members:
 * @logicalDevice                   - Must pass a valid VkDevice handle (Logical Device)
 * @framebufferCount                - Amount of VkFramebuffer handles to create (i.e the array length of @framebufferImages)
 * @framebufferImageAttachmentCount - Amount of framebuffer attachments (VkImageView->VkImage) per each framebuffer.
 * @framebufferImages               - Pointer to an array of VkImageView handles which the @renderPass instance will merge to create final VkFramebuffer.
 *                                    These VkImageView->VkImage handles must always be in a format that equals to the render pass attachment format.
 * @renderPass                      - Defines the render pass a given framebuffer is compatible with
 * @width                           - Framebuffer width in pixels
 * @height                          - Framebuffer height in pixels
 *
 * See: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkFramebufferCreateInfo.html for more info on bellow members
 *
 * @layers
 */
struct kmr_vk_framebuffer_create_info {
	VkDevice                         logicalDevice;
	uint8_t                          framebufferCount;
	uint8_t                          framebufferImageAttachmentCount;
	struct kmr_vk_framebuffer_images *framebufferImages;
	VkRenderPass                     renderPass;
	uint32_t                         width;
	uint32_t                         height;
	uint32_t                         layers;
};


/*
 * kmr_vk_framebuffer_create: Creates @framebufferCount amount of VkFramebuffer handles. Can think of this function as creating the
 *                            frames to hold the pictures in them, with each frame only containing one picture. Note framebuffer
 *                            VkImage's (@framebufferImages->@imageAttachments) must match up one to one with attachments in the
 *                            render pass. Meaning if are @renderPass instance has 1 color + 1 depth attachment. Then each VkFramebuffer
 *                            must have one VkImage for color and one VkImage for depth.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_framebuffer_create_info
 * return:
 *	on success struct kmr_vk_framebuffer
 *	on failure struct kmr_vk_framebuffer { with member nulled }
 */
struct kmr_vk_framebuffer kmr_vk_framebuffer_create(struct kmr_vk_framebuffer_create_info *kmsvk);


/*
 * struct kmr_vk_command_buffer_handle (kmsroots Vulkan Command Buffer Handle)
 *
 * @commandBuffer - Handle used to prerecord commands before they are submitted to a device queue and sent off to the GPU.
 */
struct kmr_vk_command_buffer_handle {
	VkCommandBuffer commandBuffer;
};


/*
 * struct kmr_vk_command_buffer (kmsroots Vulkan Command Buffer)
 *
 * members:
 * @logicalDevice        - VkDevice handle (Logical Device) associated with VkCommandPool
 * @commandPool          - The command pool which the buffers where allocated from.
 * @commandBufferCount   - Amount of VkCommandBuffer's alloocated
 * @commandBufferHandles - Pointer to an array of VkCommandBuffer handles
 */
struct kmr_vk_command_buffer {
	VkDevice                            logicalDevice;
	VkCommandPool                       commandPool;
	uint32_t                            commandBufferCount;
	struct kmr_vk_command_buffer_handle *commandBufferHandles;
};


/*
 * struct kmr_vk_command_buffer_create_info (kmsroots Vulkan Command Buffer Create Information)
 *
 * members:
 * @logicalDevice      - Must pass a valid VkDevice handle (Logical Device)
 * @queueFamilyIndex   - Designates a queue family with VkCommandPool. All command buffers allocated from VkCommandPool
 *                       must used same queue.
 * @commandBufferCount - The amount of command buffers to allocate from a given pool
 */
struct kmr_vk_command_buffer_create_info {
	VkDevice logicalDevice;
	uint32_t queueFamilyIndex;
	uint32_t commandBufferCount;
};


/*
 * kmr_vk_command_buffer_create: Function creates a VkCommandPool handle then allocates VkCommandBuffer handles from
 *                               that pool. The amount of VkCommandBuffer's allocated is based upon @commandBufferCount.
 *                               Function only allocates primary command buffers. VkCommandPool flags set
 *                               VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_command_buffer_create_info
 * return:
 *	on success struct kmr_vk_command_buffer
 *	on failure struct kmr_vk_command_buffer { with member nulled }
 */
struct kmr_vk_command_buffer kmr_vk_command_buffer_create(struct kmr_vk_command_buffer_create_info *kmsvk);


/*
 * struct kmr_vk_command_buffer_record_info (kmsroots Vulkan Command Buffer Record Information)
 *
 * members:
 * @commandBufferCount      - Amount of VkCommandBuffer handles allocated
 * @commandBufferHandles    - Pointer to an array of struct kmr_vk_command_buffer_handle which contains your actual VkCommandBuffer handles to start writing commands to.
 * @commandBufferUsageflags - https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkCommandBufferUsageFlagBits.html
 */
struct kmr_vk_command_buffer_record_info {
	uint32_t                            commandBufferCount;
	struct kmr_vk_command_buffer_handle *commandBufferHandles;
	VkCommandBufferUsageFlagBits        commandBufferUsageflags;
};


/*
 * kmr_vk_command_buffer_record_begin: Function starts command buffer recording for command buffers up to @commandBufferCount. Thus, allowing each
 *                                     command buffer to become writeable. Allowing for the application to write commands into it. Theses commands
 *                                     are later put into a queue to be sent off to the GPU.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_command_buffer_record_info
 * return:
 *	on success 0
 *	on failure -1
 */
int kmr_vk_command_buffer_record_begin(struct kmr_vk_command_buffer_record_info *kmsvk);


/*
 * kmr_vk_command_buffer_record_end: Function stops command buffer to recording. Thus, ending each command buffers ability to accept commands.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_command_buffer_record_info
 * return:
 *	on success 0
 *	on failure -1
 */
int kmr_vk_command_buffer_record_end(struct kmr_vk_command_buffer_record_info *kmsvk);


/*
 * struct kmr_vk_fence_handle (kmsroots Vulkan Fence Handle)
 *
 * @fence - May be used to insert a dependency from a queue to the host. Used to block host (CPU) operations
 *          until commands in a command buffer are finished. Handles CPU - GPU syncs. It is up to host to
 *          set VkFence to an unsignaled state after GPU set it to a signaled state when a resource becomes
 *          available. Host side we wait for that signal then conduct XYZ operations. This is how we block.
 */
struct kmr_vk_fence_handle {
	VkFence fence;
};


/*
 * struct kmr_vk_semaphore_handle (kmsroots Vulkan Semaphore Handle)
 *
 * @semaphore - May be used to insert a dependency between queue operations or between a queue operation
 *              and the host. Used to block queue operations until commands in a command buffer are finished.
 *              Handles GPU - GPU syncs. Solely utilized on the GPU itself. Thus, only the GPU can control
 *              the state of a semphore.
 */
struct kmr_vk_semaphore_handle {
	VkSemaphore semaphore;
};


/*
 * struct kmr_vk_sync_obj (kmsroots Vulkan Synchronization Object)
 *
 * members:
 * @logicalDevice    - VkDevice handle (Logical Device) associated with @fenceCount VkFence objects
 *                     and @semaphoreCount VkSemaphore objects
 * @fenceCount       - Amount of handles in @vkFence array
 * @fenceHandles     - Pointer to an array of VkFence handles
 * @semaphoreCount   - Amount of handles in @vkSemaphore array
 * @semaphoreHandles - Pointer to an array of VkSemaphore handles
 */
struct kmr_vk_sync_obj {
	VkDevice                       logicalDevice;
	uint32_t                       fenceCount;
	struct kmr_vk_fence_handle     *fenceHandles;
	uint32_t                       semaphoreCount;
	struct kmr_vk_semaphore_handle *semaphoreHandles;
};


/*
 * struct kmr_vk_sync_obj_create_info (kmsroots Vulkan Synchronization Object Create Information)
 *
 * members:
 * @logicalDevice  - Must pass a valid VkDevice handle (Logical Device)
 * @semaphoreType  - Specifies the type of semaphore to create.
 * @semaphoreCount - Amount of VkSemaphore objects to allocate. Initial value of each semaphore
 *                   is set to zero.
 * @fenceCount     - Amount of VkFence objects to allocate.
 */
struct kmr_vk_sync_obj_create_info {
	VkDevice        logicalDevice;
	VkSemaphoreType semaphoreType;
	uint8_t         semaphoreCount;
	uint8_t         fenceCount;
};


/*
 * kmr_vk_sync_obj_create: Creates VkFence and VkSemaphore synchronization objects. Vulkan API calls that execute work
 *                         on the GPU happen asynchronously. Vulkan API function calls return before operations are fully finished.
 *                         So we need synchronization objects to make sure operations that require other operations to finish can
 *                         happen after.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_sync_obj_create_info
 * return:
 *	on success struct kmr_vk_sync_obj
 *	on failure struct kmr_vk_sync_obj { with member nulled }
 */
struct kmr_vk_sync_obj kmr_vk_sync_obj_create(struct kmr_vk_sync_obj_create_info *kmsvk);


/*
 * enum kmr_vk_sync_obj_type (kmsroots Vulkan Synchronization Object Type)
 */
typedef enum _kmr_vk_sync_obj_type {
	KMR_VK_SYNC_OBJ_FENCE = 0,
	KMR_VK_SYNC_OBJ_SEMAPHORE = 1
} kmr_vk_sync_obj_type;


/*
 * union kmr_vk_sync_obj_handle (kmsroots Vulkan Synchronization Object Handles)
 *
 * Lessens memory as only one type of Vulkan synchronization primitive is used
 * at a given time.
 */
typedef union _kmr_vk_sync_obj_handle {
	VkFence     fence;
	VkSemaphore semaphore;
} kmr_vk_sync_obj_handle;


/*
 * struct kmr_vk_sync_obj_import_external_sync_fd_info (kmsroots Vulkan Synchronization Object Import External Synchronization File Descriptor Information)
 *
 * members:
 * @logicalDevice - Must pass a valid VkDevice handle (Logical Device)
 * @syncFd        - External Posix file descriptor to import and associate with Vulkan sync object.
 * @syncType      - Specifies the type of Vulkan sync object to bind to.
 * @syncHandle    - Must pass one valid Vulkan sync object @fence or @semaphore.
 */
struct kmr_vk_sync_obj_import_external_sync_fd_info {
	VkDevice               logicalDevice;
	int                    syncFd;
	kmr_vk_sync_obj_type   syncType;
	kmr_vk_sync_obj_handle syncHandle;
};


/*
 * kmr_vk_sync_obj_import_external_sync_fd: From external POSIX DMA-BUF synchronization file descriptor bind to choosen Vulkan
 *                                          synchronization object. The file descriptors can be acquired via a call to
 *                                          kmr_dma_buf_export_sync_file_create(3).
 *
 * args:
 * @kmrvk - pointer to a struct kmr_vk_sync_obj_import_external_sync_fd_info
 * return:
 *	on success 0
 *	on failure -1
 */
int kmr_vk_sync_obj_import_external_sync_fd(struct kmr_vk_sync_obj_import_external_sync_fd_info *kmrvk);


/*
 * struct kmr_vk_sync_obj_export_external_sync_fd_info (kmsroots Vulkan Synchronization Object Export External Synchronization File Descriptor Information)
 *
 * members:
 * @logicalDevice - Must pass a valid VkDevice handle (Logical Device)
 * @syncType      - Specifies the type of Vulkan sync object to bind to.
 * @syncHandle    - Must pass one valid Vulkan sync object @fence or @semaphore.
 */
struct kmr_vk_sync_obj_export_external_sync_fd_info {
	VkDevice               logicalDevice;
	kmr_vk_sync_obj_type   syncType;
	kmr_vk_sync_obj_handle syncHandle;
};


/*
 * kmr_vk_sync_obj_export_external_sync_fd: Creates POSIX file descriptor associated with Vulkan synchronization object.
 *                                          This file descriptor can later be associated with a DMA-BUF fd via
 *                                          kmr_dma_buf_import_sync_file_create(3).
 *
 * args:
 * @kmrvk - pointer to a struct kmr_vk_sync_obj_export_external_sync_fd_info
 * return:
 *	on success posix file descriptor associated
 *	           with Vulkan sync object
 *	on failure -1
 */
int kmr_vk_sync_obj_export_external_sync_fd(struct kmr_vk_sync_obj_export_external_sync_fd_info *kmrvk);


/*
 * struct kmr_vk_buffer (kmsroots Vulkan Buffer)
 *
 * members:
 * @logicalDevice - VkDevice handle (Logical Device) associated with VkBuffer
 * @buffer        - Header of a given buffer that stores information about the buffer
 * @deviceMemory  - Actual memory whether CPU or GPU visible associate with VkBuffer header object
 */
struct kmr_vk_buffer {
	VkDevice       logicalDevice;
	VkBuffer       buffer;
	VkDeviceMemory deviceMemory;
};


/*
 * struct kmr_vk_buffer_create_info (kmsroots Vulkan Buffer Create Information)
 *
 * members:
 * @logicalDevice         - Must pass a valid VkDevice handle (Logical Device)
 * @physDevice            - Must pass a valid VkPhysicalDevice handle as it is used to query memory properties.
 * @bufferFlags           - Used when configuring sparse buffer memory
 * @bufferSize            - Size of underlying buffer to allocate
 * @bufferUsage           - Specifies type of buffer (i.e vertex, etc...). Multiple buffer types can be selected here
 *                          via bitwise or operations.
 * @bufferSharingMode     - Vulkan buffers may be owned by one device queue family or shared by multiple device queue families.
 * @queueFamilyIndexCount - Array size of @queueFamilyIndices. Amount of queue families may own given vulkan buffer.
 * @queueFamilyIndices    - Pointer to an array of queue families to associate/own a given vulkan buffer.
 * @memPropertyFlags      - Used to determine the type of actual memory to allocated. Whether CPU (host) or GPU visible.
 */
struct kmr_vk_buffer_create_info {
	VkDevice                 logicalDevice;
	VkPhysicalDevice         physDevice;
	VkBufferCreateFlagBits   bufferFlags;
	VkDeviceSize             bufferSize;
	VkBufferUsageFlags       bufferUsage;
	VkSharingMode            bufferSharingMode;
	uint32_t                 queueFamilyIndexCount;
	const uint32_t           *queueFamilyIndices;
	VkMemoryPropertyFlagBits memPropertyFlags;
};


/*
 * kmr_vk_buffer_create: Function creates vulkan buffer header and binds actual memory to said vulkan buffer headers. This
 *                       allows host visible data (i.e vertex data) to be given to the GPU.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_buffer_create_info
 * return:
 *	on success struct kmr_vk_buffer
 *	on failure struct kmr_vk_buffer { with member nulled }
 */
struct kmr_vk_buffer kmr_vk_buffer_create(struct kmr_vk_buffer_create_info *kmsvk);


/*
 * struct kmr_vk_descriptor_set_layout (kmsroots Vulkan Descriptor Set Layout)
 *
 * members:
 * @logicalDevice       - VkDevice handle (Logical Device) associated with VkDescriptorSetLayout
 * @descriptorSetLayout - How descriptor set connects up to graphics pipeline. The layout defines
 *                        what type of descriptor set to allocate, a binding link used by vulkan
 *                        to give shader access to resources, and at what graphics pipeline stage
 *                        will the shader descriptor need access to a given resource.
 */
struct kmr_vk_descriptor_set_layout {
	VkDevice              logicalDevice;
	VkDescriptorSetLayout descriptorSetLayout;
};


/*
 * struct kmr_vk_descriptor_set_layout_create_info (kmsroots Vulkan Descriptor Set Layout Create Information)
 *
 * members:
 * @logicalDevice                   - Must pass a valid VkDevice handle (Logical Device)
 * @descriptorSetLayoutCreateflags  - Options for descriptor set layout
 * @descriptorSetLayoutBindingCount - Must pass the amount of elements in VkDescriptorSetLayoutBinding array
 * @descriptorSetLayoutBindings     - Pointer to an array of shader variable (descriptor) attributes. Attributes include the
 *                                    binding location set in the shader, the type of descriptor allowed to be allocated,
 *                                    what graphics pipeline stage will shader have access to vulkan resources assoicated with
 *                                    VkDescriptorSetLayoutBinding { @binding }.
 */
struct kmr_vk_descriptor_set_layout_create_info {
	VkDevice                         logicalDevice;
	VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateflags;
	uint32_t                         descriptorSetLayoutBindingCount;
	VkDescriptorSetLayoutBinding     *descriptorSetLayoutBindings;
};


/*
 * kmr_vk_descriptor_set_layout_create: Function creates descriptor set layout which is used during pipeline creation to
 *                                      define how a given shader may access vulkan resources. Also used during VkDescriptorSet
 *                                      creation to define what type of descriptor of descriptor to allocate within a descriptor
 *                                      set, binding locate used by both vulkan and shader to determine how shader can access vulkan
 *                                      resources, and at what pipeline stage.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_descriptor_set_layout_create_info
 * return:
 *	on success struct kmr_vk_descriptor_set_layout
 *	on failure struct kmr_vk_descriptor_set_layout { with member nulled }
 */
struct kmr_vk_descriptor_set_layout kmr_vk_descriptor_set_layout_create(struct kmr_vk_descriptor_set_layout_create_info *kmsvk);


/*
 * struct kmr_vk_descriptor_set_handle (kmsroots Vulkan Descriptor Set Handle)
 *
 * @descriptorSet - Set of descriptors. Descriptors can be defined as resources shared across draw operations.
 *                  If there is only one uniform buffer object (type of descriptor) in the shader then there is
 *                  only one descriptor in the set. If the uniform buffer object (type of descriptor) in the shader
 *                  is an array of size N. Then there are N descriptors of the same type in a given
 *                  descriptor set. Types of descriptor sets include Images, Samplers, uniform...
 */
struct kmr_vk_descriptor_set_handle {
	VkDescriptorSet descriptorSet;
};


/*
 * struct kmr_vk_descriptor_set (kmsroots Vulkan Descriptor Set)
 *
 * members:
 * @logicalDevice        - VkDevice handle (Logical Device) associated with VkDescriptorPool which contain sets
 * @descriptorPool       - Pointer to a Vulkan Descriptor Pool used to allocate and dellocate descriptor sets.
 * @descriptorSetsCount  - Number of descriptor sets in the @descriptorSetHandles array
 * @descriptorSetHandles - Pointer to an array of descriptor sets
 */
struct kmr_vk_descriptor_set {
	VkDevice                            logicalDevice;
	VkDescriptorPool                    descriptorPool;
	uint32_t                            descriptorSetsCount;
	struct kmr_vk_descriptor_set_handle *descriptorSetHandles;
};


/*
 * struct kmr_vk_descriptor_set_create_info (kmsroots Vulkan Descriptor Set Create Information)
 *
 * members:
 * @logicalDevice             - Must pass a valid VkDevice handle (Logical Device)
 * @descriptorPoolInfos       - Must pass a pointer to an array of descriptor sets information. With each elements information corresponding
 *                              to number of descriptors a given pool needs to allocate the and type of descriptors in the pool. Per my
 *                              understanding this is just so the VkDescriptorPool knows what to preallocate. No descriptor is assigned
 *                              to a set when the pool is created. Given an array of descriptor set layouts the actual assignment of descriptor
 *                              to descriptor set happens in the vkAllocateDescriptorSets function.
 * @descriptorPoolInfoCount   - Number of descriptor sets in pool or array size of @descriptorPoolSetsInfo
 * @descriptorSetLayouts      - Pointer to an array of VkDescriptorSetLayout. Each set must contain it's own layout.
 *                              This variable must be of same size as @descriptorPoolSetsInfo
 * @descriptorSetLayoutCount  - Array size of @descriptorSetLayouts corresponding to the actual number of descriptor
 *                              sets in the pool.
 * @descriptorPoolCreateflags - Enables certain operations on a pool (i.e enabling freeing of descriptor sets)
 */
struct kmr_vk_descriptor_set_create_info {
	VkDevice                    logicalDevice;
	VkDescriptorPoolSize        *descriptorPoolInfos;
	uint32_t                    descriptorPoolInfoCount;
	VkDescriptorSetLayout       *descriptorSetLayouts;
	uint32_t                    descriptorSetLayoutCount;
	VkDescriptorPoolCreateFlags descriptorPoolCreateflags;
};


/*
 * kmr_vk_descriptor_set_create: Function allocates a descriptor pool then @descriptorPoolSetsInfoCount amount of sets
 *                               from descriptor pool. This is how we establishes connection between a the shader
 *                               and vulkan resources at specific graphics pipeline stages. One can think of a
 *                               descriptor pool as a range of addresses that contain segments or sub range addresses.
 *                               Each segment is a descriptor set within the pool and each set contains a certain number
 *                               of descriptors.
 *                                                                   Descriptor Pool
 *                               [--------------------------------------------------------------------------------------------]
 *                                   descriptor set 1   |   descriptor set 2   |   descriptor set 3   |  descriptor set 4
 *                               [---------------------][---------------------][---------------------][-----------------------]
 *                   descriptor: [  1 | 2 | 3 | 4 | 5  ][  1   |   2   |  3   ][           1         ][     1     |     2     ]
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_descriptor_set_create_info
 * return:
 *	on success struct kmr_vk_descriptor_set
 *	on failure struct kmr_vk_descriptor_set { with member nulled }
 */
struct kmr_vk_descriptor_set kmr_vk_descriptor_set_create(struct kmr_vk_descriptor_set_create_info *kmsvk);


/*
 * struct kmr_vk_sampler (kmsroots Vulkan Sampler)
 *
 * members:
 * @logicalDevice - VkDevice handle (Logical Device) associated with VkSampler
 * @sampler       - VkSampler handle represent the state of an image sampler which is used by the implementation
 *                  to read image data and apply filtering and other transformations for the shader.
 *                  TAKEN FROM: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSampler.html
 */
struct kmr_vk_sampler {
	VkDevice  logicalDevice;
	VkSampler sampler;
};


/*
 * struct kmr_vk_sampler_create_info (kmsroots Vulkan Sampler Create Information)
 *
 * members:
 * @logicalDevice - Must pass a valid VkDevice handle (Logical Device)
 *
 * See: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSamplerCreateInfo.html for more information
 *
 * @samplerFlags                   -
 * @samplerMagFilter               - How to render when image is magnified on screen (Camera close to texture)
 * @samplerMinFilter               - How to render when image is minimized on screen (Camera further away from texture)
 * @samplerAddressModeU            - How to handle texture wrap in U (x) direction
 * @samplerAddressModeV            - How to handle texture wrap in U (y) direction
 * @samplerAddressModeW            - How to handle texture wrap in U (z) direction
 * @samplerBorderColor             - Used if @samplerAddressMode{U,V,W} == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
 *                                   set border beyond texture.
 * @samplerMipmapMode              - Mipmap interpolation mode
 * @samplerMipLodBias              - Level of details bias for mip level. Sets value to add to mip levels.
 * @samplerMinLod                  - Minimum level of detail to pick miplevel
 * @samplerMaxLod                  - Maximum level of detail to pick miplevel
 * @samplerUnnormalizedCoordinates - Sets if the texture coordinates should be normalized (between 0 and 1)
 * @samplerAnisotropyEnable        - Enable/Disable Anisotropy
 * @samplerMaxAnisotropy           - Anisotropy sample level
 * @samplerCompareEnable           -
 * @samplerCompareOp               -
 */
struct kmr_vk_sampler_create_info {
	VkDevice                logicalDevice;
	VkSamplerCreateFlags    samplerFlags;
	VkFilter                samplerMagFilter;
	VkFilter                samplerMinFilter;
	VkSamplerAddressMode    samplerAddressModeU;
	VkSamplerAddressMode    samplerAddressModeV;
	VkSamplerAddressMode    samplerAddressModeW;
	VkBorderColor           samplerBorderColor;
	VkBool32                samplerAnisotropyEnable;
	float                   samplerMaxAnisotropy;
	VkBool32                samplerCompareEnable;
	VkCompareOp             samplerCompareOp;
	VkSamplerMipmapMode     samplerMipmapMode;
	float                   samplerMipLodBias;
	float                   samplerMinLod;
	float                   samplerMaxLod;
	VkBool32                samplerUnnormalizedCoordinates;
};


/*
 * kmr_vk_sampler_create: Functions creates VkSampler handle this object accesses the image using pre-defined
 *                        methods. These methods cover concepts such as picking a point between two texels or
 *                        beyond the edge of the image. Describes how an image should be read.
 *
 *                        Loaded images (png, jpg)-> VkImage -> Sampler interacts with the VkImage.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_sampler_create_info
 * return:
 *	on success struct kmr_vk_sampler
 *	on failure struct kmr_vk_sampler { with member nulled }
 */
struct kmr_vk_sampler kmr_vk_sampler_create(struct kmr_vk_sampler_create_info *kmsvk);


/*
 * struct kmr_vk_map_memory_info (kmsroots Vulkan Map Memory Information)
 *
 * members:
 * @logicalDevice      - Must pass a valid VkDevice handle (Logical Device). The device associated with @deviceMemory.
 * @deviceMemory       - Pointer to Vulkan API created memory associated with @logicalDevice
 * @deviceMemoryOffset - Byte offset within @deviceMemory buffer
 * @memoryBufferSize   - Byte size of the data to copy over.
 * @bufferData         - Pointer to memory to copy into @deviceMemory at @deviceMemoryOffset
 */
struct kmr_vk_map_memory_info {
	VkDevice       logicalDevice;
	VkDeviceMemory deviceMemory;
	VkDeviceSize   deviceMemoryOffset;
	VkDeviceSize   memoryBufferSize;
	const void     *bufferData;
};


/*
 * kmr_vk_map_memory: Function maps bytes of buffer data from application generated buffer
 *                    to Vulkan generated buffer. So, that the Vulkan api can have better
 *                    understanding and control over data it utilizes.
 *                    NOTE:
 *                    Use sparingly as consistently mapping and unmapping memory is very inefficient.
 *                    Try to avoid utilizing in render loops. Although that's how it's written
 *                    in multiple examples in this repo.
 * args:
 * @kmsvk - pointer to a struct kmr_vk_map_memory_info
 */
void kmr_vk_map_memory(struct kmr_vk_map_memory_info *kmsvk);


/*
 * enum kmr_vk_copy_info (kmsroots Vulkan Copy Type)
 *
 * ENUM Used by kmr_vk_copy_info to specify type of source
 * resource to copy over to a given type of destination resource.
 */
typedef enum kmr_vk_copy_type {
	KMR_VK_COPY_VK_BUFFER_TO_VK_BUFFER,
	KMR_VK_COPY_VK_BUFFER_TO_VK_IMAGE,
	KMR_VK_COPY_VK_IMAGE_TO_VK_BUFFER
} kmr_vk_copy_type;


/*
 * struct kmr_vk_resource_copy_info (kmsroots Vulkan Resource Copy Information)
 *
 * members:
 * @resourceCopyType     - Determines what vkCmdCopyBuffer* function to utilize
 * @commandBuffer        - Command buffer used for recording. Best to utilize one already create via
 *                         kmr_vk_command_buffer_create(3). To save on unnecessary allocations.
 * @queue                - The physical device queue (graphics or transfer) to submit the copy buffer command to.
 * @srcResource          - Pointer to source vulkan resource containing raw data. (i.e Vkbuffer, VkImage, etc...)
 * @dstResource          - Pointer to destination vulkan resource to copy source resource data to.
 *                         (i.e Vkbuffer, VkImage, etc...)
 * @bufferCopyInfo       - Specifies the starting memory address of @srcResource to copy @byteSize data from
 *                         to @dstResource memory address. Memory address is found by specifying an offset.
 * @bufferImageCopyInfo  - Specifies what portion of the image to update or copy to buffer.
 * @imageLayout          - Memory layout of the destination image subresources for the copy
 */
struct kmr_vk_resource_copy_info {
	kmr_vk_copy_type        resourceCopyType;
	VkCommandBuffer         commandBuffer;
	VkQueue                 queue;
	void                    *srcResource;
	void                    *dstResource;
	VkBufferCopy            *bufferCopyInfo;      // Only allow 1 for now
	VkBufferImageCopy       *bufferImageCopyInfo; // Only allow 1 for now
	VkImageLayout           imageLayout;
};


/*
 * kmr_vk_resource_copy: Function copies data from one vulkan resource to another. Best utilized when
 *                       copying data from CPU visible buffer over to GPU visible buffer. That way the
 *                       GPU can acquire data (vertex data) more quickly.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_resource_copy_info
 * return:
 *	on success 0
 *	on failure -1
 */
int kmr_vk_resource_copy(struct kmr_vk_resource_copy_info *kmsvk);


/*
 * struct kmr_vk_resource_pipeline_barrier_info (kmsroots Vulkan Resource Pipeline Barrier Information)
 *
 * members:
 * @commandBuffer       - Command buffer used for recording. Best to utilize one already create via
 *                        kmr_vk_command_buffer_create(3). To save on unnecessary allocations.
 * @queue               - The physical device queue (graphics or transfer) to submit the pipeline barrier command to.
 * @srcPipelineStage    - Specifies in which pipeline stage operations occur before the barrier.
 * @dstPipelineStage    - Specifies in which pipeline stage operations will wait on the barrier.
 * @VkDependencyFlags   - Defines types of dependencies
 * @memoryBarrier       - specifying pipeline barrier for vulkan memory
 * @bufferMemoryBarrier - Specifies pipeline barrier for vulkan buffer resource
 * @imageMemoryBarrier  - Specifies pipeline barrier for vulkan image resource
 */
struct kmr_vk_resource_pipeline_barrier_info {
	VkCommandBuffer                       commandBuffer;
	VkQueue                               queue;
	VkPipelineStageFlags                  srcPipelineStage;
	VkPipelineStageFlags                  dstPipelineStage;
	VkDependencyFlags                     dependencyFlags;
	VkMemoryBarrier                       *memoryBarrier;
	VkBufferMemoryBarrier                 *bufferMemoryBarrier;
	VkImageMemoryBarrier                  *imageMemoryBarrier;
};


/*
 * kmr_vk_resource_pipeline_barrier: Function is used to synchronize access to vulkan resources. Basically
 *                                   ensuring that a write to a resources finishes before reading from it.
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_resource_pipeline_barrier_info
 * return:
 *	on success 0
 *	on failure -1
 */
int kmr_vk_resource_pipeline_barrier(struct kmr_vk_resource_pipeline_barrier_info *kmsvk);


/*
 * kmr_vk_get_surface_capabilities: Populates the VkSurfaceCapabilitiesKHR struct with supported GPU device surface capabilities.
 *                                  Queries what a physical device is capable of supporting for any given surface.
 *
 * args:
 * @physDev - Must pass a valid VkPhysicalDevice handle
 * @surface - Must pass a valid VkSurfaceKHR handle
 * return:
 *	populated VkSurfaceCapabilitiesKHR
 */
VkSurfaceCapabilitiesKHR kmr_vk_get_surface_capabilities(VkPhysicalDevice physDev, VkSurfaceKHR surface);


/*
 * struct kmr_vk_surface_format (kmsroots Vulkan Surface Format)
 *
 * members:
 * @surfaceFormatCount - Amount of color formats a given surface supports
 * @surfaceFormats     - Pointer to a array of VkSurfaceFormatKHR which stores color space and pixel format
 */
struct kmr_vk_surface_format {
	uint32_t           surfaceFormatCount;
	VkSurfaceFormatKHR *surfaceFormats;
};


/*
 * kmr_vk_get_surface_formats: Creates block of memory with all supported color space's and pixel formats a given physical device
 *                             supports for any given surface. Application must free struct kmr_vk_surface_format { member: surfaceFormats }
 *
 * args:
 * @physDev - Must pass a valid VkPhysicalDevice handle
 * @surface - Must pass a valid VkSurfaceKHR handle
 * return:
 *	on success struct kmr_vk_surface_format
 *	on failure struct kmr_vk_surface_format { with member nulled }
 */
struct kmr_vk_surface_format kmr_vk_get_surface_formats(VkPhysicalDevice physDev, VkSurfaceKHR surface);


/*
 * struct kmr_vk_surface_present_mode (kmsroots Vulkan Surface Present Mode)
 *
 * members:
 * @presentModeCount - Amount of present modes a given surface supports
 * @presentModes     - Pointer to an array of VkPresentModeKHR which stores values of potential surface present modes
 */
struct kmr_vk_surface_present_mode {
	uint32_t         presentModeCount;
	VkPresentModeKHR *presentModes;
};


/*
 * kmr_vk_get_surface_present_modes: Creates block of memory with all supported presentation modes for a surface
 *                                   Application must free struct kmr_vk_surface_present_mode { member: presentModes }
 *                                   More information on presentation modes can be found here:
 *                                   https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPresentModeKHR.html
 *
 * args:
 * @physDev - Must pass a valid VkPhysicalDevice handle
 * @surface - Must pass a valid VkSurfaceKHR handle
 * return:
 *	on success struct kmr_vk_surface_present_mode
 *	on failure struct kmr_vk_surface_present_mode { with member nulled }
 */
struct kmr_vk_surface_present_mode kmr_vk_get_surface_present_modes(VkPhysicalDevice physDev, VkSurfaceKHR surface);


/*
 * struct kmr_vk_phdev_format_prop (kmsroots Vulkan Surface Present Mode)
 *
 * members:
 * @formatProperties    - Pointer to an array of type (VkFormatProperties) specifying a given image format (VkFormat) properties
 * @formatPropertyCount - The amount of elements contained in @formatProperties array
 */
struct kmr_vk_phdev_format_prop {
	VkFormatProperties *formatProperties;
	uint32_t           formatPropertyCount;
};


/*
 * kmr_vk_get_phdev_format_properties: Queries a given physical device supported format properties
 *                                     Application must free struct kmr_vk_phdev_format_prop { member: formatProperties }
 *
 * args:
 * @physDev     - Must pass a valid VkPhysicalDevice handle
 * @formats     - Must pass a pointer to an array of type VkFormat to get VkFormatProperties from
 * @formatCount - Must pass the amount of elements in @formats array
 * return:
 *	on success struct kmr_vk_phdev_format_prop
 *	on failure struct kmr_vk_phdev_format_prop { with member nulled }
 */
struct kmr_vk_phdev_format_prop kmr_vk_get_phdev_format_properties(VkPhysicalDevice physDev, VkFormat *formats, uint32_t formatCount);


/*
 * kmr_vk_get_external_semaphore_properties:
 *
 *
 * args:
 * @physDev    - Must pass a valid VkPhysicalDevice handle
 * @handleType - Must pass bitwise or value specifying the external semaphore
 *               handle type for which capabilities/properties will be returned. 
 * return:
 *	Populated VkExternalSemaphoreProperties
 */
VkExternalSemaphoreProperties kmr_vk_get_external_semaphore_properties(VkPhysicalDevice physDev, VkExternalSemaphoreHandleTypeFlagBits handleType);


/*
 * struct kmr_vk_destroy (kmsroots Vulkan Destroy)
 *
 * members:
 * @instance                         - Must pass a valid VkInstance handle
 * @surface                          - Must pass a valid VkSurfaceKHR handle
 * @kmr_vk_lgdev_cnt                 - Must pass the amount of elements in struct kmr_vk_lgdev array
 * @kmr_vk_lgdev                     - Must pass a pointer to an array of valid struct kmr_vk_lgdev { free'd  members: VkDevice handle }
 * @kmr_vk_swapchain_cnt             - Must pass the amount of elements in struct kmr_vk_swapchain array
 * @kmr_vk_swapchain                 - Must pass a pointer to an array of valid struct kmr_vk_swapchain { free'd members: VkSwapchainKHR handle }
 * @kmr_vk_image_cnt                 - Must pass the amount of elements in struct kmr_vk_image array
 * @kmr_vk_image                     - Must pass a pointer to an array of valid struct kmr_vk_image { free'd members: VkImageView handle, *views, *images }
 * @kmr_vk_shader_module_cnt         - Must pass the amount of elements in struct kmr_vk_shader_module array
 * @kmr_vk_shader_module             - Must pass a pointer to an array of valid struct kmr_vk_shader_module { free'd members: VkShaderModule handle }
 * @kmr_vk_render_pass_cnt           - Must pass the amount of elements in struct kmr_vk_render_pass array
 * @kmr_vk_render_pass               - Must pass a pointer to an array of valid struct kmr_vk_render_pass { free'd members: VkRenderPass handle }
 * @kmr_vk_pipeline_layout_cnt       - Must pass the amount of elements in struct kmr_vk_pipeline_layout array [VkPipelineLayout Count]
 * @kmr_vk_pipeline_layout           - Must pass a pointer to an array of valid struct kmr_vk_pipeline_layout { free'd members: VkPipelineLayout handle }
 * @kmr_vk_graphics_pipeline_cnt     - Must pass the amount of elements in struct kmr_vk_graphics_pipeline array
 * @kmr_vk_graphics_pipeline         - Must pass a pointer to an array of valid struct kmr_vk_graphics_pipeline { free'd members: VkPipeline handle }
 * @kmr_vk_framebuffer_cnt           - Must pass the amount of elements in struct kmr_vk_framebuffer array
 * @kmr_vk_framebuffer               - Must pass a pointer to an array of valid struct kmr_vk_framebuffer { free'd members: VkFramebuffer handle, *vkfbs }
 * @kmr_vk_command_buffer_cnt        - Must pass the amount of elements in struct kmr_vk_command_buffer array
 * @kmr_vk_command_buffer            - Must pass a pointer to an array of valid struct kmr_vk_command_buffer { free'd members: VkCommandPool handle, *vkCommandbuffers }
 * @kmr_vk_sync_obj_cnt              - Must pass the amount of elements in struct kmr_vk_sync_obj array
 * @kmr_vk_sync_obj                  - Must pass a pointer to an array of valid struct kmr_vk_sync_obj { free'd members: VkFence handle, VkSemaphore handle, *vkFences, *vkSemaphores }
 * @kmr_vk_buffer_cnt                - Must pass the amount of elements in struct kmr_vk_buffer array
 * @kmr_vk_buffer                    - Must pass a pointer to an array of valid struct kmr_vk_buffer { free'd members: VkBuffer handle, VkDeviceMemory handle }
 * @kmr_vk_descriptor_set_layout_cnt - Must pass the amount of elements in struct kmr_vk_descriptor_set_layout array
 * @kmr_vk_descriptor_set_layout     - Must pass a pointer to an array of valid struct kmr_vk_descriptor_set_layout { free'd members: VkDescriptorSetLayout handle }
 * @kmr_vk_descriptor_set_cnt        - Must pass the amount of elements in struct kmr_vk_descriptor_set array
 * @kmr_vk_descriptor_set            - Must pass a pointer to an array of valid struct kmr_vk_descriptor_set { free'd members: VkDescriptorPool handle, *descriptorSets }
 * @kmr_vk_sampler_cnt               - Must pass the amount of elements in struct kmr_vk_sampler array
 * @kmr_vk_sampler                   - Must pass a pointer to an array of valid struct kmr_vk_sampler { free'd members: VkSampler handle }
 */
struct kmr_vk_destroy {
	VkInstance instance;
	VkSurfaceKHR surface;

	uint32_t kmr_vk_lgdev_cnt;
	struct kmr_vk_lgdev *kmr_vk_lgdev;

	uint32_t kmr_vk_swapchain_cnt;
	struct kmr_vk_swapchain *kmr_vk_swapchain;

	uint32_t kmr_vk_image_cnt;
	struct kmr_vk_image *kmr_vk_image;

	uint32_t kmr_vk_shader_module_cnt;
	struct kmr_vk_shader_module *kmr_vk_shader_module;

	uint32_t kmr_vk_render_pass_cnt;
	struct kmr_vk_render_pass *kmr_vk_render_pass;

	uint32_t kmr_vk_pipeline_layout_cnt;
	struct kmr_vk_pipeline_layout *kmr_vk_pipeline_layout;

	uint32_t kmr_vk_graphics_pipeline_cnt;
	struct kmr_vk_graphics_pipeline *kmr_vk_graphics_pipeline;

	uint32_t kmr_vk_framebuffer_cnt;
	struct kmr_vk_framebuffer *kmr_vk_framebuffer;

	uint32_t kmr_vk_command_buffer_cnt;
	struct kmr_vk_command_buffer *kmr_vk_command_buffer;

	uint32_t kmr_vk_sync_obj_cnt;
	struct kmr_vk_sync_obj *kmr_vk_sync_obj;

	uint32_t kmr_vk_buffer_cnt;
	struct kmr_vk_buffer *kmr_vk_buffer;

	uint32_t kmr_vk_descriptor_set_layout_cnt;
	struct kmr_vk_descriptor_set_layout *kmr_vk_descriptor_set_layout;

	uint32_t kmr_vk_descriptor_set_cnt;
	struct kmr_vk_descriptor_set *kmr_vk_descriptor_set;

	uint32_t kmr_vk_sampler_cnt;
	struct kmr_vk_sampler *kmr_vk_sampler;
};


/*
 * kmr_vk_destroy: frees any allocated memory defined by customer
 *
 * args:
 * @kmsvk - pointer to a struct kmr_vk_destroy contains all objects created during
 *          application lifetime in need freeing
 */
void kmr_vk_destroy(struct kmr_vk_destroy *kmsvk);

#endif
