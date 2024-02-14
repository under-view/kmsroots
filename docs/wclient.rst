.. default-domain:: C

wclient
=======

Header: kmsroots/wclient.h

Table of contents (click to go)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

======
Macros
======

=====
Enums
=====

1. :c:enum:`kmr_wc_interface_type`

======
Unions
======

=======
Structs
=======

1. :c:struct:`kmr_wc_core`
#. :c:struct:`kmr_wc_core_create_info`
#. :c:struct:`kmr_wc_shm_buffer`
#. :c:struct:`kmr_wc_buffer_handle`
#. :c:struct:`kmr_wc_buffer`
#. :c:struct:`kmr_wc_buffer_create_info`
#. :c:struct:`kmr_wc_surface`
#. :c:struct:`kmr_wc_surface_create_info`

=========
Functions
=========

1. :c:func:`kmr_wc_core_create`
#. :c:func:`kmr_wc_core_destroy`
#. :c:func:`kmr_wc_buffer_create`
#. :c:func:`kmr_wc_buffer_destroy`
#. :c:func:`kmr_wc_surface_create`
#. :c:func:`kmr_wc_surface_destroy`

=================
Function Pointers
=================

1. :c:func:`kmr_wc_renderer_impl`

API Documentation
~~~~~~~~~~~~~~~~~

Well format documentation

https://wayland.app/protocols/

=====================
kmr_wc_interface_type
=====================

.. c:enum:: kmr_wc_interface_type

	.. c:macro::
		KMR_WC_INTERFACE_NULL
		KMR_WC_INTERFACE_WL_COMPOSITOR
		KMR_WC_INTERFACE_XDG_WM_BASE
		KMR_WC_INTERFACE_WL_SHM
		KMR_WC_INTERFACE_WL_SEAT
		KMR_WC_INTERFACE_ZWP_FULLSCREEN_SHELL_V1
		KMR_WC_INTERFACE_ALL

	Values may be or'd together to select
	wayland interface to bind to.

	:c:macro:`KMR_WC_INTERFACE_NULL`
		| Bind no supported wayland interfaces to a client

	:c:macro:`KMR_WC_INTERFACE_WL_COMPOSITOR`
		| Bind client to `wl_compositor`_ interface

	:c:macro:`KMR_WC_INTERFACE_XDG_WM_BASE`
		| Bind client to `xdg_wm_base`_ interface

	:c:macro:`KMR_WC_INTERFACE_WL_SHM`
		| Bind client to `wl_shm`_ interface

	:c:macro:`KMR_WC_INTERFACE_WL_SEAT`
		| Bind client to `wl_seat`_ interface

	:c:macro:`KMR_WC_INTERFACE_ZWP_FULLSCREEN_SHELL_V1`
		| Bind client to `zwp_fullscreen_shell_v1`_ interface

	:c:macro:`KMR_WC_INTERFACE_ALL`
		| Bind to all supported global interfaces to a client

=========================================================================================================================================

===========
kmr_wc_core
===========

.. c:struct:: kmr_wc_core

	.. c:member::
		kmr_wc_interface_type          iType;
		struct wl_display              *wlDisplay;
		struct wl_registry             *wlRegistry;
		struct wl_compositor           *wlCompositor;
		struct xdg_wm_base             *xdgWmBase;
		struct wl_shm                  *wlShm;
		struct wl_seat                 *wlSeat;
		struct zwp_fullscreen_shell_v1 *fullScreenShell;

	:c:member:`iType`
		| Wayland objects/interfaces to bind to client. These objects/interfaces
		| defines what requests are possible by a given wayland client.

	:c:member:`wlDisplay`
		| A global object representing the whole connection to wayland server.
		| This is object ID 1 which is pre-allocated and can be utilzed to
		| bootstrap all other objects.

	:c:member:`wlRegistry`
		| A global object used to recieve events from the server. Events may include
		| monitor hotplugging for instance.

	:c:member:`wlCompositor`
		| A singleton global object representing the compositor itself largely utilized to
		| allocate surfaces & regions to put pixels into. The compositor itself will combine
		| the contents of multiple surfaces and put them into one displayable output.

	:c:member:`xdgWmBase`
		| A singleton global object that enables clients to turn their surfaces
		| (rectangle box of pixels) into windows in a desktop environment.

	:c:member:`wlShm`
		| A singleton global object that provides support for shared memory.
		| Used to create a simple way of getting pixels from client to compositor.
		| Pixels are stored in an unsigned 8 bit integer the buffer is created
		| with `mmap(2)`_.

	:c:member:`wlSeat`
		| A singleton global object used to represent a group of
		| hot-pluggable input devices.

	:c:member:`fullScreenShell`
		| A singleton global object that has an interface that allow for
		| displaying of fullscreen surfaces.

