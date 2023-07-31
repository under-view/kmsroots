#ifndef KMR_XCB_CLIENT_H
#define KMR_XCB_CLIENT_H

#include "utils.h"

#include <stdbool.h>
#include <xcb/xcb.h>


/*
 * struct kmr_xcb_window (kmsroots XCB Window)
 *
 * members:
 * @conn       - A structure that contain all data that XCB needs to communicate with an X server.
 * @window     - Stores the XID of the current window. XID is neeed to create
 *               windows and manage its properties
 * @delWindow  - Used to by kmr_xcb_window_wait_for_event() to verify when the window manager
 *               attempts to destroy the window.
 */
struct kmr_xcb_window {
	xcb_connection_t        *conn;
	xcb_window_t            window;
	xcb_intern_atom_reply_t *delWindow;
};


/*
 * struct kmr_xcb_window (kmsroots XCB Window Create Information)
 *
 * members:
 * @display     - The X server's display name. When set to NULL, the DISPLAY
 *                environment variable is used.
 * @screen      - Number for the screen that should be connected. When set to NULL,
 *                the screen number is set to 0.
 * @appName     - Sets the window name. Can not be more than 60 characters.
 * @width       - Width of window in pixels
 * @height      - Height of window in pixels
 * @fullscreen  - Set to true to go fullscreen, false to display normal window.
 * @transparent - Set to true to have fully transparent window. False will display black background.
 */
struct kmr_xcb_window_create_info {
	const char *display;
	int        *screen;
	const char *appName;
	uint16_t   width;
	uint16_t   height;
	bool       fullscreen;
	bool       transparent;
};


/*
 * kmr_xcb_window_create: create a fulls xcb client window (can be fullscreen).
 *
 * parameters:
 * @kmsxcb - pointer to a struct kmr_xcb_window_create_info contains all information
 *           required to created an xcb client and some added window configuration options.
 * returns:
 *	on success struct kmr_xcb_window
 *	on failure struct kmr_xcb_window { with members nulled }
 */
struct kmr_xcb_window kmr_xcb_window_create(struct kmr_xcb_window_create_info *kmsxcb);


/*
 * kmr_xcb_window_make_visible: Creates the window that we display on by informing server to map the window to the screen.
 *                              NOTE: If window is created before vulkan can establish VkSurfaceKHR/VkFramebuffer
 *                              objects it leads to window being in a deadlock state. With no way of recovery without
 *                              a power recycle. Validation layers report - vkCreateFramebuffer(): VkFramebufferCreateInfo
 *                              attachment #0 mip level 0 has width (1848) smaller than the corresponding framebuffer width (1920).
 *
 * parameters:
 * @kmsxcb - pointer to a struct kmr_xcb_window contains all objects necessary
 *           for an xcb client window to display.
 */
void kmr_xcb_window_make_visible(struct kmr_xcb_window *kmsxcb);


/*
 * kmsroots Implementation
 * Function pointer used by struct kmr_xcb_window_wait_for_event_info
 * Allows to pass the address of an external function you want to run
 * Given that the arguments of the function are a pointer to a boolean,
 * pointer to an integer, and a pointer to void data type
 */
typedef void (*kmr_xcb_renderer_impl)(volatile bool*, uint32_t*, void*);


/*
 * struct kmr_xcb_window_handle_event_info (kmsroots XCB Window Handle Event Information)
 *
 * members:
 * @xcbWindowObject       - Pointer to a struct kmr_xcb_window contains all objects necessary to manage xcb client
 * @renderer              - Function pointer that allows custom external renderers to be executed by the api
 *                          when before registering a frame wl_callback. Renderer before presenting
 * @rendererData          - Pointer to an optional address. This address may be the address of a struct. Reference
 *                          passed depends on external render function.
 * @rendererCurrentBuffer - Pointer to an integer used by the api to update the current displayable buffer
 * @rendererRunning       - Pointer to a boolean that determines if a given window/surface is actively running
 */
struct kmr_xcb_window_handle_event_info {
	struct kmr_xcb_window *xcbWindowObject;
	kmr_xcb_renderer_impl renderer;
	void                  *rendererData;
	uint32_t              *rendererCurrentBuffer;
	volatile bool         *rendererRunning;
};


/*
 * kmr_xcb_window_handle_event: In an X program, everything is driven by events. This functions calls
 *                              xcb_poll_for_event which doesn't block operations and returns events from
 *                              X server when available. The main event watched by function is KEY_PRESSING,
 *                              when either the 'Q' or 'ESC' keys are pressed the function will return
 *                              failure status. Function is meant to be utilized as a while loop conditional/expression.
 *
 *                              https://xcb.freedesktop.org/tutorial/events
 *
 * parameters:
 * @client - pointer to a struct kmr_xcb_window_wait_for_event_info contains all objects necessary for an
 *           xcb client to run and pointer to custom renderer to execute and the arguments used by said renderer.
 * returns:
 *	on success 1
 *	on failure 0
 */
int kmr_xcb_window_handle_event(struct kmr_xcb_window_handle_event_info *kmsxcb);


/*
 * struct kmsxcb (kmsroots XCB Destroy)
 *
 * members:
 * @kmr_xcb_window - Must pass a valid struct kmr_xcb_window { free'd members: xcb_connection_t *conn, xcb_window_t window }
 */
struct kmr_xcb_destroy {
	struct kmr_xcb_window kmr_xcb_window;
};


/*
 * kmr_xcb_destroy: frees all allocated memory contained in struct kmsxcb
 *
 * parameters:
 * @kmsxcb - pointer to a struct kmsxcb_destroy contains all objects
 *           created during window lifetime in need of destruction
 */
void kmr_xcb_destroy(struct kmr_xcb_destroy *kmsxcb);

#endif
