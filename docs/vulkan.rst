.. default-domain:: C

Vulkan
======

Header: kmsroots/vulkan.h

Table of contents (click to go)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Enums:

1. :c:enum:`kmr_vk_surface_type`

Structs:

1. :c:struct:`kmr_vk_instance_create_info`
#. :c:struct:`kmr_vk_surface_create_info`
#. :c:struct:`kmr_vk_phdev`
#. :c:struct:`kmr_vk_phdev_create_info`

Functions:

1. :c:func:`kmr_vk_instance_create`
#. :c:func:`kmr_vk_surface_create`
#. :c:func:`kmr_vk_phdev_create`

API Documentation
~~~~~~~~~~~~~~~~~

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
	if passed KMS device node file descriptor (struct :c:struct:`kmr_vk_phdev_create_info` { **kmsfd** })
	is equal to the physical device suggested by (struct :c:struct:`kmr_vk_phdev_create_info` { **deviceType** }).
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
