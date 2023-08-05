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

Functions:

1. :c:func:`kmr_vk_instance_create`
#. :c:func:`kmr_vk_surface_create`
#. :c:func:`kmr_vk_phdev_create`
#. :c:func:`kmr_vk_queue_create`
#. :c:func:`kmr_vk_lgdev_create`

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
                const char *appName
                const char *engineName
                uint32_t   enabledLayerCount
                const char **enabledLayerNames
                uint32_t   enabledExtensionCount
                const char **enabledExtensionNames

        :c:member:`appName`

        A member of the `VkApplicationInfo`_ structure reserved for the name of the application.

        :c:member:`engineName`

        A member of the `VkApplicationInfo`_ structure reserved for the name of the engine
        name (if any) used to create application.

        :c:member:`enabledLayerCount`

        A member of the `VkInstanceCreateInfo`_ structure used to pass the number of Vulkan
        Validation Layers a client wants to enable.

        :c:member:`enabledLayerNames`

        A member of the `VkInstanceCreateInfo`_ structure used to pass a pointer to an array
        of strings containing the name of the Vulkan Validation Layers one wants to enable.

        :c:member:`enabledExtensionCount`

        A member of the `VkInstanceCreateInfo`_ structure used to pass the the number of vulkan
        instance extensions a client wants to enable.

        :c:member:`enabledExtensionNames`

        A member of the `VkInstanceCreateInfo`_ structure used to pass a pointer to an array
        of strings containing the name of the Vulkan Instance Extensions one wants to enable.

.. c:function:: VkInstance kmr_vk_instance_create(struct kmr_vk_instance_create_info *kmrvk)

        Creates a `VkInstance`_ object and establishes a connection to the Vulkan API.
        It also acts as an easy wrapper that allows one to define instance extensions.
        Instance extensions basically allow developers to define what an app is setup to do.
        So, if a client wants the application to work with wayland surface or X11 surface etc...
        Client should enable those extensions inorder to gain access to those particular capabilities.

        :parameters:
                :kmrvk: pointer to a struct :c:struct:`kmr_vk_instance_create_info`
        :returns:
                :on success: `VkInstance`_
                :on faliure: `VK_NULL_HANDLE`_

===========================================================================================================

.. c:enum:: kmr_vk_surface_type

        .. c:macro::
                KMR_SURFACE_WAYLAND_CLIENT
                KMR_SURFACE_XCB_CLIENT

        Display server protocol options. Used by :c:func:`kmr_vk_surface_create`
        to create a `VkSurfaceKHR`_ object based upon platform specific information

.. c:struct:: kmr_vk_surface_create_info

        .. c:member::
                kmr_vk_surface_type surfaceType
                VkInstance          instance
                void                *surface
                void                *display
                unsigned int        window

        :c:member:`surfaceType`

        Must pass a valid enum :c:enum:`kmr_vk_surface_type` value. Used in determine what vkCreate*SurfaceKHR
        function and associated structs to utilize when creating the `VkSurfaceKHR`_ object.

        :c:member:`instance`

        Must pass a valid `VkInstance`_ handle to create/associate surfaces for an application

        :c:member:`surface`

        Must pass a pointer to a struct wl_surface object

        :c:member:`display`

        Must pass either a pointer to struct wl_display object or a pointer to an xcb_connection_t

        :c:member:`window`

        Must pass an xcb_window_t window id or an unsigned int representing XID

.. c:function:: VkSurfaceKHR kmr_vk_surface_create(struct kmr_vk_surface_create_info *kmrvk)

        Creates a `VkSurfaceKHR`_ object based upon platform specific information about the given surface.
        `VkSurfaceKHR`_ are the interface between the window and Vulkan defined images in a given swapchain
        if vulkan swapchain exists.

        :parameters:
                :kmrvk: pointer to a struct :c:struct:`kmr_vk_surface_create_info`
        :returns:
                :on success: `VkSurfaceKHR`_
                :on faliure: `VK_NULL_HANDLE`_

===========================================================================================================

.. c:struct:: kmr_vk_phdev

	.. c:member::
		VkInstance                       instance
		VkPhysicalDevice                 physDevice
		VkPhysicalDeviceProperties       physDeviceProperties
		VkPhysicalDeviceFeatures         physDeviceFeatures
		int                              kmsfd
		VkPhysicalDeviceDrmPropertiesEXT physDeviceDrmProperties

	:c:member:`instance`

	Must pass a valid `VkInstance`_ handle which to find and create a `VkPhysicalDevice`_ with.

	:c:member:`physDevice`

	Must pass one of the supported `VkPhysicalDeviceType`_'s.

	:c:member:`physDeviceProperties`

	Structure specifying physical device properties. Like allocation limits for Image Array Layers
	or maximum resolution that the device supports.

	:c:member:`physDeviceFeatures`

	Structure describing the features that can be supported by an physical device

	**Only included if meson option kms set true**

	:c:member:`kmsfd`

	KMS device node file descriptor passed via struct :c:struct:`kmr_vk_phdev_create_info`

	:c:member:`physDeviceDrmProperties`

	Structure containing DRM information of a physical device. A `VkPhysicalDeviceProperties2`_ structure
	is utilzed to populate this member. Member information is then checked by the implementation to see
	if passed KMS device node file descriptor (struct :c:struct:`kmr_vk_phdev_create_info` { **@kmsfd** })
	is equal to the physical device suggested by (struct :c:struct:`kmr_vk_phdev_create_info` { **@deviceType** }).
	Contains data stored after associate a DRM file descriptor with a vulkan physical device.

