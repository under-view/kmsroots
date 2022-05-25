#ifndef UVR_WAYLAND_CLIENT_H
#define UVR_WAYLAND_CLIENT_H

#include "common.h"
#include "utils.h"
#include <wayland-client.h>


/*
 * enum uvr_wc_interface_type (Underview Renderer Wayland Client Interface Type)
 *
 * Used to select what core wayland interfaces to bind with a given client
 */
typedef enum _uvr_wc_interface_type {
  UVR_WC_NULL_INTERFACE = 0x00000000,
  UVR_WC_WL_COMPOSITOR_INTERFACE = 0x00000001,
  UVR_WC_XDG_WM_BASE_INTERFACE = 0x00000002,
  UVR_WC_WL_SHM_INTERFACE = 0x00000010,
  UVR_WC_WL_SEAT_INTERFACE = 0x00000020,
  UVR_WC_ALL_INTERFACES = 0XFFFFFFFF,
} uvr_wc_interface_type;


/*
 * struct uvr_wc_core_interface (Underview Renderer Wayland Client Core Interface)
 *
 * members:
 * @iType      - Wayland interface to include or exclude
 * @display    - A global singleton representing the whole connection to wayland server.
 * @registry   - A global singleton used to query information about server
 * @compositor - A global singleton representing the compositor itself
 *               largely utilized to allocate surfaces & regions to put
 *               pixels into
 * @shell      - An interface/global object that enables clients to turn
 * @wm_base    - An interface/global object that enables clients to turn
 *               their wl_surfaces into windows in a desktop environment.
 * @shm        - Wayland Shared memory interface. Used to create a simple way of getting
 *               pixels from client to compositor. Pixels are stored in an unsigned 8 bit
 *               integer the buffer is created with mmap(2)
 * @seat       - A interface used to represent a group of hot-pluggable input devices
 */
struct uvr_wc_core_interface {
  uvr_wc_interface_type iType;
  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_compositor *compositor;
  struct xdg_wm_base *wm_base;
  struct wl_shm *shm;
  struct wl_seat *seat;
};


/*
 * struct uvr_wc_core_interface_create_info (Underview Renderer Wayland Client Core Interface Create Information)
 *
 * args:
 * @iType           - Wayland interface to include or exclude
 * @wl_display_name - Specify the wayland server unix domain socket a client should communicate with. This
 *                    will set the $WAYLAND_DISPLAY variable to the desired display. Passing NULL will
 *                    result in opening unix domain socket /run/user/1000/wayland-0
 */
struct uvr_wc_core_interface_create_info {
  uvr_wc_interface_type iType;
  const char *wl_display_name;
};


/*
 * uvr_wc_core_interface_create: Establishes connection to the wayland display server and sets up global objects/interfaces.
 *
 * args:
 * @uvrwc - Pointer to a struct uvr_wc_core_interface_create_info which contains the name of the server
 *          to connected to and the core interfaces to bind/expose to a given client
 *
 * return:
 *    on success struct uvr_wc_core_interface
 *    on failure struct uvr_wc_core_interface { with members nulled }
 */
struct uvr_wc_core_interface uvr_wc_core_interface_create(struct uvr_wc_core_interface_create_info *uvrwc);


/*
 * struct uvr_wc_buffer (Underview Renderer Wayland Client Buffer)
 *
 * members:
 * @shm_fd        - A file descriptor to an open shm object
 * @shm_pool_size - Size of the amount of bytes in a given buffer pixels
 *                  Value = width * height * bytes_per_pixel * @buffer_count
 * @shm_pool_data - Actual pixel data to be displayed. Buffer created via mmap
 * @shm_pool      - Object used to encapsulate a piece of memory shared between the
 *                  compositor and client. @buffers (wl_buffer) can be created from
 *                  shm_pool.
 * @buffer_count  - The amount of buffers contained in a given memory pool
 * @buffers       - An array of type struct wl_buffer. Pixel storage place understood by the compositor.
 */
struct uvr_wc_buffer {
  int shm_fd;
  size_t shm_pool_size;
  uint8_t *shm_pool_data;
  struct wl_shm_pool *shm_pool;

  int buffer_count;
  struct wl_buffer **buffers;
};


/*
 * struct uvr_wc_buffer_create_info (Underview Renderer Wayland Client Buffer Create Information)
 *
 * members:
 * @uvrwccore       - Must pass a valid pointer to all binded/exposed wayland core interfaces
 *                    (important interfaces used: wl_shm)
 * @buffer_count    - The amount of buffers to allocate when storing pixel data
 *                    2 is generally a good option as it allows for double buffering
 * @width           - Amount of pixel horizontally (i.e 3840, 1920, ...)
 * @height          - Amount of pixel vertically (i.e 2160, 1080, ...)
 * @bytes_per_pixel - The amount of bytes per pixel generally going to 4 bytes (32 bits)
 * @wl_pix_format   - Memory layout of an individual pixel
 */
