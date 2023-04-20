/* NOTE: Most Comments taken from wlroots tinwyl implementation */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/log.h>
#include "wserver.h"

/*
 * struct uvr_ws_output (Underview Renderer Wayland Server Output)
 *
 * members:
 * @link                  - Represents the sentinel head of a output device. Essential the head
 *                          of a doubly-linked list.
 * @core                  - Pointer to a struct uvr_ws_core
 * @wlrOutput             -
 * @frameListener         - Points to function that is called every time an output wants to display a frame.
 * @requestStateListener  - Points to function that handles request coming from backend
 * @destroyListener       - Points to function that destroys the output device node in the doubly-linked list
 *                          and free's any allocated memory for that particular node.
 */
struct uvr_ws_output {
	struct wl_list     link;
	struct uvr_ws_core *core;
	struct wlr_output  *wlrOutput;
	struct wl_listener frameListener;
	struct wl_listener requestStateListener;
	struct wl_listener destroyListener;
};


/*
 * This function is called every time an output is ready to display a frame,
 * generally at the output's refresh rate (e.g. 60Hz).
 */
static void server_output_device_handle_frame(struct wl_listener *listener, void UNUSED *data)
{
	struct uvr_ws_output *output = wl_container_of(listener, output, frameListener);
	struct wlr_scene_output *sceneOutput = wlr_scene_get_scene_output(output->core->wlrSceneGraph, output->wlrOutput);

	/* Render the scene if needed and commit the output */
	wlr_scene_output_commit(sceneOutput);

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	wlr_scene_output_send_frame_done(sceneOutput, &now);
}


/*
 * This function is called when the backend requests a new state for
 * the output. For example, Wayland and X11 backends request a new mode
 * when the output window is resized.
 */
static void server_output_device_handle_state_request(struct wl_listener *listener, void *data)
{
	struct uvr_ws_output *output = wl_container_of(listener, output, requestStateListener);
	const struct wlr_output_event_request_state *event = data;
	wlr_output_commit_state(output->wlrOutput, event->state);
}


static void server_output_device_destroy(struct wl_listener *listener, void UNUSED *data)
{
	struct uvr_ws_output *output = wl_container_of(listener, output, destroyListener);

	wl_list_remove(&output->frameListener.link);
	wl_list_remove(&output->requestStateListener.link);
	wl_list_remove(&output->destroyListener.link);
	wl_list_remove(&output->link);
	free(output);
}


/*
 * This event is raised by the backend when a new output
 * (aka a display or monitor) becomes available.
 */
static void server_output_add_new_device(struct wl_listener *listener, void *data)
{
	struct uvr_ws_core *core = wl_container_of(listener, core, newOutputListener);
	struct wlr_output *wlrOutput = data;

	/*
	 * Configures the output created by the backend to use our allocator
	 * and our renderer. Must be done once, before commiting the output.
	 */
	wlr_output_init_render(wlrOutput, core->wlrRendererAllocator, core->wlrRenderer);

	/*
	 * Some backends don't have modes. DRM+KMS does, and we need to set a mode
	 * before we can use the output. The mode is a tuple of (width, height,
	 * refresh rate), and each monitor supports only a specific set of modes. We
	 * just pick the monitor's preferred mode, a more sophisticated compositor
	 * would let the user configure it.
	 */
	if (!wl_list_empty(&wlrOutput->modes)) {
		struct wlr_output_mode *mode = wlr_output_preferred_mode(wlrOutput);
		if (mode) {
			wlr_output_set_mode(wlrOutput, mode);
			wlr_output_enable(wlrOutput, true);
			if (!wlr_output_commit(wlrOutput))
				return;
		}
	}

	/* Allocates and configures our state for this output */
	struct uvr_ws_output *output = calloc(1, sizeof(struct uvr_ws_output));
	if (!output) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		return;
	}

	output->wlrOutput = wlrOutput;
	output->core = core;

	/* Sets up a listener for the frame event. */
	output->frameListener.notify = server_output_device_handle_frame;
	wl_signal_add(&wlrOutput->events.frame, &output->frameListener);

	/* Sets up a listener for the state request event. */
	output->requestStateListener.notify = server_output_device_handle_state_request;
	wl_signal_add(&wlrOutput->events.request_state, &output->requestStateListener);

	/* Sets up a listener for the destroy event. */
	output->destroyListener.notify = server_output_device_destroy;
	wl_signal_add(&wlrOutput->events.destroy, &output->destroyListener);

	/* Insert new display output to end of list */
	wl_list_insert(&core->outputList, &output->link);

	/*
	 * Adds this to the output layout. The add_auto function arranges outputs
	 * from left-to-right in the order they appear. A more sophisticated
	 * compositor would let the user configure the arrangement of outputs in the
	 * layout.
	 *
	 * The output layout utility automatically adds a wl_output global to the
	 * display, which Wayland clients can see to find out information about the
	 * output (such as DPI, scale factor, manufacturer, etc).
	 */
	wlr_output_layout_add_auto(core->wlrOutputLayout, wlrOutput);
}


