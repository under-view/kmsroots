.. default-domain:: C

drm-node
========

Header: kmsroots/drm-node.h

Table of contents (click to go)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

======
Macros
======

=====
Enums
=====

======
Unions
======

=======
Structs
=======

1. :c:struct:`kmr_drm_node`
#. :c:struct:`kmr_drm_node_create_info`
#. :c:struct:`kmr_drm_node_device_capabilites`
#. :c:struct:`kmr_drm_node_display_object_props_data`
#. :c:struct:`kmr_drm_node_display_object_props`
#. :c:struct:`kmr_drm_node_display_mode_data`
#. :c:struct:`kmr_drm_node_display`
#. :c:struct:`kmr_drm_node_display_create_info`
#. :c:struct:`kmr_drm_node_display_mode_info`
#. :c:struct:`kmr_drm_node_atomic_request`
#. :c:struct:`kmr_drm_node_atomic_request_create_info`
#. :c:struct:`kmr_drm_node_handle_drm_event_info`

=========
Functions
=========

1. :c:func:`kmr_drm_node_create`
#. :c:func:`kmr_drm_node_destroy`
#. :c:func:`kmr_drm_node_get_device_capabilities`
#. :c:func:`kmr_drm_node_display_create`
#. :c:func:`kmr_drm_node_display_destroy`
#. :c:func:`kmr_drm_node_display_mode_set`
#. :c:func:`kmr_drm_node_display_mode_reset`
#. :c:func:`kmr_drm_node_atomic_request_create`
#. :c:func:`kmr_drm_node_atomic_request_destroy`
#. :c:func:`kmr_drm_node_handle_drm_event`

=================
Function Pointers
=================

1. :c:func:`kmr_drm_node_renderer_impl`

API Documentation
~~~~~~~~~~~~~~~~~

============
kmr_drm_node
============

.. c:struct:: kmr_drm_node

	.. c:member::
		int                kmsfd;
		struct kmr_session *session;

	:c:member:`kmsfd`
		| A valid file descriptor to an open DRI device node
	

	**Only included if libseat is used**

	:c:member:`session`
 		| Stores address of ``struct`` :c:struct:`kmr_session`. Used when
		| opening and releasing a device.

========================
kmr_drm_node_create_info
========================

.. c:struct:: kmr_drm_node_create_info

	.. c:member::
		const char         *kmsNode;
		struct kmr_session *session;

	:c:member:`kmsNode`
		| Path to character device associated with GPU. If set to NULL. List of
		| available kmsnode's will be queried and one will be automatically
		| choosen for you.

	**Only included if libseat is used**

	:c:member:`session`
 		| Address of ``struct`` :c:struct:`kmr_session`. Which members are
		| used to communicate with systemd-logind via D-Bus systemd-logind
		| interface. Needed by :c:func:`kmr_drm_node_create` to acquire and taken
		| control of a device without the need of being root.

===================
kmr_drm_node_create
===================

.. c:function:: struct kmr_drm_node *kmr_drm_node_create(struct kmr_drm_node_create_info *nodeInfo);

	Function opens a DRI device node. If a systemd-logind session available one can take
	control of a device node. Returned fd is exposed to all planes (overlay, primary, and cursor)
	and has access to the aspect ratio information in modes in userspace. In order
	to drive KMS, we need to be 'master'. Function fails if we aren't DRM-Master more
	info here: `DRM-Master and DRM-Auth`_. So, if a graphical session is already active on
	the current VT this function will fail.

	Parameters:
		| **nodeInfo:**
		| Pointer to a ``struct`` :c:struct:`kmr_drm_node_create_info` used to pass a DRI/KMS
		| device file that we may want to use and to store information about
		| the current seatd/sytemd-logind D-bus session.

	Returns:
		| **on success:** Pointer to a ``struct`` :c:struct:`kmr_drm_node`
		| **on failure:** NULL

