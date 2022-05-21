#include "xclient.h"
#include <xcb/xcb_ewmh.h>

xcb_connection_t *uvr_xcb_client_create(struct uvr_xcb_window_create_info *uvrxcb) {
  const xcb_setup_t *xcbsetup = NULL;
  xcb_screen_t *xcbscreen = NULL;
  xcb_connection_t *conn = NULL;

  /* Connect to Xserver */
  conn = xcb_connect(uvrxcb->display, uvrxcb->screen);
  if (xcb_connection_has_error(conn)) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_xcb_create_client: xcb_connect Failed");
    if (!uvrxcb->display)
      uvr_utils_log(UVR_INFO, "Try setting the DISPLAY environment variable");
    goto error_exit_xcb_client_create;
  }

  /* access properties of the X server and its display environment */
  xcbsetup = xcb_get_setup(conn);
  if (!xcbsetup) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_xcb_create_client: xcb_get_setup Failed");
    goto error_exit_xcb_client_create;
  }

  /*
   * xcb_screen_iterator_t structure has a data field of type
   * xcb_screen_t*, which represents the screen.
   * Retrieve the first screen of the connection.
   * Once we have the screen we can then create a window.
   */
  xcbscreen = xcb_setup_roots_iterator(xcbsetup).data;
  if (!xcbscreen) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_xcb_create_client: xcb_setup_roots_iterator data member NULL");
    goto error_exit_xcb_client_create;
  }

  uvr_utils_log(UVR_INFO, "Informations of screen %" PRIu32, xcbscreen->root);
  uvr_utils_log(UVR_WARNING, "Screen dimensions: %d, %d", xcbscreen->width_in_pixels, xcbscreen->height_in_pixels);

  /* Create window */
  uint32_t prop_name = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  uint32_t prop_value[2] = { xcbscreen->black_pixel, XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_KEY_PRESS };

  uvrxcb->window = xcb_generate_id(conn);

  xcb_create_window(conn, xcbscreen->root_depth, uvrxcb->window, xcbscreen->root, 0,
                    0, xcbscreen->width_in_pixels, xcbscreen->height_in_pixels, 0,
                    XCB_WINDOW_CLASS_COPY_FROM_PARENT, xcbscreen->root_visual,
                    prop_name, prop_value);

  /* Change window property name */
  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, uvrxcb->window,
                      XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
                      strnlen(uvrxcb->appname, 60), uvrxcb->appname);

  /* Change window property to make fullscreen */
  if (uvrxcb->fullscreen) {
    xcb_ewmh_connection_t xcbewmh = {};
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, &xcbewmh);
    if (!xcb_ewmh_init_atoms_replies(&xcbewmh, cookie, NULL)) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_xcb_create_client: xcb_ewmh_init_atoms_replies Couldn't initialise ewmh atom");
      goto error_exit_xcb_client_create;
    }

    xcb_change_property(conn, XCB_PROP_MODE_PREPEND, uvrxcb->window,
                        xcbewmh._NET_WM_STATE, XCB_ATOM_ATOM, 32, 1,
                        &(xcbewmh._NET_WM_STATE_FULLSCREEN));
  }

  return conn;

error_exit_xcb_client_create:
  if (uvrxcb->window) {
    xcb_destroy_window(conn, uvrxcb->window);
    uvrxcb->window = 0;
  }

  if (conn)
    xcb_disconnect(conn);

  return NULL;
}


void uvr_xcb_display_window(struct uvr_xcb *uvrxcb) {
  xcb_map_window(uvrxcb->conn, uvrxcb->window);
  xcb_flush(uvrxcb->conn);
}


void uvr_xcb_destory(struct uvr_xcb_destroy *uvrxcb) {
  if (uvrxcb->window)
    xcb_destroy_window(uvrxcb->conn, uvrxcb->window);

  /* Disconnect from X server */
  if (uvrxcb->conn)
    xcb_disconnect(uvrxcb->conn);
}
