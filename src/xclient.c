#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <xcb/xcb_ewmh.h>

#include "xclient.h"


struct kmr_xcb_window *
kmr_xcb_window_create (struct kmr_xcb_window_create_info *xcbWindowInfo)
{
	int ret = 0;

	uint32_t prop_value[2];
	uint32_t prop_name = 0, opacity = 0;

	const xcb_setup_t *xcbSetup = NULL;
	xcb_screen_t *screen = NULL;

	xcb_ewmh_connection_t xcbEWMH = {};
	xcb_intern_atom_cookie_t *cookie = NULL;

	xcb_intern_atom_cookie_t windowOpacity;
	xcb_intern_atom_reply_t *windowOpacityReply = NULL;

	xcb_intern_atom_cookie_t windowDelete;
	xcb_intern_atom_cookie_t windowProtosCookie;
	xcb_intern_atom_reply_t *windowDeleteReply = NULL;
	xcb_intern_atom_reply_t *windowProtosReply = NULL;

	struct kmr_xcb_window *xcb = NULL;

	memset(prop_value, 0, sizeof(prop_value));

	xcb = calloc(1, sizeof(struct kmr_xcb_window));
	if (!xcb) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto error_exit_xcb_window_create;
	}

	/* Connect to Xserver */
	xcb->conn = xcb_connect(xcbWindowInfo->display, xcbWindowInfo->screen);
	ret = xcb_connection_has_error(xcb->conn);
	if (ret) {
		kmr_utils_log(KMR_DANGER, "[x] xcb_connect: xcb_connect Failed");
		if (!xcbWindowInfo->display)
			kmr_utils_log(KMR_INFO, "Try setting the DISPLAY environment variable");
		goto error_exit_xcb_window_create;
	}

	/* access properties of the X server and its display environment */
	xcbSetup = xcb_get_setup(xcb->conn);
	if (!xcbSetup) {
		kmr_utils_log(KMR_DANGER, "[x] xcb_get_setup: Failed to access properties of the X server and its display environment");
		goto error_exit_xcb_window_create;
	}

	/* Retrieve XID */
	xcb->window = xcb_generate_id(xcb->conn);

	/*
	 * xcb_screen_iterator_t structure has a data field of type
	 * xcb_screen_t*, which represents the screen.
	 * Retrieve the first screen of the connection.
	 * Once we have the screen we can then create a window.
	 */
	screen = xcb_setup_roots_iterator(xcbSetup).data;
	if (!screen) {
		kmr_utils_log(KMR_DANGER, "[x] xcb_setup_roots_iterator: Failed to retrieve connected screen");
		goto error_exit_xcb_window_create;
	}

	/* Create window */
	prop_name = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	prop_value[0] = screen->black_pixel;
	prop_value[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_KEY_PRESS;

	xcb_create_window(xcb->conn, screen->root_depth, xcb->window, screen->root, 0,
	                  0, xcbWindowInfo->width, xcbWindowInfo->height, 0,
	                  XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
	                  prop_name, prop_value);

	/* Change window property name */
	xcb_change_property(xcb->conn, XCB_PROP_MODE_REPLACE, xcb->window, XCB_ATOM_WM_NAME,
			    XCB_ATOM_STRING, 8, strnlen(xcbWindowInfo->appName, 60),
	                    xcbWindowInfo->appName);

	/* Change window property to make fullscreen */
	if (xcbWindowInfo->fullscreen) {
		cookie = xcb_ewmh_init_atoms(xcb->conn, &xcbEWMH);
		if (!xcb_ewmh_init_atoms_replies(&xcbEWMH, cookie, NULL)) {
			kmr_utils_log(KMR_DANGER, "[x] xcb_ewmh_init_atoms_replies: Failed");
			goto error_exit_xcb_window_create;
		}

		xcb_change_property(xcb->conn, XCB_PROP_MODE_PREPEND, xcb->window,
		                    xcbEWMH._NET_WM_STATE, XCB_ATOM_ATOM, 32, 1,
		                    &(xcbEWMH._NET_WM_STATE_FULLSCREEN));
	}

	/* Change window property to make window fully transparent */
	if (xcbWindowInfo->transparent) {
		opacity = -1 * 0xffffffff;
		windowOpacity = xcb_intern_atom(xcb->conn, 0, strlen("_NET_WM_WINDOW_OPACITY"), "_NET_WM_WINDOW_OPACITY");
		windowOpacityReply = xcb_intern_atom_reply(xcb->conn, windowOpacity, NULL);
		xcb_change_property(xcb->conn, XCB_PROP_MODE_REPLACE,
		                    xcb->window, windowOpacityReply->atom,
		                    XCB_ATOM_CARDINAL, 32, 1L, &opacity);
		free(windowOpacityReply);
	}

	/* Instructs the XServer to watch when the window manager attempts to destroy the window. */
	windowDelete = xcb_intern_atom(xcb->conn, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
	windowProtosCookie = xcb_intern_atom(xcb->conn, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
	windowDeleteReply = xcb_intern_atom_reply(xcb->conn, windowDelete, NULL);
	windowProtosReply = xcb_intern_atom_reply(xcb->conn, windowProtosCookie, NULL);
	xcb_change_property(xcb->conn, XCB_PROP_MODE_REPLACE, xcb->window,
	                    windowProtosReply->atom, 4, 32, 1, &windowDeleteReply->atom);
	free(windowProtosReply);

	xcb->delWindow = windowDeleteReply;

	return xcb;

error_exit_xcb_window_create:
	kmr_xcb_window_destroy(xcb);
	return NULL;
}


void
kmr_xcb_window_destroy(struct kmr_xcb_window *xcb)
{
	if (!xcb)
		return;

	free(xcb->delWindow);

	if (xcb->window)
		xcb_destroy_window(xcb->conn, xcb->window);
	if (xcb->conn)
		xcb_disconnect(xcb->conn);

	free(xcb);
}


void
kmr_xcb_window_make_visible (struct kmr_xcb_window *xcb)
{
	xcb_map_window(xcb->conn, xcb->window);
	xcb_flush(xcb->conn);
}


int
kmr_xcb_window_handle_event (struct kmr_xcb_window_handle_event_info *xcbEventInfo)
{
	xcb_generic_event_t *event = NULL;
	xcb_key_press_event_t *press = NULL;
	xcb_client_message_event_t *message = NULL;

	event = xcb_poll_for_event(xcbEventInfo->xcbWindowObject->conn);
	if (!event) {
		goto exit_xcb_window_event_loop;
	}

	switch (event->response_type & ~0x80) {
		case XCB_KEY_PRESS:
		{
			press = (xcb_key_press_event_t *) event;

			/* ESCAPE key, Q key */
			if (press->detail == 9 || press->detail == 24) {
				xcbEventInfo->rendererRunning = false;
				goto error_exit_xcb_window_event_loop;
			}
			break;
		}
		case XCB_CLIENT_MESSAGE:
		{
			message = (xcb_client_message_event_t *) event;

			if (message->data.data32[0] == xcbEventInfo->xcbWindowObject->delWindow->atom) {
				xcbEventInfo->rendererRunning = false;
				goto error_exit_xcb_window_event_loop;
			}
			break;
		}
		default: /* Unknown event type, ignore it */
			break;
  }

exit_xcb_window_event_loop:
	/* Execute application defined renderer */
	xcbEventInfo->renderer(xcbEventInfo->rendererRunning,
	                       xcbEventInfo->rendererCurrentBuffer,
	                       xcbEventInfo->rendererData);
	free(event);
	return 1;

error_exit_xcb_window_event_loop:
	free(event);
	return 0;
}
