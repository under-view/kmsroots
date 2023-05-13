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

static int xdgWmBaseVersion = 0;

static void noop()
{
	// This space intentionally left blank
}


/* Hint to compositor that client hasn't become deadlocked */
static void xdg_wm_base_ping(void UNUSED *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial)
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
static void registry_handle_global(void *data,
                                   struct wl_registry *registry,
                                   uint32_t name,
                                   const char *interface,
                                   uint32_t version)
{
	kmr_utils_log(KMR_INFO, "interface: '%s', version: %d, name: %d", interface, version, name);

	struct kmr_wc_core_interface *coreInterface = (struct kmr_wc_core_interface *) data;

	if (coreInterface->iType & KMR_WC_INTERFACE_WL_COMPOSITOR) {
		if (!strcmp(interface, wl_compositor_interface.name)) {
			coreInterface->wlCompositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);
		}
	}

	if (coreInterface->iType & KMR_WC_INTERFACE_XDG_WM_BASE) {
		if (!strcmp(interface, xdg_wm_base_interface.name)) {
			coreInterface->xdgWmBase = wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
			xdg_wm_base_add_listener(coreInterface->xdgWmBase, &xdg_wm_base_listener, NULL);
		}
	}

	if (coreInterface->iType & KMR_WC_INTERFACE_WL_SHM) {
		if (!strcmp(interface, wl_shm_interface.name)) {
			coreInterface->wlShm = wl_registry_bind(registry, name, &wl_shm_interface, version);
			xdgWmBaseVersion = version;
		}
	}

	if (coreInterface->iType & KMR_WC_INTERFACE_WL_SEAT) {
		if (!strcmp(interface, wl_seat_interface.name)) {
			coreInterface->wlSeat = wl_registry_bind(registry, name, &wl_seat_interface, version);
		}
	}

	if (coreInterface->iType & KMR_WC_INTERFACE_ZWP_FULLSCREEN_SHELL_V1) {
		if (!strcmp(interface, "zwp_fullscreen_shell_v1")) {
			coreInterface->fullScreenShell = wl_registry_bind(registry, name, &zwp_fullscreen_shell_v1_interface, version);
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


struct kmr_wc_core_interface kmr_wc_core_interface_create(struct kmr_wc_core_interface_create_info *kmswc)
{
	struct kmr_wc_core_interface interfaces;
	memset(&interfaces, 0, sizeof(struct kmr_wc_core_interface));
	interfaces.iType = kmswc->iType;

	/* Connect to wayland server */
	interfaces.wlDisplay = wl_display_connect(kmswc->displayName);
	if (!interfaces.wlDisplay) {
		kmr_utils_log(KMR_DANGER, "[x] wl_display_connect: Failed to connect to Wayland display.");
		goto error_exit_core_interface_create;
	}

	kmr_utils_log(KMR_SUCCESS, "Connection to Wayland display established.");

	/*
	 * Emit a global event to retrieve each global object currently supported/in action by the server.
	 */
	interfaces.wlRegistry = wl_display_get_registry(interfaces.wlDisplay);
	if (!interfaces.wlRegistry) {
		kmr_utils_log(KMR_DANGER, "[x] wl_display_get_registry: Failed to set global objects/interfaces");
		goto error_exit_core_interface_create_disconnect_display;
	}

	/*
	 * Bind the wl_registry_listener to a given registry. So that we can implement
	 * how we handle events comming from the wayland server.
	 */
	wl_registry_add_listener(interfaces.wlRegistry, &registry_listener, &interfaces);

	/* synchronously wait for the server respondes */
	if (!wl_display_roundtrip(interfaces.wlDisplay))
		goto error_exit_core_interface_create_destroy_objects;

	return (struct kmr_wc_core_interface) { .iType = kmswc->iType, .wlDisplay = interfaces.wlDisplay, .wlRegistry = interfaces.wlRegistry,
	                                        .wlCompositor = interfaces.wlCompositor, .xdgWmBase = interfaces.xdgWmBase, .wlShm = interfaces.wlShm,
	                                        .wlSeat = interfaces.wlSeat };

error_exit_core_interface_create_destroy_objects:
	if (interfaces.wlSeat)
		wl_seat_destroy(interfaces.wlSeat);
	if (interfaces.wlShm)
		wl_shm_destroy(interfaces.wlShm);
	if (interfaces.xdgWmBase)
		xdg_wm_base_destroy(interfaces.xdgWmBase);
	if (interfaces.wlCompositor)
		wl_compositor_destroy(interfaces.wlCompositor);
	if (interfaces.wlRegistry)
		wl_registry_destroy(interfaces.wlRegistry);
error_exit_core_interface_create_disconnect_display:
	if (interfaces.wlDisplay)
		wl_display_disconnect(interfaces.wlDisplay);
error_exit_core_interface_create:
	return (struct kmr_wc_core_interface) { .iType = KMR_WC_INTERFACE_NULL, .wlDisplay = NULL, .wlRegistry = NULL,
	                                        .wlCompositor = NULL, .xdgWmBase = NULL, .wlShm = NULL, .wlSeat = NULL };
}


struct kmr_wc_buffer kmr_wc_buffer_create(struct kmr_wc_buffer_create_info *kmswc)
{
	struct kmr_wc_shm_buffer *shmBufferObjects = NULL;
	struct kmr_wc_wl_buffer_handle *wlBufferHandles = NULL;
	int c;

	if (!kmswc->coreInterface->wlShm) {
		kmr_utils_log(KMR_DANGER, "[x] kmr_wc_buffer_create(!kmswc->kmswccore->shm): Must bind the wl_shm interface");
		goto error_kmr_wc_create_buffer_exit;
	}

	shmBufferObjects = (struct kmr_wc_shm_buffer *) calloc(kmswc->bufferCount, sizeof(struct kmr_wc_shm_buffer));
	if (!shmBufferObjects) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto error_kmr_wc_create_buffer_exit;
	}

	wlBufferHandles = (struct kmr_wc_wl_buffer_handle *) calloc(kmswc->bufferCount, sizeof(struct kmr_wc_wl_buffer_handle));
	if (!wlBufferHandles) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto error_kmr_wc_create_buffer_free_shmBufferObjects;
	}

	/*
	 * @shm_pool - Object used to encapsulate a piece of memory shared between the compositor and client.
	 *             @buffers (wl_buffer) can be created from shm_pool.
	 */
	struct wl_shm_pool *shmPool = NULL;
	int stride = kmswc->width * kmswc->bytesPerPixel;

	for (c = 0; c < kmswc->bufferCount; c++) {
		shmBufferObjects[c].shmPoolSize = stride * kmswc->height;
		shmBufferObjects[c].shmFd = -1;

		/* Create POSIX shared memory file of size shm_pool_size */
		shmBufferObjects[c].shmFd = allocate_shm_file(shmBufferObjects[c].shmPoolSize);
		if (shmBufferObjects[c].shmFd == -1) {
			kmr_utils_log(KMR_DANGER, "[x] allocate_shm_file: failed to create file discriptor");
			goto error_kmr_wc_create_buffer_free_objects;
		}

		/* associate shared memory address range with open file */
		shmBufferObjects[c].shmPoolData = mmap(NULL, shmBufferObjects[c].shmPoolSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmBufferObjects[c].shmFd, 0);
		if (shmBufferObjects[c].shmPoolData == (void*)-1) {
			kmr_utils_log(KMR_DANGER, "[x] mmap: %s", strerror(errno));
			goto error_kmr_wc_create_buffer_free_objects;
		}

		/* Create pool of memory shared between client and compositor */
		shmPool = wl_shm_create_pool(kmswc->coreInterface->wlShm, shmBufferObjects[c].shmFd, shmBufferObjects[c].shmPoolSize);
		if (!shmPool) {
			kmr_utils_log(KMR_DANGER, "[x] wl_shm_create_pool: failed to create wl_shm_pool");
			goto error_kmr_wc_create_buffer_free_objects;
		}

		wlBufferHandles[c].buffer = wl_shm_pool_create_buffer(shmPool, 0, kmswc->width, kmswc->height, stride, kmswc->pixelFormat);
		if (!wlBufferHandles[c].buffer) {
			kmr_utils_log(KMR_DANGER, "[x] wl_shm_pool_create_buffer: failed to create wl_buffer from a wl_shm_pool");
			goto error_kmr_wc_create_buffer_free_objects;
		}

		/* Can destroy shm pool after creating buffer */
		wl_shm_pool_destroy(shmPool);
		shmPool = NULL;
	}

	return (struct kmr_wc_buffer) { .bufferCount = kmswc->bufferCount, .shmBufferObjects = shmBufferObjects, .wlBufferHandles = wlBufferHandles };

error_kmr_wc_create_buffer_free_objects:
	if (shmBufferObjects && wlBufferHandles) {
		for (c = 0; c < kmswc->bufferCount; c++) {
			if (shmBufferObjects[c].shmFd != -1)
				close(shmBufferObjects[c].shmFd);
			if (shmBufferObjects[c].shmPoolData)
				munmap(shmBufferObjects[c].shmPoolData, shmBufferObjects[c].shmPoolSize);
			if (wlBufferHandles[c].buffer)
				wl_buffer_destroy(wlBufferHandles[c].buffer);
		}
		if (shmPool)
			wl_shm_pool_destroy(shmPool);
	}
//error_kmr_wc_create_buffer_free_kmswcwlbufs:
	free(wlBufferHandles);
error_kmr_wc_create_buffer_free_shmBufferObjects:
	free(shmBufferObjects);
error_kmr_wc_create_buffer_exit:
	return (struct kmr_wc_buffer) { .bufferCount = 0, .shmBufferObjects = NULL, .wlBufferHandles = NULL };
}