=======================
kmr_wc_core_create_info
=======================

.. c:struct:: kmr_wc_core_create_info

	.. c:member::
		kmr_wc_interface_type iType;
		const char            *displayName;

	:c:member:`iType`
		| Wayland global objects to include or exclude when the registry emits events

	:c:member:`displayName`
		| Specify the wayland server unix domain socket a client should communicate with.
		| This will set the ``$WAYLAND_DISPLAY`` variable to the desired display.
		| Passing ``NULL`` will result in opening unix domain socket ``/run/user/1000/wayland-0``.

==================
kmr_wc_core_create
==================

.. c:function:: struct kmr_wc_core *kmr_wc_core_create(struct kmr_wc_core_create_info *coreInfo);

	Establishes connection to the wayland display server and sets up client
	specified global objects that each define what requests and events are possible.

	Parameters:
		| **coreInfo**
		| Pointer to a ``struct`` :c:struct:`kmr_wc_core_create_info` which contains
		| the name of the server to connected to and the client specified
		| globals to bind to a given wayland client. Only if the server
		| supports these globals.

	Returns:
		| **on success:** pointer to a ``struct`` :c:struct:`kmr_wc_core`
		| **on failure:** NULL

===================
kmr_wc_core_destroy
===================

.. c:function:: void kmr_wc_core_destroy(struct kmr_wc_core *core);

	Frees any allocated memory and closes FD’s (if open) created after
	:c:func:`kmr_wc_core_create` call.

	.. code-block::

		/* Free'd members with fd's closed */
		struct kmr_wc_core {
			struct wl_display              *wlDisplay;
			struct wl_registry             *wlRegistry;
			struct wl_compositor           *wlCompositor;
			struct xdg_wm_base             *xdgWmBase;
			struct wl_shm                  *wlShm;
			struct wl_seat                 *wlSeat;
			struct zwp_fullscreen_shell_v1 *fullScreenShell;
		}

=========================================================================================================================================

=================
kmr_wc_shm_buffer
=================

.. c:struct:: kmr_wc_shm_buffer

	.. c:member::
		int     shmFd;
		size_t  shmPoolSize;
		uint8_t *shmPoolData;

	:c:member:`shmFd`
		| A file descriptor to an open wayland shared memory object.

	:c:member:`shmPoolSize`
		| The size of the amount of bytes in a given buffer pixels.
		| [Value = width * height * bytesPerPixel]

	:c:member:`shmPoolData`
		| Actual linear buffer to put pixel data into inorder to display.
		| The buffer itself created via `mmap(2)`_.

====================
kmr_wc_buffer_handle
====================

.. c:struct:: kmr_wc_buffer_handle

	.. c:member::
		struct wl_buffer *buffer;

	:c:member:`buffer`
		| Buffer understood by the compositor attached to CPU
		| visible shm buffer.

=============
kmr_wc_buffer
=============

.. c:struct:: kmr_wc_buffer

	.. c:member::
		int                         bufferCount;
		struct kmr_wc_shm_buffer    *shmBufferObjects;
		struct kmr_wc_buffer_handle *wlBufferHandles;

	:c:member:`bufferCount`
		| The amount of wayland buffers allocated from a given `wl_shm_pool`_.

	:c:member:`shmBufferObjects`
		| Pointer to an array of ``struct`` :c:struct:`kmr_wc_shm_buffer` containing all
		| information required to populate/release wayland shared memory
		| pixel buffer.

	:c:member:`wlBufferHandles`
		| Pointer to an array of ``struct`` :c:struct:`kmr_wc_buffer_handle` containing
		| compositor assigned buffer object.

=========================
kmr_wc_buffer_create_info
=========================

