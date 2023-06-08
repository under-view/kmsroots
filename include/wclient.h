#ifndef KMR_WAYLAND_CLIENT_H
#define KMR_WAYLAND_CLIENT_H

#include "utils.h"

#include <stdbool.h>
#include <wayland-client.h>


/*
 * enum kmr_wc_interface_type (kmsroots Wayland Client Interface Type)
 *
 * Used to select what core wayland interfaces to bind with a given client
 */
typedef enum _kmr_wc_interface_type {
	KMR_WC_INTERFACE_NULL                    = 0x00000000,
	KMR_WC_INTERFACE_WL_COMPOSITOR           = (1 << 0),
	KMR_WC_INTERFACE_XDG_WM_BASE             = (1 << 1),
	KMR_WC_INTERFACE_WL_SHM                  = (1 << 2),
	KMR_WC_INTERFACE_WL_SEAT                 = (1 << 3),
	KMR_WC_INTERFACE_ZWP_FULLSCREEN_SHELL_V1 = (1 << 4),
	KMR_WC_INTERFACE_ALL                     = 0XFFFFFFFF,
} kmr_wc_interface_type;


/*
 * struct kmr_wc_core_interface (kmsroots Wayland Client Core Interface)
 *
 * members:
 * @iType              - Wayland object to bind to or not. These objects give access to interfaces which defines what requests
 *                       are possible by a given wayland client.
 * @wlDisplay          - A global object representing the whole connection to wayland server. This is object ID 1
 *                       which is pre-allocated and can be utilzed to bootstrap all other objects.
 * @wlRegistry         - A global object used to recieve events from the server. Events may include
 *                       monitor hotplugging for instance.
 * @wlCompositor       - A singleton global object representing the compositor itself largely utilized to
 *                       allocate surfaces & regions to put pixels into. The compositor itself will combine
 *                       the contents of multiple surfaces and put them into one displayable output.
 * @xdgWmBase          - A singleton global object that enables clients to turn their surfaces (rectangle box of pixels)
 *                       into windows in a desktop environment.
 * @wlShm              - A singleton global object that provides support for shared memory. Used to create
 *                       a simple way of getting pixels from client to compositor. Pixels are stored in an
 *                       unsigned 8 bit integer the buffer is created with mmap(2).
 * @wlSeat             - A singleton global object used to represent a group of hot-pluggable input devices
 * @fullScreenShell    - A singleton global object that has an interface that allow for displaying of fullscreen surfaces.
 */
struct kmr_wc_core_interface {
	kmr_wc_interface_type          iType;
	struct wl_display              *wlDisplay;
	struct wl_registry             *wlRegistry;
	struct wl_compositor           *wlCompositor;
	struct xdg_wm_base             *xdgWmBase;
	struct wl_shm                  *wlShm;
	struct wl_seat                 *wlSeat;
	struct zwp_fullscreen_shell_v1 *fullScreenShell;
};


/*
 * struct kmr_wc_core_interface_create_info (kmsroots Wayland Client Core Interface Create Information)
 *
 * args:
 * @iType       - Wayland global objects to include or exclude when the registry emits events
 * @displayName - Specify the wayland server unix domain socket a client should communicate with. This
 *                will set the $WAYLAND_DISPLAY variable to the desired display. Passing NULL will
 *                result in opening unix domain socket /run/user/1000/wayland-0
 */
struct kmr_wc_core_interface_create_info {
	kmr_wc_interface_type iType;
	const char            *displayName;
};


/*
 * kmr_wc_core_interface_create: Establishes connection to the wayland display server and sets up client specified
 *                               global objects that define what requests and events are possible.
 *
 * args:
 * @kmrwc - Pointer to a struct kmr_wc_core_interface_create_info which contains the name of the server
 *          to connected to and the core client specified globals to bind to a given client. Only if the
 *          server supports these globals.
 *
 * return:
 *	on success struct kmr_wc_core_interface
 *	on failure struct kmr_wc_core_interface { with members nulled }
 */
struct kmr_wc_core_interface kmr_wc_core_interface_create(struct kmr_wc_core_interface_create_info *kmrwc);