.. c:struct:: kmr_vk_phdev_create_info

	.. c:member::
		VkInstance           instance
		VkPhysicalDeviceType deviceType
		int                  kmsfd

	:c:member:`instance`

	Must pass a valid `VkInstance`_ handle which to find `VkPhysicalDevice`_ with.

	:c:member:`deviceType`

	Must pass one of the supported `VkPhysicalDeviceType`_'s.

	**Only included if meson option kms set true**

	:c:member:`kmsfd`

	Must pass a valid kms file descriptor for which a `VkPhysicalDevice`_ will be created
	if corresponding DRM properties match.

.. c:function::	struct kmr_vk_phdev kmr_vk_phdev_create(struct kmr_vk_phdev_create_info *kmrvk)

	Retrieves a `VkPhysicalDevice`_ handle if certain characteristics of a physical device are meet.
	Also retrieves a given physical device properties and features to be later used by the application.

	:parameters:
		:kmrvk: pointer to a struct :c:struct:`kmr_vk_phdev_create_info`
	:returns:
		:on success: struct :c:struct:`kmr_vk_phdev`
		:on failure: struct :c:struct:`kmr_vk_phdev` { with members nulled, int's set to -1 }

===========================================================================================================

.. c:struct:: kmr_vk_queue

	.. c:member::
		char    name[20]
		VkQueue queue
		int     familyIndex
		int     queueCount

	:c:member:`name`

	Stores the name of the queue in string format. **Not required by API**.

	:c:member:`queue`

	`VkQueue`_ handle used when submitting command buffers to physical device. Address given to
	handle in :c:func:`kmr_vk_lgdev_create` after `VkDevice`_ handle creation.

	:c:member:`familyIndex`

	`VkQueue`_ family index associate with selected struct :c:struct:`kmr_vk_queue_create_info` { **@queueFlag** }.

	:c:member:`queueCount`

	Number of queues in a given `VkQueue`_ family

.. c:struct:: kmr_vk_queue_create_info

	.. c:member::
		VkPhysicalDevice physDevice
		VkQueueFlags     queueFlag

	:c:member:`physDevice`

	Must pass a valid `VkPhysicalDevice`_ handle to query queues associate with phsyical device

	:c:member:`queueFlag`

	Must pass one `VkQueueFlagBits`_, if multiple flags are bitwised or'd function will fail
	to return `VkQueue`_ family index (struct :c:struct:`kmr_vk_queue`).

.. c:function::	struct kmr_vk_queue kmr_vk_queue_create(struct kmr_vk_queue_create_info *kmrvk);

	Queries the queues a given physical device contains. Then returns a queue
	family index and the queue count given a single `VkQueueFlagBits`_. Queue
	are used in vulkan to submit commands up to the GPU.

	:parameters:
		:kmrvk: pointer to a struct :c:struct:`kmr_vk_queue_create_info`
	:returns:
		:on success: struct :c:struct:`kmr_vk_queue`
		:on failure: struct :c:struct:`kmr_vk_queue` { with members nulled, int's set to -1 }

===========================================================================================================

.. c:struct:: kmr_vk_lgdev

	.. c:member::
		VkDevice            logicalDevice
		uint32_t            queueCount
		struct kmr_vk_queue *queues

	:c:member:`logicalDevice`

	Returned `VkDevice`_ handle which represents vulkan's access to physical device

	:c:member:`queueCount`

	Amount of elements in pointer to array of struct :c:struct:`kmr_vk_queue`. This information
	gets populated with the data pass through struct :c:struct:`kmr_vk_lgdev_create_info` { **@queueCount** }.

	:c:member:`queues`

	Pointer to an array of struct :c:struct:`kmr_vk_queue`. This information gets populated with the
	data pass through struct :c:struct:`kmr_vk_lgdev_create_info` { **@queues** }.

	Members :c:member:`queueCount` & :c:member:`queues` are strictly for struct :c:struct:`kmr_vk_lgdev`
	to have extra information amount `VkQueue`_'s

.. c:struct:: kmr_vk_lgdev_create_info

	.. c:member::
		VkInstance               instance
		VkPhysicalDevice         physDevice
		VkPhysicalDeviceFeatures *enabledFeatures
		uint32_t                 enabledExtensionCount
		const char *const        *enabledExtensionNames
		uint32_t                 queueCount
		struct kmr_vk_queue      *queues

	:c:member:`instance`

	Must pass a valid `VkInstance`_ handle to create `VkDevice`_ handle from.

	:c:member:`physDevice`

	Must pass a valid `VkPhysicalDevice`_ handle to associate `VkDevice`_ handle with.

	:c:member:`enabledFeatures`

	Must pass a valid pointer to a `VkPhysicalDeviceFeatures`_ with X features enabled

	:c:member:`enabledExtensionCount`

	Must pass the amount of Vulkan Device extensions to enable.

	:c:member:`enabledExtensionNames`

	Must pass an array of strings containing Vulkan Device extension to enable.

	:c:member:`queueCount`

	Must pass the amount of struct :c:struct:`kmr_vk_queue` { **@queue**, **@familyIndex** } to
	create along with a given logical device

	:c:member:`queues`

	Must pass a pointer to an array of struct :c:struct:`kmr_vk_queue` { **@queue**, **@familyIndex** } to
	create along with a given logical device

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

	:parameters:
		:kmrvk: pointer to a struct :c:struct:`kmr_vk_lgdev_create_info`
	:returns:
		:on success: struct :c:struct:`kmr_vk_lgdev`
		:on failure: struct :c:struct:`kmr_vk_lgdev` { with members nulled, int's set to -1 }

===========================================================================================================

.. _VK_NULL_HANDLE: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_NULL_HANDLE.html
.. _VkInstance: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstance.html
.. _VkInstanceCreateInfo: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstanceCreateInfo.html
.. _VkApplicationInfo: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkApplicationInfo.html
.. _VkSurfaceKHR: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceKHR.html
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