struct uvr_wc_buffer_create_info {
  struct uvr_wc_core_interface *uvrwccore;
  const int buffer_count;
  const int width;
  const int height;
  const int bytes_per_pixel;
  uint64_t wl_pix_format;
};


/*
 * uvr_wc_buffer_create: Adds way to get pixels from client to compositor by creating writable shared buffers.
 *
 * args:
 * @uvrwc - Must pass a valid pointer to a struct uvr_wc_buffer_create_info which gives
 *          details of how a buffer should be allocated and how many to allocations to make.
 *
 * return:
 *    on success struct uvr_wc_buffer
 *    on failure struct uvr_wc_buffer { with members nulled, integers set to -1 }
 */
struct uvr_wc_buffer uvr_wc_buffer_create(struct uvr_wc_buffer_create_info *uvrwc);


/*
 * struct uvr_wc_surface (Underview Renderer Wayland Client Surface)
 *
 * members:
 * @xdg_toplevel  - An interface with many requests and events to manage application windows
 * @xdg_surface   - Top level surface for the window. Useful if one wants to create
 *                  a tree of surfaces. Provides additional requests for assigning
 *                  roles.
 * @surface       - The image displayed on screen. If wl_shm is binded one must attach
 *                  a wl_buffer (@buffers) to display pixels on screen. Depending on
 *                  rendering backend wl_buffer may not need to be allocated.
 * @buffer_count  - The amount of buffers contained in a given memory pool
 * @buffers       - An array of type struct wl_buffer *. Pixel storage place understood by compositor.
 */
struct uvr_wc_surface {
  struct xdg_toplevel *xdg_toplevel;
  struct xdg_surface *xdg_surface;
  struct wl_surface *surface;

  int buffer_count;
  struct wl_buffer **buffers;
};


/*
 * struct uvr_wc_surface_create_info (Underview Renderer Wayland Client Surface Create Information)
 *
 * members:
 * @uvrwccore  - pointer to a struct uvr_wc_core_interface contains all objects/interfaces
 *               necessary for a client to run.
 * @uvrwcbuff  - Must pass a valid pointer to a struct uvr_wc_buffer need to attach a wl_buffer object
 *               to a wl_surface object. If NULL passed no buffer will be attached to surface. Thus nothing
 *               will be displayed
 * @appname    - Sets the window name.
 * @fullscreen - Determines if window should be fullscreen or not
 */
struct uvr_wc_surface_create_info {
  struct uvr_wc_core_interface *uvrwccore;
  struct uvr_wc_buffer *uvrwcbuff;
  const char *appname;
  bool fullscreen;
};


/*
 * uvr_wc_surface_create: Creates a surface to place pixels in and a window for displaying surface pixels.
 *
 * args:
 * @uvrwc - Must pass a valid pointer to a struct uvr_wc_surface_create_info which gives
 *          details on what buffers are associated with surface object, the name of the
 *          window, how the window should be displayed, (more info TBA).
 *
 * return:
 *    on success struct uvr_wc_surface
 *    on failure struct uvr_wc_surface { with members nulled }
 */
struct uvr_wc_surface uvr_wc_surface_create(struct uvr_wc_surface_create_info *uvrwc);


/*
 * uvr_wc_process_events: Processes incoming Wayland server events.
 *                        Best utilized with epoll(POLLIN)
 *
 * args:
 * @uvrwc - pointer to a struct uvr_wc_core_interface contains all objects/interfaces
 *          necessary for a client to run.
 * return:
 *    -1 on failure
 */
int uvr_wc_process_events(struct uvr_wc_core_interface *uvrwc);


/*
 * uvr_wc_flush_request: Flush all outgoing request to a Wayland server.
 *
 * args:
 * @uvrwc - pointer to a struct uvr_wc_core_interface contains all objects/interfaces
 *          necessary for a client to run.
 * return:
 *    -1 on failure
 */
int uvr_wc_flush_request(struct uvr_wc_core_interface *uvrwc);


/*
 * struct uvr_wc_destory (Underview Renderer Wayland Client Destroy)
 *
 * members:
 * @wccinterface - Must pass a valid struct uvr_wc_core_interface which contains all
 *                 objects/interfaces a given user decided to bind.
 * @wcbuff       - Must pass a valid struct uvr_wc_buffer, to free all allocated memory
 * @wcsurface    - Must pass a valid pointer to a struct uvr_wc_surface, to free all allocated memory
 */
struct uvr_wc_destory {
  struct uvr_wc_core_interface wccinterface;
  struct uvr_wc_buffer wcbuff;
  struct uvr_wc_surface wcsurface;
};


/*
 * uvr_wc_destory: frees any remaining allocated memory contained in struct uvr_wc_destory
 *
 * args:
 * @uvrwc - pointer to a struct uvr_wc contains all objects/interfaces
 *          necessary for an xcb client to run.
 */
void uvr_wc_destory(struct uvr_wc_destory *uvrwc);

#endif
