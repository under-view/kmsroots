#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#ifdef __linux__
#include <linux/input-event-codes.h>
#elif __FreeBSD__
#include <dev/evdev/input-event-codes.h>
#endif

#include "fullscreen-shell-unstable-v1-client-protocol.h"

/* https://wayland.app/protocols/xdg-shell */
#include "xdg-shell-client-protocol.h"
#include "wclient.h"

/*****************************
 * START OF GLOBAL VARIABLES *
 *****************************/

static int xdgWmBaseVersion = 0;

/***************************
 * END OF GLOBAL VARIABLES *
 ***************************/


/***************************************************
 * START OF kmr_wc_core_{create,destroy} FUNCTIONS *
 ***************************************************/

static void
noop ()
{
	// This space intentionally left blank
}


/* Hint to compositor that client hasn't become deadlocked */
static void
xdg_wm_base_ping (void UNUSED *data,
                  struct xdg_wm_base *xdg_wm_base,
                  uint32_t serial)
{
	xdg_wm_base_pong(xdg_wm_base, serial);
}


static const struct xdg_wm_base_listener xdg_wm_base_listener = {
	.ping = xdg_wm_base_ping,
};


/*
 * Get information about extensions and versions of the Wayland API that the server supports.
 * Binds other wayland server global objects to a client. These objects have interfaces that define
 * possible requests and events.
 */
static void
registry_handle_global (void *data,
                        struct wl_registry *registry,
                        uint32_t name,
                        const char *interface,
                        uint32_t version)
{
	kmr_utils_log(KMR_INFO, "interface: '%s', version: %d, name: %d", interface, version, name);

	struct kmr_wc_core *core = (struct kmr_wc_core *) data;

	if (core->iType & KMR_WC_INTERFACE_WL_COMPOSITOR) {
		if (!strcmp(interface, wl_compositor_interface.name)) {
			core->wlCompositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);
		}
	}

	if (core->iType & KMR_WC_INTERFACE_XDG_WM_BASE) {
		if (!strcmp(interface, xdg_wm_base_interface.name)) {
			core->xdgWmBase = wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
			xdg_wm_base_add_listener(core->xdgWmBase, &xdg_wm_base_listener, NULL);
		}
	}

	if (core->iType & KMR_WC_INTERFACE_WL_SHM) {
		if (!strcmp(interface, wl_shm_interface.name)) {
			core->wlShm = wl_registry_bind(registry, name, &wl_shm_interface, version);
			xdgWmBaseVersion = version;
		}
	}

	if (core->iType & KMR_WC_INTERFACE_WL_SEAT) {
		if (!strcmp(interface, wl_seat_interface.name)) {
			core->wlSeat = wl_registry_bind(registry, name, &wl_seat_interface, version);
		}
	}

	if (core->iType & KMR_WC_INTERFACE_ZWP_FULLSCREEN_SHELL_V1) {
		if (!strcmp(interface, "zwp_fullscreen_shell_v1")) {
			core->fullScreenShell = wl_registry_bind(registry, name, &zwp_fullscreen_shell_v1_interface, version);
		}
	}
}


/*
 * Due to device (i.e monitor) hotplugging globals may come and go.
 * The wayland server global registry object will send out global
 * and global_remove events to keep the wayland client up to date
 * with changes.
 */
static const struct wl_registry_listener registry_listener = {
	.global = registry_handle_global,
	.global_remove = noop,
};


