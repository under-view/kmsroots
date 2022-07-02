#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <xcb/xcb_ewmh.h>

#include "xclient.h"


struct uvr_xcb_window uvr_xcb_window_create(struct uvr_xcb_window_create_info *uvrxcb) {
  xcb_connection_t *conn = NULL;
  xcb_window_t window;

  const xcb_setup_t *xcbsetup = NULL;
  xcb_screen_t *screen = NULL;

  /* Connect to Xserver */
  conn = xcb_connect(uvrxcb->display, uvrxcb->screen);
  if (xcb_connection_has_error(conn)) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_xcb_create_client: xcb_connect Failed");
    if (!uvrxcb->display)
      uvr_utils_log(UVR_INFO, "Try setting the DISPLAY environment variable");
    goto error_exit_xcb_window_create;
  }

  /* access properties of the X server and its display environment */
  xcbsetup = xcb_get_setup(conn);
  if (!xcbsetup) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_xcb_create_client: xcb_get_setup Failed");
    goto error_exit_xcb_window_disconnect;
  }

  /* Retrieve XID */
  window = xcb_generate_id(conn);

  /*
   * xcb_screen_iterator_t structure has a data field of type
   * xcb_screen_t*, which represents the screen.
   * Retrieve the first screen of the connection.
   * Once we have the screen we can then create a window.
   */
  screen = xcb_setup_roots_iterator(xcbsetup).data;
  if (!screen) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_xcb_create_client: xcb_setup_roots_iterator data member NULL");
    goto error_exit_xcb_window_destroy;
  }

  uvr_utils_log(UVR_INFO, "Informations of screen %" PRIu32, screen->root);
  uvr_utils_log(UVR_WARNING, "Screen dimensions: %d, %d", screen->width_in_pixels, screen->height_in_pixels);

  /* Create window */
  uint32_t prop_name = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  uint32_t prop_value[2] = { screen->black_pixel, XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_KEY_PRESS };

  xcb_create_window(conn, screen->root_depth, window, screen->root, 0,
                    0, screen->width_in_pixels, screen->height_in_pixels, 0,
                    XCB_WINDOW_CLASS_COPY_FROM_PARENT, screen->root_visual,
                    prop_name, prop_value);

  /* Change window property name */
  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME,
                      XCB_ATOM_STRING, 8, strnlen(uvrxcb->appname, 60), uvrxcb->appname);

  /* Change window property to make fullscreen */
  if (uvrxcb->fullscreen) {
    xcb_ewmh_connection_t xcbewmh = {};
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, &xcbewmh);
    if (!xcb_ewmh_init_atoms_replies(&xcbewmh, cookie, NULL)) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_xcb_create_client: xcb_ewmh_init_atoms_replies Couldn't initialise ewmh atom");
      goto error_exit_xcb_window_destroy;
    }

    xcb_change_property(conn, XCB_PROP_MODE_PREPEND, window,
                        xcbewmh._NET_WM_STATE, XCB_ATOM_ATOM, 32, 1,
                        &(xcbewmh._NET_WM_STATE_FULLSCREEN));
  }

  return (struct uvr_xcb_window) { .conn = conn, .window = window };

error_exit_xcb_window_destroy:
  if (window)
    xcb_destroy_window(conn, window);
error_exit_xcb_window_disconnect:
  if (conn)
    xcb_disconnect(conn);
error_exit_xcb_window_create:
  return (struct uvr_xcb_window) { .conn = NULL, .window = 0 };
}


void uvr_xcb_display_window(struct uvr_xcb_window *uvrxcb) {
  xcb_map_window(uvrxcb->conn, uvrxcb->window);
  xcb_flush(uvrxcb->conn);
}


void uvr_xcb_destory(struct uvr_xcb_destroy *uvrxcb) {
  if (uvrxcb->xcbwindow.window)
    xcb_destroy_window(uvrxcb->xcbwindow.conn, uvrxcb->xcbwindow.window);
  if (uvrxcb->xcbwindow.conn)
    xcb_disconnect(uvrxcb->xcbwindow.conn);
}