/*
 * struct kmr_wc_renderer_info (kmsroots Wayland Client Render Information)
 *
 * members:
 * @coreInterface         - Pointer to a struct kmr_wc_core_interface contains all client choosen global objects defined in wayland.xml.
 * @surfaceObject         - Pointer to a struct kmr_wc_surface used to get wl_surface object and buffers associate with surface.
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
	struct kmr_wc_core_interface *coreInterface;
	struct kmr_wc_surface *surfaceObject;
	kmr_wc_renderer_impl renderer;
	void *rendererData;
	uint32_t *rendererCurrentBuffer;
	bool *rendererRunning;
	bool waitForConfigure;
};


static void drawframe(void *data, struct wl_callback *callback, uint32_t time);


static void xdg_surface_handle_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
	struct kmr_wc_renderer_info UNUSED *rendererInfo = (struct kmr_wc_renderer_info *) data;
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
static void xdg_toplevel_handle_close(void *data, struct xdg_toplevel UNUSED *xdg_toplevel)
{
	struct kmr_wc_renderer_info *rendererInfo = (struct kmr_wc_renderer_info *) data;
	*rendererInfo->rendererRunning = false;
}


static struct xdg_toplevel_listener xdg_toplevel_listener;


static const struct wl_callback_listener frame_listener;


static void drawframe(void *data, struct wl_callback *callback, uint32_t UNUSED time)
{
	struct kmr_wc_renderer_info *rendererInfo = (struct kmr_wc_renderer_info *) data;
	struct kmr_wc_surface *surfaceObject = rendererInfo->surfaceObject;

	if (callback)
		wl_callback_destroy(callback);

	/* Register a frame callback to know when we need to draw the next frame */
	surfaceObject->wlCallback = wl_surface_frame(surfaceObject->wlSurface);
	wl_callback_add_listener(surfaceObject->wlCallback, &frame_listener, rendererInfo);
	/* Submit a frame for this event */
	wl_surface_commit(surfaceObject->wlSurface);

	/* Execute application defined renderer */
	rendererInfo->renderer(rendererInfo->rendererRunning, rendererInfo->rendererCurrentBuffer, rendererInfo->rendererData);

	if (surfaceObject->wlBufferHandles)
		wl_surface_attach(surfaceObject->wlSurface, surfaceObject->wlBufferHandles[*rendererInfo->rendererCurrentBuffer].buffer, 0, 0);

	/* Indicate that the entire surface as changed and needs to be redrawn */
	wl_surface_damage_buffer(surfaceObject->wlSurface, 0, 0, INT32_MAX, INT32_MAX);

	/* Submit a frame for this event */
	wl_surface_commit(surfaceObject->wlSurface);

	if (surfaceObject->bufferCount)
		*rendererInfo->rendererCurrentBuffer = (*rendererInfo->rendererCurrentBuffer + 1) % surfaceObject->bufferCount;
}


