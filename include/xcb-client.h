
#ifndef UVR_XCB_CLIENT_H
#define UVR_XCB_CLIENT_H

#include "common.h"
#include <xcb/xcb.h>

typedef struct _uvrxcb {
  xcb_connection_t *conn;
  xcb_window_t window;
} uvrxcb;

/*
 * Function creates a fullscreen xcb client
 *
 * ARGS:
 *    const char *display:
 *        The X server's display name. When set to NULL,
 *        the DISPLAY environment variable is used.
 *    int *screen:
 *        Number for the screen that should be connected.
 *        When set to NULL, the screen number is set to 0.
 *    const char *appname:
 *        Sets the window name. Can not be more than 60 characters
 *    bool fullscreen:
 *        Set to true to go fullscreen, false to display normal window
 * return on failure:
 *    conn = NULL, window = UINT32_MAX
 */
uvrxcb uvr_xcb_create_client(const char *display, int *screen, const char *appname, bool fullscreen);

/* Displays the window */
void uvr_xcb_display_window(uvrxcb *client);

/* frees all allocated memory contained in uvrxcb */
void uvr_xcb_destory(uvrxcb *client);

#endif
