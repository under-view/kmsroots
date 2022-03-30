
#ifndef UVR_WAYLAND_CLIENT_H
#define UVR_WAYLAND_CLIENT_H

#include "common.h"
#include <wayland-client.h>

/*
 * typedef struct _uvrwc uvrwc
 *
 * @wl_display_fd - file descriptor associated with a given display
 * @display - a global singleton representing the whole connection
 *            to wayland server.
 * @registry - a global singleton used to query information about server
 *             state/properties and allocate other objects/interfaces
 * @compositor - a global singleton representing the compositor itself
 *               largely utilized to allocate surfaces & regions to put
 *               pixels into
 * @shell -
 * @xdg_toplevel -
 * @shm - Shared memory, used to transfer a file descriptor for the
 *        compositor to mmap.
 * @surface -
 * @xdg_surface -
 */
typedef struct _uvrwc {
  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_compositor *compositor;

  struct xdg_wm_base *shell;
  struct xdg_toplevel *xdg_toplevel;

  struct wl_shm *shm;

  struct wl_surface *surface;
  struct xdg_surface *xdg_surface;
} uvrwc;


/*
 * uvr_wclient_display_connect: Establish connection to Wayland display server
 *
 * args:
 * @wl_display_name - Specify the Wayland display to run a client on. This will set
 *                    the $WAYLAND_DISPLAY variable to the desired display.
 * return:
 *    NULL on failure
 */
struct wl_display *uvr_wclient_display_connect(const char *wl_display_name);


/*
 * uvr_wclient_get_interfaces: Sets up global objects/interfaces.
 *
 * args:
 * @client - pointer to a struct _uvrwc contains all objects/interfaces
 *           necessary for a client to run.
 * return:
 *    0 on success
 *   -1 on failure
 */
int uvr_wclient_alloc_interfaces(uvrwc *client);


/*
 * uvr_wclient_process_events: Processes incoming Wayland server events.
 *                             Best utilized with epoll(POLLIN)
 * args:
 * @client - pointer to a struct _uvrwc contains all objects/interfaces
 *           necessary for a client to run.
 * return:
 *    -1 on failure
 */
int uvr_wclient_process_events(uvrwc *client);


/*
 * uvr_wclient_flush_request: Flush all outgoing request to a Wayland server.
 * args:
 * @client - pointer to a struct _uvrwc contains all objects/interfaces
 *           necessary for a client to run.
 * return:
 *    -1 on failure
 */
int uvr_wclient_flush_request(uvrwc *client);


void uvr_wclient_destory(uvrwc *client);

#endif