====================
kmr_drm_node_destroy
====================

.. c:function:: void kmr_drm_node_destroy(struct kmr_drm_node *node);

	Frees any allocated memory and closes FD's (if open) created after
	:c:func:`kmr_drm_node_create` call.

	Parameters:
		| **node:** Pointer to a valid ``struct`` :c:struct:`kmr_drm_node`

	.. code-block::

		/* Free'd members with fd's closed */
		struct kmr_drm_node {
			int kmsfd;
		}

=========================================================================================================================================

===============================
kmr_drm_node_device_capabilites
===============================

.. c:struct:: kmr_drm_node_device_capabilites

	.. c:member::
		bool CAP_ADDFB2_MODIFIERS;
		bool CAP_TIMESTAMP_MONOTONIC;
		bool CAP_CRTC_IN_VBLANK_EVENT;
		bool CAP_DUMB_BUFFER;

	More information can be found at `drm uapi`_

	:c:member:`CAP_ADDFB2_MODIFIERS`
		| If set to ``true``, the driver supports supplying modifiers
		| in the ``DRM_IOCTL_MODE_ADDFB2`` ioctl.

	:c:member:`CAP_TIMESTAMP_MONOTONIC`
		| If set to ``false``, the kernel will report timestamps with ``CLOCK_REALTIME``
		| in ``struct`` :c:struct:`drm_event_vblank`. If set to ``true``, the kernel will
		| report timestamps with ``CLOCK_MONOTONIC``. See `clock_gettime(2)`_
		| for the definition of these clocks.

	:c:member:`CAP_CRTC_IN_VBLANK_EVENT`
		| If set to ``true``, the kernel supports reporting the CRTC ID in
		| ``drm_event_vblank.crtc_id`` for the ``DRM_EVENT_VBLANK`` and
		| ``DRM_EVENT_FLIP_COMPLETE`` events.

	:c:member:`CAP_DUMB_BUFFER`
		| If set to ``true``, the driver supports creating dumb buffers via
		| the ``DRM_IOCTL_MODE_CREATE_DUMB`` ioctl.

====================================
kmr_drm_node_get_device_capabilities
====================================

.. c:function:: struct kmr_drm_node_device_capabilites kmr_drm_node_get_device_capabilities(int kmsfd);

	Function takes in a DRM device fd and populates the ``struct`` :c:struct:`kmr_drm_node_device_capabilites`
	to give details on what capabilites the particular kms device supports. Function is called
	by :c:func:`kmr_drm_node_create`, but is exposed to the application developer for their own use.

	Parameters:
		| **kmsfd:** Number associated with open KMS device node.

	Returns:
		| ``struct`` :c:struct:`kmr_drm_node_device_capabilites`

=========================================================================================================================================

======================================
kmr_drm_node_display_object_props_data
======================================

.. c:struct:: kmr_drm_node_display_object_props_data

	.. c:member::
		uint32_t id;
		uint64_t value;

	:c:member:`id`
		| Driver assigned ID of a given property belonging to a KMS object.

	:c:member:`value`
		| Enum value of given KMS object property. Can be used for instance to check if plane
		| object is a primary plane (``DRM_PLANE_TYPE_PRIMARY``).

=================================
kmr_drm_node_display_object_props
=================================

.. c:struct:: kmr_drm_node_display_object_props

	.. c:member::
		uint32_t                                      id;
		struct kmr_drm_node_display_object_props_data *propsData;
		uint8_t                                       propsDataCount;

	:c:member:`id`
		| Driver assigned ID of the KMS object.

	:c:member:`propsData`
		| Stores array of data about the properties of a KMS object
		| used during KMS atomic operations.

	:c:member:`propsDataCount`
		| Array size of :c:member:`propsData`.

==============================
kmr_drm_node_display_mode_data
==============================

