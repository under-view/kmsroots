.. default-domain:: C

Vulkan
======

Header: kmsroots/vulkan.h

Table of contents (click to go)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Macros:

1. :c:macro:`KMR_VK_INSTANCE_PROC_ADDR`
#. :c:macro:`KMR_VK_DEVICE_PROC_ADDR`

Enums:

1. :c:enum:`kmr_vk_surface_type`

Structs:

1. :c:struct:`kmr_vk_instance_create_info`
#. :c:struct:`kmr_vk_surface_create_info`
#. :c:struct:`kmr_vk_phdev`
#. :c:struct:`kmr_vk_phdev_create_info`
#. :c:struct:`kmr_vk_queue`
#. :c:struct:`kmr_vk_queue_create_info`
#. :c:struct:`kmr_vk_lgdev`
#. :c:struct:`kmr_vk_lgdev_create_info`
#. :c:struct:`kmr_vk_swapchain`
#. :c:struct:`kmr_vk_swapchain_create_info`
#. :c:struct:`kmr_vk_image_handle`
#. :c:struct:`kmr_vk_image_view_handle`
#. :c:struct:`kmr_vk_image`
#. :c:struct:`kmr_vk_image_view_create_info`
#. :c:struct:`kmr_vk_vimage_create_info`
#. :c:struct:`kmr_vk_image_create_info`

Functions:

1. :c:func:`kmr_vk_instance_create`
#. :c:func:`kmr_vk_surface_create`
#. :c:func:`kmr_vk_phdev_create`
#. :c:func:`kmr_vk_queue_create`
#. :c:func:`kmr_vk_lgdev_create`
#. :c:func:`kmr_vk_swapchain_create`
#. :c:func:`kmr_vk_image_create`

API Documentation
~~~~~~~~~~~~~~~~~

