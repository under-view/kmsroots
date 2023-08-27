.. default-domain:: C

Vulkan
======

Header: kmsroots/vulkan.h

Table of contents (click to go)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

======
Macros
======

1. :c:macro:`KMR_VK_INSTANCE_PROC_ADDR`
#. :c:macro:`KMR_VK_DEVICE_PROC_ADDR`

=====
Enums
=====

1. :c:enum:`kmr_vk_surface_type`
#. :c:enum:`kmr_vk_sync_obj_type`
#. :c:enum:`kmr_vk_resource_copy_type`

======
Unions
======

1. :c:union:`kmr_vk_sync_obj_handle`

=======
Structs
=======

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
#. :c:struct:`kmr_vk_shader_module`
#. :c:struct:`kmr_vk_shader_module_create_info`
#. :c:struct:`kmr_vk_pipeline_layout`
#. :c:struct:`kmr_vk_pipeline_layout_create_info`
#. :c:struct:`kmr_vk_render_pass`
#. :c:struct:`kmr_vk_render_pass_create_info`
#. :c:struct:`kmr_vk_graphics_pipeline`
#. :c:struct:`kmr_vk_graphics_pipeline_create_info`
#. :c:struct:`kmr_vk_framebuffer_handle`
#. :c:struct:`kmr_vk_framebuffer`
#. :c:struct:`kmr_vk_framebuffer_images`
#. :c:struct:`kmr_vk_framebuffer_create_info`
#. :c:struct:`kmr_vk_command_buffer_handle`
#. :c:struct:`kmr_vk_command_buffer`
#. :c:struct:`kmr_vk_command_buffer_create_info`
#. :c:struct:`kmr_vk_command_buffer_record_info`
#. :c:struct:`kmr_vk_fence_handle`
#. :c:struct:`kmr_vk_semaphore_handle`
#. :c:struct:`kmr_vk_sync_obj`
#. :c:struct:`kmr_vk_sync_obj_create_info`
#. :c:struct:`kmr_vk_sync_obj_import_external_sync_fd_info`
#. :c:struct:`kmr_vk_sync_obj_export_external_sync_fd_info`
#. :c:struct:`kmr_vk_buffer`
#. :c:struct:`kmr_vk_buffer_create_info`
#. :c:struct:`kmr_vk_descriptor_set_layout`
#. :c:struct:`kmr_vk_descriptor_set_layout_create_info`
#. :c:struct:`kmr_vk_descriptor_set_handle`
#. :c:struct:`kmr_vk_descriptor_set`
#. :c:struct:`kmr_vk_descriptor_set_create_info`
#. :c:struct:`kmr_vk_sampler`
#. :c:struct:`kmr_vk_sampler_create_info`
#. :c:struct:`kmr_vk_resource_copy_buffer_to_buffer_info`
#. :c:struct:`kmr_vk_resource_copy_buffer_to_image_info`
#. :c:struct:`kmr_vk_resource_copy_info`

=========
Functions
=========

1. :c:func:`kmr_vk_instance_create`
#. :c:func:`kmr_vk_surface_create`
#. :c:func:`kmr_vk_phdev_create`
#. :c:func:`kmr_vk_queue_create`
#. :c:func:`kmr_vk_lgdev_create`
#. :c:func:`kmr_vk_swapchain_create`
#. :c:func:`kmr_vk_image_create`
#. :c:func:`kmr_vk_shader_module_create`
#. :c:func:`kmr_vk_pipeline_layout_create`
#. :c:func:`kmr_vk_render_pass_create`
#. :c:func:`kmr_vk_graphics_pipeline_create`
#. :c:func:`kmr_vk_framebuffer_create`
#. :c:func:`kmr_vk_command_buffer_create`
#. :c:func:`kmr_vk_command_buffer_record_begin`
#. :c:func:`kmr_vk_command_buffer_record_end`
#. :c:func:`kmr_vk_sync_obj_create`
#. :c:func:`kmr_vk_sync_obj_import_external_sync_fd`
#. :c:func:`kmr_vk_sync_obj_export_external_sync_fd`
#. :c:func:`kmr_vk_buffer_create`
#. :c:func:`kmr_vk_descriptor_set_layout_create`
#. :c:func:`kmr_vk_descriptor_set_create`
#. :c:func:`kmr_vk_sampler_create_info`
#. :c:func:`kmr_vk_resource_copy`

API Documentation
~~~~~~~~~~~~~~~~~

=========================
KMR_VK_INSTANCE_PROC_ADDR
=========================

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

=========================================================================================================================================

=======================
KMR_VK_DEVICE_PROC_ADDR
=======================

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

=========================================================================================================================================

===========================
kmr_vk_instance_create_info
===========================

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

======================
kmr_vk_instance_create
======================

.. c:function:: VkInstance kmr_vk_instance_create(struct kmr_vk_instance_create_info *kmrvk);

        Creates a `VkInstance`_ object which allows the Vulkan API to better reference & store object
	state/data. It also acts as an easy wrapper that allows one to define instance extensions.
        `VkInstance`_ extensions basically allow developers to define what an app is setup to do.
        So, if a client wants the application to work with wayland surface or X11 surface etc...
        Client should enable those extensions inorder to gain access to those particular capabilities.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_instance_create_info`

        Returns:
                | **on success:** `VkInstance`_
                | **on faliure:** `VK_NULL_HANDLE`_

=========================================================================================================================================

===================
kmr_vk_surface_type
===================

.. c:enum:: kmr_vk_surface_type

        .. c:macro::
                KMR_SURFACE_WAYLAND_CLIENT
                KMR_SURFACE_XCB_CLIENT

        Display server protocol options. ENUM used by :c:func:`kmr_vk_surface_create`
        to create a `VkSurfaceKHR`_ object based upon platform specific information

	:c:macro:`KMR_SURFACE_WAYLAND_CLIENT`
		| Value set to ``0``

	:c:macro:`KMR_SURFACE_XCB_CLIENT`
		| Value set to ``1``

==========================
kmr_vk_surface_create_info
==========================

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
		| Must pass a pointer to a ``struct`` wl_surface object

        :c:member:`display`
		| Must pass either a pointer to ``struct`` wl_display object or a pointer to an xcb_connection_t

        :c:member:`window`
		| Must pass an xcb_window_t window id or an unsigned int representing XID

=====================
kmr_vk_surface_create
=====================

.. c:function:: VkSurfaceKHR kmr_vk_surface_create(struct kmr_vk_surface_create_info *kmrvk);

        Creates a `VkSurfaceKHR`_ object based upon platform specific information about the given surface.
        `VkSurfaceKHR`_ are the interface between the window and Vulkan defined images in a given swapchain
        if vulkan swapchain exists.

        Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_surface_create_info`

        Returns:
                | **on success:** `VkSurfaceKHR`_
                | **on faliure:** `VK_NULL_HANDLE`_

=========================================================================================================================================

============
kmr_vk_phdev
============

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
		| KMS device node file descriptor passed via ``struct`` :c:struct:`kmr_vk_phdev_create_info`

	:c:member:`physDeviceDrmProperties`
		| Structure containing DRM information of a physical device. A `VkPhysicalDeviceProperties2`_
		| structure is utilized to populate this member. Member information is then checked by the
		| implementation to see if passed KMS device node file descriptor
		| (``struct`` :c:struct:`kmr_vk_phdev_create_info` { ``kmsfd`` }) is equal to the physical device
		| suggested by (``struct`` :c:struct:`kmr_vk_phdev_create_info` { ``deviceType`` }).
		| Contains data stored after associating a DRM file descriptor with a vulkan physical device.

========================
kmr_vk_phdev_create_info
========================

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

===================
kmr_vk_phdev_create
===================

