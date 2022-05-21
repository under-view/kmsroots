#ifndef UVR_XCB_CLIENT_H
#define UVR_XCB_CLIENT_H

#include "common.h"
#include "utils.h"
#include <xcb/xcb.h>

/*
 * struct uvr_xcb_window (Underview Renderer XCB Client Window Create Information)
 *
 * members:
 * @display    - The X server's display name. When set to NULL, the DISPLAY
 *               environment variable is used.
 * @screen     - Number for the screen that should be connected. When set to NULL,
 *               the screen number is set to 0.
 * @appname    - Sets the window name. Can not be more than 60 characters.
 * @fullscreen - Set to true to go fullscreen, false to display normal window.
 * @window     - Stores the XID of the current window. XID is neeed to create
 *               windows and manage its properties.
 */
struct uvr_xcb_window_create_info {
  const char *display;
  int *screen;
  const char *appname;
  bool fullscreen;
  xcb_window_t window;
};


/*
 * uvr_xcb_create_client: create a fulls xcb client window (can be fullscreen).
 *
 * args:
 * @client - pointer to a struct uvrxcb contains all objects necessary
 *           for an xcb client to run.
 * return:
 *    xcb_connection_t on success
 *    NULL on failure
 */
xcb_connection_t *uvr_xcb_client_create(struct uvr_xcb_window_create_info *uvrxcb);


/*
 * struct uvr_xcb (Underview Renderer XCB Client)
 *
 * members:
 * @conn   - A structure that contain all data that XCB needs to communicate with an X server.
 * @window - Stores the XID of the current window. XID is neeed to create
 *           windows and manage its properties
 */
struct uvr_xcb {
  xcb_connection_t *conn;
  xcb_window_t window;
};


/*
 * uvr_xcb_display_window: Displays the window
 *
 * args:
 * @client - pointer to a struct uvrwc contains all objects necessary
 *           for an xcb client to run.
 */
void uvr_xcb_display_window(struct uvr_xcb *uvrxcb);


/*
 * struct uvrxcb (Underview Renderer XCB Client Destroy)
 *
 * members:
 * @conn   - A structure that contain all data that XCB needs to communicate with an X server.
 * @window - Stores the XID of the current window. XID is neeed to create windows and manage its properties.
 */
struct uvr_xcb_destroy {
  xcb_connection_t *conn;
  xcb_window_t window;
};

/*
 * uvr_xcb_destory: frees all allocated memory contained in struct uvrxcb
 *
 * args:
 * @uvrxcb - pointer to a struct uvrxcb_destroy contains all objects
 *           created during window lifetime in need of destruction
 */
void uvr_xcb_destory(struct uvr_xcb_destroy *uvrxcb);

#endif