/*
 * struct kmr_wc_shm_buffer (kmsroots Wayland Client Shared Memory Buffer)
 *
 * members:
 * @shmFd       - A file descriptor to an open wayland shared memory object
 * @shmPoolSize - The size of the amount of bytes in a given buffer pixels.
 *                [Value = width * height * bytes_per_pixel]
 * @shmPoolData - Actual linear buffer to put pixel data into inorder to display.
 *                The buffer itself created via mmap
 */
struct kmr_wc_shm_buffer {
	int     shmFd;
	size_t  shmPoolSize;
	uint8_t *shmPoolData;
};


/*
 * struct kmr_wc_wl_buffer_handle (kmsroots Wayland Client Wayland Buffer)
 *
 * members:
 * @buffer - Buffer understood by the compositor attached to CPU visible shm bufferp
 */
struct kmr_wc_wl_buffer_handle {
	struct wl_buffer *buffer;
};


/*
 * struct kmr_wc_buffer (kmsroots Wayland Client Buffer)
 *
 * members:
 * @bufferCount      - The amount of wayland buffers allocated from a given wl_shm_pool
 * @shmBufferObjects - Pointer to an array of struct kmrwcshmbuf containing all information required to
 *                     populate/release wayland shared memory pixel buffer.
 * @wlBufferHandles  - Pointer to an array of struct kmrwcwlbuf containing compositor assigned buffer object
 */
struct kmr_wc_buffer {
	int bufferCount;
	struct kmr_wc_shm_buffer *shmBufferObjects;
	struct kmr_wc_wl_buffer_handle *wlBufferHandles;
};


/*
 * struct kmr_wc_buffer_create_info (kmsroots Wayland Client Buffer Create Information)
 *
 * members:
 * @coreInterface - Must pass a valid pointer to all binded/exposed wayland core interfaces
 *                  (important interfaces used: wl_shm)
 * @bufferCount   - The amount of buffers to allocate when storing pixel data
 *                  2 is generally a good option as it allows for double buffering
 * @width         - Amount of pixel horizontally (i.e 3840, 1920, ...)
 * @height        - Amount of pixel vertically (i.e 2160, 1080, ...)
 * @bytesPerPixel - The amount of bytes per pixel generally going to be 4 bytes (32 bits)
 * @pixelFormat   - Memory layout of an individual pixel
 */
struct kmr_wc_buffer_create_info {
	struct kmr_wc_core_interface *coreInterface;
	int                          bufferCount;
	int                          width;
	int                          height;
	int                          bytesPerPixel;
	uint64_t                     pixelFormat;
};


/*
 * kmr_wc_buffer_create: Adds way to get pixels from client to compositor by creating writable shared memory buffers.
 *
 * args:
 * @kmrwc - Must pass a valid pointer to a struct kmr_wc_buffer_create_info which gives
 *          details of how a buffer should be allocated and how many to allocations to make.
 *
 * return:
 *	on success struct kmr_wc_buffer
 *	on failure struct kmr_wc_buffer { with members nulled, integers set to -1 }
 */
struct kmr_wc_buffer kmr_wc_buffer_create(struct kmr_wc_buffer_create_info *kmrwc);


/*
 * kmsroots Implementation
 * Function pointer used by struct kmr_wc_surface_create_info*
 * Allows to pass the address of an external function you want to run
 * Given that the arguments of the function are a pointer to a boolean,
 * pointer to an integer, and a pointer to void data type
 */
typedef void (*kmr_wc_renderer_impl)(volatile bool*, uint32_t*, void*);


/*
 * struct kmr_wc_surface (kmsroots Wayland Client Surface)
 *
 * members:
 * @xdgToplevel     - An interface with many requests and events to manage application windows
 * @xdgSurface      - Top level surface for the window. Useful if one wants to create
 *                    a tree of surfaces. Provides additional requests for assigning
 *                    roles.
 * @wlSurface       - The image displayed on screen. If wl_shm is binded one must attach
 *                    a wl_buffer (struct kmrwcwlbuf) to display pixels on screen. Depending on
 *                    rendering backend wl_buffer may not need to be allocated.
 * @wlCallback      - The amount of pixel (uint8_t) buffers allocated. The array length of struct kmrwcwlbuf.
 * @bufferCount     - Amount of elements in pointer to array of @wlBufferHandles
 * @wlBufferHandles - A pointer to an array of type struct wl_buffer *. Pixel storage place understood by compositor.
 */
