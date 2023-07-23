.. default-domain:: C

Vulkan
======

Header: kmsroots/vulkan.h

Table of contents (click to go)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Structs:

1. :c:struct:`kmr_vk_instance_create_info`

Enums:

Functions:

1. :c:func:`kmr_vk_instance_create`


Functions documentation
~~~~~~~~~~~~~~~~~~~~~~~

.. c:struct:: kmr_vk_instance_create_info

        .. c:member::
                const char *appName
                const char *engineName
                uint32_t enabledLayerCount
                const char **enabledLayerNames
                uint32_t enabledExtensionCount
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

        Creates a VkInstance object and establishes a connection to the Vulkan API.
        It also acts as an easy wrapper that allows one to define instance extensions.
        Instance extensions basically allow developers to define what an app is setup to do.
        So, if a client wants the application to work with wayland surface or X11 surface etc...
        Client should enable those extensions inorder to gain access to those particular capabilities.

        **Parameters**

                :kmrvk: pointer to a struct :c:struct:`kmr_vk_instance_create_info`

        **returns** `VkInstance`_

.. _VkInstance: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstance.html
.. _VkInstanceCreateInfo: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkInstanceCreateInfo.html
.. _VkApplicationInfo: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkApplicationInfo.html