struct kmr_wc_core *
kmr_wc_core_create (struct kmr_wc_core_create_info *coreInfo)
{
	struct kmr_wc_core *core = NULL;

	core = calloc(1, sizeof(struct kmr_wc_core));
	if (!core) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_wc_core_create;
	}

	core->iType = coreInfo->iType;

	/* Connect to wayland server */
	core->wlDisplay = wl_display_connect(coreInfo->displayName);
	if (!core->wlDisplay) {
		kmr_utils_log(KMR_DANGER, "[x] wl_display_connect: Failed to connect to Wayland display.");
		goto exit_error_kmr_wc_core_create;
	}

	kmr_utils_log(KMR_SUCCESS, "Connection to Wayland display established.");

	/*
	 * Emit a global event to retrieve each global object currently supported/in action by the server.
	 */
	core->wlRegistry = wl_display_get_registry(core->wlDisplay);
	if (!core->wlRegistry) {
		kmr_utils_log(KMR_DANGER, "[x] wl_display_get_registry: Failed to set global objects/interfaces");
		goto exit_error_kmr_wc_core_create;
	}

	/*
	 * Bind the wl_registry_listener to a given registry. So that we can implement
	 * how we handle events comming from the wayland server.
	 */
	wl_registry_add_listener(core->wlRegistry, &registry_listener, core);

	/* synchronously wait for the server respondes */
	if (!wl_display_roundtrip(core->wlDisplay))
		goto exit_error_kmr_wc_core_create;

	return core;

exit_error_kmr_wc_core_create:
	kmr_wc_core_destroy(core);
	return NULL;
}


void
kmr_wc_core_destroy (struct kmr_wc_core *core)
{
	if (!core)
		return;

	if (core->wlSeat)
		wl_seat_destroy(core->wlSeat);
	if (core->wlShm)
		wl_shm_destroy(core->wlShm);
	if (core->xdgWmBase)
		xdg_wm_base_destroy(core->xdgWmBase);
	if (core->fullScreenShell)
		zwp_fullscreen_shell_v1_release(core->fullScreenShell);
	if (core->wlCompositor)
		wl_compositor_destroy(core->wlCompositor);
	if (core->wlRegistry)
		wl_registry_destroy(core->wlRegistry);
	if (core->wlDisplay) {
		wl_display_flush(core->wlDisplay);
		wl_display_disconnect(core->wlDisplay);
	}

	free(core);
}

/*************************************************
 * END OF kmr_wc_core_{create,destroy} FUNCTIONS *
 *************************************************/


/*****************************************************
 * START OF kmr_wc_buffer_{create,destroy} FUNCTIONS *
 *****************************************************/

struct kmr_wc_buffer *
kmr_wc_buffer_create (struct kmr_wc_buffer_create_info *bufferInfo)
{
	int c, stride = 0;

	/*
	 * @shm_pool - Object used to encapsulate a piece of memory shared between the compositor and client.
	 *             @buffers (wl_buffer) can be created from shm_pool.
	 */
	struct wl_shm_pool *shmPool = NULL;

	struct kmr_wc_buffer *buffer = NULL;
	struct kmr_wc_shm_buffer *shmBufferObjects = NULL;
	struct kmr_wc_buffer_handle *wlBufferHandles = NULL;

	buffer = calloc(1, sizeof(struct kmr_wc_buffer));
	if (!buffer) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_wc_buffer_create;
	}

	if (!bufferInfo->core->wlShm) {
		kmr_utils_log(KMR_DANGER, "[x] kmr_wc_buffer_create(!bufferInfo->core->wlShm): Must bind the wl_shm interface");
		goto exit_error_kmr_wc_buffer_create;
	}

	shmBufferObjects = calloc(bufferInfo->bufferCount, sizeof(struct kmr_wc_shm_buffer));
	if (!shmBufferObjects) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_wc_buffer_create;
	}

	buffer->shmBufferObjects = shmBufferObjects;

	wlBufferHandles = calloc(bufferInfo->bufferCount, sizeof(struct kmr_wc_buffer_handle));
	if (!wlBufferHandles) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_wc_buffer_create;
	}

	buffer->wlBufferHandles = wlBufferHandles;

	stride = bufferInfo->width * bufferInfo->bytesPerPixel;

	for (c = 0; c < bufferInfo->bufferCount; c++) {
		shmBufferObjects[c].shmPoolSize = stride * bufferInfo->height;
		shmBufferObjects[c].shmFd = -1;

		/* Create POSIX shared memory file of size shm_pool_size */
		shmBufferObjects[c].shmFd = allocate_shm_file(shmBufferObjects[c].shmPoolSize);
		if (shmBufferObjects[c].shmFd == -1) {
			kmr_utils_log(KMR_DANGER, "[x] allocate_shm_file: failed to create file discriptor");
			goto exit_error_kmr_wc_buffer_create;
		}

		/* associate shared memory address range with open file */
		shmBufferObjects[c].shmPoolData = mmap(NULL,
		                                       shmBufferObjects[c].shmPoolSize,
						       PROT_READ | PROT_WRITE,
						       MAP_SHARED,
						       shmBufferObjects[c].shmFd,
						       0);
		if (shmBufferObjects[c].shmPoolData == (void*)-1) {
			kmr_utils_log(KMR_DANGER, "[x] mmap: %s", strerror(errno));
			goto exit_error_kmr_wc_buffer_create;
		}

		/* Create pool of memory shared between client and compositor */
		shmPool = wl_shm_create_pool(bufferInfo->core->wlShm,
		                             shmBufferObjects[c].shmFd,
					     shmBufferObjects[c].shmPoolSize);
		if (!shmPool) {
			kmr_utils_log(KMR_DANGER, "[x] wl_shm_create_pool: failed to create wl_shm_pool");
			goto exit_error_kmr_wc_buffer_create;
		}

		wlBufferHandles[c].buffer = wl_shm_pool_create_buffer(shmPool,
		                                                      0,
								      bufferInfo->width,
								      bufferInfo->height,
								      stride,
								      bufferInfo->pixelFormat);
		if (!wlBufferHandles[c].buffer) {
			kmr_utils_log(KMR_DANGER, "[x] wl_shm_pool_create_buffer: failed to create wl_buffer from a wl_shm_pool");
			goto exit_error_kmr_wc_buffer_create;
		}

		/* Can destroy shm pool after creating buffer */
		wl_shm_pool_destroy(shmPool);
		shmPool = NULL;
	}

	return buffer;