struct uvr_ws_core uvr_ws_core_create(struct uvr_ws_core_create_info *uvrws)
{
	if (uvrws->includeWlrDebugLogs)
		wlr_log_init(WLR_DEBUG, NULL);

	struct uvr_ws_core core;
	memset(&core, 0, sizeof(core));

	/*
	 * The Wayland display is managed by libwayland. It handles accepting
	 * clients from the Unix socket, managing Wayland globals, and so on.
	 */
	core.wlDisplay = wl_display_create();
	if (!core.wlDisplay) {
		uvr_utils_log(UVR_DANGER, "[x] wl_display_create: Failed to create unix socket.");
		goto exit_ws_core;
	}

	core.wlrBackend = wlr_backend_autocreate(core.wlDisplay, NULL);
	if (!core.wlrBackend) {
		uvr_utils_log(UVR_DANGER, "[x] wlr_backend_autocreate: Failed to create backend {DRM/KMS, Wayland, X, Headless}");
		goto exit_ws_core_wl_display_destroy;
	}

	// TODO: Implement custom renderer
	core.wlrRenderer = wlr_renderer_autocreate(core.wlrBackend);
	if (!core.wlrRenderer) {
		uvr_utils_log(UVR_DANGER, "[X] wlr_renderer_autocreate: failed to create wlr_renderer");
		goto exit_ws_core_wlr_backend_destroy;
	}

	wlr_renderer_init_wl_display(core.wlrRenderer, core.wlDisplay);

	/* Bridge between the renderer and the backend that handles the buffer creation */
	core.wlrRendererAllocator = wlr_allocator_autocreate(core.wlrBackend, core.wlrRenderer);
	if (!core.wlrRendererAllocator) {
		uvr_utils_log(UVR_DANGER, "[X] wlr_allocator_autocreate: failed to create wlr_allocator");
		goto exit_ws_core_wlr_renderer_destroy;
	}

	/*
	 * This creates some hands-off wlroots interfaces viewable by the client. The
	 * wl_compositor interface is necessary for clients to allocate surfaces, the
	 * wl_subcompositor interface allows to assign the role of subsurfaces to surfaces,
	 * and the data device manager handles the clipboard. Note: that the clients
	 * cannot set the selection directly without compositor approval, see the
	 * handling of the request_set_selection event below.
	 */
	wlr_compositor_create(core.wlDisplay, 5, core.wlrRenderer);
	wlr_subcompositor_create(core.wlDisplay);
	wlr_data_device_manager_create(core.wlDisplay);

	core.wlrOutputLayout = wlr_output_layout_create();
	if (!core.wlrOutputLayout) {
		uvr_utils_log(UVR_DANGER, "[X] wlr_output_layout_create: failed to create wlr_output_layout");
		goto exit_ws_core_wlr_renderer_allocator_destroy;
	}

	/* Configure a listener to be notified when new outputs are available on the backend. */
	wl_list_init(&core.outputList); // Initailize Doubly Linked List
	core.newOutputListener.notify = server_output_add_new_device;
	wl_signal_add(&core.wlrBackend->events.new_output, &core.newOutputListener);

	/*
	 * Create a scene graph. This is a wlroots abstraction that handles all
	 * rendering and damage tracking.
	 */
	core.wlrSceneGraph = wlr_scene_create();
	if (!core.wlrSceneGraph) {
		uvr_utils_log(UVR_DANGER, "[X] wlr_scene_create: failed to create wlr_scene");
		goto exit_ws_core_wlr_output_layout_destroy;
	}

