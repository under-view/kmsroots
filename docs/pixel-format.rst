.. default-domain:: C

pixel-format
============

Header: kmsroots/pixel-format.h

Table of contents (click to go)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

======
Macros
======

=====
Enums
=====

1. :c:enum:`kmr_pixel_format_conv_type`
2. :c:enum:`kmr_pixel_format_type`

======
Unions
======

=======
Structs
=======

=========
Functions
=========

1. :c:func:`kmr_pixel_format_convert_name`
#. :c:func:`kmr_pixel_format_get_name`

=================
Function Pointers
=================

API Documentation
~~~~~~~~~~~~~~~~~

==========================
kmr_pixel_format_conv_type
==========================

.. c:enum:: kmr_pixel_format_conv_type

	.. c:macro::
		KMR_PIXEL_FORMAT_CONV_GBM_TO_VK
		KMR_PIXEL_FORMAT_CONV_GBM_TO_DRM
		KMR_PIXEL_FORMAT_CONV_VK_TO_GBM
		KMR_PIXEL_FORMAT_CONV_VK_TO_DRM
		KMR_PIXEL_FORMAT_CONV_DRM_TO_VK
		KMR_PIXEL_FORMAT_CONV_DRM_TO_GBM

	Specifies the type of pixel format name conversion to perform.
	Values may not be bitwise or'd together.

	:c:macro:`KMR_PIXEL_FORMAT_CONV_GBM_TO_VK`
		| Convert `GBM format`_ to `VkFormat`_

	:c:macro:`KMR_PIXEL_FORMAT_CONV_GBM_TO_DRM`
		| Convert `GBM format`_ to `DRM format`_

	:c:macro:`KMR_PIXEL_FORMAT_CONV_VK_TO_GBM`
		| Convert `VkFormat`_ to `GBM format`_

	:c:macro:`KMR_PIXEL_FORMAT_CONV_VK_TO_DRM`
		| Convert `VkFormat`_ to `DRM format`_

	:c:macro:`KMR_PIXEL_FORMAT_CONV_DRM_TO_VK`
		| Convert `DRM format`_ to `VkFormat`_

	:c:macro:`KMR_PIXEL_FORMAT_CONV_DRM_TO_GBM`
		| Convert `DRM Format`_ to `GBM format`_

=============================
kmr_pixel_format_convert_name
=============================

.. c:function:: uint32_t kmr_pixel_format_convert_name(kmr_pixel_format_conv_type conv, uint32_t format);

	Converts the unsigned 32bit integer name given to one pixel format to
	the unsigned 32bit name of another. GBM format name to VK format name.
	The underlying buffer will be in the same pixel format. Makes it easier
	to determine to format to create VkImage's with given the format of the
	GBM buffer object.

	Parameters:
		| **conv**
		| Enum constant specifying the format to convert from then to.
		| **format**
		| Unsigned 32bit integer representing the type of pixel format.

	Returns:
		| **on success:** GBM/DRM/VK format (unsigned 32bit integer)
		| **on failure:** UINT32_MAX

=========================================================================================================================================

=====================
kmr_pixel_format_type
=====================

.. c:enum:: kmr_pixel_format_type

	.. c:macro::
		KMR_PIXEL_FORMAT_VK

	Specifies the type of pixel format to choose from which API (DRM/GBM/VK)

	:c:macro:`KMR_PIXEL_FORMAT_VK`
		| Use `Vkformat`_ for operations

=========================
kmr_pixel_format_get_name
=========================

.. c:function:: const char *kmr_pixel_format_get_name(kmr_pixel_format_type formatType, uint32_t format);

	Parameters:
		| **formatType**
		| Enum constant specifying the API (GBM/DRM/VK) format name.
		| **format**
		| Unsigned 32bit integer representing the type of pixel format.

	Returns:
		**on success:** GBM/DRM/VK format in string form
		**on failure:** NULL

=========================================================================================================================================

.. _VkFormat: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFormat.html
.. _DRM format: https://github.com/under-view/kmsroots/blob/master/src/pixel-format.c
.. _GBM format: https://github.com/under-view/kmsroots/blob/master/src/pixel-format.c