exit_error_kmr_wc_buffer_create:
	if (shmPool)
		wl_shm_pool_destroy(shmPool);
	kmr_wc_buffer_destroy(buffer);
	return NULL;
}


void
kmr_wc_buffer_destroy (struct kmr_wc_buffer *buffer)
{
	int i;

	if (!buffer)
		return;

	for (i = 0; i < buffer->bufferCount; i++) {
		if (buffer->shmBufferObjects[i].shmFd != -1)
			close(buffer->shmBufferObjects[i].shmFd);
		if (buffer->shmBufferObjects[i].shmPoolData)
			munmap(buffer->shmBufferObjects[i].shmPoolData, buffer->shmBufferObjects[i].shmPoolSize);
		if (buffer->wlBufferHandles[i].buffer)
			wl_buffer_destroy(buffer->wlBufferHandles[i].buffer);
	}

	free(buffer->shmBufferObjects);
	free(buffer->wlBufferHandles);
	free(buffer);
}

/***************************************************
 * END OF kmr_wc_buffer_{create,destroy} FUNCTIONS *
 ***************************************************/


/******************************************************
 * START OF kmr_wc_surface_{create,destroy} FUNCTIONS *
 ******************************************************/

/*
 * struct kmr_wc_renderer_info (kmsroots Wayland Client Render Information)
 *
 * members:
 * @core                  - Pointer to a struct kmr_wc_core contains all client choosen global objects defined in wayland.xml.
 * @surface               - Pointer to a struct kmr_wc_surface used to get wl_surface object and buffers associate with surface.
 * @renderer              - Function pointer that allows custom external renderers to be executed by the api before registering
 *                          a frame wl_callback.
 * @rendererData          - Pointer to an address that can be optional this address is passed to the external render function.
 *                          This address may be the address of a struct. Reference passed depends on external render function.
 * @rendererCurrentBuffer - Pointer to an integer used by the api to update the current displayable buffer
 * @rendererRunning       - Pointer to a boolean indicating that surface/window is active
 * @waitForConfigure      - Boolean to help gate draw frame call. One should not attach buffer to surface before
 *                          first xdg_surface.configure event it recieved.
 */
struct kmr_wc_renderer_info {
	struct kmr_wc_core    *core;
	struct kmr_wc_surface *surface;
	kmr_wc_renderer_impl  renderer;
	void                  *rendererData;
	uint8_t               *rendererCurrentBuffer;
	volatile bool         *rendererRunning;
	bool                  waitForConfigure;
};


