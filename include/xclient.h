
#ifndef UVR_XCB_CLIENT_H
#define UVR_XCB_CLIENT_H

#include "common.h"
#include <xcb/xcb.h>

/*
 * typedef struct _uvrxcb uvrxcb (Underview Renderer XCB Client)
 *
 * @conn   - value of the DISPLAY environment variable and our connection
 *           to the Xserver
 * @window - stores the XID of the current window. XID is neeed to create
 *           windows and manage its properties
 */
typedef struct _uvrxcb {
  xcb_connection_t *conn;
  xcb_window_t window;
} uvrxcb;


/*
 * uvr_xcb_create_client: create a fullscreen xcb client. Only works on one screen
 *
 * args:
 * @client     - pointer to a struct _uvrxcb contains all objects necessary
 *               for an xcb client to run.
 *
 *               Members that recieve addresses and values:
 *               @conn, @window
 * @display    - The X server's display name. When set to NULL, the DISPLAY
 *               environment variable is used.
 * @screen     - Number for the screen that should be connected. When set to NULL,
 *               the screen number is set to 0.
 * @appname    - Sets the window name. Can not be more than 60 characters
 * @fullscreen - Set to true to go fullscreen, false to display normal window
 * return:
 *    0 on success
 *   -1 on failure
 */
int uvr_xcb_create_client(uvrxcb *client,
                          const char *display,
                          int *screen,
                          const char *appname,
                          bool fullscreen);


/*
 * uvr_xcb_display_window: Displays the window
 *
 * args:
 * @client - pointer to a struct _uvrwc contains all objects necessary
 *           for an xcb client to run.
 */
void uvr_xcb_display_window(uvrxcb *client);


/*
 * uvr_xcb_destory: frees all allocated memory contained in struct _uvrxcb
 *
 * args:
 * @client - pointer to a struct _uvrxcb contains all objects necessary
 *           for an xcb client to run.
 */
void uvr_xcb_destory(uvrxcb *client);

#endif
