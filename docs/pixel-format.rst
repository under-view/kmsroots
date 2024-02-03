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

	:c:macro:`KMR_PIXEL_FORMAT_CONV_GBM_TO_VK`
		| Value set to ``0``

	:c:macro:`KMR_PIXEL_FORMAT_CONV_GBM_TO_DRM`
		| Value set to ``1``

	:c:macro:`KMR_PIXEL_FORMAT_CONV_VK_TO_GBM`
		| Value set to ``2``

	:c:macro:`KMR_PIXEL_FORMAT_CONV_VK_TO_DRM`
		| Value set to ``3``

	:c:macro:`KMR_PIXEL_FORMAT_CONV_DRM_TO_VK`
		| Value set to ``4``

	:c:macro:`KMR_PIXEL_FORMAT_CONV_DRM_TO_GBM`
		| Value set to ``5``

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
		| **inputInfo**
		| Pointer to a struct kmr_input_create_info used store information about
		| the current seatd/sytemd-logind D-Bus session

	Returns:
		| **on success:** Pointer to a ``struct`` :c:struct:`kmr_input`
		| **on failure:** ``NULL``

=========================================================================================================================================

=====================
kmr_pixel_format_type
=====================

.. c:enum:: kmr_pixel_format_type

	.. c:macro::
		KMR_PIXEL_FORMAT_VK

	Specifies the type of pixel format to choose from which API (DRM/GBM/VK)

	:c:macro:`KMR_PIXEL_FORMAT_VK`
		| Value set to ``0``

=========================
kmr_pixel_format_get_name
=========================

.. c:function:: const char *kmr_pixel_format_get_name(kmr_pixel_format_type formatType, uint32_t format);

	Parameters:
		| **formatType** - Enum constant specifying the API (GBM/DRM/VK) format name.
		| **format** - Unsigned 32bit integer representing the type of pixel format.

	Returns:
		**on success:** GBM/DRM/VK format in string form
		**on failure:** NULL