static void
drawframe (void *data,
           struct wl_callback *callback,
	   uint32_t time);


static void
xdg_surface_handle_configure (void *data,
                              struct xdg_surface *xdg_surface,
			      uint32_t serial)
{
	struct kmr_wc_renderer_info *rendererInfo = (struct kmr_wc_renderer_info *) data;
	xdg_surface_ack_configure(xdg_surface, serial);

	/*
	 * The xdg_wm_base protocol requires one waits until the first xdg_surface.configure
	 * event is received before attaching a buffer to a surface.
	 */
	if (rendererInfo->waitForConfigure) {
		drawframe(data, NULL, 0);
		rendererInfo->waitForConfigure = true;
	}
}


static const struct xdg_surface_listener xdg_surface_listener = {
	.configure = xdg_surface_handle_configure,
};


/* Handles what happens when a window is closed */
static void
xdg_toplevel_handle_close (void *data,
                           struct xdg_toplevel UNUSED *xdg_toplevel)
{
	struct kmr_wc_renderer_info *rendererInfo = (struct kmr_wc_renderer_info *) data;
	*rendererInfo->rendererRunning = false;
}


static struct xdg_toplevel_listener xdg_toplevel_listener;


static const struct wl_callback_listener frame_listener;


static void 
drawframe (void *data,
           struct wl_callback *callback,
	   uint32_t UNUSED time)
{
	struct kmr_wc_renderer_info *rendererInfo = (struct kmr_wc_renderer_info *) data;
	struct kmr_wc_surface *surface = rendererInfo->surface;

	if (callback)
		wl_callback_destroy(callback);

	/* Register a frame callback to know when we need to draw the next frame */
	surface->wlCallback = wl_surface_frame(surface->wlSurface);
	wl_callback_add_listener(surface->wlCallback, &frame_listener, rendererInfo);
	/* Submit a frame for this event */
	wl_surface_commit(surface->wlSurface);

	/* Execute application defined renderer */
	rendererInfo->renderer(rendererInfo->rendererRunning, rendererInfo->rendererCurrentBuffer, rendererInfo->rendererData);

	if (surface->wlBufferHandles)
		wl_surface_attach(surface->wlSurface, surface->wlBufferHandles[*rendererInfo->rendererCurrentBuffer].buffer, 0, 0);

	/* Indicate that the entire surface as changed and needs to be redrawn */
	wl_surface_damage_buffer(surface->wlSurface, 0, 0, INT32_MAX, INT32_MAX);

	/* Submit a frame for this event */
	wl_surface_commit(surface->wlSurface);

	if (surface->bufferCount)
		*rendererInfo->rendererCurrentBuffer = (*rendererInfo->rendererCurrentBuffer + 1) % surface->bufferCount;
}


static const struct wl_callback_listener frame_listener = {
	.done = drawframe,
};


