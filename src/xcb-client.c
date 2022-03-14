
#include "xclient.h"
#include <xcb/xcb_ewmh.h>

uvrxcb uvr_xcb_create_client(const char *display, int *screen, const char *appname, bool fullscreen) {
  uvrxcb ret = {NULL,UINT32_MAX};
  const xcb_setup_t *xcbsetup = NULL;
  xcb_screen_t *xcbscreen = NULL;

  /* Connect to Xserver */
  ret.conn = xcb_connect(display, screen);
  if (xcb_connection_has_error(ret.conn)) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_xcb_create_client: xcb_connect Failed");
    if (!display)
      uvr_utils_log(UVR_INFO, "Try setting the DISPLAY environment variable");
    return ret;
  }

  /* access properties of the X server and its display environment */
  xcbsetup = xcb_get_setup(ret.conn);
  if (!xcbsetup) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_xcb_create_client: xcb_get_setup Failed");
    return ret;
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
    return ret;
  }

  uvr_utils_log(UVR_INFO, "Informations of screen %" PRIu32, xcbscreen->root);
  uvr_utils_log(UVR_WARNING, "Screen dimensions: %d, %d", xcbscreen->width_in_pixels, xcbscreen->height_in_pixels);

  /* Create window */
  uint32_t prop_name = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
  uint32_t prop_value[2] = { xcbscreen->black_pixel, XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_KEY_PRESS };

  ret.window = xcb_generate_id(ret.conn);

  xcb_create_window(ret.conn, xcbscreen->root_depth, ret.window, xcbscreen->root, 0,
                    0, xcbscreen->width_in_pixels, xcbscreen->height_in_pixels, 0,
                    XCB_WINDOW_CLASS_COPY_FROM_PARENT, xcbscreen->root_visual,
                    prop_name, prop_value);

  /* Change window property name */
  xcb_change_property(ret.conn, XCB_PROP_MODE_REPLACE, ret.window,
                      XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
                      strnlen(appname, 60), appname);

  /* Change window property to make fullscreen */
  if (fullscreen) {
    xcb_ewmh_connection_t xcbewmh = {};
    if (!xcb_ewmh_init_atoms_replies(&xcbewmh, xcb_ewmh_init_atoms(ret.conn, &xcbewmh), NULL)) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_xcb_create_client: xcb_ewmh_init_atoms_replies Couldn't initialise ewmh atom");
      return ret;
    }

    xcb_change_property(ret.conn, XCB_PROP_MODE_PREPEND, ret.window,
                        xcbewmh._NET_WM_STATE, XCB_ATOM_ATOM, 32, 1,
                        &(xcbewmh._NET_WM_STATE_FULLSCREEN));
  }

  return ret;
}

void uvr_xcb_display_window(uvrxcb *client) {
  xcb_map_window(client->conn, client->window);
  xcb_flush(client->conn);
}

void uvr_xcb_destory(uvrxcb *client) {
  if (client->window != UINT32_MAX)
    xcb_destroy_window(client->conn, client->window);

  /* Disconnect from X server */
  if (client->conn)
    xcb_disconnect(client->conn);
}