.. c:struct:: kmr_wc_buffer_create_info

	.. c:member::
		struct kmr_wc_core *core;
		int                bufferCount;
		int                width;
		int                height;
		int                bytesPerPixel;
		uint64_t           pixelFormat;

	:c:member:`core`
		| Must pass a valid pointer to all binded/exposed wayland core interfaces
		| (important interfaces used: `wl_shm`_)

	:c:member:`bufferCount`
		| The amount of buffers to allocate when storing pixel data
		| ``2`` is generally a good option as it allows for double buffering

	:c:member:`width`
		| Amount of pixel horizontally (i.e ``3840``, ``1920``, ...)

	:c:member:`height`
		| Amount of pixel vertically (i.e ``2160``, ``1080``, ...)

	:c:member:`bytesPerPixel`
		| The amount of bytes per pixel generally going to be ``4`` bytes (``32 bits``)

	:c:member:`pixelFormat`
		| Memory layout of an individual pixel

====================
kmr_wc_buffer_create
====================

.. c:function:: struct kmr_wc_buffer *kmr_wc_buffer_create(struct kmr_wc_buffer_create_info *bufferInfo);

	Adds way to get pixels from client to compositor by creating
	writable shared memory buffers.

	Parameters:
		| **bufferInfo**
		| Must pass a valid pointer to a ``struct`` :c:struct:`kmr_wc_buffer_create_info`
		| which gives details of how a buffer should be allocated and how
		| many to allocations to make.

	Returns:
		| **on success:** pointer to a ``struct`` :c:struct:`kmr_wc_buffer`
		| **on failure:** NULL

=====================
kmr_wc_buffer_destroy
=====================

.. c:function:: void kmr_wc_buffer_destroy(struct kmr_wc_buffer *buffer);

	Frees any allocated memory and closes FD’s (if open) created after
	:c:func:`kmr_wc_buffer_create` call.

	Parameters:
		| **buffer**
		| Must pass a valid pointer to a ``struct`` :c:struct:`kmr_wc_buffer`.

	.. code-block::

		/* Free'd members with fd's closed */
		struct kmr_wc_buffer {
			struct kmr_wc_shm_buffer {
				int     shmFd;
				uint8_t *shmPoolData;
			} *shmBufferObjects;

			struct kmr_wc_buffer_handle {
				struct wl_buffer *buffer;
			} *wlBufferHandles;
		}

=========================================================================================================================================

====================
kmr_wc_renderer_impl
====================

.. c:function:: void kmr_wc_renderer_impl(volatile bool*, uint8_t*, int*, void*);

	.. code-block::

		typedef void (*kmr_wc_renderer_impl)(volatile bool*, uint8_t*, void*);

	Function pointer used by ``struct`` :c:struct:`kmr_wc_surface_create_info`
	Allows to pass the address of an external function you want to run
	Given that the arguments of the function are a pointer to a boolean,
	pointer to an integer, and a pointer to void data type

	volatile bool *
		| A pointer to a boolean determining if the renderer is running.
		| Used to exit rendering operations.

	uint8_t *
		| A pointer to an unsigned 8 bit integer determining current shm
		| buffer being rendered to used.

	void *
		| A pointer to any arbitrary data the custom renderer may want pass
		| during rendering operations.

==============
kmr_wc_surface
==============

.. c:struct:: kmr_wc_surface

	.. c:member::
		struct xdg_toplevel         *xdgToplevel;
		struct xdg_surface          *xdgSurface;
		struct wl_surface           *wlSurface;
		struct wl_callback          *wlCallback;
		uint8_t                     bufferCount;
		struct kmr_wc_buffer_handle *wlBufferHandles;
		void                        *rendererInfo;

	:c:member:`xdgToplevel`
		| An interface with many requests and events to manage application windows

	:c:member:`xdgSurface`
		| Top level surface for the window. Useful if one wants to create
		| a tree of surfaces. Provides additional requests for assigning
		| roles.

	:c:member:`wlSurface`
		| The image displayed on screen. If a `wl_shm`_ interface binded to client user
		| must attach a `wl_buffer`_ (``struct`` :c:struct:`kmr_wc_buffer_handle`) to display pixels on
		| screen. Depending on rendering backend `wl_buffer`_ may not need to be allocated.

	:c:member:`wlCallback`
		| The amount of pixel (``uint8_t``) buffers allocated.
		| The array length of ``struct`` :c:struct:`kmr_wc_buffer_handle`.

	:c:member:`bufferCount`
		| Amount of elements in pointer to array of :c:member:`wlBufferHandles`

	:c:member:`wlBufferHandles`
		| A pointer to an array of ``struct`` :c:struct:`kmr_wc_buffer_handle`.
		| Which holds a ``struct`` `wl_buffer`_ the pixel storage place
		| understood by compositor.

	:c:member:`rendererInfo`
		| Used by the implementation to free data. **DO NOT MODIFY**.

