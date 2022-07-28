#ifndef UVR_XCB_CLIENT_H
#define UVR_XCB_CLIENT_H

#include "utils.h"

#include <stdbool.h>
#include <xcb/xcb.h>


/*
 * struct uvr_xcb_window (Underview Renderer XCB Window)
 *
 * members:
 * @conn       - A structure that contain all data that XCB needs to communicate with an X server.
 * @window     - Stores the XID of the current window. XID is neeed to create
 *               windows and manage its properties
 * @delWindow  - Used to by uvr_xcb_window_wait_for_event() to verify when the window manager
 *               attempts to destroy the window.
 */
struct uvr_xcb_window {
  xcb_connection_t        *conn;
  xcb_window_t            window;
  xcb_intern_atom_reply_t *delWindow;
};


/*
 * struct uvr_xcb_window (Underview Renderer XCB Window Create Information)
 *
 * members:
 * @display     - The X server's display name. When set to NULL, the DISPLAY
 *                environment variable is used.
 * @screen      - Number for the screen that should be connected. When set to NULL,
 *                the screen number is set to 0.
 * @appName     - Sets the window name. Can not be more than 60 characters.
 * @fullscreen  - Set to true to go fullscreen, false to display normal window.
 * @transparent - Set to true to have fully transparent window. False will display black background.
 */
struct uvr_xcb_window_create_info {
  const char *display;
  int        *screen;
  const char *appName;
  bool       fullscreen;
  bool       transparent;
};


/*
 * uvr_xcb_window_create: create a fulls xcb client window (can be fullscreen).
 *
 * args:
 * @client - pointer to a struct uvrxcb contains all objects necessary
 *           for an xcb client to run.
 * return:
 *    on success struct uvr_xcb_window
 *    on failure struct uvr_xcb_window { with members nulled }
 */
struct uvr_xcb_window uvr_xcb_window_create(struct uvr_xcb_window_create_info *uvrxcb);


/*
 * Underview Renderer Implementation
 * Function pointer used by struct uvr_xcb_window_wait_for_event_info
 * Allows to pass the address of an external function you want to run
 * Given that the arguments of the function are a pointer to a boolean,
 * pointer to an integer, and a pointer to void data type
 */
typedef void (*uvr_xcb_renderer_impl)(bool*, uint32_t*, void*);


/*
 * struct uvr_xcb_window_wait_for_event_info (Underview Renderer XCB Wait For Event Information)
 *
 * members:
 * @uvrXcbWindow   - Pointer to a struct uvr_xcb_window contains all objects necessary to manage xcb client
 * @renderer       - Function pointer that allows custom external renderers to be executed by the api
 *                   when before registering a frame wl_callback. Renderer before presenting
 * @rendererData   - Pointer to an optional address. This address may be the address of a struct. Reference
 *                   passed depends on external render function.
 * @rendererCbuf   - Pointer to an integer used by the api to update the current displayable buffer
 * @rendererRuning - Pointer to a boolean that determines if a given window/surface is actively running
 */
struct uvr_xcb_window_wait_for_event_info {
  struct uvr_xcb_window *uvrXcbWindow;
  uvr_xcb_renderer_impl renderer;
  void                  *rendererData;
  uint32_t              *rendererCbuf;
  bool                  *rendererRuning;
};


/*
 * uvr_xcb_window_wait_for_event: In an X program, everything is driven by events. This functions calls
 *                                xcb_wait_for_event which blocks until an event is queued in the X server.
 *                                The main event watched by function is KEY_PRESSING, when either the 'Q'
 *                                or 'ESC' keys are pressed the function will return failure status. Function
 *                                is meant to be utilized as a while loop conditional/expression.
 *
 *                                https://xcb.freedesktop.org/tutorial/events
 * args:
 * @client - pointer to a struct uvr_xcb_window_wait_for_event_info contains all objects necessary for an
 *           xcb client to run and pointers to execute custom renderers.
 * return:
 *    on success 1
 *    on failure 0
 */
int uvr_xcb_window_wait_for_event(struct uvr_xcb_window_wait_for_event_info *uvrxcb);


/*
 * struct uvrxcb (Underview Renderer XCB Destroy)
 *
 * members:
 * @uvr_xcb_window - Must pass a valid struct uvr_xcb_window { free'd members: xcb_connection_t *conn, xcb_window_t window }
 */
struct uvr_xcb_destroy {
  struct uvr_xcb_window uvr_xcb_window;
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