.. c:struct:: kmr_drm_node_display_mode_data

	.. c:member::
		uint32_t        id;
		drmModeModeInfo modeInfo;

	:c:member:`id`
		| Stores the highest mode (resolution + refresh) property id.
		| When we perform an atomic commit, the driver expects a CRTC
		| property named ``"MODE_ID"``, which points to the id given to one
		| of connected display resolution & refresh rate. At the moment the
		| highest mode is choosen.

	:c:member:`modeInfo`
		| Stores the highest mode data (display resolution + refresh)
		| associated with display.

====================
kmr_drm_node_display
====================

.. c:struct:: kmr_drm_node_display

	.. c:member::
		int                                      kmsfd;
		uint16_t                                 width;
		uint16_t                                 height;
		clockid_t                                presClock;
		struct kmr_drm_node_display_mode_data    modeData;
		struct kmr_drm_node_display_object_props connector;
		struct kmr_drm_node_display_object_props crtc;
		struct kmr_drm_node_display_object_props plane;

	More information can be found at `drm-kms`_

	:c:member:`kmsfd`
		| Pollable file descriptor to an open KMS (GPU) device file.

	:c:member:`width`
		| Highest mode (display resolution) width for :c:member:`connector` attached to display.

	:c:member:`height`
		| Highest mode (display resolution) width for :c:member:`connector` attached to display.

	:c:member:`presClock`
		| Presentation clock stores the type of clock to utilize for fps tracking. Clock will
		| either be set to `CLOCK_MONOTONIC`_ or `CLOCK_REALTIME`_ depending upon the
		| System/DRM device capabilities. `CLOCK_MONOTONIC`_ will return the elapsed time
		| from system boot, can only increase, and can't be manually modified. While
		| `CLOCK_REALTIME`_ will return the real time system clock as set by the user.
		| This clock can however be modified.

	:c:member:`modeData`
		| Stores highest mode (display resolution & refresh) along with the modeid property
		| used during KMS atomic operations.

	:c:member:`connecter`
		| Anything that can transfer pixels in some form. (i.e HDMI). Connectors can
		| be hotplugged and unplugged at runtime. Stores connector properties used
		| during KMS atomic modesetting and page-flips.

	:c:member:`crtc`
		| Represents a part of the chip that contains a pointer to a scanout buffer.
		| Stores crtc properties used during KMS atomic modesetting and page-flips.

	:c:member:`plane`
		| A plane represents an image source that can be blended with or overlayed on top of
		| a CRTC during the scanout process. Planes are associated with a frame buffer to crop
		| a portion of the image memory (source) and optionally scale it to a destination size.
		| The result is then blended with or overlayed on top of a CRTC. Stores primary plane
		| properties used during KMS atomic modesetting and page-flips.

================================
kmr_drm_node_display_create_info
================================

.. c:struct:: kmr_drm_node_display_create_info

	.. c:member::
		int kmsfd;

	:c:member:`kmsfd`
		| The file descriptor associated with open KMS device node.

===========================
kmr_drm_node_display_create
===========================

.. c:function:: struct kmr_drm_node_display *kmr_drm_node_display_create(struct kmr_drm_node_display_create_info *displayInfo);

	Function takes in a pointer to a ``struct`` :c:struct:`kmr_drm_node_display_create_info`
	and produces one connector->encoder->CRTC->plane display output chain.
	Populating the members of ``struct`` :c:struct:`kmr_drm_node_display` whose
	information will be later used in modesetting.

	Parameters:
		| **displayInfo:**
		| Pointer to a ``struct`` :c:struct:`kmr_drm_node_display_create_info` used
		| to determine what operation will happen in the function

	Returns:
		| **on success:** Pointer to a ``struct`` :c:struct:`kmr_drm_node_display`
		| **on failure:** NULL 

============================
kmr_drm_node_display_destroy
============================

