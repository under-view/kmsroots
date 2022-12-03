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
  uvr_utils_log(UVR_INFO, "interface: '%s', version: %d, name: %d", interface, version, name);

  struct uvr_wc_core_interface *coreInterface = (struct uvr_wc_core_interface *) data;

  if (coreInterface->iType & UVR_WC_INTERFACE_WL_COMPOSITOR) {
    if (!strcmp(interface, wl_compositor_interface.name)) {
      coreInterface->wlCompositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);
    }
  }

  if (coreInterface->iType & UVR_WC_INTERFACE_XDG_WM_BASE) {
    if (!strcmp(interface, xdg_wm_base_interface.name)) {
      coreInterface->xdgWmBase = wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
      xdg_wm_base_add_listener(coreInterface->xdgWmBase, &xdg_wm_base_listener, NULL);
    }
  }

  if (coreInterface->iType & UVR_WC_INTERFACE_WL_SHM) {
    if (!strcmp(interface, wl_shm_interface.name)) {
      coreInterface->wlShm = wl_registry_bind(registry, name, &wl_shm_interface, version);
    }
  }

  if (coreInterface->iType & UVR_WC_INTERFACE_WL_SEAT) {
    if (!strcmp(interface, wl_seat_interface.name)) {
      coreInterface->wlSeat = wl_registry_bind(registry, name, &wl_seat_interface, version);
    }
  }

  if (coreInterface->iType & UVR_WC_INTERFACE_ZWP_FULLSCREEN_SHELL_V1) {
    if (!strcmp(interface, "zwp_fullscreen_shell_v1")) {
      coreInterface->zwpFullscreenShell = wl_registry_bind(registry, name, &zwp_fullscreen_shell_v1_interface, version);
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


struct uvr_wc_core_interface uvr_wc_core_interface_create(struct uvr_wc_core_interface_create_info *uvrwc)
{
  struct uvr_wc_core_interface interfaces;
  memset(&interfaces, 0, sizeof(struct uvr_wc_core_interface));
  interfaces.iType = uvrwc->iType;

  /* Connect to wayland server */
  interfaces.wlDisplay = wl_display_connect(uvrwc->displayName);
  if (!interfaces.wlDisplay) {
    uvr_utils_log(UVR_DANGER, "[x] wl_display_connect: Failed to connect to Wayland display.");
    goto error_exit_core_interface_create;
  }

  uvr_utils_log(UVR_SUCCESS, "Connection to Wayland display established.");

  /*
   * Emit a global event to retrieve each global object currently supported/in action by the server.
   */
  interfaces.wlRegistry = wl_display_get_registry(interfaces.wlDisplay);
  if (!interfaces.wlRegistry) {
    uvr_utils_log(UVR_DANGER, "[x] wl_display_get_registry: Failed to set global objects/interfaces");
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

  return (struct uvr_wc_core_interface) { .iType = uvrwc->iType, .wlDisplay = interfaces.wlDisplay, .wlRegistry = interfaces.wlRegistry,
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
  return (struct uvr_wc_core_interface) { .iType = UVR_WC_INTERFACE_NULL, .wlDisplay = NULL, .wlRegistry = NULL, .wlCompositor = NULL,
                                          .xdgWmBase = NULL, .wlShm = NULL, .wlSeat = NULL };
}


struct uvr_wc_buffer uvr_wc_buffer_create(struct uvr_wc_buffer_create_info *uvrwc)
{
  struct uvr_wc_shm_buffer *shmBufferObjects = NULL;
  struct uvr_wc_wl_buffer_handle *wlBufferHandles = NULL;

  if (!uvrwc->coreInterface->wlShm) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_wc_buffer_create(!uvrwc->uvrwccore->shm): Must bind the wl_shm interface");
    goto error_uvr_wc_create_buffer_exit;
  }

  shmBufferObjects = (struct uvr_wc_shm_buffer *) calloc(uvrwc->bufferCount, sizeof(struct uvr_wc_shm_buffer));
  if (!shmBufferObjects) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto error_uvr_wc_create_buffer_exit;
  }

  wlBufferHandles = (struct uvr_wc_wl_buffer_handle *) calloc(uvrwc->bufferCount, sizeof(struct uvr_wc_wl_buffer_handle));
  if (!wlBufferHandles) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto error_uvr_wc_create_buffer_free_shmBufferObjects;
  }

  /*
   * @shm_pool - Object used to encapsulate a piece of memory shared between the compositor and client.
   *             @buffers (wl_buffer) can be created from shm_pool.
   */
  struct wl_shm_pool *shmPool = NULL;
  int stride = uvrwc->width * uvrwc->bytesPerPixel;

  for (int c=0; c < uvrwc->bufferCount; c++) {
    shmBufferObjects[c].shmPoolSize = stride * uvrwc->height;
    shmBufferObjects[c].shmFd = -1;

    /* Create POSIX shared memory file of size shm_pool_size */
    shmBufferObjects[c].shmFd = allocate_shm_file(shmBufferObjects[c].shmPoolSize);
    if (shmBufferObjects[c].shmFd == -1) {
      uvr_utils_log(UVR_DANGER, "[x] allocate_shm_file: failed to create file discriptor");
      goto error_uvr_wc_create_buffer_free_objects;
    }

    /* associate shared memory address range with open file */
    shmBufferObjects[c].shmPoolData = mmap(NULL, shmBufferObjects[c].shmPoolSize, PROT_READ | PROT_WRITE, MAP_SHARED, shmBufferObjects[c].shmFd, 0);
    if (shmBufferObjects[c].shmPoolData == (void*)-1) {
      uvr_utils_log(UVR_DANGER, "[x] mmap: %s", strerror(errno));
      goto error_uvr_wc_create_buffer_free_objects;
    }

    /* Create pool of memory shared between client and compositor */
    shmPool = wl_shm_create_pool(uvrwc->coreInterface->wlShm, shmBufferObjects[c].shmFd, shmBufferObjects[c].shmPoolSize);
    if (!shmPool) {
      uvr_utils_log(UVR_DANGER, "[x] wl_shm_create_pool: failed to create wl_shm_pool");
      goto error_uvr_wc_create_buffer_free_objects;
    }

    wlBufferHandles[c].buffer = wl_shm_pool_create_buffer(shmPool, 0, uvrwc->width, uvrwc->height, stride, uvrwc->pixelFormat);
    if (!wlBufferHandles[c].buffer) {
      uvr_utils_log(UVR_DANGER, "[x] wl_shm_pool_create_buffer: failed to create wl_buffer from a wl_shm_pool");
      goto error_uvr_wc_create_buffer_free_objects;
    }

    /* Can destroy shm pool after creating buffer */
    wl_shm_pool_destroy(shmPool);
    shmPool = NULL;
  }

  return (struct uvr_wc_buffer) { .bufferCount = uvrwc->bufferCount, .shmBufferObjects = shmBufferObjects, .wlBufferHandles = wlBufferHandles };

error_uvr_wc_create_buffer_free_objects:
  if (shmBufferObjects && wlBufferHandles) {
    for (int c=0; c < uvrwc->bufferCount; c++) {
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
//error_uvr_wc_create_buffer_free_uvrwcwlbufs:
  free(wlBufferHandles);
error_uvr_wc_create_buffer_free_shmBufferObjects:
  free(shmBufferObjects);
error_uvr_wc_create_buffer_exit:
  return (struct uvr_wc_buffer) { .bufferCount = 0, .shmBufferObjects = NULL, .wlBufferHandles = NULL };
}


/*
 * struct uvr_wc_renderer_info (Underview Renderer Wayland Client Render Information)
 *
 * members:
 * @coreInterface         - Pointer to a struct uvr_wc_core_interface contains all client choosen global objects defined in wayland.xml.
 * @surfaceObject         - Pointer to a struct uvr_wc_surface used to get wl_surface object and buffers associate with surface.
 * @renderer              - Function pointer that allows custom external renderers to be executed by the api before registering
 *                          a frame wl_callback.
 * @rendererData          - Pointer to an address that can be optional this address is passed to the external render function.
 *                          This address may be the address of a struct. Reference passed depends on external render function.
 * @rendererCurrentBuffer - Pointer to an integer used by the api to update the current displayable buffer
 * @rendererRunning       - Pointer to a boolean indicating that surface/window is active
 * @waitForConfigure      - Boolean to help gate draw frame call. One should not attach buffer to surface before
 *                          first xdg_surface.configure event it recieved.
 */
struct uvr_wc_renderer_info {
  struct uvr_wc_core_interface *coreInterface;
  struct uvr_wc_surface *surfaceObject;
  uvr_wc_renderer_impl renderer;
  void *rendererData;
  uint32_t *rendererCurrentBuffer;
  bool *rendererRunning;
  bool waitForConfigure;
};


static void drawframe(void *data, struct wl_callback *callback, uint32_t time);


static void xdg_surface_handle_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial)
{
  struct uvr_wc_renderer_info UNUSED *rendererInfo = (struct uvr_wc_renderer_info *) data;
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
  struct uvr_wc_renderer_info *rendererInfo = (struct uvr_wc_renderer_info *) data;
  *rendererInfo->rendererRunning = false;
}


static const struct xdg_toplevel_listener xdg_toplevel_listener = {
  .configure = noop,
  .close = xdg_toplevel_handle_close,
};


static const struct wl_callback_listener frame_listener;


static void drawframe(void *data, struct wl_callback *callback, uint32_t UNUSED time)
{
  struct uvr_wc_renderer_info *rendererInfo = (struct uvr_wc_renderer_info *) data;
  struct uvr_wc_surface *surfaceObject = rendererInfo->surfaceObject;

  if (callback)
    wl_callback_destroy(callback);

  /* Register a frame callback to know when we need to draw the next frame */
  surfaceObject->wlCallback = wl_surface_frame(surfaceObject->wlSurface);
  wl_callback_add_listener(surfaceObject->wlCallback, &frame_listener, rendererInfo);
  /* Submit a frame for this event */
  wl_surface_commit(surfaceObject->wlSurface);

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


struct uvr_wc_surface uvr_wc_surface_create(struct uvr_wc_surface_create_info *uvrwc)
{
  static struct uvr_wc_surface surfaceObject;
  memset(&surfaceObject, 0, sizeof(surfaceObject));

  static struct uvr_wc_renderer_info rendererInfo;
  memset(&rendererInfo, 0, sizeof(rendererInfo));

  if (uvrwc->wcBufferObject) {
    surfaceObject.bufferCount = uvrwc->wcBufferObject->bufferCount;
    surfaceObject.wlBufferHandles = uvrwc->wcBufferObject->wlBufferHandles;
  } else if (uvrwc->bufferCount) {
    surfaceObject.bufferCount = uvrwc->bufferCount;
  } else {
    surfaceObject.bufferCount = 0;
  }

  /* Assign pointer and data that a given client suface needs to watch */
  if (uvrwc->renderer) {
    rendererInfo.coreInterface = uvrwc->coreInterface;
    rendererInfo.surfaceObject = &surfaceObject;
    rendererInfo.renderer = uvrwc->renderer;
    rendererInfo.rendererData = uvrwc->rendererData;
    rendererInfo.rendererCurrentBuffer = uvrwc->rendererCurrentBuffer;
    rendererInfo.rendererRunning = uvrwc->rendererRunning;
  }

  /* Use wl_compositor interface to create a wl_surface */
  surfaceObject.wlSurface = wl_compositor_create_surface(uvrwc->coreInterface->wlCompositor);
  if (!surfaceObject.wlSurface) {
    uvr_utils_log(UVR_DANGER, "[x] wl_compositor_create_surface: Can't create wl_surface interface");
    goto exit_error_wc_surface_create;
  }

  if (uvrwc->fullscreen && uvrwc->coreInterface->zwpFullscreenShell) {
    zwp_fullscreen_shell_v1_present_surface(uvrwc->coreInterface->zwpFullscreenShell, surfaceObject.wlSurface, ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_DEFAULT, NULL);
  } else {
    /* Use xdg_wm_base interface and wl_surface just created to create an xdg_surface object */
    surfaceObject.xdgSurface = xdg_wm_base_get_xdg_surface(uvrwc->coreInterface->xdgWmBase, surfaceObject.wlSurface);
    if (!surfaceObject.xdgSurface) {
      uvr_utils_log(UVR_DANGER, "[x] xdg_wm_base_get_xdg_surface: Can't create xdg_surface interface");
      goto exit_error_wc_surface_create_wl_surface_destroy;
    }

    /* Create xdg_toplevel interface from which we manage application window from */
    surfaceObject.xdgToplevel = xdg_surface_get_toplevel(surfaceObject.xdgSurface);
    if (!surfaceObject.xdgToplevel) {
      uvr_utils_log(UVR_DANGER, "[x] xdg_surface_get_toplevel: Can't create xdg_toplevel interface");
      goto exit_error_wc_surface_create_xdg_surface_destroy;
    }

    /*
     * Bind the xdg_surface_listener to a given xdg_surface objects. So that we can implement
     * how we handle events associate with object comming from the wayland server.
     */
    if (xdg_surface_add_listener(surfaceObject.xdgSurface, &xdg_surface_listener, &rendererInfo)) {
      uvr_utils_log(UVR_DANGER, "[x] xdg_surface_add_listener: Failed");
      goto exit_error_wc_surface_create_xdg_toplevel_destroy;
    }

    /*
     * Bind the xdg_toplevel_listener to a given xdg_toplevel objects. So that we can implement
     * how we handle events associate with objects comming from the wayland server.
     */
    if (xdg_toplevel_add_listener(surfaceObject.xdgToplevel, &xdg_toplevel_listener, &rendererInfo)) {
      uvr_utils_log(UVR_DANGER, "[x] xdg_toplevel_add_listener: Failed");
      goto exit_error_wc_surface_create_xdg_toplevel_destroy;
    }

    xdg_toplevel_set_title(surfaceObject.xdgToplevel, uvrwc->appName);
  }

  wl_surface_commit(surfaceObject.wlSurface);
  rendererInfo.waitForConfigure = true;

  return (struct uvr_wc_surface) { .xdgToplevel = surfaceObject.xdgToplevel, .xdgSurface = surfaceObject.xdgSurface, .wlSurface = surfaceObject.wlSurface,
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
  return (struct uvr_wc_surface) { .xdgToplevel = NULL, .xdgSurface = NULL, .wlSurface = NULL, .bufferCount = -1, .wlBufferHandles = NULL };
}


void uvr_wc_destroy(struct uvr_wc_destroy *uvrwc)
{
  /* Destroy all wayland client surface interfaces/objects */
  if (uvrwc->uvr_wc_surface.xdgToplevel)
    xdg_toplevel_destroy(uvrwc->uvr_wc_surface.xdgToplevel);
  if (uvrwc->uvr_wc_surface.xdgSurface)
    xdg_surface_destroy(uvrwc->uvr_wc_surface.xdgSurface);
  if (uvrwc->uvr_wc_surface.wlSurface)
    wl_surface_destroy(uvrwc->uvr_wc_surface.wlSurface);

  /* Destroy all wayland client buffer interfaces/objects */
  if (uvrwc->uvr_wc_buffer.shmBufferObjects && uvrwc->uvr_wc_buffer.wlBufferHandles) {
    for (int b=0; b < uvrwc->uvr_wc_buffer.bufferCount; b++) {
      if (uvrwc->uvr_wc_buffer.shmBufferObjects[b].shmFd != -1)
        close(uvrwc->uvr_wc_buffer.shmBufferObjects[b].shmFd);
      if (uvrwc->uvr_wc_buffer.shmBufferObjects[b].shmPoolData)
        munmap(uvrwc->uvr_wc_buffer.shmBufferObjects[b].shmPoolData, uvrwc->uvr_wc_buffer.shmBufferObjects[b].shmPoolSize);
      if (uvrwc->uvr_wc_buffer.wlBufferHandles[b].buffer)
        wl_buffer_destroy(uvrwc->uvr_wc_buffer.wlBufferHandles[b].buffer);
    }
    free(uvrwc->uvr_wc_buffer.shmBufferObjects);
    free(uvrwc->uvr_wc_buffer.wlBufferHandles);
  }

  /* Destroy core wayland interfaces if allocated */
  if (uvrwc->uvr_wc_core_interface.wlSeat)
    wl_seat_destroy(uvrwc->uvr_wc_core_interface.wlSeat);
  if (uvrwc->uvr_wc_core_interface.wlShm)
    wl_shm_destroy(uvrwc->uvr_wc_core_interface.wlShm);
  if (uvrwc->uvr_wc_core_interface.xdgWmBase)
    xdg_wm_base_destroy(uvrwc->uvr_wc_core_interface.xdgWmBase);
  if (uvrwc->uvr_wc_core_interface.zwpFullscreenShell)
    zwp_fullscreen_shell_v1_release(uvrwc->uvr_wc_core_interface.zwpFullscreenShell);
  if (uvrwc->uvr_wc_core_interface.wlCompositor)
    wl_compositor_destroy(uvrwc->uvr_wc_core_interface.wlCompositor);
  if (uvrwc->uvr_wc_core_interface.wlRegistry)
    wl_registry_destroy(uvrwc->uvr_wc_core_interface.wlRegistry);
  if (uvrwc->uvr_wc_core_interface.wlDisplay) {
    wl_display_flush(uvrwc->uvr_wc_core_interface.wlDisplay);
    wl_display_disconnect(uvrwc->uvr_wc_core_interface.wlDisplay);
  }
}