struct kmr_wc_surface *
kmr_wc_surface_create (struct kmr_wc_surface_create_info *surfaceInfo)
{
	int ret = 0;

	struct kmr_wc_surface *surface = NULL;
	struct kmr_wc_renderer_info *rendererInfo = NULL;

	surface = calloc(1, sizeof(struct kmr_wc_surface));
	if (!surface) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_wc_surface_create;
	}

	rendererInfo = calloc(1, sizeof(struct kmr_wc_renderer_info));
	if (!rendererInfo) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_wc_surface_create;
	}

	if (surfaceInfo->buffer) {
		surface->bufferCount = surfaceInfo->buffer->bufferCount;
		surface->wlBufferHandles = surfaceInfo->buffer->wlBufferHandles;
	}

	/* Assign pointer and data that a given client suface needs to watch */
	surface->rendererInfo = rendererInfo;
	rendererInfo->core = surfaceInfo->core;
	rendererInfo->surface = surface;
	rendererInfo->renderer = surfaceInfo->renderer;
	rendererInfo->rendererData = surfaceInfo->rendererData;
	rendererInfo->rendererCurrentBuffer = surfaceInfo->rendererCurrentBuffer;
	rendererInfo->rendererRunning = surfaceInfo->rendererRunning;

	/* Use wl_compositor interface to create a wl_surface */
	surface->wlSurface = wl_compositor_create_surface(surfaceInfo->core->wlCompositor);
	if (!surface->wlSurface) {
		kmr_utils_log(KMR_DANGER, "[x] wl_compositor_create_surface: Can't create wl_surface interface");
		goto exit_error_wc_surface_create;
	}

	if (surfaceInfo->fullscreen && surfaceInfo->core->fullScreenShell) {
		zwp_fullscreen_shell_v1_present_surface(surfaceInfo->core->fullScreenShell,
		                                        surface->wlSurface,
							ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_DEFAULT,
							NULL);
	} else {
		/* Use xdg_wm_base interface and wl_surface just created to create an xdg_surface object */
		surface->xdgSurface = xdg_wm_base_get_xdg_surface(surfaceInfo->core->xdgWmBase, surface->wlSurface);
		if (!surface->xdgSurface) {
			kmr_utils_log(KMR_DANGER, "[x] xdg_wm_base_get_xdg_surface: Can't create xdg_surface interface");
			goto exit_error_wc_surface_create;
		}

		/* Create xdg_toplevel interface from which we manage application window from */
		surface->xdgToplevel = xdg_surface_get_toplevel(surface->xdgSurface);
		if (!surface->xdgToplevel) {
			kmr_utils_log(KMR_DANGER, "[x] xdg_surface_get_toplevel: Can't create xdg_toplevel interface");
			goto exit_error_wc_surface_create;
		}

		/*
		 * Bind the xdg_surface_listener to a given xdg_surface objects. So that we can implement
		 * how we handle events associate with object comming from the wayland server.
		 */
		ret = xdg_surface_add_listener(surface->xdgSurface, &xdg_surface_listener, rendererInfo);
		if (ret) {
			kmr_utils_log(KMR_DANGER, "[x] xdg_surface_add_listener: Failed");
			goto exit_error_wc_surface_create;
		}

		/*
		 * xdg_wm_base,xdg_toplevel interfaces are version specific. Adding new members to structs between
		 * version. Which can lead to bugs such as "listener function for opcode 2 of xdg_toplevel is NULL"
		 * Assign unimplemented members to noop so above bug doesn't occur.
		 */
		xdg_toplevel_listener.configure = noop;
		xdg_toplevel_listener.close = xdg_toplevel_handle_close;
		if (xdgWmBaseVersion <= 4)
			xdg_toplevel_listener.configure_bounds = noop;
		if (xdgWmBaseVersion <= 5)
			xdg_toplevel_listener.wm_capabilities = noop;

		/*
		 * Bind the xdg_toplevel_listener to a given xdg_toplevel objects. So that we can implement
		 * how we handle events associate with objects comming from the wayland server.
		 */
		ret = xdg_toplevel_add_listener(surface->xdgToplevel, &xdg_toplevel_listener, rendererInfo);
		if (ret) {
			kmr_utils_log(KMR_DANGER, "[x] xdg_toplevel_add_listener: Failed");
			goto exit_error_wc_surface_create;
		}

		xdg_toplevel_set_title(surface->xdgToplevel, surfaceInfo->appName);
	}

	wl_surface_commit(surface->wlSurface);
	rendererInfo->waitForConfigure = true;

	return surface;

exit_error_wc_surface_create:
	kmr_wc_surface_destroy(surface);
	return NULL;
}


void
kmr_wc_surface_destroy (struct kmr_wc_surface *surface)
{
	if (!surface)
		return;

	if (surface->xdgToplevel)
		xdg_toplevel_destroy(surface->xdgToplevel);
	if (surface->xdgSurface)
		xdg_surface_destroy(surface->xdgSurface);
	if (surface->wlSurface)
		wl_surface_destroy(surface->wlSurface);

	free(surface->rendererInfo);
	free(surface);
}

/****************************************************
 * END OF kmr_wc_surface_{create,destroy} FUNCTIONS *
 ****************************************************/
