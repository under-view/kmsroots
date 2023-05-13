#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <xcb/xcb_ewmh.h>

#include "xclient.h"


struct kmr_xcb_window kmr_xcb_window_create(struct kmr_xcb_window_create_info *kmsxcb)
{
	xcb_connection_t *conn = NULL;
	xcb_window_t window;

	const xcb_setup_t *xcbsetup = NULL;
	xcb_screen_t *screen = NULL;

	/* Connect to Xserver */
	conn = xcb_connect(kmsxcb->display, kmsxcb->screen);
	if (xcb_connection_has_error(conn)) {
		kmr_utils_log(KMR_DANGER, "[x] xcb_connect: xcb_connect Failed");
		if (!kmsxcb->display)
			kmr_utils_log(KMR_INFO, "Try setting the DISPLAY environment variable");
		goto error_exit_xcb_window_create;
	}

	/* access properties of the X server and its display environment */
	xcbsetup = xcb_get_setup(conn);
	if (!xcbsetup) {
		kmr_utils_log(KMR_DANGER, "[x] xcb_get_setup: Failed to access properties of the X server and its display environment");
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
		kmr_utils_log(KMR_DANGER, "[x] xcb_setup_roots_iterator: Failed to retrieve connected screen");
		goto error_exit_xcb_window_destroy;
	}

	/* Create window */
	uint32_t prop_name = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t prop_value[2] = { screen->black_pixel, XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_KEY_PRESS };

	xcb_create_window(conn, screen->root_depth, window, screen->root, 0,
	                  0, kmsxcb->width, kmsxcb->height, 0,
	                  XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual,
	                  prop_name, prop_value);

	/* Change window property name */
	xcb_change_property(conn, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME,
			    XCB_ATOM_STRING, 8, strnlen(kmsxcb->appName, 60), kmsxcb->appName);

	/* Change window property to make fullscreen */
	if (kmsxcb->fullscreen) {
		xcb_ewmh_connection_t xcbewmh = {};
		xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(conn, &xcbewmh);
		if (!xcb_ewmh_init_atoms_replies(&xcbewmh, cookie, NULL)) {
			kmr_utils_log(KMR_DANGER, "[x] xcb_ewmh_init_atoms_replies: Failed");
			goto error_exit_xcb_window_destroy;
		}

		xcb_change_property(conn, XCB_PROP_MODE_PREPEND, window,
		                    xcbewmh._NET_WM_STATE, XCB_ATOM_ATOM, 32, 1,
		                    &(xcbewmh._NET_WM_STATE_FULLSCREEN));
	}

	/* Change window property to make window fully transparent */
	if (kmsxcb->transparent) {
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

	return (struct kmr_xcb_window) { .conn = conn, .window = window, .delWindow = windeletereply };

error_exit_xcb_window_destroy:
	if (window)
		xcb_destroy_window(conn, window);
error_exit_xcb_window_disconnect:
	if (conn)
		xcb_disconnect(conn);
error_exit_xcb_window_create:
	return (struct kmr_xcb_window) { .conn = NULL, .window = 0, .delWindow = 0 };
}


void kmr_xcb_window_make_visible(struct kmr_xcb_window *kmsxcb)
{
	xcb_map_window(kmsxcb->conn, kmsxcb->window);
	xcb_flush(kmsxcb->conn);
}


int kmr_xcb_window_handle_event(struct kmr_xcb_window_handle_event_info *kmsxcb)
{
	xcb_generic_event_t *event = NULL;

	event = xcb_poll_for_event(kmsxcb->xcbWindowObject->conn);
	if (!event) {
		goto exit_xcb_window_event_loop;
	}

	switch (event->response_type & ~0x80) {
		case XCB_KEY_PRESS:
		{
			xcb_key_press_event_t *press = (xcb_key_press_event_t *) event;
			//kmr_utils_log(KMR_INFO, "kmr_xcb_window_wait_for_event: Key pressed %d", press->detail);
			/* ESCAPE key, Q key */
			if (press->detail == 9 || press->detail == 24) {
				kmsxcb->rendererRunning = false;
				goto error_exit_xcb_window_event_loop;
			}
			break;
		}
		case XCB_CLIENT_MESSAGE:
		{
			xcb_client_message_event_t *message = (xcb_client_message_event_t *) event;
			if (message->data.data32[0] == kmsxcb->xcbWindowObject->delWindow->atom) {
				kmsxcb->rendererRunning = false;
				goto error_exit_xcb_window_event_loop;
			}
			break;
		}
		default: /* Unknown event type, ignore it */
			break;
  }

exit_xcb_window_event_loop:
	/* Execute application defined renderer */
	kmsxcb->renderer(kmsxcb->rendererRunning, kmsxcb->rendererCurrentBuffer, kmsxcb->rendererData);
	free(event);
	return 1;

error_exit_xcb_window_event_loop:
	free(event);
	return 0;
}


void kmr_xcb_destroy(struct kmr_xcb_destroy *kmsxcb)
{
	free(kmsxcb->kmr_xcb_window.delWindow);
	if (kmsxcb->kmr_xcb_window.window)
		xcb_destroy_window(kmsxcb->kmr_xcb_window.conn, kmsxcb->kmr_xcb_window.window);
	if (kmsxcb->kmr_xcb_window.conn)
		xcb_disconnect(kmsxcb->kmr_xcb_window.conn);
}