static const struct wl_callback_listener frame_listener = {
	.done = drawframe,
};


struct kmr_wc_surface kmr_wc_surface_create(struct kmr_wc_surface_create_info *kmswc)
{
	static struct kmr_wc_surface surfaceObject;
	memset(&surfaceObject, 0, sizeof(surfaceObject));

	static struct kmr_wc_renderer_info rendererInfo;
	memset(&rendererInfo, 0, sizeof(rendererInfo));

	if (kmswc->wcBufferObject) {
		surfaceObject.bufferCount = kmswc->wcBufferObject->bufferCount;
		surfaceObject.wlBufferHandles = kmswc->wcBufferObject->wlBufferHandles;
	} else if (kmswc->bufferCount) {
		surfaceObject.bufferCount = kmswc->bufferCount;
	} else {
		surfaceObject.bufferCount = 0;
	}

	/* Assign pointer and data that a given client suface needs to watch */
	if (kmswc->renderer) {
		rendererInfo.coreInterface = kmswc->coreInterface;
		rendererInfo.surfaceObject = &surfaceObject;
		rendererInfo.renderer = kmswc->renderer;
		rendererInfo.rendererData = kmswc->rendererData;
		rendererInfo.rendererCurrentBuffer = kmswc->rendererCurrentBuffer;
		rendererInfo.rendererRunning = kmswc->rendererRunning;
	}

