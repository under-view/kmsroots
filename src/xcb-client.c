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
    uvr_utils_log(UVR_DANGER, "[x] xcb_connect: xcb_connect Failed");
    if (!uvrxcb->display)
      uvr_utils_log(UVR_INFO, "Try setting the DISPLAY environment variable");
    goto error_exit_xcb_window_create;
  }

  /* access properties of the X server and its display environment */
  xcbsetup = xcb_get_setup(conn);
  if (!xcbsetup) {
    uvr_utils_log(UVR_DANGER, "[x] xcb_get_setup: Failed to access properties of the X server and its display environment");
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
    uvr_utils_log(UVR_DANGER, "[x] xcb_setup_roots_iterator: Failed to retrieve connected screen");
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
                      XCB_ATOM_STRING, 8, strnlen(uvrxcb->appName, 60), uvrxcb->appName);

  /* Change window property to make fullscreen */
  if (uvrxcb->fullscreen) {
    xcb_ewmh_connection_t xcbewmh = {};
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, &xcbewmh);
    if (!xcb_ewmh_init_atoms_replies(&xcbewmh, cookie, NULL)) {
      uvr_utils_log(UVR_DANGER, "[x] xcb_ewmh_init_atoms_replies: Failed");
      goto error_exit_xcb_window_destroy;
    }

    xcb_change_property(conn, XCB_PROP_MODE_PREPEND, window,
                        xcbewmh._NET_WM_STATE, XCB_ATOM_ATOM, 32, 1,
                        &(xcbewmh._NET_WM_STATE_FULLSCREEN));
  }

  /* Change window property to make window fully transparent */
  if (uvrxcb->transparent) {
    uint32_t opacity = -1 * 0xffffffff;
    xcb_intern_atom_cookie_t winopacity = xcb_intern_atom(conn, 0, strlen("_NET_WM_WINDOW_OPACITY"), "_NET_WM_WINDOW_OPACITY");
    xcb_intern_atom_reply_t *winopacityreply = xcb_intern_atom_reply(conn, winopacity, NULL);
    xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, winopacityreply->atom, XCB_ATOM_CARDINAL, 32, 1L, &opacity);
    free(winopacityreply);
  }

  /* Instructs the XServer to watch when the window manager attempts to destroy the window. */
  xcb_intern_atom_cookie_t windelete = xcb_intern_atom(conn, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
  xcb_intern_atom_cookie_t winprotoscookie = xcb_intern_atom(conn, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
  xcb_intern_atom_reply_t *windeletereply = xcb_intern_atom_reply(conn, windelete, NULL);
  xcb_intern_atom_reply_t *winprotosreply = xcb_intern_atom_reply(conn, winprotoscookie, NULL);
  xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, winprotosreply->atom, 4, 32, 1, &windeletereply->atom);
  free(winprotosreply);

  xcb_map_window(conn, window);
  xcb_flush(conn);

  return (struct uvr_xcb_window) { .conn = conn, .window = window, .delWindow = windeletereply };

error_exit_xcb_window_destroy:
  if (window)
    xcb_destroy_window(conn, window);
error_exit_xcb_window_disconnect:
  if (conn)
    xcb_disconnect(conn);
error_exit_xcb_window_create:
  return (struct uvr_xcb_window) { .conn = NULL, .window = 0, .delWindow = 0 };
}


int uvr_xcb_window_wait_for_event(struct uvr_xcb_window_wait_for_event_info *uvrxcb) {
  xcb_generic_event_t *event = NULL;

  uvrxcb->renderer(uvrxcb->rendererRuning, uvrxcb->rendererCbuf, uvrxcb->rendererData);

  event = xcb_poll_for_event(uvrxcb->uvrXcbWindow->conn);
  if (!event) {
    goto exit_xcb_window_event_loop;
  }

  switch (event->response_type & ~0x80) {
    case XCB_KEY_PRESS: {
      xcb_key_press_event_t *press = (xcb_key_press_event_t *) event;
      //uvr_utils_log(UVR_INFO, "uvr_xcb_window_wait_for_event: Key pressed %d", press->detail);
      if (press->detail == 9 || press->detail == 24) /* ESCAPE key, Q key */
        goto error_exit_xcb_window_event_loop;
      break;
    }

    case XCB_CLIENT_MESSAGE: {
      xcb_client_message_event_t *message = (xcb_client_message_event_t *) event;
      if (message->data.data32[0] == uvrxcb->uvrXcbWindow->delWindow->atom)
        goto error_exit_xcb_window_event_loop;
      break;
    }

    default: /* Unknown event type, ignore it */
      break;
  }

exit_xcb_window_event_loop:
  free(event);
  return 1;

error_exit_xcb_window_event_loop:
  free(event);
  return 0;
}


void uvr_xcb_destory(struct uvr_xcb_destroy *uvrxcb) {
  free(uvrxcb->uvr_xcb_window.delWindow);
  if (uvrxcb->uvr_xcb_window.window)
    xcb_destroy_window(uvrxcb->uvr_xcb_window.conn, uvrxcb->uvr_xcb_window.window);
  if (uvrxcb->uvr_xcb_window.conn)
    xcb_disconnect(uvrxcb->uvr_xcb_window.conn);
}
