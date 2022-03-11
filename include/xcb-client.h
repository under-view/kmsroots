
#ifndef UVC_XCB_CLIENT_H
#define UVC_XCB_CLIENT_H

#include "common.h"
#include <xcb/xcb.h>

typedef struct _uvcxcb {
  xcb_connection_t *conn;
  xcb_window_t window;
} uvcxcb;

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
uvcxcb uvc_xcb_create_client(const char *display, int *screen, const char *appname, bool fullscreen);

/* Displays the window */
void uvc_xcb_display_window(uvcxcb *client);

/* frees all allocated memory contained in uvcxcb */
void uvc_xcb_destory(uvcxcb *client);

#endif