.. c:function::	struct kmr_vk_phdev kmr_vk_phdev_create(struct kmr_vk_phdev_create_info *kmrvk);

	Retrieves a `VkPhysicalDevice`_ handle if certain characteristics of a physical device are meet.
	Also retrieves a given physical device properties and features to be later used by the application.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_phdev_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_phdev`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_phdev` { with members nulled, int's set to -1 }

=========================================================================================================================================

============
kmr_vk_queue
============

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
		| `VkQueue`_ family index associate with selected ``struct`` :c:struct:`kmr_vk_queue_create_info`
		| { ``queueFlag`` }.

	:c:member:`queueCount`
		| Number of queues in a given `VkQueue`_ family

========================
kmr_vk_queue_create_info
========================

.. c:struct:: kmr_vk_queue_create_info

	.. c:member::
		VkPhysicalDevice physDevice;
		VkQueueFlags     queueFlag;

	:c:member:`physDevice`
		| Must pass a valid `VkPhysicalDevice`_ handle to query queues associate with phsyical device

	:c:member:`queueFlag`
		| Must pass one `VkQueueFlagBits`_, if multiple flags are bitwised or'd function will fail
		| to return `VkQueue`_ family index (``struct`` :c:struct:`kmr_vk_queue`).

===================
kmr_vk_queue_create
===================

.. c:function::	struct kmr_vk_queue kmr_vk_queue_create(struct kmr_vk_queue_create_info *kmrvk);

	Queries the queues a given physical device contains. Then returns a queue
	family index and the queue count given a single `VkQueueFlagBits`_. Queue
	are used in vulkan to submit commands up to the GPU.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_queue_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_queue`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_queue` { with members nulled, int's set to -1 }

=========================================================================================================================================

============
kmr_vk_lgdev
============

.. c:struct:: kmr_vk_lgdev

	.. c:member::
		VkDevice            logicalDevice;
		uint32_t            queueCount;
		struct kmr_vk_queue *queues;

	:c:member:`logicalDevice`
		| Returned `VkDevice`_ handle which represents vulkan's access to physical device

	:c:member:`queueCount`
		| Amount of elements in pointer to array of ``struct`` :c:struct:`kmr_vk_queue`. This information
		| gets populated with the data pass through ``struct`` :c:struct:`kmr_vk_lgdev_create_info`
		| { ``queueCount`` }.

	:c:member:`queues`
		| Pointer to an array of ``struct`` :c:struct:`kmr_vk_queue`. This information gets populated with the
		| data pass through ``struct`` :c:struct:`kmr_vk_lgdev_create_info` { ``queues`` }.

		| Members :c:member:`queueCount` & :c:member:`queues` are strictly for ``struct`` :c:struct:`kmr_vk_lgdev`
		| to have extra information amount `VkQueue`_'s

========================
kmr_vk_lgdev_create_info
========================

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
		| Must pass the amount of ``struct`` :c:struct:`kmr_vk_queue` { ``queue``, ``familyIndex`` } to
		| create along with a given logical device

	:c:member:`queues`
		| Must pass a pointer to an array of ``struct`` :c:struct:`kmr_vk_queue` { ``queue``, ``familyIndex`` } to
		| create along with a given logical device

===================
kmr_vk_lgdev_create
===================

.. c:function:: struct kmr_vk_lgdev kmr_vk_lgdev_create(struct kmr_vk_lgdev_create_info *kmrvk);

	Creates a `VkDevice`_ handle and allows vulkan to have a connection to a given physical device.
	The `VkDevice`_ handle is more of a local object its state and operations are local
	to it and are not seen by other logical devices. Function also acts as an easy wrapper
	that allows client to define device extensions. Device extensions basically allow developers
	to define what operations a given logical device is capable of doing. So, if one wants the
	device to be capable of utilizing a swap chain, etc... You have to enable those extensions
	inorder to gain access to those particular capabilities. Allows for creation of multiple
	`VkQueue`_'s although the only one we needis the Graphics queue.

	``struct`` :c:struct:`kmr_vk_queue` { ``queue`` } handle is assigned in this function as `vkGetDeviceQueue`_
	requires a logical device handle.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_lgdev_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_lgdev`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_lgdev` { with members nulled, int's set to -1 }

=========================================================================================================================================

================
kmr_vk_swapchain
================

.. c:struct:: kmr_vk_swapchain

	.. c:member::
		VkDevice       logicalDevice;
		VkSwapchainKHR swapchain;

	:c:member:`logicalDevice`
		| `VkDevice`_ handle (Logical Device) that stores `VkSwapchainKHR`_ state/data.

	:c:member:`swapchain`
		| Vulkan object storing reference to swapchain state/data.

============================
kmr_vk_swapchain_create_info
============================

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

=======================
kmr_vk_swapchain_create
=======================

.. c:function:: struct kmr_vk_swapchain kmr_vk_swapchain_create(struct kmr_vk_swapchain_create_info *kmrvk);

	Creates `VkSwapchainKHR`_ object that provides ability to present renderered results to a given `VkSurfaceKHR`_
	Minimum image count is equal to `VkSurfaceCapabilitiesKHR`_.minImageCount + 1.
	The `VkSwapchainKHR`_ can be defined as a set of images that can be drawn to and presented to a `VkSurfaceKHR`_.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_swapchain_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_swapchain`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_swapchain` { with members nulled }

=========================================================================================================================================

===================
kmr_vk_image_handle
===================

.. c:struct:: kmr_vk_image_handle

	.. c:member::
		VkImage        image;
		VkDeviceMemory deviceMemory[4];
		uint8_t        deviceMemoryCount;

	:c:member:`image`
		| Reference to data about `VkImage`_ itself. May be a texture, etc...

	:c:member:`deviceMemory`
		| Actual memory buffer whether CPU or GPU visible associate with `VkImage`_ object.
		| If ``struct`` :c:struct:`kmr_vk_image_create_info` {  ``useExternalDmaBuffer`` } set to **true**
		| :c:member:`deviceMemory` represents Vulkan API usable memory associated with
		| external DMA-BUFS.

	:c:member:`deviceMemoryCount`
		| The amount of DMA-BUF fds (drmFormatModifierPlaneCount) per `VkImage`_ Resource.

========================
kmr_vk_image_view_handle
========================

.. c:struct:: kmr_vk_image_view_handle

	.. c:member::
		VkImageView view;

	:c:member:`view`
		| Represents a way to interface with the actual VkImage itself. Describes to the
		| vulkan api how to interface with a given VkImage. How to read the given image and
		| exactly what in the image to read (color channels, etc...)

============
kmr_vk_image
============

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

=============================
kmr_vk_image_view_create_info
=============================

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

=========================
kmr_vk_vimage_create_info
=========================

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
		| and stored in ``struct`` :c:struct:`kmr_buffer`.bufferObjects[0].modifier.

	:c:member:`imageDmaBufferCount`
		| Amount of elements in :c:member:`imageDmaBufferFds`, :c:member:`imageDmaBufferResourceInfo`,
		| and :c:member:`imageDmaBufferMemTypeBits`. Value should be
		| ``struct`` :c:struct:`kmr_buffer`.bufferObjects[0].planeCount.

	:c:member:`imageDmaBufferFds`
		| Array of DMA-BUF fds. Acquired when a call to :c:func:`kmr_buffer_create` is made and
		| stored in ``struct`` :c:struct:`kmr_buffer`.bufferObjects[0].dmaBufferFds[4].

	:c:member:`imageDmaBufferResourceInfo`
		| Info about the DMA-BUF including offset, size, pitch, etc. Most of which is acquired after a
		| call to :c:func:`kmr_buffer_create` is made and stored in
		| ``struct`` :c:struct:`kmr_buffer`.bufferObjects[0].{pitches[4], offsets[4], etc..}

	:c:member:`imageDmaBufferMemTypeBits`
		| Array of `VkMemoryRequirements`_.memoryTypeBits that can be acquired after a call to
		| :c:func:`kmr_vk_get_external_fd_memory_properties`.

========================
kmr_vk_image_create_info
========================

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

===================
kmr_vk_image_create
===================

.. c:function:: struct kmr_vk_image kmr_vk_image_create(struct kmr_vk_image_create_info *kmrvk);

	Function creates/retrieve `VkImage`_'s and associates `VkImageView`_'s with said images.
	If a `VkSwapchainKHR`_ reference is passed function retrieves all images in the swapchain
	and uses that to associate `VkImageView`_ objects. If `VkSwapchainKHR`_ reference is not
	passed function creates `VkImage`_ object's given the passed data. Then associates
	`VkDeviceMemory`_ & `VkImageView`_ objects with the `VkImage`_. Amount of images created is
	based upon ``struct`` :c:struct:`kmr_vk_image_create_info` { ``imageCount`` }.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_image_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_image`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_image` { with members nulled }

=========================================================================================================================================

====================
kmr_vk_shader_module
====================

.. c:struct:: kmr_vk_shader_module

	.. c:member::
		VkDevice       logicalDevice;
		VkShaderModule shaderModule;
		const char     *shaderName;

	:c:member:`logicalDevice`
		| `VkDevice`_ handle (Logical Device) associated with `VkShaderModule`_.

	:c:member:`shaderModule`
		| Contains shader code and one or more entry points.

	:c:member:`shaderName`
		| Name given to shader module can be safely ignored not required by API.

================================
kmr_vk_shader_module_create_info
================================

.. c:struct:: kmr_vk_shader_module_create_info

	.. c:member::
		VkDevice            logicalDevice;
		size_t              sprivByteSize;
		const unsigned char *sprivBytes;
		const char          *shaderName;

	:c:member:`logicalDevice`
		| Must pass a valid `VkDevice`_ handle (Logical Device) to associate `VkShaderModule`_
		| state/data with.

	:c:member:`sprivByteSize`
		| Must pass the sizeof SPIR-V byte code

	:c:member:`sprivBytes`
		| Must pass pointer to SPIR-V byte code itself

	:c:member:`shaderName`
		| Name given to shader module can be safely ignored not required by API.

===========================
kmr_vk_shader_module_create
===========================

.. c:function:: struct kmr_vk_shader_module kmr_vk_shader_module_create(struct kmr_vk_shader_module_create_info *kmrvk);

	Function creates `VkShaderModule`_ from passed SPIR-V byte code.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_shader_module_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_shader_module`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_shader_module` { with members nulled }

=========================================================================================================================================

======================
kmr_vk_pipeline_layout
======================

.. c:struct:: kmr_vk_pipeline_layout

	.. c:member::
		VkDevice         logicalDevice;
		VkPipelineLayout pipelineLayout;

	:c:member:`logicalDevice`
		| `VkDevice`_ handle (Logical Device) associated with `VkPipelineLayout`_

	:c:member:`pipelineLayout`
		| Stores collection of data describing the vulkan resources that are needed to
		| produce final image. This data is later used during graphics pipeline runtime.

==================================
kmr_vk_pipeline_layout_create_info
==================================

.. c:struct:: kmr_vk_pipeline_layout_create_info

	.. c:member::
		VkDevice                    logicalDevice;
		uint32_t                    descriptorSetLayoutCount;
		const VkDescriptorSetLayout *descriptorSetLayouts;
		uint32_t                    pushConstantRangeCount;
		const VkPushConstantRange   *pushConstantRanges;

	Most members may also be located at `VkPipelineLayoutCreateInfo`_.

	:c:member:`logicalDevice`
		| Must pass a valid `VkDevice`_ handle (Logical Device)

	:c:member:`descriptorSetLayoutCount`
		| Must pass the array size of :c:member:`descriptorSetLayouts`

	:c:member:`descriptorSetLayouts`
		| Must pass a pointer to an array of descriptor set layouts so a given graphics
		| pipeline can know how a shader can access a given vulkan resource.

	:c:member:`pushConstantRangeCount`
		| Must pass the array size of :c:member:`pushConstantRanges`

	:c:member:`pushConstantRanges`
		| Must pass a pointer to an array of push constant definitions that describe at what
		| shader stage and the sizeof the data being pushed to the GPU to be later utilized by
		| the shader at a given stage. If the shader needs to recieve smaller values quickly
		| instead of creating a dynamic uniform buffer and updating the value at memory address.
		| Push constants allow for smaller data to be more efficiently passed up to the GPU by
		| passing values directly to the shader.

=============================
kmr_vk_pipeline_layout_create
=============================

.. c:function:: struct kmr_vk_pipeline_layout kmr_vk_pipeline_layout_create(struct kmr_vk_pipeline_layout_create_info *kmrvk);

	Function creates a `VkPipelineLayout`_ handle that is then later used by the graphics pipeline
	itself so that is knows what vulkan resources are need to produce the final image, at what shader
	stages these resources will be accessed, and how to access them. Describes the layout of the
	data that will be given to the pipeline for a single draw operation.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_pipeline_layout_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_pipeline_layout`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_pipeline_layout` { with members nulled }

=========================================================================================================================================

==================
kmr_vk_render_pass
==================

.. c:struct:: kmr_vk_render_pass

	.. c:member::
		VkDevice     logicalDevice;
		VkRenderPass renderPass;

	:c:member:`logicalDevice`
		| `VkDevice`_ handle (Logical Device) associated with render pass instance

	:c:member:`renderPass`
		| Represents a collection of attachments, subpasses, and dependencies between the subpasses

==============================
kmr_vk_render_pass_create_info
==============================

.. c:struct:: kmr_vk_render_pass_create_info

	.. c:member::
		VkDevice                      logicalDevice;
		uint32_t                      attachmentDescriptionCount;
		const VkAttachmentDescription *attachmentDescriptions;
		uint32_t                      subpassDescriptionCount;
		const VkSubpassDescription    *subpassDescriptions;
		uint32_t                      subpassDependencyCount;
		const VkSubpassDependency     *subpassDependencies;

	Most members may also be located at `VkRenderPassCreateInfo`_.
	
	:c:member:`logicalDevice`
		| Must pass a valid `VkDevice`_ handle (Logical Device)

	:c:member:`attachmentDescriptionCount`
		| Must pass array size of :c:member:`attachmentDescriptions`

	:c:member:`attachmentDescriptions`
		| Describes the type of location to output fragment data to
		| Depth attachment outputs to a `VkImage`_ used for depth
		| Color attachment outputs to a `VkImage`_ used for coloring insides of a triangle

	:c:member:`subpassDescriptionCount`
		| Must pass array size of :c:member:`subpassDescriptions`

	:c:member:`subpassDescriptions`
		| What type of pipeline attachments are bounded to (Graphics being the one we want) and
		| the final layout of the image before its presented on the screen.

	:c:member:`subpassDependencyCount`
		| Must pass array size of :c:member:`subpassDependencies`

	:c:member:`subpassDependencies`
		| Pointer to an array of subpass dependencies that define stages in a pipeline where image
		| transitions need to occur before sending output to framebuffer then later the viewport.

=========================
kmr_vk_render_pass_create
=========================

.. c:function:: struct kmr_vk_render_pass kmr_vk_render_pass_create(struct kmr_vk_render_pass_create_info *kmrvk);

	Function creates a `VkRenderPass`_ handle that is then later used by the graphics pipeline
	itself so that is knows how many attachments (color, depth, etc...) there will be per `VkFramebuffer`_,
	how many samples an attachment has (samples to use for multisampling), and how their contents should
	be handled throughout rendering operations. Subpasses within a render pass then references the attachments
	for every draw operations and connects attachments (i.e. `VkImage`_'s connect to a `VkFramebuffer`_) to the graphics
	pipeline. In short the render pass is the intermediary step between your graphics pipeline and the framebuffer.
	It describes how you want to render things to the viewport upon render time. Example at render time we wish to
	color in the center of a triangle. We want to give the appearance of depth to an image.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_render_pass_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_render_pass`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_render_pass` { with members nulled }

=========================================================================================================================================

========================
kmr_vk_graphics_pipeline
========================

.. c:struct:: kmr_vk_graphics_pipeline

	.. c:member::
		VkDevice   logicalDevice;
		VkPipeline graphicsPipeline;

	:c:member:`logicalDevice`
		| `VkDevice`_ handle (Logical Device) associated with `VkPipeline`_ (Graphics Pipeline)

	:c:member:`graphicsPipeline`
		| Handle to a pipeline object. Storing what to do during each stage of the graphics pipeline.

====================================
kmr_vk_graphics_pipeline_create_info
====================================

.. c:struct:: kmr_vk_graphics_pipeline_create_info

	.. c:member::
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

	Most members may also be located at `VkGraphicsPipelineCreateInfo`_.

	:c:member:`logicalDevice`
		| Must pass a valid `VkDevice`_ handle (Logical Device) to associate graphics pipeline
		| state/data with.

	:c:member:`shaderStageCount`
		| Must pass the array size of :c:member:`shaderStages`. Amount of shaders being used by
		| the graphics pipeline.

	:c:member:`shaderStages`
		| Defines shaders (via `VkShaderModule`_) and at what shader stages the created `VkPipeline`_
		| will utilize them.

	:c:member:`vertexInputState`
		| Defines the layout and format of vertex input data. Provides details for loading vertex data.
		| So the graphics pipeline understands how the vertices are stored in the buffer.

	:c:member:`inputAssemblyState`
		| Defines how to assemble vertices to primitives (i.e. triangles or lines).
		| For more info see `VkPrimitiveTopology`_.

	:c:member:`tessellationState`
		| TBA

	:c:member:`viewportState`
		| `VkViewPort`_ defines how to populate image with pixel data (i.e populate only the top half
		| or bottom half). `Scissor`_ defines how to crop an image. How much of image should be drawn
		| (i.e draw whole image, right half, middle, etc...)

	:c:member:`rasterizationState`
		| Handles how raw vertex data turns into cordinates on screen and in a pixel buffer.
		| Handle computation of fragments (pixels) from primitives (i.e. triangles or lines).

	:c:member:`multisampleState`
		| If you want to do clever anti-aliasing through multisampling. Stores multisampling information.

	:c:member:`depthStencilState`
		| How to handle depth + stencil data. If a draw has 2 or more objects we don't want to be
		| drawing the back object on top of the object that should be in front of it.

	:c:member:`colorBlendState`
		| Defines how to blend fragments at the end of the pipeline.

	:c:member:`dynamicState`
		| Graphics pipelines settings are static once set they can't change. To get new settings you'd
		| have to create a whole new pipeline. There are settings however that can be changed at
		| runtime. We define which settings here.

	:c:member:`pipelineLayout`
		| Pass `VkPipelineLayout`_ handle to define the resources (i.e. descriptor sets, push constants)
		| given to the pipeline for a single draw operation.

	:c:member:`renderPass`
		| Pass `VkRenderPass`_ handle which holds a pipeline and handles how it is execute. With final
		| outputs being to a framebuffer. One can have multiple smaller subpasses inside of render
		| pass. Used to bind a given render pass to the graphics pipeline. Contains multiple
		| attachments that go to all plausible pipeline outputs (i.e Depth, Color, etc..).

	:c:member:`subpass`
		| Pass the index of the subpass to use in the :c:member:`renderPass` instance.

===============================
kmr_vk_graphics_pipeline_create
===============================

.. c:function:: struct kmr_vk_graphics_pipeline kmr_vk_graphics_pipeline_create(struct kmr_vk_graphics_pipeline_create_info *kmrvk);

	Function creates a `VkPipeline`_ handle that references a sequence of operations that first takes
	raw vertices (points on a coordinate plane), textures coordinates, color coordinates, tangent,
	etc.. [**NOTE:** Data combinded is called a mesh]. Utilizes shaders to plot the points on a coordinate
	system then the rasterizer converts all plotted points and turns it into fragments/pixels for your
	fragment shader to then color in.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_graphics_pipeline_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_graphics_pipeline`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_graphics_pipeline` { with members nulled }

=========================================================================================================================================

=========================
kmr_vk_framebuffer_handle
=========================

.. c:struct:: kmr_vk_framebuffer_handle

	.. c:member::
		VkFramebuffer framebuffer;

	:c:member:`framebuffer`
		| Framebuffers represent a collection of specific memory attachments that a render pass
		| instance uses. Connection between an image (or images) and the render pass instance.

==================
kmr_vk_framebuffer
==================

.. c:struct:: kmr_vk_framebuffer

	.. c:member::
		VkDevice                          logicalDevice;
		uint8_t                           framebufferCount;
		struct kmr_vk_framebuffer_handle  *framebufferHandles;

	:c:member:`logicalDevice`
		| `VkDevice`_ handle (Logical Device) associated with :c:member:`framebufferCount` `VkFramebuffer`_'s.

	:c:member:`framebufferCount`
		| Amount of `VkFramebuffer`_ handles created.

	:c:member:`framebufferHandles`
		| Pointer to an array of `VkFramebuffer`_ handles.

=========================
kmr_vk_framebuffer_images
=========================

.. c:struct:: kmr_vk_framebuffer_images

	.. c:member::
		VkImageView imageAttachments[6];

	:c:member:`imageAttachments`
		| Allow at most 6 attachments (`VkImageView`_ -> `VkImage`_) per `VkFramebuffer`_.

==============================
kmr_vk_framebuffer_create_info
==============================

.. c:struct:: kmr_vk_framebuffer_create_info

	.. c:member::
		VkDevice                         logicalDevice;
		uint8_t                          framebufferCount;
		uint8_t                          framebufferImageAttachmentCount;
		struct kmr_vk_framebuffer_images *framebufferImages;
		VkRenderPass                     renderPass;
		uint32_t                         width;
		uint32_t                         height;
		uint32_t                         layers;

	Most members may also be located at `VkFramebufferCreateInfo`_.

	:c:member:`logicalDevice`
		| Must pass a valid `VkDevice`_ handle (Logical Device).

	:c:member:`framebufferCount`
		| Amount of `VkFramebuffer`_ handles to create (i.e the array length of :c:member:`framebufferImages`)

	:c:member:`framebufferImageAttachmentCount`
		| Amount of framebuffer attachments (`VkImageView`_ -> `VkImage`_) per `VkFramebuffer`_.

	:c:member:`framebufferImages`
		| Pointer to an array of `VkImageView`_ handles which the :c:member:`renderPass` instance will
		| merge to create final `VkFramebuffer`_. These `VkImageView`_ -> `VkImage`_ handles must always
		| be in a format that equals to the render pass attachment format.

	:c:member:`renderPass`
		| Defines the render pass a given framebuffer is compatible with

	:c:member:`width`
		| Framebuffer width in pixels

	:c:member:`height`
		| Framebuffer height in pixels

	:c:member:`layers`
		| TBA

=========================
kmr_vk_framebuffer_create
=========================

.. c:function:: struct kmr_vk_framebuffer kmr_vk_framebuffer_create(struct kmr_vk_framebuffer_create_info *kmrvk);

	Creates ``framebufferCount`` amount of `VkFramebuffer`_ handles. Can think of this function as creating the
	frames to hold the pictures in them, with each frame only containing one picture. Note framebuffer
	`VkImage`_'s (``framebufferImages`` -> ``imageAttachments``) must match up one to one with attachments in the
	`VkRenderpass`_ instance. Meaning if are ``renderPass`` instance has 1 color + 1 depth attachment. Then each `VkFramebuffer`_
	must have one `VkImage`_ for color and one `VkImage`_ for depth.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_framebuffer_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_framebuffer`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_framebuffer` { with members nulled }

=========================================================================================================================================

============================
kmr_vk_command_buffer_handle
============================

.. c:struct:: kmr_vk_command_buffer_handle

	.. c:member::
		VkCommandBuffer commandBuffer;

	:c:member:`commandBuffer`
		| Handle used to pre-record commands before they are submitted to a device queue and
		| sent off to the GPU.

=====================
kmr_vk_command_buffer
=====================

.. c:struct:: kmr_vk_command_buffer

	.. c:member::
		VkDevice                            logicalDevice;
		VkCommandPool                       commandPool;
		uint32_t                            commandBufferCount;
		struct kmr_vk_command_buffer_handle *commandBufferHandles;

	:c:member:`logicalDevice`
		| `VkDevice`_ handle (Logical Device) associated with `VkCommandPool`_

	:c:member:`commandPool`
		| The memory pool which the buffers where allocated from.

	:c:member:`commandBufferCount`
		| Amount of `VkCommandBuffer`_'s allocated from memory pool.
		| Array size of :c:member:`commandBufferHandles`.

	:c:member:`commandBufferHandles`
		| Pointer to an array of `VkCommandBuffer`_ handles

=================================
kmr_vk_command_buffer_create_info
=================================

.. c:struct:: kmr_vk_command_buffer_create_info

	.. c:member::
		VkDevice logicalDevice;
		uint32_t queueFamilyIndex;
		uint32_t commandBufferCount;

	:c:member:`logicalDevice`
		| Must pass a valid `VkDevice`_ handle (Logical Device)

	:c:member:`queueFamilyIndex`
		| Designates a queue family with `VkCommandPool`_. All command buffers allocated
		| from VkCommandPool must used same queue.

	:c:member:`commandBufferCount`
		| The amount of command buffers to allocate from a given pool

============================
kmr_vk_command_buffer_create
============================

.. c:function:: struct kmr_vk_command_buffer kmr_vk_command_buffer_create(struct kmr_vk_command_buffer_create_info *kmrvk);

	Function creates a `VkCommandPool`_ handle then allocates `VkCommandBuffer`_ handles from
	that pool. The amount of `VkCommandBuffer`_'s allocated is based upon ``commandBufferCount``.
	Function only allocates primary command buffers. `VkCommandPool`_ flags set
	`VK_COMMAND_POOL_CREATE_TRANSIENT_BIT`_ |
	`VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT`_

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_command_buffer_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_command_buffer`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_command_buffer` { with members nulled }

=========================================================================================================================================

=================================
kmr_vk_command_buffer_record_info
=================================

.. c:struct:: kmr_vk_command_buffer_record_info

	.. c:member::
		uint32_t                            commandBufferCount;
		struct kmr_vk_command_buffer_handle *commandBufferHandles;
		VkCommandBufferUsageFlagBits        commandBufferUsageflags;

	:c:member:`commandBufferCount`
		| Array size of :c:member:`commandBufferHandles`

	:c:member:`commandBufferHandles`
		| Pointer to an array of :c:struct:`kmr_vk_command_buffer_handle` which contains your
		| actual `VkCommandBuffer`_ handles to start writing commands to.

	:c:member:`commandBufferUsageflags`
		| `VkCommandBufferUsageFlagBits`_

==================================
kmr_vk_command_buffer_record_begin
==================================

.. c:function:: int kmr_vk_command_buffer_record_begin(struct kmr_vk_command_buffer_record_info *kmrvk);

	Function sets recording command in command buffers up to ``commandBufferCount``. Thus, allowing each
	command buffer to become writeable. Allowing for the application to write commands into it. Theses commands
	are later put into a queue to be sent off to the GPU.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_command_buffer_record_info`

	Returns:
		| **on success:** 0
		| **on failure:** -1

================================
kmr_vk_command_buffer_record_end
================================

.. c:function:: int kmr_vk_command_buffer_record_end(struct kmr_vk_command_buffer_record_info *kmrvk);

	Function stops command buffer to recording. Thus, ending each command buffers ability to accept commands.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_command_buffer_record_info`

	Returns:
		| **on success:** 0
		| **on failure:** -1

=========================================================================================================================================

===================
kmr_vk_fence_handle
===================

.. c:struct:: kmr_vk_fence_handle

	.. c:member::
		VkFence fence;

	:c:member:`fence`
		| May be used to insert a dependency from a queue to the host. Used to block host (CPU)
		| operations until commands in a command buffer are finished. Handles CPU - GPU syncs.
		| It is up to host to set `VkFence`_ to an unsignaled state after GPU set it to a signaled
		| state when a resource becomes available. Host side we wait for that signal then
		| conduct XYZ operations. This is how we block.

=======================
kmr_vk_semaphore_handle
=======================

.. c:struct:: kmr_vk_semaphore_handle

	.. c:member::
		VkSemaphore semaphore;

	:c:member:`semaphore`
		| May be used to insert a dependency between queue operations or between a queue
		| operation and the host. Used to block queue operations until commands in a
		| command buffer are finished. Handles GPU - GPU syncs. Solely utilized on the
		| GPU itself. Thus, only the GPU can control the state of a semphore.

===============
kmr_vk_sync_obj
===============

.. c:struct:: kmr_vk_sync_obj

	.. c:member::
		VkDevice                       logicalDevice;
		uint32_t                       fenceCount;
		struct kmr_vk_fence_handle     *fenceHandles;
		uint32_t                       semaphoreCount;
		struct kmr_vk_semaphore_handle *semaphoreHandles;

	:c:member:`logicalDevice`
		| `VkDevice`_ handle (Logical Device) associated with :c:member:`fenceCount`
		| `VkFence`_ objects and :c:member:`semaphoreCount` `VkSemaphore`_

	:c:member:`fenceCount`
		| Array size of :c:member:`fenceHandles` array

	:c:member:`fenceHandles`
		| Pointer to an array of `VkFence`_ handles

	:c:member:`semaphoreCount`
		| Array size of :c:member:`semaphoreHandles` array

	:c:member:`semaphoreHandles`
		| Pointer to an array of `VkSemaphore`_ handles

===========================
kmr_vk_sync_obj_create_info
===========================

.. c:struct:: kmr_vk_sync_obj_create_info

	.. c:member::
		VkDevice        logicalDevice;
		VkSemaphoreType semaphoreType;
		uint8_t         semaphoreCount;
		uint8_t         fenceCount;

	:c:member:`logicalDevice`
		| Must pass a valid `VkDevice`_ handle (Logical Device)

	:c:member:`semaphoreType`
		| Specifies the type of semaphore to create (`VkSemaphoreType`_).

	:c:member:`semaphoreCount`
		| Amount of `VkSemaphore`_ objects to allocate.
		| Initial value of each semaphore is set to zero.

	:c:member:`fenceCount`
		| Amount of `VkFence`_ objects to allocate.

======================
kmr_vk_sync_obj_create
======================

.. c:function:: struct kmr_vk_sync_obj kmr_vk_sync_obj_create(struct kmr_vk_sync_obj_create_info *kmrvk);

	Creates `VkFence`_ and `VkSemaphore`_ synchronization objects. Vulkan API calls that execute work
	on the GPU happen asynchronously. Vulkan API function calls return before operations are fully finished.
	So we need synchronization objects to make sure operations that require other operations to finish can
	happen after.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_sync_obj_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_sync_obj`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_sync_obj` { with members nulled }

=========================================================================================================================================

====================
kmr_vk_sync_obj_type
====================

.. c:enum:: kmr_vk_sync_obj_type

        .. c:macro::
		KMR_VK_SYNC_OBJ_FENCE
		KMR_VK_SYNC_OBJ_SEMAPHORE

	:c:macro:`KMR_VK_SYNC_OBJ_FENCE`
		| Value set to ``0``

	:c:macro:`KMR_VK_SYNC_OBJ_SEMAPHORE`
		| Value set to ``1``

======================
kmr_vk_sync_obj_handle
======================

.. c:union:: kmr_vk_sync_obj_handle

	.. c:member::
		VkFence     fence;
		VkSemaphore semaphore;

	Lessens memory as only one type of Vulkan synchronization primitive
	is used at a given time.

	:c:member:`fence`
		| See ``struct`` :c:struct:`kmr_vk_fence_handle`

	:c:member:`semaphore`
		| See ``struct`` :c:struct:`kmr_vk_semaphore_handle`

============================================
kmr_vk_sync_obj_import_external_sync_fd_info
============================================

.. c:struct:: kmr_vk_sync_obj_import_external_sync_fd_info

	.. c:member::
		VkDevice               logicalDevice;
		int                    syncFd;
		kmr_vk_sync_obj_type   syncType;
		kmr_vk_sync_obj_handle syncHandle;

	:c:member:`logicalDevice`
		| Must pass a valid `VkDevice`_ handle (Logical Device)

	:c:member:`syncFd`
		| External Posix file descriptor to import and associate with Vulkan sync object.

	:c:member:`syncType`
		| Specifies the type of Vulkan sync object to bind to.

	:c:member:`syncHandle`
		| Must pass one valid Vulkan sync object `VkFence`_ or `VkSemaphore`_.

=======================================
kmr_vk_sync_obj_import_external_sync_fd
=======================================

.. c:function:: int kmr_vk_sync_obj_import_external_sync_fd(struct kmr_vk_sync_obj_import_external_sync_fd_info *kmrvk);

	From external POSIX DMA-BUF synchronization file descriptor bind to choosen Vulkan
	synchronization object. The file descriptors can be acquired via a call to
	:c:func:`kmr_dma_buf_export_sync_file_create`.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_sync_obj_import_external_sync_fd_info`

	Returns:
		| **on success:** 0
		| **on failure:** -1

============================================
kmr_vk_sync_obj_export_external_sync_fd_info
============================================

.. c:struct:: kmr_vk_sync_obj_export_external_sync_fd_info

	.. c:member::
		VkDevice               logicalDevice;
		kmr_vk_sync_obj_type   syncType;
		kmr_vk_sync_obj_handle syncHandle;

	:c:member:`logicalDevice`
		| Must pass a valid `VkDevice`_ handle (Logical Device)

	:c:member:`syncType`
		| Specifies the type of Vulkan sync object to bind to.

	:c:member:`syncHandle`
		| Must pass one valid Vulkan sync object `VkFence`_ or `VkSemaphore`_.

=======================================
kmr_vk_sync_obj_export_external_sync_fd
=======================================

.. c:function:: int kmr_vk_sync_obj_export_external_sync_fd(struct kmr_vk_sync_obj_export_external_sync_fd_info *kmrvk);

	Creates POSIX file descriptor associated with Vulkan synchronization object.
	This file descriptor can later be associated with a DMA-BUF fd via
	:c:func:`kmr_dma_buf_import_sync_file_create`.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_sync_obj_export_external_sync_fd_info`

	Returns:
		| **on success:** POSIX file descriptor associated with Vulkan sync object
		| **on failure:** -1

=========================================================================================================================================

=============
kmr_vk_buffer
=============

.. c:struct:: kmr_vk_buffer

	.. c:member:: 
		VkDevice       logicalDevice;
		VkBuffer       buffer;
		VkDeviceMemory deviceMemory;

	:c:member:`logicalDevice`
		| `VkDevice`_ handle (Logical Device) associated with `VkBuffer`_

	:c:member:`buffer`
		| Header for the given buffer that stores information about the buffer

	:c:member:`deviceMemory`
		| Pointer to actual memory whether CPU or GPU visible associated with
		| `VkBuffer`_ header object.

=========================
kmr_vk_buffer_create_info
=========================

.. c:struct:: kmr_vk_buffer_create_info

	.. c:member:: 
		VkDevice                 logicalDevice;
		VkPhysicalDevice         physDevice;
		VkBufferCreateFlagBits   bufferFlags;
		VkDeviceSize             bufferSize;
		VkBufferUsageFlags       bufferUsage;
		VkSharingMode            bufferSharingMode;
		uint32_t                 queueFamilyIndexCount;
		const uint32_t           *queueFamilyIndices;
		VkMemoryPropertyFlagBits memPropertyFlags;
 
	:c:member:`logicalDevice`
		| Must pass a valid `VkDevice`_ handle (Logical Device)

	:c:member:`physDevice`
		| Must pass a valid `VkPhysicalDevice`_ handle as it is used to
		| query memory properties.

	:c:member:`bufferFlags`
		| Used when configuring sparse buffer memory

	:c:member:`bufferSize`
		| Size of underlying buffer to allocate

	:c:member:`bufferUsage`
		| Specifies type of buffer (i.e vertex, etc...). Multiple buffer types
		| can be selected here via bitwise or operations.

	:c:member:`bufferSharingMode`
		| Vulkan buffers may be owned by one device queue family or shared by
		| multiple device queue families.

	:c:member:`queueFamilyIndexCount`
		| Must pass array size of :c:member:`queueFamilyIndices`. Amount of
		| queue families may own given `VkBuffer`_.

	:c:member:`queueFamilyIndices`
		| Pointer to an array of queue family indices to associate/own a
		| given `VkBuffer`_.

	:c:member:`memPropertyFlags`
		| Used to determine the type of actual memory to allocated.
		| Whether CPU (host) or GPU visible.

====================
kmr_vk_buffer_create
====================

.. c:function:: struct kmr_vk_buffer kmr_vk_buffer_create(struct kmr_vk_buffer_create_info *kmrvk);

	Function creates `VkBuffer`_ header and binds pointer to actual memory (`VkDeviceMemory`_)
	to said `VkBuffer`_ headers. This allows host visible data (i.e vertex data) to be given
	to the GPU.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_buffer_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_buffer`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_buffer` { with members nulled }

=========================================================================================================================================

============================
kmr_vk_descriptor_set_layout
============================

.. c:struct:: kmr_vk_descriptor_set_layout

	.. c:member::
		VkDevice              logicalDevice;
		VkDescriptorSetLayout descriptorSetLayout;

	:c:member:`logicalDevice`
		| `VkDevice`_ handle (Logical Device) associated with `VkDescriptorSetLayout`_

	:c:member:`descriptorSetLayout`
		| Describes how a descriptor set connects up to graphics pipeline. The layout
		| defines what type of descriptor set to allocate, a binding link used by vulkan
		| to give shader access to resources, and at what graphics pipeline stage
		| will the shader descriptor need access to a given resource.

========================================
kmr_vk_descriptor_set_layout_create_info
========================================

.. c:struct:: kmr_vk_descriptor_set_layout_create_info

	.. c:member::
		VkDevice                         logicalDevice;
		VkDescriptorSetLayoutCreateFlags descriptorSetLayoutCreateflags;
		uint32_t                         descriptorSetLayoutBindingCount;
		VkDescriptorSetLayoutBinding     *descriptorSetLayoutBindings;

	:c:member:`logicalDevice`
		| Must pass a valid `VkDevice`_ handle (Logical Device)

	:c:member:`descriptorSetLayoutCreateflags`
		| Options for descriptor set layout

	:c:member:`descriptorSetLayoutBindingCount`
		| Must pass the array size of :c:member:`descriptorSetLayoutBindings`

	:c:member:`descriptorSetLayoutBindings`
		| Pointer to an array of shader variable (descriptor) attributes. Attributes include the
		| binding location set in the shader, the type of descriptor allowed to be allocated,
		| and what graphics pipeline stage will shader have access to vulkan resources assoicated
		| with `VkDescriptorSetLayoutBinding`_ { ``binding`` }.

===================================
kmr_vk_descriptor_set_layout_create
===================================

.. c:function:: struct kmr_vk_descriptor_set_layout kmr_vk_descriptor_set_layout_create(struct kmr_vk_descriptor_set_layout_create_info *kmrvk);

	Function creates descriptor set layout which is used during pipeline creation to
	define how a given shader may access vulkan resources. Also used during `VkDescriptorSet`_
	creation to define what type of descriptor to allocate within a descriptor set, binding
	locate used by both vulkan and shader to determine how shader can access vulkan
	resources, and at what pipeline stage.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_descriptor_set_layout_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_descriptor_set_layout`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_descriptor_set_layout` { with members nulled }

=========================================================================================================================================

============================
kmr_vk_descriptor_set_handle
============================

.. c:struct:: kmr_vk_descriptor_set_handle

	.. c:member::
		VkDescriptorSet descriptorSet;

	:c:member:`descriptorSet`
		| Represents a set of descriptors. Descriptors can be defined as resources shared across
		| draw operations. If there is only one uniform buffer object (type of descriptor) in the
		| shader then there is only one descriptor in the set. If the uniform buffer object
		| (type of descriptor) in the shader is an array of size N. Then there are N descriptors
		| of the same type in a given descriptor set. Types of descriptor sets include Images,
		| Samplers, uniform...

=====================
kmr_vk_descriptor_set
=====================

.. c:struct:: kmr_vk_descriptor_set

	.. c:member::
		VkDevice                            logicalDevice;
		VkDescriptorPool                    descriptorPool;
		uint32_t                            descriptorSetsCount;
		struct kmr_vk_descriptor_set_handle *descriptorSetHandles;

	:c:member:`logicalDevice`
		| `VkDevice`_ handle (Logical Device) associated with `VkDescriptorPool`_ which
		| contains one or more descriptor sets.

	:c:member:`descriptorPool`
		| Pointer to a Vulkan Descriptor Pool used to allocate and dellocate descriptor sets.

	:c:member:`descriptorSetsCount`
		| Number of descriptor sets in the :c:member:`descriptorSetHandles` array

	:c:member:`descriptorSetHandles`
		| Pointer to an array of descriptor sets

=================================
kmr_vk_descriptor_set_create_info
=================================

.. c:struct:: kmr_vk_descriptor_set_create_info

	.. c:member::
		VkDevice                    logicalDevice;
		VkDescriptorPoolSize        *descriptorPoolInfos;
		uint32_t                    descriptorPoolInfoCount;
		VkDescriptorSetLayout       *descriptorSetLayouts;
		uint32_t                    descriptorSetLayoutCount;
		VkDescriptorPoolCreateFlags descriptorPoolCreateflags;

	:c:member:`logicalDevice`
		| Must pass a valid `VkDevice`_ handle (Logical Device)

	:c:member:`descriptorPoolInfos`
		| Must pass a pointer to an array of descriptor sets information. With each
		| elements information corresponding to number of descriptors a given pool
		| needs to allocate and the type of descriptors in the pool. Per my understanding
		| this is just so the `VkDescriptorPool`_ knows what to preallocate. No descriptor
		| is assigned to a set when the pool is created. Given an array of descriptor
		| set layouts the actual assignment of descriptor to descriptor set happens in
                | the `vkAllocateDescriptorSets`_ function.

	:c:member:`descriptorPoolInfoCount`
		| Number of descriptor sets in pool or array size of :c:member:`descriptorPoolInfos`

	:c:member:`descriptorSetLayouts`
		| Pointer to an array of `VkDescriptorSetLayout`_. Each set must contain it's own layout.
		| This variable must be of same size as :c:member:`descriptorPoolInfos`

	:c:member:`descriptorSetLayoutCount`
		| Array size of :c:member:`descriptorSetLayouts` corresponding to the actual number of descriptor
		| sets in the pool.

	:c:member:`descriptorPoolCreateflags`
		| Enables certain operations on a pool (i.e enabling freeing of descriptor sets)

============================
kmr_vk_descriptor_set_create
============================

.. c:function:: struct kmr_vk_descriptor_set kmr_vk_descriptor_set_create(struct kmr_vk_descriptor_set_create_info *kmrvk);

	Function allocates a descriptor pool then ``descriptorPoolInfoCount`` amount of sets
	from descriptor pool. This is how we establishes connection between a the shader
	and vulkan resources at specific graphics pipeline stages. One can think of a
	descriptor pool as a range of addresses that contain segments or sub range addresses.
	Each segment is a descriptor set within the pool and each set contains a certain number
	of descriptors.

	Descriptor Pool
	---------------

	=============== =========  =====  === ===
	Descriptor Set      1        2     3   4
	=============== =========  =====  === ===
	Descriptors     1|2|3|4|5  1|2|3   1  1|2
	=============== =========  =====  === ===

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_descriptor_set_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_descriptor_set`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_descriptor_set` { with members nulled }

=========================================================================================================================================

==============
kmr_vk_sampler
==============

.. c:struct:: kmr_vk_sampler

	.. c:member::
		VkDevice  logicalDevice;
		VkSampler sampler;

	:c:member:`logicalDevice`
		| `VkDevice`_ handle (Logical Device) associated with `VkSampler`_

	:c:member:`sampler`
		| `VkSampler` handle represent the state of an image sampler which is used
		| by the implementation to read image data and apply filtering and other
		| transformations for the shader.

==========================
kmr_vk_sampler_create_info
==========================

.. c:struct:: kmr_vk_sampler_create_info

	.. c:member::
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

	Most members may also be located at `VkSamplerCreateInfo`_.

	:c:member:`logicalDevice`
		| Must pass a valid `VkDevice`_ handle (Logical Device)

	:c:member:`samplerFlags`
		| TBA

	:c:member:`samplerMagFilter`
		| How to render when image is magnified on screen (Camera close to texture)

	:c:member:`samplerMinFilter`
		| How to render when image is minimized on screen (Camera further away from texture)

	:c:member:`samplerAddressModeU`
		| How to handle texture wrap in U (x) direction

	:c:member:`samplerAddressModeV`
		| How to handle texture wrap in V (y) direction

	:c:member:`samplerAddressModeW`
		| How to handle texture wrap in W (z) direction

	:c:member:`samplerBorderColor`
		| Used if ``samplerAddressMode{U,V,W}`` == `VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER`_
		| set border beyond texture.

	:c:member:`samplerMipmapMode`
		| Mipmap interpolation mode

	:c:member:`samplerMipLodBias`
		| Level of details bias for mip level. Sets value to add to mip levels.

	:c:member:`samplerMinLod`
		| Minimum level of detail to pick miplevel

	:c:member:`samplerMaxLod`
		| Maximum level of detail to pick miplevel

	:c:member:`samplerUnnormalizedCoordinates`
		| Sets if the texture coordinates should be normalized (between 0 and 1)

	:c:member:`samplerAnisotropyEnable`
		| Enable/Disable Anisotropy

	:c:member:`samplerMaxAnisotropy`
		| Anisotropy sample level

	:c:member:`samplerCompareEnable`
		| TBA

	:c:member:`samplerCompareOp`
		| TBA

=====================
kmr_vk_sampler_create
=====================

.. c:function:: struct kmr_vk_sampler kmr_vk_sampler_create(struct kmr_vk_sampler_create_info *kmrvk);

	Functions creates `VkSampler`_ handle this object accesses the image using pre-defined
	methods. These methods cover concepts such as picking a point between two texels or
	beyond the edge of the image. Describes how an image should be read.

	Loaded images (png, jpg) -> `VkImage`_ -> Sampler interacts with the `VkImage`_.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_sampler_create_info`

	Returns:
		| **on success:** ``struct`` :c:struct:`kmr_vk_sampler`
		| **on failure:** ``struct`` :c:struct:`kmr_vk_sampler` { with members nulled }

=========================================================================================================================================

=========================
kmr_vk_resource_copy_type
=========================

.. c:enum:: kmr_vk_resource_copy_type

	.. c:macro::
		KMR_VK_RESOURCE_COPY_VK_BUFFER_TO_VK_BUFFER
		KMR_VK_RESOURCE_COPY_VK_BUFFER_TO_VK_IMAGE
		KMR_VK_RESOURCE_COPY_VK_IMAGE_TO_VK_BUFFER

	ENUM Used by ``struct`` :c:struct:`kmr_vk_resource_copy_info` to specify type of source
	resource to copy over to a given type of destination resource.

	:c:macro:`KMR_VK_RESOURCE_COPY_VK_BUFFER_TO_VK_BUFFER`
		| Value set to ``0``

	:c:macro:`KMR_VK_RESOURCE_COPY_VK_BUFFER_TO_VK_IMAGE`
		| Value set to ``1``

	:c:macro:`KMR_VK_RESOURCE_COPY_VK_IMAGE_TO_VK_BUFFER`
		| Value set to ``2``

==========================================
kmr_vk_resource_copy_buffer_to_buffer_info
==========================================

.. c:struct:: kmr_vk_resource_copy_buffer_to_buffer_info

	.. c:member::
		VkBufferCopy *copyRegion;

	:c:member:`copyRegion`
		| Specifies the byte offset to use for given
		| ``struct`` :c:struct:`kmr_vk_resource_copy_info` { ``srcResource`` } memory address.
		| Then, specifies the byte offset to use for given
		| ``struct`` :c:struct:`kmr_vk_resource_copy_info` { ``dstResource`` } memory address.
		| Along with including the byte size (`VkBufferCopy`_ { ``size`` }) to copy
		| from ``srcResource`` to ``dstResource``.

=========================================
kmr_vk_resource_copy_buffer_to_image_info
=========================================

.. c:struct:: kmr_vk_resource_copy_buffer_to_image_info

	.. c:member::
		VkBufferImageCopy *copyRegion;
		VkImageLayout     imageLayout;

	:c:member:`copyRegion`
		| Specifies the byte offset to use for given
		| ``struct`` :c:struct:`kmr_vk_resource_copy_info` { ``srcResource`` } memory address.
		| Along with specifying what portion of the (``dstResource``) image to update or copy
		| given ``srcResource``.
		| `VkBufferImageCopy`_.

	:c:member:`imageLayout`
		| Memory layout of the destination image subresources after the copy

=========================
kmr_vk_resource_copy_info
=========================

.. c:struct:: kmr_vk_resource_copy_info

	.. c:member::
		kmr_vk_resource_copy_type resourceCopyType;
		void                      *resourceCopyInfo;
		VkCommandBuffer           commandBuffer;
		VkQueue                   queue;
		void                      *srcResource;
		void                      *dstResource;

	:c:member:`resourceCopyType`
		| Determines what vkCmdCopyBuffer* function to utilize

	:c:member:`resourceCopyInfo`
		| The structs to pass to vkCmdCopyBuffer*

	:c:member:`commandBuffer`
		| Command buffer used for recording. Best to utilize one already create via
		| :c:func:`kmr_vk_command_buffer_create`. To save on unnecessary allocations.

	:c:member:`queue`
		| The physical device queue (graphics or transfer) to submit the copy buffer command to.

	:c:member:`srcResource`
		| Pointer to source vulkan resource containing raw data.
		| (i.e `Vkbuffer`_, `VkImage`_, etc...)

	:c:member:`dstResource`
		| Pointer to destination vulkan resource to copy :c:member:`srcResource` data to.
		| (i.e `Vkbuffer`_, `VkImage`_, etc...)

====================
kmr_vk_resource_copy
====================

.. c:function:: int kmr_vk_resource_copy(struct kmr_vk_resource_copy_info *kmrvk);

	Function copies data from one vulkan resource to another. Best utilized when
	copying data from CPU visible buffer over to GPU visible buffer. That way the
	GPU can acquire data (vertex data) more quickly.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_resource_copy_info`

	Returns:
		| **on success:** 0
		| **on failure:** -1

=========================================================================================================================================

=====================================
kmr_vk_resource_pipeline_barrier_info
=====================================

.. c:struct:: kmr_vk_resource_pipeline_barrier_info

	.. c:member::
		VkCommandBuffer                       commandBuffer;
		VkQueue                               queue;
		VkPipelineStageFlags                  srcPipelineStage;
		VkPipelineStageFlags                  dstPipelineStage;
		VkDependencyFlags                     dependencyFlags;
		VkMemoryBarrier                       *memoryBarrier;
		VkBufferMemoryBarrier                 *bufferMemoryBarrier;
		VkImageMemoryBarrier                  *imageMemoryBarrier;

	:c:member:`commandBuffer`
		| Command buffer used for recording. Best to utilize one already create via
		| :c:func:`kmr_vk_command_buffer_create`. To save on unnecessary allocations.

	:c:member:`queue`
		| The physical device queue (graphics or transfer) to submit the pipeline
		| barrier command to.

	:c:member:`srcPipelineStage`
		| Specifies in which pipeline stage operations occur before the barrier.

	:c:member:`dstPipelineStage`
		| Specifies in which pipeline stage operations will wait on the barrier.

	:c:member:`dependencyFlags`
		| Defines types of dependencies

	:c:member:`memoryBarrier`
		| Specifies pipeline barrier for vulkan memory

	:c:member:`bufferMemoryBarrier`
		| Specifies pipeline barrier for vulkan buffer resource

	:c:member:`imageMemoryBarrier`
		| Specifies pipeline barrier for vulkan image resource

================================
kmr_vk_resource_pipeline_barrier
================================

.. c:function:: int kmr_vk_resource_pipeline_barrier(struct kmr_vk_resource_pipeline_barrier_info *kmrvk);

	Function is used to synchronize access to vulkan resources. Basically
	ensuring that a write to a resources finishes before reading from it.

	Parameters:
		| **kmrvk:** pointer to a ``struct`` :c:struct:`kmr_vk_resource_pipeline_barrier_info`

	Returns:
		| **on success:** 0
		| **on failure:** -1

=========================================================================================================================================

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
.. _VkShaderModule: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkShaderModule.html
.. _VkPipelineLayout: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineLayout.html
.. _VkPipelineLayoutCreateInfo: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineLayoutCreateInfo.html
.. _VkRenderPass: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkRenderPass.html
.. _VkRenderPassCreateInfo: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkRenderPassCreateInfo.html
.. _VkFramebuffer: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkFramebuffer.html
.. _VkFramebufferCreateInfo: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkFramebufferCreateInfo.html
.. _VkPipeline: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkPipeline.html
.. _VkGraphicsPipelineCreateInfo: https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkGraphicsPipelineCreateInfo.html
.. _VkPrimitiveTopology: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPrimitiveTopology.html
.. _VkViewport: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkViewport.html
.. _VkCommandPool: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandPool.html
.. _VkCommandBuffer: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandBuffer.html
.. _VkCommandPoolCreateFlagBits: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandPoolCreateFlagBits.html
.. _VkCommandBufferUsageFlagBits: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandBufferUsageFlagBits.html
.. _VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandPoolCreateFlagBits.html
.. _VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkCommandPoolCreateFlagBits.html
.. _VkFence: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFence.html
.. _VkSemaphore: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSemaphore.html
.. _VkSemaphoreType: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSemaphoreType.html
.. _VkBuffer: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBuffer.html
.. _VkBufferCreateInfo: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBufferCreateInfo.html
.. _VkDescriptorSetLayout: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSetLayout.html
.. _VkDescriptorSetLayoutBinding: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSetLayoutBinding.html
.. _VkDescriptorSet: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorSet.html
.. _VkDescriptorPool: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorPool.html
.. _vkAllocateDescriptorSets: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkAllocateDescriptorSets.html
.. _VkSampler: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSampler.html
.. _VkSamplerCreateInfo: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSamplerCreateInfo.html
.. _VkSamplerAddressMode: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSamplerAddressMode.html
.. _VK_SAMPLER_ADDRESS_MODE_REPEAT: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSamplerAddressMode.html
.. _VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSamplerAddressMode.html
.. _VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSamplerAddressMode.html
.. _VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSamplerAddressMode.html
.. _VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSamplerAddressMode.html
.. _VkBufferCopy: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBufferCopy.html
.. _VkBufferImageCopy: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkBufferImageCopy.html
.. _Scissor: https://registry.khronos.org/vulkan/specs/1.3-extensions/html/vkspec.html#fragops-scissor