.. c:function:: void kmr_drm_node_display_destroy(struct kmr_drm_node_display *display);

	Frees any allocated memory and closes FD's (if open) created after
	:c:func:`kmr_drm_node_display_create` call.

	Parameters:
		| **display:** Pointer to a valid ``struct`` :c:struct:`kmr_drm_node_display`

	.. code-block::

		/* Free'd members */
		struct kmr_drm_node_display {
			struct kmr_drm_node_display_object_props connector.propsData;
			struct kmr_drm_node_display_object_props crtc.propsData;
			struct kmr_drm_node_display_object_props plane.propsData;
		}

==============================
kmr_drm_node_display_mode_info
==============================

.. c:struct:: kmr_drm_node_display_mode_info

	.. c:member::
		int                         fbid;
		struct kmr_drm_node_display *display;

	:c:member:`fbid`
		| KMS ID of framebuffer associated with gbm or dump buffer.
		| This ID is used during kms atomic modesetting.

	:c:member:`display`
		| Pointer to a plane->crtc->encoder->connector pair

=============================
kmr_drm_node_display_mode_set
=============================

.. c:function:: int kmr_drm_node_display_mode_set(struct kmr_drm_node_display_mode_info *displayModeInfo);

	Sets the display connected to ``display->connecter`` screen resolution
	and refresh to the highest possible value.

	Parameters:
		| **displayModeInfo:**
		| Pointer to a ``struct`` :c:struct:`kmr_drm_node_display_mode_info` used to
		| set highest display mode.

	Returns:
		| **on success:** 0
		| **on failure:** -1
 
===============================
kmr_drm_node_display_mode_reset
===============================

.. c:function:: int kmr_drm_node_display_mode_reset(struct kmr_drm_node_display_mode_info *displayModeInfo);

	Clears the current display mode setting

	Parameters:
		| **displayModeInfo:**
		| Pointer to a ``struct`` :c:struct:`kmr_drm_node_display_mode_info` used to
		| set highest display mode.

	Returns:
		| **on success:** 0
		| **on failure:** -1

=========================================================================================================================================

==========================
kmr_drm_node_renderer_impl
==========================

.. c:function:: void kmr_drm_node_renderer_impl(volatile bool*, uint8_t*, int*, void*);

	.. code-block::

		typedef void (*kmr_drm_node_renderer_impl)(volatile bool*, uint8_t*, int*, void*);

	Function pointer used by ``struct`` :c:struct:`kmr_drm_node_atomic_request_create_info`
	used to pass the address of an external function you want to run
	Given that the arguments of the function are:

	volatile bool *
		| A pointer to a boolean determining if the renderer is running.
		| Used to exit rendering operations.

	uint8_t *
		| A pointer to an unsigned 8 bit integer determining current buffer
		| GBM/DUMP buffer being used.

	int *
		| A pointer to an integer storing KMS framebuffer ID associated with
		| the GBM(GEM DMA Buf) or DUMP buffer. Used by the implementation
		| during atomic modesetting operations.

	void *
		| A pointer to any arbitrary data the custom renderer may want pass
		| during rendering operations.

===========================
kmr_drm_node_atomic_request
===========================

.. c:struct:: kmr_drm_node_atomic_request

	.. c:member::
		drmModeAtomicReq *atomicRequest;
		void             *rendererInfo;

	:c:member:`atomicRequest`
		| Pointer to a KMS atomic request instance

	:c:member:`rendererInfo`
		| Used by the implementation to free data. **DO NOT MODIFY.**

=======================================
kmr_drm_node_atomic_request_create_info
=======================================