	wlr_scene_attach_output_layout(core.wlrSceneGraph, core.wlrOutputLayout);

	/* Add a Unix socket to the Wayland display. */
	const char *socket = NULL;
	if (uvrws->unixSockName) {
		/* If NULL passed function fails */
		if (wl_display_add_socket(core.wlDisplay, uvrws->unixSockName) == -1) {
			uvr_utils_log(UVR_DANGER, "[x] wl_display_add_socket: Unable to create unix socket %s for Wayland display.", uvrws->unixSockName);
			goto exit_ws_core_wlr_output_layout_destroy;
		}
	} else {
		socket = wl_display_add_socket_auto(core.wlDisplay);
		if (!socket) {
			uvr_utils_log(UVR_DANGER, "[x] wl_display_add_socket_auto: Unable to create unix socket for Wayland display.");
			goto exit_ws_core_wlr_output_layout_destroy;
		}
	}

	char *xdgRuntimeDir = getenv("XDG_RUNTIME_DIR");
	int unixSockPathLen = strnlen(xdgRuntimeDir, 64) + strnlen(uvrws->unixSockName ? uvrws->unixSockName : socket, 64) + 1;
	char *unixSocket = alloca(unixSockPathLen);

	strncpy(unixSocket, xdgRuntimeDir, strnlen(xdgRuntimeDir, unixSockPathLen));
	strncat(unixSocket, "/", 2);
	strncat(unixSocket, uvrws->unixSockName ? uvrws->unixSockName : socket,
	                    strnlen(uvrws->unixSockName ? uvrws->unixSockName : socket, unixSockPathLen));

	uvr_utils_log(UVR_SUCCESS, "Succefully established socket for wayland clients to connect at %s", unixSocket);

	core.kmsfd = wlr_backend_get_drm_fd(core.wlrBackend);

	/*
	 * Set the WAYLAND_DISPLAY environment variable to our socket
	 */
	setenv("WAYLAND_DISPLAY", uvrws->unixSockName ? uvrws->unixSockName : socket, true);

	return core;

exit_ws_core_wlr_output_layout_destroy:
	if (core.wlrOutputLayout)
		wlr_output_layout_destroy(core.wlrOutputLayout);
exit_ws_core_wlr_renderer_allocator_destroy:
	if (core.wlrRendererAllocator)
		wlr_allocator_destroy(core.wlrRendererAllocator);
exit_ws_core_wlr_renderer_destroy:
	if (core.wlrRenderer)
		wlr_renderer_destroy(core.wlrRenderer);
exit_ws_core_wlr_backend_destroy:
	if (core.wlrBackend)
		wlr_backend_destroy(core.wlrBackend);
exit_ws_core_wl_display_destroy:
	if (core.wlDisplay)
		wl_display_destroy(core.wlDisplay);
exit_ws_core:
	return (struct uvr_ws_core) { .kmsfd = -1, .wlDisplay = NULL, .wlrBackend = NULL, .wlrRenderer = NULL,
	                              .wlrRendererAllocator = NULL, .wlrOutputLayout = NULL };
}


void uvr_ws_destroy(struct uvr_ws_destroy *uvrws)
{
	if (uvrws->uvr_ws_core.wlDisplay) {
		wl_display_terminate(uvrws->uvr_ws_core.wlDisplay);
		wl_display_destroy_clients(uvrws->uvr_ws_core.wlDisplay);
	}
	if (uvrws->uvr_ws_core.wlrOutputLayout)
		wlr_output_layout_destroy(uvrws->uvr_ws_core.wlrOutputLayout);
	if (uvrws->uvr_ws_core.wlrRendererAllocator)
		wlr_allocator_destroy(uvrws->uvr_ws_core.wlrRendererAllocator);
	if (uvrws->uvr_ws_core.wlrRenderer)
		wlr_renderer_destroy(uvrws->uvr_ws_core.wlrRenderer);
	if (uvrws->uvr_ws_core.wlrBackend)
		wlr_backend_destroy(uvrws->uvr_ws_core.wlrBackend);
	if (uvrws->uvr_ws_core.wlDisplay)
		wl_display_destroy(uvrws->uvr_ws_core.wlDisplay);
}