.. c:macro:: KMR_VK_INSTANCE_PROC_ADDR

	Due to Vulkan not directly exposing functions for all platforms.
	Dynamically (at runtime) retrieve or acquire the address of a
	`VkInstance`_ function. Via token concatenation and String-izing Tokens.

	.. code-block::

		#define KMR_VK_INSTANCE_PROC_ADDR(inst, var, func) \
			do { \
				var = (PFN_vk##func) vkGetInstanceProcAddr(inst, "vk" #func); \
				assert(var); \
			} while(0);

===========================================================================================================

.. c:macro:: KMR_VK_DEVICE_PROC_ADDR

	Due to Vulkan not directly exposing functions for all platforms.
	Dynamically (at runtime) retrieve or acquire the address of a
	`VkDevice`_ (logical device) function. Via token concatenation
	and String-izing Tokens

	.. code-block::

		#define KMR_VK_DEVICE_PROC_ADDR(dev, var, func) \
			do { \
				var = (PFN_vk##func) vkGetDeviceProcAddr(dev, "vk" #func); \
				assert(var); \
			} while(0);

===========================================================================================================

.. c:struct:: kmr_vk_instance_create_info

        .. c:member::
                const char *appName;
                const char *engineName;
                uint32_t   enabledLayerCount;
                const char **enabledLayerNames;
                uint32_t   enabledExtensionCount;
                const char **enabledExtensionNames;

        :c:member:`appName`
		| A member of the `VkApplicationInfo`_ structure reserved for the name of the application.

        :c:member:`engineName`
		| A member of the `VkApplicationInfo`_ structure reserved for the name of the engine
		| name (if any) used to create application.

        :c:member:`enabledLayerCount`
		| A member of the `VkInstanceCreateInfo`_ structure used to pass the number of Vulkan
		| Validation Layers a client wants to enable.

        :c:member:`enabledLayerNames`
		| A member of the `VkInstanceCreateInfo`_ structure used to pass a pointer to an array
		| of strings containing the name of the Vulkan Validation Layers one wants to enable.

        :c:member:`enabledExtensionCount`
		| A member of the `VkInstanceCreateInfo`_ structure used to pass the the number of vulkan
		| instance extensions a client wants to enable.

        :c:member:`enabledExtensionNames`
		| A member of the `VkInstanceCreateInfo`_ structure used to pass a pointer to an array
		| of strings containing the name of the Vulkan Instance Extensions one wants to enable.

.. c:function:: VkInstance kmr_vk_instance_create(struct kmr_vk_instance_create_info *kmrvk);

        Creates a `VkInstance`_ object which allows the Vulkan API to better reference & store object
	state/data. It also acts as an easy wrapper that allows one to define instance extensions.
        `VkInstance`_ extensions basically allow developers to define what an app is setup to do.
        So, if a client wants the application to work with wayland surface or X11 surface etc...
        Client should enable those extensions inorder to gain access to those particular capabilities.

	Parameters:
		| **kmrvk:** pointer to a struct :c:struct:`kmr_vk_instance_create_info`

        Returns:
                | **on success:** `VkInstance`_
                | **on faliure:** `VK_NULL_HANDLE`_

===========================================================================================================

.. c:enum:: kmr_vk_surface_type

        .. c:macro::
                KMR_SURFACE_WAYLAND_CLIENT
                KMR_SURFACE_XCB_CLIENT

        Display server protocol options. ENUM used by :c:func:`kmr_vk_surface_create`
        to create a `VkSurfaceKHR`_ object based upon platform specific information

.. c:struct:: kmr_vk_surface_create_info

        .. c:member::
                kmr_vk_surface_type surfaceType;
                VkInstance          instance;
                void                *surface;
                void                *display;
                unsigned int        window;

        :c:member:`surfaceType`
		| Must pass a valid enum :c:enum:`kmr_vk_surface_type` value. Used in determine what
		| vkCreate*SurfaceKHR function and associated structs to utilize when creating the
		| `VkSurfaceKHR`_ object.

        :c:member:`instance`
		| Must pass a valid `VkInstance`_ handle to associate `VkSurfaceKHR`_ with a `VkInstance`_.

        :c:member:`surface`
		| Must pass a pointer to a struct wl_surface object

        :c:member:`display`
		| Must pass either a pointer to struct wl_display object or a pointer to an xcb_connection_t

        :c:member:`window`
		| Must pass an xcb_window_t window id or an unsigned int representing XID

.. c:function:: VkSurfaceKHR kmr_vk_surface_create(struct kmr_vk_surface_create_info *kmrvk);

        Creates a `VkSurfaceKHR`_ object based upon platform specific information about the given surface.
        `VkSurfaceKHR`_ are the interface between the window and Vulkan defined images in a given swapchain
        if vulkan swapchain exists.

        Parameters:
		| **kmrvk:** pointer to a struct :c:struct:`kmr_vk_surface_create_info`

        Returns:
                | **on success:** `VkSurfaceKHR`_
                | **on faliure:** `VK_NULL_HANDLE`_

===========================================================================================================

.. c:struct:: kmr_vk_phdev

	.. c:member::
		VkInstance                       instance;
		VkPhysicalDevice                 physDevice;
		VkPhysicalDeviceProperties       physDeviceProperties;
		VkPhysicalDeviceFeatures         physDeviceFeatures;
		int                              kmsfd;
		VkPhysicalDeviceDrmPropertiesEXT physDeviceDrmProperties;

	:c:member:`instance`
		| Must pass a valid `VkInstance`_ handle associated with `VkPhysicalDevice`_.

	:c:member:`physDevice`
		| Must pass one of the supported `VkPhysicalDeviceType`_'s.

	:c:member:`physDeviceProperties`
		| Structure specifying physical device properties. Like allocation limits for Image
		| Array Layers or maximum resolution that the device supports.

	:c:member:`physDeviceFeatures`
		| Structure describing the features that can be supported by an physical device

	**Only included if meson option kms set true**

	:c:member:`kmsfd`
		| KMS device node file descriptor passed via struct :c:struct:`kmr_vk_phdev_create_info`

	:c:member:`physDeviceDrmProperties`
		| Structure containing DRM information of a physical device. A `VkPhysicalDeviceProperties2`_
		| structure is utilized to populate this member. Member information is then checked by the
		| implementation to see if passed KMS device node file descriptor
		| (struct :c:struct:`kmr_vk_phdev_create_info` { **@kmsfd** }) is equal to the physical device
		| suggested by (struct :c:struct:`kmr_vk_phdev_create_info` { **@deviceType** }).
		| Contains data stored after associating a DRM file descriptor with a vulkan physical device.

.. c:struct:: kmr_vk_phdev_create_info

	.. c:member::
		VkInstance           instance;
		VkPhysicalDeviceType deviceType;
		int                  kmsfd;

	:c:member:`instance`
		| Must pass a valid `VkInstance`_ handle which to find `VkPhysicalDevice`_ with.

	:c:member:`deviceType`
		| Must pass one of the supported `VkPhysicalDeviceType`_'s.

	**Only included if meson option kms set true**

	:c:member:`kmsfd`
		| Must pass a valid kms file descriptor for which a `VkPhysicalDevice`_ will be created
		| if corresponding DRM properties match.

.. c:function::	struct kmr_vk_phdev kmr_vk_phdev_create(struct kmr_vk_phdev_create_info *kmrvk);

	Retrieves a `VkPhysicalDevice`_ handle if certain characteristics of a physical device are meet.
	Also retrieves a given physical device properties and features to be later used by the application.

	Parameters:
		| **kmrvk:** pointer to a struct :c:struct:`kmr_vk_phdev_create_info`

	Returns:
		| **on success:** struct :c:struct:`kmr_vk_phdev`
		| **on failure:** struct :c:struct:`kmr_vk_phdev` { with members nulled, int's set to -1 }

===========================================================================================================

.. c:struct:: kmr_vk_queue

	.. c:member::
		char    name[20];
		VkQueue queue;
		int     familyIndex;
		int     queueCount;

	:c:member:`name`
		| Stores the name of the queue in string format. **Not required by API**.

	:c:member:`queue`
		| `VkQueue`_ handle used when submitting command buffers to physical device. Address
		| given to handle in :c:func:`kmr_vk_lgdev_create` after `VkDevice`_ handle creation.

	:c:member:`familyIndex`
		| `VkQueue`_ family index associate with selected struct :c:struct:`kmr_vk_queue_create_info`
		| { **@queueFlag** }.

	:c:member:`queueCount`
		| Number of queues in a given `VkQueue`_ family

.. c:struct:: kmr_vk_queue_create_info

	.. c:member::
		VkPhysicalDevice physDevice;
		VkQueueFlags     queueFlag;

	:c:member:`physDevice`
		| Must pass a valid `VkPhysicalDevice`_ handle to query queues associate with phsyical device

	:c:member:`queueFlag`
		| Must pass one `VkQueueFlagBits`_, if multiple flags are bitwised or'd function will fail
		| to return `VkQueue`_ family index (struct :c:struct:`kmr_vk_queue`).

.. c:function::	struct kmr_vk_queue kmr_vk_queue_create(struct kmr_vk_queue_create_info *kmrvk);

	Queries the queues a given physical device contains. Then returns a queue
	family index and the queue count given a single `VkQueueFlagBits`_. Queue
	are used in vulkan to submit commands up to the GPU.

	Parameters:
		| **kmrvk:** pointer to a struct :c:struct:`kmr_vk_queue_create_info`

	Returns:
		| **on success:** struct :c:struct:`kmr_vk_queue`
		| **on failure:** struct :c:struct:`kmr_vk_queue` { with members nulled, int's set to -1 }

===========================================================================================================

.. c:struct:: kmr_vk_lgdev

	.. c:member::
		VkDevice            logicalDevice;
		uint32_t            queueCount;
		struct kmr_vk_queue *queues;

	:c:member:`logicalDevice`
		| Returned `VkDevice`_ handle which represents vulkan's access to physical device

	:c:member:`queueCount`
		| Amount of elements in pointer to array of struct :c:struct:`kmr_vk_queue`. This information
		| gets populated with the data pass through struct :c:struct:`kmr_vk_lgdev_create_info`
		| { **@queueCount** }.

	:c:member:`queues`
		| Pointer to an array of struct :c:struct:`kmr_vk_queue`. This information gets populated with the
		| data pass through struct :c:struct:`kmr_vk_lgdev_create_info` { **@queues** }.

		| Members :c:member:`queueCount` & :c:member:`queues` are strictly for struct :c:struct:`kmr_vk_lgdev`
		| to have extra information amount `VkQueue`_'s

.. c:struct:: kmr_vk_lgdev_create_info

	.. c:member::
		VkInstance               instance;
		VkPhysicalDevice         physDevice;
		VkPhysicalDeviceFeatures *enabledFeatures;
		uint32_t                 enabledExtensionCount;
		const char *const        *enabledExtensionNames;
		uint32_t                 queueCount;
		struct kmr_vk_queue      *queues;

	:c:member:`instance`
		| Must pass a valid `VkInstance`_ handle to create `VkDevice`_ handle from.

	:c:member:`physDevice`
		| Must pass a valid `VkPhysicalDevice`_ handle to associate `VkDevice`_ handle with.

	:c:member:`enabledFeatures`
		| Must pass a valid pointer to a `VkPhysicalDeviceFeatures`_ with X features enabled

	:c:member:`enabledExtensionCount`
		| Must pass the amount of Vulkan Device extensions to enable.

	:c:member:`enabledExtensionNames`
		| Must pass an array of strings containing Vulkan Device extension to enable.

	:c:member:`queueCount`
		| Must pass the amount of struct :c:struct:`kmr_vk_queue` { **@queue**, **@familyIndex** } to
		| create along with a given logical device

	:c:member:`queues`
		| Must pass a pointer to an array of struct :c:struct:`kmr_vk_queue` { **@queue**, **@familyIndex** } to
		| create along with a given logical device

.. c:function:: struct kmr_vk_lgdev kmr_vk_lgdev_create(struct kmr_vk_lgdev_create_info *kmrvk);

	Creates a `VkDevice`_ handle and allows vulkan to have a connection to a given physical device.
	The `VkDevice`_ handle is more of a local object its state and operations are local
	to it and are not seen by other logical devices. Function also acts as an easy wrapper
	that allows client to define device extensions. Device extensions basically allow developers
	to define what operations a given logical device is capable of doing. So, if one wants the
	device to be capable of utilizing a swap chain, etc... You have to enable those extensions
	inorder to gain access to those particular capabilities. Allows for creation of multiple
	`VkQueue`_'s although the only one we needis the Graphics queue.

	struct :c:struct:`kmr_vk_queue` { **@queue** } handle is assigned in this function as `vkGetDeviceQueue`_
	requires a logical device handle.

	Parameters:
		| **kmrvk:** pointer to a struct :c:struct:`kmr_vk_lgdev_create_info`

	Returns:
		| **on success:** struct :c:struct:`kmr_vk_lgdev`
		| **on failure:** struct :c:struct:`kmr_vk_lgdev` { with members nulled, int's set to -1 }

===========================================================================================================

.. c:struct:: kmr_vk_swapchain

	.. c:member::
		VkDevice       logicalDevice;
		VkSwapchainKHR swapchain;

	:c:member:`logicalDevice`
		| `VkDevice`_ handle (Logical Device) that stores `VkSwapchainKHR`_ state/data.

	:c:member:`swapchain`
		| Vulkan object storing reference to swapchain state/data.

.. c:struct:: kmr_vk_swapchain_create_info

	.. c:member::
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

	Most members may also be located at `VkSwapchainCreateInfoKHR`_.

	:c:member:`logicalDevice`
		| Must pass a valid `VkDevice`_ handle (Logical Device) to associate swapchain state/data with.

	:c:member:`surface`
		| Must pass a valid `VkSurfaceKHR`_ handle. Can be acquired with a call to
		| :c:func:`kmr_vk_surface_create`

	:c:member:`surfaceCapabilities`
		| Passed the queried surface capabilities. Can be acquired with a call to
		| :c:func:`kmr_vk_get_surface_capabilities`.

	:c:member:`surfaceFormat`
		| Pass colorSpace & pixel format of choice. Recommend querrying first via
		| :c:func:`kmr_vk_get_surface_formats` then check if pixel format and colorSpace
		| you want is supported by a given physical device.

	:c:member:`extent2D`
		| The width and height in pixels of the images in the `VkSwapchainKHR`_.

	:c:member:`imageArrayLayers`
		| Number of views in a multiview/stereo surface.

	:c:member:`imageUsage`
		| Intended use of images in `VkSwapchainKHR`_.

	:c:member:`imageSharingMode`
		| Sets whether images can only be accessed by a single `VkQueue`_ or multiple `VkQueue`_'s.

	:c:member:`queueFamilyIndexCount`
		| Amount of `VkQueue`_ families that may have access to the `VkSwapchainKHR`_ images. Only
		| set if :c:member:`imageSharingMode` is not set to `VK_SHARING_MODE_EXCLUSIVE`_.

	:c:member:`queueFamilyIndices`
		| Pointer to an array of `VkQueue`_ family indices that have access to images in
		| the `VkSwapchainKHR`_.

	:c:member:`compositeAlpha`
		| How to blend images with external graphics.

	:c:member:`presentMode`
		| How images are queued and presented internally by the swapchain (FIFO, MAIL_BOX
		| are the only ones known not to lead to tearing).

	:c:member:`clipped`
		| Allow vulkan to clip images not in view. (i.e clip/display part of the image
		| if it's behind a window).

	:c:member:`oldSwapChain`
		| If a `VkSwapchainKHR`_ is still in use while a window is resized passing pointer to old
		| `VkSwapchainKHR`_ may aid in resource (memory) reuse as the application is allowed to
		| present images already acquired from old `VkSwapchainKHR`_. Thus, no need to waste
		| memory & clock cycles creating new images.

.. c:function:: struct kmr_vk_swapchain kmr_vk_swapchain_create(struct kmr_vk_swapchain_create_info *kmrvk);

	Creates `VkSwapchainKHR`_ object that provides ability to present renderered results to a given `VkSurfaceKHR`_
	Minimum image count is equal to `VkSurfaceCapabilitiesKHR`_.minImageCount + 1.
	The `VkSwapchainKHR`_ can be defined as a set of images that can be drawn to and presented to a `VkSurfaceKHR`_.

	Parameters:
		| **kmrvk:** pointer to a struct :c:struct:`kmr_vk_swapchain_create_info`

	Returns:
		| **on success:** struct :c:struct:`kmr_vk_swapchain`
		| **on failure:** struct :c:struct:`kmr_vk_swapchain` { with members nulled }

===========================================================================================================

.. c:struct:: kmr_vk_image_handle

	.. c:member::
		VkImage        image;
		VkDeviceMemory deviceMemory[4];
		uint8_t        deviceMemoryCount;

	:c:member:`image`
		| Reference to data about `VkImage`_ itself. May be a texture, etc...

	:c:member:`deviceMemory`
		| Actual memory buffer whether CPU or GPU visible associate with `VkImage`_ object.
		| If @useExternalDmaBuffer set to true
		| :c:member:`deviceMemory` represents Vulkan API usable memory associated with
		| external DMA-BUFS.

	:c:member:`deviceMemoryCount`
		| The amount of DMA-BUF fds (drmFormatModifierPlaneCount) per `VkImage`_ Resource.

.. c:struct:: kmr_vk_image_view_handle

	.. c:member::
		VkImageView view;

	:c:member:`view`
		| Represents a way to interface with the actual VkImage itself. Describes to the
		| vulkan api how to interface with a given VkImage. How to read the given image and
		| exactly what in the image to read (color channels, etc...)

.. c:struct:: kmr_vk_image

	.. c:member::
		VkDevice                         logicalDevice;
		uint32_t                         imageCount;
		struct kmr_vk_image_handle       *imageHandles;
		struct kmr_vk_image_view_handle  *imageViewHandles;
		VkSwapchainKHR                   swapchain;

	:c:member:`logicalDevice`
		| VkDevice handle (Logical Device) associated with `VkImageView`_ & `VkImage`_ objects.

	:c:member:`imageCount`
		| Amount of VkImage's created. If `VkSwapchainKHR`_ reference is passed value would
		| be the amount of images in the given swapchain.

	:c:member:`imageHandles`
		| Pointer to an array of `VkImage`_ handles.

	:c:member:`imageViewHandles`
		| Pointer to an array of `VkImageView`_ handles.

	:c:member:`swapchain`
		| Member not required, but used for storage purposes. A valid `VkSwapchainKHR`_
		| reference to the `VkSwapchainKHR`_ passed to :c:struct:`kmr_vk_image_create`.
		| Represents the swapchain that created `VkImage`_'s.

.. c:struct:: kmr_vk_image_view_create_info

	.. c:member::
		VkImageViewCreateFlags   imageViewflags;
		VkImageViewType          imageViewType;
		VkFormat                 imageViewFormat;
		VkComponentMapping       imageViewComponents;
		VkImageSubresourceRange  imageViewSubresourceRange;

	Most members may also be located at `VkImageViewCreateInfo`_.

	:c:member:`imageViewflags`
		| Specifies additional prameters associated with VkImageView. Normally set to zero.

	:c:member:`imageViewType`
		| Specifies what the image view type is. Specifies coordinate system utilized by the
		| image when being addressed. :c:member:`imageViewType` type must have compatible
		| `VkImageType`_ when `VkSwapChainKHR`_ == `VK_NULL_HANDLE`_.

	:c:member:`imageViewFormat`
		Image Format (Bits per color channel, the color channel ordering, etc...).

	:c:member:`imageViewComponents`
		| Makes it so that we can select what value goes to what color channel. Basically if we
		| want to assign red channel value to green channel. Or set all (RGBA) color channel values
		| to the value at B channel this is how we achieve that.

	:c:member:`imageViewSubresourceRange`
		| Gates an image so that only a part of an image is allowed to be viewable.

.. c:struct:: kmr_vk_vimage_create_info

	.. c:member::
		VkImageCreateFlags        imageflags;
		VkImageType               imageType;
		VkFormat                  imageFormat;
		VkExtent3D                imageExtent3D;
		uint32_t                  imageMipLevels;
		uint32_t                  imageArrayLayers;
		VkSampleCountFlagBits     imageSamples;
		VkImageTiling             imageTiling;
		VkImageUsageFlags         imageUsage;
		VkSharingMode             imageSharingMode;
		uint32_t                  imageQueueFamilyIndexCount;
		const uint32_t            *imageQueueFamilyIndices;
		VkImageLayout             imageInitialLayout;
		uint64_t                  imageDmaBufferFormatModifier;
		uint32_t                  imageDmaBufferCount;
		int                       *imageDmaBufferFds;
		const VkSubresourceLayout *imageDmaBufferResourceInfo;
		uint32_t                  *imageDmaBufferMemTypeBits;

	Most members may also be located at `VkImageCreateInfo`_.

	:c:member:`imageflags`
		| Bits used to specify additional parameters for a given `VkImage`_.

	:c:member:`imageType`
		| Coordinate system the pixels in the image will use when being addressed.

	:c:member:`imageFormat`
		| Image Format (Bits per color channel, the color channel ordering, etc...).
	
	:c:member:`imageExtent3D`
		| Dimension (i.e. width, height, and depth) of the given image(s).

	:c:member:`imageMipLevels`
		| The number of levels of detail available for minified sampling of the image.

	:c:member:`imageArrayLayers`
		| The number of layers in the image.

	:c:member:`imageSamples`
		| Bitmask specifying sample counts supported for an image used for storage operations.

	:c:member:`imageTiling`
		| Specifies the tiling arrangement (image layout) of data in an image (linear, optimal).

	:c:member:`imageUsage`
		| Describes to vulkan the intended usage for the `VkImage`_.

	:c:member:`imageSharingMode`
		| Vulkan image may be owned by one device queue family or shared by multiple device
		| queue families. Sets whether images can only be accessed by a single queue or
		| multiple queues.

	:c:member:`imageQueueFamilyIndexCount`
		| Array size of :c:member:`imageQueueFamilyIndices`. Amount of queue families may own given vulkan image.

	:c:member:`imageQueueFamilyIndices`
		| Pointer to an array of queue families to associate/own a given vulkan image.

	:c:member:`imageInitialLayout`
		| Set the inital memory layout of a `VkImage`_.

	:c:member:`imageDmaBufferFormatModifier`
		| A 64-bit, vendor-prefixed, semi-opaque unsigned integer describing vendor-specific details
 		| of an image’s memory layout. Acquired when a call to :c:func:`kmr_buffer_create` is made
		| and stored in struct :c:struct:`kmr_buffer`.bufferObjects[0].modifier.

	:c:member:`imageDmaBufferCount`
		| Amount of elements in :c:member:`imageDmaBufferFds`, :c:member:`imageDmaBufferResourceInfo`,
		| and :c:member:`imageDmaBufferMemTypeBits`. Value should be
		| struct :c:struct:`kmr_buffer`.bufferObjects[0].planeCount.

	:c:member:`imageDmaBufferFds`
		| Array of DMA-BUF fds. Acquired when a call to :c:func:`kmr_buffer_create` is made and
		| stored in struct :c:struct:`kmr_buffer`.bufferObjects[0].dmaBufferFds[4].

	:c:member:`imageDmaBufferResourceInfo`
		| Info about the DMA-BUF including offset, size, pitch, etc. Most of which is acquired after a
		| call to :c:func:`kmr_buffer_create` is made and stored in
		| struct :c:struct:`kmr_buffer`.bufferObjects[0].{pitches[4], offsets[4], etc..}

	:c:member:`imageDmaBufferMemTypeBits`
		| Array of `VkMemoryRequirements`_.memoryTypeBits that can be acquired after a call to
		| :c:func:`kmr_vk_get_external_fd_memory_properties`.

.. c:struct:: kmr_vk_image_create_info

	.. c:member::
		VkDevice                             logicalDevice;
		VkSwapchainKHR                       swapchain;
		uint32_t                             imageCount;
		struct kmr_vk_image_view_create_info *imageViewCreateInfos;
		struct kmr_vk_vimage_create_info     *imageCreateInfos;
		VkPhysicalDevice                     physDevice;
		VkMemoryPropertyFlagBits             memPropertyFlags;
		bool                                 useExternalDmaBuffer;

	:c:member:`logicalDevice`
		| Must pass a valid `VkDevice`_ handle (Logical Device) to associate `VkImage`_/`VkImageView`_
		| state/data with.

	:c:member:`swapchain`
		| Must pass a valid `VkSwapchainKHR`_ handle. Used when retrieving references to
		| underlying `VkImage`_ If `VkSwapchainKHR`_ reference is not passed value. Application
		| will need to set amount of `VkImage`_'s/`VkImageView`_'s via :c:member:`imageCount`.

	:c:member:`imageCount`
		| Must pass amount of `VkImage`_'s/`VkImageView`_'s to create.
		| if :c:member:`swapchain` == `VK_NULL_HANDLE`_ set to 0.

	:c:member:`imageViewCreateInfos`
		| Pointer to an array of size :c:member:`imageCount` containing everything required to create an individual
		| `VkImageView`_. If a :c:member:`imageCount` value given array size must be at least :c:member:`imageCount`
		| in size. If not given array size must equal 1.

	**Bellow only required if swapchain == VK_NULL_HANDLE**

	:c:member:`imageCreateInfos`
		| Pointer to an array of size :c:member:`imageCount` containing everything required to create an individual
		| `VkImage`_. If a :c:member:`imageCount` value given array size must be at least :c:member:`imageCount` in
		| size. If not given array size must equal 1.

	:c:member:`physDevice`
		| Must pass a valid `VkPhysicalDevice`_ handle as it is used to query memory properties.

	:c:member:`memPropertyFlags`
		| Used to determine the type of actual memory to allocated. Whether CPU (host) or GPU visible.

	:c:member:`useExternalDmaBuffer`
		| Set to true if `VkImage`_ resources created needs to be associated with an external DMA-BUF created by GBM.

.. c:function:: struct kmr_vk_image kmr_vk_image_create(struct kmr_vk_image_create_info *kmrvk);

	Function creates/retrieve `VkImage`_'s and associates `VkImageView`_'s with said images.
	If a `VkSwapchainKHR`_ reference is passed function retrieves all images in the swapchain
	and uses that to associate `VkImageView`_ objects. If `VkSwapchainKHR`_ reference is not
	passed function creates `VkImage`_ object's given the passed data. Then associates
	`VkDeviceMemory`_ & `VkImageView`_ objects with the `VkImage`_. Amount of images created is
	based upon struct :c:struct:`kmr_vk_image_create_info` { **@imageCount** }.

	Parameters:
		| **kmrvk:** pointer to a struct :c:struct:`kmr_vk_image_create_info`

	Returns:
		| **on success:** struct :c:struct:`kmr_vk_image`
		| **on failure:** struct :c:struct:`kmr_vk_image` { with members nulled }

===========================================================================================================

.. _VK_NULL_HANDLE: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_NULL_HANDLE.html
.. _VkInstance: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstance.html
.. _VkInstanceCreateInfo: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstanceCreateInfo.html
.. _VkApplicationInfo: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkApplicationInfo.html
.. _VkSurfaceKHR: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceKHR.html
.. _VkSurfaceCapabilitiesKHR: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceCapabilitiesKHR.html
.. _VkPhysicalDevice: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDevice.html
.. _VkPhysicalDeviceType: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceType.html
.. _VkPhysicalDeviceFeatures: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceFeatures.html
.. _VkPhysicalDeviceProperties: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceProperties.html
.. _VkPhysicalDeviceProperties2: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceProperties2.html
.. _VkPhysicalDeviceDrmPropertiesEXT: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceDrmPropertiesEXT.html
.. _VkDevice: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkDevice.html
.. _VkQueue: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkQueue.html
.. _VkQueueFlagBits: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkQueueFlagBits.html
.. _vkGetDeviceQueue: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/vkGetDeviceQueue.html
.. _VkSwapchainKHR: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkSwapchainKHR.html
.. _VkSwapchainCreateInfoKHR: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkSwapchainCreateInfoKHR.html
.. _VkSharingMode: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSharingMode.html
.. _VK_SHARING_MODE_EXCLUSIVE: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSharingMode.html
.. _VK_SHARING_MODE_CONCURRENT: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSharingMode.html
.. _VkImage: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImage.html
.. _VkImageType: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageType.html
.. _VkImageCreateInfo: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageCreateInfo.html
.. _VkImageView: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageView.html
.. _VkImageViewCreateInfo: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkImageViewCreateInfo.html
.. _VkMemoryRequirements: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkMemoryRequirements.html
.. _VkDeviceMemory: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDeviceMemory.html