struct kmr_wc_surface {
	struct xdg_toplevel            *xdgToplevel;
	struct xdg_surface             *xdgSurface;
	struct wl_surface              *wlSurface;
	struct wl_callback             *wlCallback;
	uint32_t                       bufferCount;
	struct kmr_wc_wl_buffer_handle *wlBufferHandles;
};


/*
 * struct kmr_wc_surface_create_info (kmsroots Wayland Client Surface Create Information)
 *
 * members:
 * @coreInterface         - Pointer to a struct kmr_wc_core_interface contains all objects/interfaces
 *                          necessary for a client to run.
 * @wcBufferObject        - Must pass a valid pointer to a struct kmr_wc_buffer need to attach a wl_buffer object
 *                          to a wl_surface object. If NULL passed no buffer will be attached to surface. Thus nothing
 *                          will be displayed. Passing this variable also enables in API swapping between shm buffers.
 *                          Swapping goes up to @bufferCount.
 * @bufferCount           - Unrelated to @wcBufferObject. If value assigned not zero and @wcBufferObject equals NULL. Then
 *                          this variable will be utilzed to swap between buffers.
 * @appName               - Sets the window name.
 * @fullscreen            - Determines if window should be fullscreen or not
 * @renderer              - Function pointer that allows custom external renderers to be executed by the api
 *                          when before registering a frame wl_callback. Renderer before presenting
 * @rendererData          - Pointer to an optional address. This address may be the address of a struct. Reference
 *                          passed depends on external renderer function.
 * @rendererCurrentBuffer - Pointer to an integer used by the api to update the current displayable buffer
 * @rendererRunning       - Pointer to a boolean that determines if a given window/surface is actively running
 */
struct kmr_wc_surface_create_info {
	struct kmr_wc_core_interface *coreInterface;
	struct kmr_wc_buffer         *wcBufferObject;
	uint32_t                     bufferCount;
	const char                   *appName;
	bool                         fullscreen;
	kmr_wc_renderer_impl         renderer;
	void                         *rendererData;
	uint32_t                     *rendererCurrentBuffer;
	volatile bool                *rendererRunning;
};


/*
 * kmr_wc_surface_create: Creates a surface to place pixels in and a window for displaying surface pixels.
 *
 * args:
 * @kmrwc - Must pass a valid pointer to a struct kmr_wc_surface_create_info which gives
 *          details on what buffers are associated with surface object, the name of the
 *          window, how the window should be displayed, (more info TBA).
 *
 * return:
 *	on success struct kmr_wc_surface
 *	on failure struct kmr_wc_surface { with members nulled }
 */
struct kmr_wc_surface kmr_wc_surface_create(struct kmr_wc_surface_create_info *kmrwc);


/*
 * struct kmr_wc_destroy (kmsroots Wayland Client Destroy)
 *
 * members:
 * @kmr_wc_core_interface - Must pass a valid struct kmr_wc_core_interface, to release all binded members
 * @kmr_wc_buffer         - Must pass a valid struct kmr_wc_buffer, to free all allocated memory
 * @kmr_wc_surface        - Must pass a valid struct kmr_wc_surface, to free all allocated memory
 */
struct kmr_wc_destroy {
	struct kmr_wc_core_interface kmr_wc_core_interface;
	struct kmr_wc_buffer         kmr_wc_buffer;
	struct kmr_wc_surface        kmr_wc_surface;
};


/*
 * kmr_wc_destroy: frees any remaining allocated memory contained in struct kmr_wc_destroy
 *
 * args:
 * @kmrwc - pointer to a struct kmr_wc contains all objects/interfaces
 *          necessary for an xcb client to run.
 */
void kmr_wc_destroy(struct kmr_wc_destroy *kmrwc);

#endif
