.. default-domain:: C

xclient
=======

Header: kmsroots/xclient.h

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

1. :c:struct:`kmr_xcb_window`
#. :c:struct:`kmr_xcb_window_create_info`
#. :c:struct:`kmr_xcb_window_handle_event_info`

=========
Functions
=========

1. :c:func:`kmr_xcb_window_create`
#. :c:func:`kmr_xcb_window_destroy`
#. :c:func:`kmr_xcb_window_handle_event`

=================
Function Pointers
=================

1. :c:func:`kmr_xcb_renderer_impl`

API Documentation
~~~~~~~~~~~~~~~~~

==============
kmr_xcb_window
==============

.. c:struct:: kmr_xcb_window

	.. c:member::
		xcb_connection_t        *conn;
		xcb_window_t            window;
		xcb_intern_atom_reply_t *delWindow;

	:c:member:`conn`
		| A structure that contain all data that XCB needs to communicate
		| with an X server.

	:c:member:`window`
		| Stores the XID of the current window. XID is neeed to create
		| windows and manage its properties

	:c:member:`delWindow`
		| Used to by :c:func:`kmr_xcb_window_wait_for_event` to verify when
		| the window manager attempts to destroy the window.

==========================
kmr_xcb_window_create_info
==========================

.. c:struct:: kmr_xcb_window_create_info

	.. c:member::
		const char *display;
		int        *screen;
		const char *appName;
		uint16_t   width;
		uint16_t   height;
		bool       fullscreen;
		bool       transparent;

	:c:member:`display`
		| The X server's display name. When set to NULL, the DISPLAY
		| environment variable is used.

	:c:member:`screen`
		| Number for the screen that should be connected. When set to NULL,
		| the screen number is set to 0.

	:c:member:`appName`
		| Sets the window name. Can not be more than 60 characters.

	:c:member:`width`
		| Width of window in pixels

	:c:member:`height`
		| Height of window in pixels

	:c:member:`fullscreen`
		| Set to true to go fullscreen, false to display normal window.

	:c:member:`transparent`
		| Set to true to have fully transparent window. False will display black background.

=====================
kmr_xcb_window_create
=====================

.. c:function:: struct kmr_xcb_window *kmr_xcb_window_create(struct kmr_xcb_window_create_info *xcbWindowInfo);

	Create an xcb client window instance (can be fullscreen).

	Parameters:
		| **xcbWindowInfo**
		| Pointer to a ``struct`` :c:struct:`kmr_xcb_window_create_info` contains all information
		| required to created an xcb client and some added window configuration options.

	Returns:
		| **on success:** pointer to a ``struct`` :c:struct:`kmr_xcb_window`
		| **on failure:** NULL

======================
kmr_xcb_window_destroy
======================

.. c:function:: void kmr_xcb_window_destroy(struct kmr_xcb_window *xcb);

	Frees any allocated memory and closes FD’s (if open) created after
	:c:func:`kmr_xcb_window_create` call.

	Parameters:
		| **xcb**
		| Must past a valid pointer to a ``struct`` :c:struct:`kmr_xcb_window`

		.. code-block::

			/* Free'd and file descriptors closed members */
			struct kmr_xcb_window {
				xcb_connection_t        *conn;
				xcb_window_t            window;
				xcb_intern_atom_reply_t *delWindow;
			};

=========================================================================================================================================

===========================
kmr_xcb_window_make_visible
===========================

.. c:function:: void kmr_xcb_window_make_visible(struct kmr_xcb_window *xcb);

	Creates the window that we display on by informing server to map the window to the screen.
	NOTE: If window is created before vulkan can establish `VkSurfaceKHR`_/`VkFramebuffer`_
	objects it leads to window being in a deadlock state. With no way of recovery without
	a power recycle.

	Validation layers report

	`vkCreateFramebuffer()`_: `VkFramebufferCreateInfo`_ attachment #0 mip level 0 has width (1848)
	smaller than the corresponding framebuffer width (1920).

	Parameters:
		| **xcb**
		| Pointer to a ``struct`` :c:struct:`kmr_xcb_window` contains all objects necessary
		| for an xcb client window to display.

=========================================================================================================================================

=====================
kmr_xcb_renderer_impl
=====================

.. c:function:: void kmr_xcb_renderer_impl(volatile bool*, uint8_t*, int*, void*);

	.. code-block::

		typedef void (*kmr_xcb_renderer_impl)(volatile bool*, uint8_t*, void*);

	Function pointer used by ``struct`` :c:struct:`kmr_xcb_window_wait_for_event_info`
	Allows to pass the address of an external function you want to run
	Given that the arguments of the function are a pointer to a boolean,
	pointer to an integer, and a pointer to void data type

	volatile bool *
		| A pointer to a boolean determining if the renderer is running.
		| Used to exit rendering operations.

	uint8_t *
		| A pointer to an unsigned 8 bit integer determining current vulkan
		| swapchain image being used.

	void *
		| A pointer to any arbitrary data the custom renderer may want pass
		| during rendering operations.

================================
kmr_xcb_window_handle_event_info
================================

.. c:struct:: kmr_xcb_window_handle_event_info

	.. c:member::
		struct kmr_xcb_window *xcbWindowObject;
		kmr_xcb_renderer_impl renderer;
		void                  *rendererData;
		uint8_t               *rendererCurrentBuffer;
		volatile bool         *rendererRunning;

	:c:member:`xcbWindowObject`
		| Pointer to a ``struct`` :c:struct:`kmr_xcb_window` contains all objects necessary
		| to manage xcb client.

	:c:member:`renderer`
		| Function pointer that allows custom external renderers to be
		| executed by the api.

	:c:member:`rendererData`
		| Pointer to an optional address that will be passed though.
		| This address may be the address of a struct. Reference passed
		| depends on external render function.

	:c:member:`rendererCurrentBuffer`
		| Pointer to an integer used by the api to update the current displayable buffer

	:c:member:`rendererRunning`
		| Pointer to a boolean that determines if a given window/surface is actively running

===========================
kmr_xcb_window_handle_event
===========================

.. c:function:: int kmr_xcb_window_handle_event(struct kmr_xcb_window_handle_event_info *xcbEventInfo);

	In an X program, everything is driven by events. This functions calls
	`xcb_poll_for_event`_ which doesn't block operations and returns events from
	X server when available. The main event watched by function is KEY_PRESSING,
	when either the **'Q'** or **'ESC'** keys are pressed the function will return
	failure status.

	Function is meant to be utilized as a while loop conditional/expression.

	See: https://xcb.freedesktop.org/tutorial/events

	Parameters:
		| **xcbEventInfo**
		| Pointer to a ``struct`` :c:struct:`kmr_xcb_window_wait_for_event_info` contains all
		| objects necessary for an xcb client to run and pointer to custom renderer
		| to execute and the arguments used by said renderer.

	Returns:
		| **on success:** 1
		| **on failure:** 0

=========================================================================================================================================

.. _VkSurfaceKHR: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkSurfaceKHR.html
.. _VkFramebuffer: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFramebuffer.html
.. _VkFramebufferCreateInfo: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkFramebufferCreateInfo.html
.. _vkCreateFramebuffer(): https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCreateFramebuffer.html
.. _xcb_poll_for_event: https://xcb.freedesktop.org/manual/group__XCB__Core__API.html#ga3289f98c49afa3aa56f84f741557a434