	/* Use wl_compositor interface to create a wl_surface */
	surfaceObject.wlSurface = wl_compositor_create_surface(kmswc->coreInterface->wlCompositor);
	if (!surfaceObject.wlSurface) {
		kmr_utils_log(KMR_DANGER, "[x] wl_compositor_create_surface: Can't create wl_surface interface");
		goto exit_error_wc_surface_create;
	}

	if (kmswc->fullscreen && kmswc->coreInterface->fullScreenShell) {
		zwp_fullscreen_shell_v1_present_surface(kmswc->coreInterface->fullScreenShell, surfaceObject.wlSurface, ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_DEFAULT, NULL);
	} else {
		/* Use xdg_wm_base interface and wl_surface just created to create an xdg_surface object */
		surfaceObject.xdgSurface = xdg_wm_base_get_xdg_surface(kmswc->coreInterface->xdgWmBase, surfaceObject.wlSurface);
		if (!surfaceObject.xdgSurface) {
			kmr_utils_log(KMR_DANGER, "[x] xdg_wm_base_get_xdg_surface: Can't create xdg_surface interface");
			goto exit_error_wc_surface_create_wl_surface_destroy;
		}

		/* Create xdg_toplevel interface from which we manage application window from */
		surfaceObject.xdgToplevel = xdg_surface_get_toplevel(surfaceObject.xdgSurface);
		if (!surfaceObject.xdgToplevel) {
			kmr_utils_log(KMR_DANGER, "[x] xdg_surface_get_toplevel: Can't create xdg_toplevel interface");
			goto exit_error_wc_surface_create_xdg_surface_destroy;
		}

		/*
		 * Bind the xdg_surface_listener to a given xdg_surface objects. So that we can implement
		 * how we handle events associate with object comming from the wayland server.
		 */
		if (xdg_surface_add_listener(surfaceObject.xdgSurface, &xdg_surface_listener, &rendererInfo)) {
			kmr_utils_log(KMR_DANGER, "[x] xdg_surface_add_listener: Failed");
			goto exit_error_wc_surface_create_xdg_toplevel_destroy;
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
		if (xdg_toplevel_add_listener(surfaceObject.xdgToplevel, &xdg_toplevel_listener, &rendererInfo)) {
			kmr_utils_log(KMR_DANGER, "[x] xdg_toplevel_add_listener: Failed");
			goto exit_error_wc_surface_create_xdg_toplevel_destroy;
		}

		xdg_toplevel_set_title(surfaceObject.xdgToplevel, kmswc->appName);
	}

	wl_surface_commit(surfaceObject.wlSurface);
	rendererInfo.waitForConfigure = true;

	return (struct kmr_wc_surface) { .xdgToplevel = surfaceObject.xdgToplevel, .xdgSurface = surfaceObject.xdgSurface, .wlSurface = surfaceObject.wlSurface,
	                                 .bufferCount = surfaceObject.bufferCount, .wlBufferHandles = surfaceObject.wlBufferHandles };

exit_error_wc_surface_create_xdg_toplevel_destroy:
	if (surfaceObject.xdgToplevel)
		xdg_toplevel_destroy(surfaceObject.xdgToplevel);
exit_error_wc_surface_create_xdg_surface_destroy:
	if (surfaceObject.xdgSurface)
		xdg_surface_destroy(surfaceObject.xdgSurface);
exit_error_wc_surface_create_wl_surface_destroy:
	if (surfaceObject.wlSurface)
		wl_surface_destroy(surfaceObject.wlSurface);
exit_error_wc_surface_create:
	return (struct kmr_wc_surface) { .xdgToplevel = NULL, .xdgSurface = NULL, .wlSurface = NULL, .bufferCount = -1, .wlBufferHandles = NULL };
}


void kmr_wc_destroy(struct kmr_wc_destroy *kmswc)
{
	int i;

	/* Destroy all wayland client surface interfaces/objects */
	if (kmswc->kmr_wc_surface.xdgToplevel)
		xdg_toplevel_destroy(kmswc->kmr_wc_surface.xdgToplevel);
	if (kmswc->kmr_wc_surface.xdgSurface)
		xdg_surface_destroy(kmswc->kmr_wc_surface.xdgSurface);
	if (kmswc->kmr_wc_surface.wlSurface)
		wl_surface_destroy(kmswc->kmr_wc_surface.wlSurface);

	/* Destroy all wayland client buffer interfaces/objects */
	if (kmswc->kmr_wc_buffer.shmBufferObjects && kmswc->kmr_wc_buffer.wlBufferHandles) {
		for (i = 0; i < kmswc->kmr_wc_buffer.bufferCount; i++) {
			if (kmswc->kmr_wc_buffer.shmBufferObjects[i].shmFd != -1)
				close(kmswc->kmr_wc_buffer.shmBufferObjects[i].shmFd);
			if (kmswc->kmr_wc_buffer.shmBufferObjects[i].shmPoolData)
				munmap(kmswc->kmr_wc_buffer.shmBufferObjects[i].shmPoolData, kmswc->kmr_wc_buffer.shmBufferObjects[i].shmPoolSize);
			if (kmswc->kmr_wc_buffer.wlBufferHandles[i].buffer)
				wl_buffer_destroy(kmswc->kmr_wc_buffer.wlBufferHandles[i].buffer);
		}
		free(kmswc->kmr_wc_buffer.shmBufferObjects);
		free(kmswc->kmr_wc_buffer.wlBufferHandles);
	}

	/* Destroy core wayland interfaces if allocated */
	if (kmswc->kmr_wc_core_interface.wlSeat)
		wl_seat_destroy(kmswc->kmr_wc_core_interface.wlSeat);
	if (kmswc->kmr_wc_core_interface.wlShm)
		wl_shm_destroy(kmswc->kmr_wc_core_interface.wlShm);
	if (kmswc->kmr_wc_core_interface.xdgWmBase)
		xdg_wm_base_destroy(kmswc->kmr_wc_core_interface.xdgWmBase);
	if (kmswc->kmr_wc_core_interface.fullScreenShell)
		zwp_fullscreen_shell_v1_release(kmswc->kmr_wc_core_interface.fullScreenShell);
	if (kmswc->kmr_wc_core_interface.wlCompositor)
		wl_compositor_destroy(kmswc->kmr_wc_core_interface.wlCompositor);
	if (kmswc->kmr_wc_core_interface.wlRegistry)
		wl_registry_destroy(kmswc->kmr_wc_core_interface.wlRegistry);
	if (kmswc->kmr_wc_core_interface.wlDisplay) {
		wl_display_flush(kmswc->kmr_wc_core_interface.wlDisplay);
		wl_display_disconnect(kmswc->kmr_wc_core_interface.wlDisplay);
	}
}