.. c:struct:: kmr_drm_node_atomic_request_create_info

	.. c:member::
		int                         kmsfd;
		struct kmr_drm_node_display *display;
		kmr_drm_node_renderer_impl  renderer;
		volatile bool               *rendererRunning;
		uint8_t                     *rendererCurrentBuffer;
		int                         *rendererFbId;
		void                        *rendererData;

	:c:member:`kmsfd`
		| File descriptor to an open KMS device node.

	:c:member:`display`
		| Pointer to a struct containing all plane->crtc->connector data used during
		| KMS atomic mode setting.

	:c:member:`renderer`
		| Function pointer that allows custom external renderers to be executed by the api
		| upon :c:member:`kmsfd` polled events.

	:c:member:`rendererRunning`
		| Pointer to a boolean that determines if a given renderer is running and in need
		| of stopping.

	:c:member:`rendererCurrentBuffer`
		| Pointer to an integer used by the api to update the current displayable buffer.

	:c:member:`rendererFbId`
		| Pointer to an integer used as the value of the ``"FB_ID"`` property for a plane
		| related to the CRTC during the atomic modeset operation.

	:c:member:`rendererData`
		| Pointer to an optional address. This address may be the address of a struct.
		| Reference/Address passed depends on external renderer function.

==================================
kmr_drm_node_atomic_request_create
==================================

.. c:function:: struct kmr_drm_node_atomic_request *kmr_drm_node_atomic_request_create(struct kmr_drm_node_atomic_request_create_info *atomicInfo);

	Function creates a KMS atomic request instance. Sets the interface that allows
	callers of API to setup custom renderer implementation. Performs the initial modeset
	operation. After all the application needs to do is wait for page-flip events to happen.

	Parameters:
		| **atomicInfo:**
		| Pointer to a ``struct`` :c:struct:`kmr_drm_node_atomic_request_create_info` used to set
		| external renderer and arguments of the external renderer.

	Returns:
		| **on success:** pointer to a ``struct`` :c:struct:`kmr_drm_node_atomic_request`
		| **on failure:** NULL

===================================
kmr_drm_node_atomic_request_destroy
===================================

.. c:function:: void kmr_drm_node_atomic_request_destroy(struct kmr_drm_node_atomic_request *atomic);

	Frees any allocated memory and closes FD's (if open) created after
	:c:func:`kmr_drm_node_atomic_request_create` call.

	Parameters:
		| **atomic:** Pointer to a valid ``struct`` :c:struct:`kmr_drm_node_atomic_request`

		.. code-block::

			/* Free'd members */
			struct kmr_drm_node_atomic_request {
				drmModeAtomicReq *atomicRequest;
				void             *rendererInfo;
			}

=========================================================================================================================================

==================================
kmr_drm_node_handle_drm_event_info
==================================

.. c:struct:: kmr_drm_node_handle_drm_event_info

	.. c:member::
		int kmsfd;

	:c:member:`kmsfd`
		| File descriptor to an open KMS device node.

=============================
kmr_drm_node_handle_drm_event
=============================

.. c:function:: int kmr_drm_node_handle_drm_event (struct kmr_drm_node_handle_drm_event_info *eventInfo);

	Function calls `drmHandleEvent(3)`_ which processes outstanding DRM events
	on the DRM file-descriptor. This function should be called after the DRM
	file-descriptor has polled readable.

	Parameters:
		| **eventInfo:** Pointer to a ``struct`` :c:struct:`kmr_drm_node_handle_drm_event_info`

	Returns:
		| **on success:** 0
		| **om failure:** -1

=========================================================================================================================================

.. _DRM-Master and DRM-Auth: https://en.wikipedia.org/wiki/Direct_Rendering_Manager#DRM-Master_and_DRM-Auth
.. _drm uapi: https://github.com/torvalds/linux/blob/master/include/uapi/drm/drm.h#L627
.. _drm-kms: https://manpages.org/drm-kms/7
.. _clock_gettime(2): https://linux.die.net/man/2/clock_gettime
.. _CLOCK_MONOTONIC: https://linux.die.net/man/2/clock_gettime
.. _CLOCK_REALTIME: https://linux.die.net/man/2/clock_gettime
.. _drmHandleEvent(3): https://man.archlinux.org/man/extra/libdrm/drmHandleEvent.3.en