==========================
kmr_wc_surface_create_info
==========================

.. c:struct:: kmr_wc_surface_create_info

	.. c:member::
		struct kmr_wc_core   *core;
		struct kmr_wc_buffer *buffer;
		uint8_t              bufferCount;
		const char           *appName;
		bool                 fullscreen;
		kmr_wc_renderer_impl renderer;
		void                 *rendererData;
		uint8_t              *rendererCurrentBuffer;
		volatile bool        *rendererRunning;

	:c:member:`core`
		| Pointer to a ``struct`` :c:struct:`kmr_wc_core` contains all objects/interfaces
		| necessary for a client to run.

	:c:member:`buffer`
		| Must pass a valid pointer to a ``struct`` :c:struct:`kmr_wc_buffer` need to attach
		| a `wl_buffer`_ object to a `wl_surface`_ object. If ``NULL`` passed no buffer
		| will be attached to surface. Thus nothing will be displayed. Passing
		| this variable also enables in API swapping between shm buffers.
		| Swapping goes up to ``struct`` :c:struct:`kmr_wc_buffer` { ``bufferCount`` }.

	:c:member:`appName`
		| Sets the window name.

	:c:member:`fullscreen`
		| Determines if window should be fullscreen or not

	:c:member:`renderer`
		| Function pointer that allows custom external renderers to be executed by the api
		| when before registering a frame `wl_callback`_. Renderer before presenting

	:c:member:`rendererData`
		| Pointer to an optional address. This address may be the address of a struct.
		| Reference passed depends on external renderer function.

	:c:member:`rendererCurrentBuffer`
		| Pointer to an integer used by the api to update the current displayable buffer.

	:c:member:`rendererRunning`
		| Pointer to a boolean that determines if a given window/surface is actively running.

=====================
kmr_wc_surface_create
=====================

.. c:function:: struct kmr_wc_surface *kmr_wc_surface_create(struct kmr_wc_surface_create_info *surfaceInfo);

	Creates a surface to place pixels in and a window for displaying surface pixels.

	Parameters:
		| **surfaceInfo**
		| Must pass a valid pointer to a ``struct`` :c:struct:`kmr_wc_surface_create_info`
		| which gives details on what buffers are associated with surface
		| object, the name of the window, and how the window should be displayed.

	Returns:
		| **on success:** pointer to a ``struct`` :c:struct:`kmr_wc_surface`
		| **on failure:** NULL

======================
kmr_wc_surface_destroy
======================

.. c:function:: void kmr_wc_surface_destroy(struct kmr_wc_surface *surface);

	Frees any allocated memory and closes FD’s (if open) created after
	:c:func:`kmr_wc_surface_create` call.

	Parameters:
		| **surface**
		| Must pass a valid pointer to a ``struct`` :c:struct:`kmr_wc_surface`.

	.. code-block::

		/* Free'd members with fd's closed */
		struct kmr_wc_surface {
			struct xdg_toplevel *xdgToplevel;
			struct xdg_surface  *xdgSurface;
			struct wl_surface   *wlSurface;
			void                *rendererInfo;
		}

=========================================================================================================================================

.. _mmap(2): https://www.man7.org/linux/man-pages/man2/mmap.2.html
.. _wl_compositor: https://wayland.app/protocols/wayland#wl_compositor
.. _xdg_wm_base: https://wayland.app/protocols/xdg-shell#xdg_wm_base
.. _wl_seat: https://wayland.app/protocols/wayland#wl_seat
.. _wl_buffer: https://wayland.app/protocols/wayland#wl_buffer
.. _wl_surface: https://wayland.app/protocols/wayland#wl_surface
.. _wl_callback: https://wayland.app/protocols/wayland#wl_callback
.. _wl_shm: https://wayland.app/protocols/wayland#wl_shm
.. _wl_shm_pool: https://wayland.app/protocols/wayland#wl_shm_pool
.. _zwp_fullscreen_shell_v1: https://wayland.app/protocols/fullscreen-shell-unstable-v1
