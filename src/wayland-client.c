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


static void noop() {
  // This space intentionally left blank
}


/* Hint to compositor that client hasn't become deadlocked */
static void xdg_wm_base_ping(void UNUSED *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial) {
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

  struct uvr_wc_core_interface *core = (struct uvr_wc_core_interface *) data;

  if (core->iType & UVR_WC_WL_COMPOSITOR_INTERFACE) {
    if (!strcmp(interface, wl_compositor_interface.name)) {
      core->wlCompositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);
    }
  }

  if (core->iType & UVR_WC_XDG_WM_BASE_INTERFACE) {
    if (!strcmp(interface, xdg_wm_base_interface.name)) {
      core->xdgWmBase = wl_registry_bind(registry, name, &xdg_wm_base_interface, 3);
      xdg_wm_base_add_listener(core->xdgWmBase, &xdg_wm_base_listener, NULL);
    }
  }

  if (core->iType & UVR_WC_WL_SHM_INTERFACE) {
    if (!strcmp(interface, wl_shm_interface.name)) {
      core->wlShm = wl_registry_bind(registry, name, &wl_shm_interface, version);
    }
  }

  if (core->iType & UVR_WC_WL_SEAT_INTERFACE) {
    if (!strcmp(interface, wl_seat_interface.name)) {
      core->wlSeat = wl_registry_bind(registry, name, &wl_seat_interface, version);
    }
  }

  if (core->iType & UVR_WC_ZWP_FULLSCREEN_SHELL_V1) {
    if (!strcmp(interface, "zwp_fullscreen_shell_v1")) {
      core->zwpFullscreenShell = wl_registry_bind(registry, name, &zwp_fullscreen_shell_v1_interface, version);
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


struct uvr_wc_core_interface uvr_wc_core_interface_create(struct uvr_wc_core_interface_create_info *uvrwc) {
  struct uvr_wc_core_interface interfaces;
  memset(&interfaces, 0, sizeof(struct uvr_wc_core_interface));
  interfaces.iType = uvrwc->iType;

  /* Connect to wayland server */
  interfaces.wlDisplay = wl_display_connect(uvrwc->wlDisplayName);
  if (!interfaces.wlDisplay) {
    uvr_utils_log(UVR_DANGER, "[x] wl_display_connect: Failed to connect to Wayland display.");
    goto core_interface_exit;
  }

  uvr_utils_log(UVR_SUCCESS, "Connection to Wayland display established.");

  /*
   * Emit a global event to retrieve each global object currently supported/in action by the server.
   */
  interfaces.wlRegistry = wl_display_get_registry(interfaces.wlDisplay);
  if (!interfaces.wlRegistry) {
    uvr_utils_log(UVR_DANGER, "[x] wl_display_get_registry: Failed to set global objects/interfaces");
    goto core_interface_disconnect_display;
  }

   /*
   * Bind the wl_registry_listener to a given registry. So that we can implement
   * how we handle events comming from the wayland server.
   */
  wl_registry_add_listener(interfaces.wlRegistry, &registry_listener, &interfaces);

  /* synchronously wait for the server respondes */
  if (!wl_display_roundtrip(interfaces.wlDisplay)) goto core_interface_exit_destroy;

  return (struct uvr_wc_core_interface) { .iType = uvrwc->iType, .wlDisplay = interfaces.wlDisplay, .wlRegistry = interfaces.wlRegistry,
                                          .wlCompositor = interfaces.wlCompositor, .xdgWmBase = interfaces.xdgWmBase, .wlShm = interfaces.wlShm,
                                          .wlSeat = interfaces.wlSeat };

core_interface_exit_destroy:
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
core_interface_disconnect_display:
  if (interfaces.wlDisplay)
    wl_display_disconnect(interfaces.wlDisplay);
core_interface_exit:
  return (struct uvr_wc_core_interface) { .iType = UVR_WC_NULL_INTERFACE, .wlDisplay = NULL, .wlRegistry = NULL, .wlCompositor = NULL,
                                          .xdgWmBase = NULL, .wlShm = NULL, .wlSeat = NULL };
}


struct uvr_wc_buffer uvr_wc_buffer_create(struct uvr_wc_buffer_create_info *uvrwc) {
  struct uvr_wc_shm_buffer *uvrwcshmbufs = NULL;
  struct uvr_wc_wl_buffer *uvrwcwlbufs = NULL;

  if (!uvrwc->uvrWcCore->wlShm) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_wc_buffer_create(!uvrwc->uvrwccore->shm): Must bind the wl_shm interface");
    goto error_uvr_wc_create_buffer_exit;
  }

  uvrwcshmbufs = (struct uvr_wc_shm_buffer *) calloc(uvrwc->bufferCount, sizeof(struct uvr_wc_shm_buffer));
  if (!uvrwcshmbufs) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto error_uvr_wc_create_buffer_exit;
  }

  uvrwcwlbufs = (struct uvr_wc_wl_buffer *) calloc(uvrwc->bufferCount, sizeof(struct uvr_wc_wl_buffer));
  if (!uvrwcwlbufs) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
    goto error_uvr_wc_create_buffer_free_uvrwcshmbufs;
  }

  /*
   * @shm_pool - Object used to encapsulate a piece of memory shared between the compositor and client.
   *             @buffers (wl_buffer) can be created from shm_pool.
   */
  struct wl_shm_pool *shm_pool = NULL;
  int stride = uvrwc->width * uvrwc->bytesPerPixel;

  for (int c=0; c < uvrwc->bufferCount; c++) {
    uvrwcshmbufs[c].shmPoolSize = stride * uvrwc->height * sizeof(uint8_t*);
    uvrwcshmbufs[c].shmFd = -1;

    /* Create POSIX shared memory file of size shm_pool_size */
    uvrwcshmbufs[c].shmFd = allocate_shm_file(uvrwcshmbufs[c].shmPoolSize);
    if (uvrwcshmbufs[c].shmFd == -1) {
      uvr_utils_log(UVR_DANGER, "[x] allocate_shm_file: failed to create file discriptor");
      goto error_uvr_wc_create_buffer_free_objects;
    }

    /* associate shared memory address range with open file */
    uvrwcshmbufs[c].shmPoolData = mmap(NULL, uvrwcshmbufs[c].shmPoolSize, PROT_READ | PROT_WRITE, MAP_SHARED, uvrwcshmbufs[c].shmFd, 0);
    if (uvrwcshmbufs[c].shmPoolData == (void*)-1) {
      uvr_utils_log(UVR_DANGER, "[x] mmap: %s", strerror(errno));
      goto error_uvr_wc_create_buffer_free_objects;
    }

    /* Create pool of memory shared between client and compositor */
    shm_pool = wl_shm_create_pool(uvrwc->uvrWcCore->wlShm, uvrwcshmbufs[c].shmFd, uvrwcshmbufs[c].shmPoolSize);
    if (!shm_pool) {
      uvr_utils_log(UVR_DANGER, "[x] wl_shm_create_pool: failed to create wl_shm_pool");
      goto error_uvr_wc_create_buffer_free_objects;
    }

    uvrwcwlbufs[c].buffer = wl_shm_pool_create_buffer(shm_pool, 0, uvrwc->width, uvrwc->height, stride, uvrwc->wlBufferPixelFormat);
    if (!uvrwcwlbufs[c].buffer) {
      uvr_utils_log(UVR_DANGER, "[x] wl_shm_pool_create_buffer: failed to create wl_buffer from a wl_shm_pool");
      goto error_uvr_wc_create_buffer_free_objects;
    }

    /* Can destroy shm pool after creating buffer */
    wl_shm_pool_destroy(shm_pool);
    shm_pool = NULL;
  }

  return (struct uvr_wc_buffer) {  .bufferCount = uvrwc->bufferCount, .uvrWcShmBuffers = uvrwcshmbufs, .uvrWcWlBuffers = uvrwcwlbufs };

error_uvr_wc_create_buffer_free_objects:
  if (uvrwcshmbufs && uvrwcwlbufs) {
    for (int c=0; c < uvrwc->bufferCount; c++) {
      if (uvrwcshmbufs[c].shmFd != -1)
        close(uvrwcshmbufs[c].shmFd);
      if (uvrwcshmbufs[c].shmPoolData)
        munmap(uvrwcshmbufs[c].shmPoolData, uvrwcshmbufs[c].shmPoolSize);
      if (uvrwcwlbufs[c].buffer)
        wl_buffer_destroy(uvrwcwlbufs[c].buffer);
    }

    if (shm_pool)
      wl_shm_pool_destroy(shm_pool);
  }
//error_uvr_wc_create_buffer_free_uvrwcwlbufs:
  free(uvrwcwlbufs);
error_uvr_wc_create_buffer_free_uvrwcshmbufs:
  free(uvrwcshmbufs);
error_uvr_wc_create_buffer_exit:
  return (struct uvr_wc_buffer) {  .bufferCount = 0, .uvrWcShmBuffers = NULL, .uvrWcWlBuffers = NULL };
}


/*
 * struct uvr_wc_renderer_info (Underview Renderer Wayland Client Render Information)
 *
 * members:
 * @wccore       - Pointer to a struct uvr_wc_core_interface contains all client choosen global objects defined in wayland.xml.
 * @wcsurf       - Pointer to a struct uvr_wc_surface used to get wl_surface object and buffers associate with surface.
 * @renderer     - Function pointer that allows custom external renderers to be executed by the api before registering
 *                 a frame wl_callback.
 * @rendererdata - Pointer to an address that can be optional this address is passed to the external render function.
 *                 This address may be the address of a struct. Reference passed depends on external render function.
 * @cbuf         - Pointer to an integer used by the api to update the current displayable buffer
 * @running      - Boolean indicating that surface/window is active
 */
struct uvr_wc_renderer_info {
  struct uvr_wc_core_interface *wccore;
  struct uvr_wc_surface *wcsurf;
  uvr_wc_renderer_impl renderer;
  void *rendererdata;
  int *cbuf;
  bool *running;
};


static void drawframe(void *data, struct wl_callback *callback, uint32_t time);


static void xdg_surface_handle_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
  struct uvr_wc_renderer_info UNUSED *rinfo = (struct uvr_wc_renderer_info *) data;
  xdg_surface_ack_configure(xdg_surface, serial);
}


static const struct xdg_surface_listener xdg_surface_listener = {
  .configure = xdg_surface_handle_configure,
};


/* Handles what happens when a window is closed */
static void xdg_toplevel_handle_close(void *data, struct xdg_toplevel UNUSED *xdg_toplevel) {
  struct uvr_wc_renderer_info *rinfo = (struct uvr_wc_renderer_info *) data;
  *rinfo->running = false;
}


static const struct xdg_toplevel_listener xdg_toplevel_listener = {
  .configure = noop,
  .close = xdg_toplevel_handle_close,
};


static const struct wl_callback_listener frame_listener;


static void drawframe(void *data, struct wl_callback *callback, uint32_t UNUSED time) {
  struct uvr_wc_renderer_info *rinfo = (struct uvr_wc_renderer_info *) data;
  struct uvr_wc_surface *wcsurf = rinfo->wcsurf;

  if (callback)
    wl_callback_destroy(callback);

  // Register a frame callback to know when we need to draw the next frame
  wcsurf->wlCallback = wl_surface_frame(wcsurf->wlSurface);
  wl_callback_add_listener(wcsurf->wlCallback, &frame_listener, rinfo);

  rinfo->renderer(rinfo->running, rinfo->cbuf, rinfo->rendererdata);

  if (wcsurf->uvrWcWlBuffers)
    wl_surface_attach(wcsurf->wlSurface, wcsurf->uvrWcWlBuffers[*rinfo->cbuf].buffer, 0, 0);

  /* Indicate that the entire surface as changed and needs to be redrawn */
  wl_surface_damage_buffer(wcsurf->wlSurface, 0, 0, INT32_MAX, INT32_MAX);

  /* Submit a frame for this event */
  wl_surface_commit(wcsurf->wlSurface);

  if (wcsurf->bufferCount)
    *rinfo->cbuf = (*rinfo->cbuf + 1) % wcsurf->bufferCount;
}


static const struct wl_callback_listener frame_listener = {
  .done = drawframe,
};


struct uvr_wc_surface uvr_wc_surface_create(struct uvr_wc_surface_create_info *uvrwc) {
  static struct uvr_wc_surface uvrwc_surf;
  memset(&uvrwc_surf, 0, sizeof(uvrwc_surf));

  static struct uvr_wc_renderer_info rinfo;
  memset(&rinfo, 0, sizeof(rinfo));

  if (uvrwc->uvrwcbuff) {
    uvrwc_surf.bufferCount = uvrwc->uvrwcbuff->bufferCount;
    uvrwc_surf.uvrWcWlBuffers = uvrwc->uvrwcbuff->uvrWcWlBuffers;
  }

  /* Assign pointer and data that a given client suface needs to watch */
  if (uvrwc->renderer) {
    rinfo.wccore = uvrwc->uvrWcCore;
    rinfo.wcsurf = &uvrwc_surf;
    rinfo.renderer = uvrwc->renderer;
    rinfo.rendererdata = uvrwc->rendererData;
    rinfo.cbuf = uvrwc->rendererCbuf;
    rinfo.running = uvrwc->rendererRuning;
  }

  /* Use wl_compositor interface to create a wl_surface */
  uvrwc_surf.wlSurface = wl_compositor_create_surface(uvrwc->uvrWcCore->wlCompositor);
  if (!uvrwc_surf.wlSurface) {
    uvr_utils_log(UVR_DANGER, "[x] wl_compositor_create_surface: Can't create wl_surface interface");
    goto error_create_surf_exit;
  }

  /* Use xdg_wm_base interface and wl_surface just created to create an xdg_surface object */
  uvrwc_surf.xdgSurface = xdg_wm_base_get_xdg_surface(uvrwc->uvrWcCore->xdgWmBase, uvrwc_surf.wlSurface);
  if (!uvrwc_surf.xdgSurface) {
    uvr_utils_log(UVR_DANGER, "[x] xdg_wm_base_get_xdg_surface: Can't create xdg_surface interface");
    goto error_create_surf_wl_surface_destroy;
  }

  /* Create xdg_toplevel interface from which we manage application window from */
  uvrwc_surf.xdgToplevel = xdg_surface_get_toplevel(uvrwc_surf.xdgSurface);
  if (!uvrwc_surf.xdgToplevel) {
    uvr_utils_log(UVR_DANGER, "[x] xdg_surface_get_toplevel: Can't create xdg_toplevel interface");
    goto error_create_surf_xdg_surface_destroy;
  }

  /*
   * Bind the xdg_surface_listener to a given xdg_surface objects. So that we can implement
   * how we handle events associate with object comming from the wayland server.
   */
  if (xdg_surface_add_listener(uvrwc_surf.xdgSurface, &xdg_surface_listener, &rinfo)) {
    uvr_utils_log(UVR_DANGER, "[x] xdg_surface_add_listener: Failed");
    goto error_create_surf_xdg_toplevel_destroy;
  }

  /*
   * Bind the xdg_toplevel_listener to a given xdg_toplevel objects. So that we can implement
   * how we handle events associate with objects comming from the wayland server.
   */
  if (xdg_toplevel_add_listener(uvrwc_surf.xdgToplevel, &xdg_toplevel_listener, &rinfo)) {
    uvr_utils_log(UVR_DANGER, "[x] xdg_toplevel_add_listener: Failed");
    goto error_create_surf_xdg_toplevel_destroy;
  }

  xdg_toplevel_set_title(uvrwc_surf.xdgToplevel, uvrwc->appName);

  if (uvrwc->fullscreen && uvrwc->uvrWcCore->zwpFullscreenShell) {
    zwp_fullscreen_shell_v1_present_surface(uvrwc->uvrWcCore->zwpFullscreenShell, uvrwc_surf.wlSurface, ZWP_FULLSCREEN_SHELL_V1_PRESENT_METHOD_DEFAULT, NULL);
  }

  wl_surface_commit(uvrwc_surf.wlSurface);

  // Draw the first frame
  if (uvrwc->renderer) {
    drawframe(&rinfo, NULL, 0);
  }

  return (struct uvr_wc_surface) { .xdgToplevel = uvrwc_surf.xdgToplevel, .xdgSurface = uvrwc_surf.xdgSurface, .wlSurface = uvrwc_surf.wlSurface,
                                   .bufferCount = uvrwc_surf.bufferCount, .uvrWcWlBuffers = uvrwc_surf.uvrWcWlBuffers };

error_create_surf_xdg_toplevel_destroy:
  if (uvrwc_surf.xdgToplevel)
    xdg_toplevel_destroy(uvrwc_surf.xdgToplevel);
error_create_surf_xdg_surface_destroy:
  if (uvrwc_surf.xdgSurface)
    xdg_surface_destroy(uvrwc_surf.xdgSurface);
error_create_surf_wl_surface_destroy:
  if (uvrwc_surf.wlSurface)
    wl_surface_destroy(uvrwc_surf.wlSurface);
error_create_surf_exit:
  return (struct uvr_wc_surface) { .xdgToplevel = NULL, .xdgSurface = NULL, .wlSurface = NULL,
                                   .bufferCount = -1, .uvrWcWlBuffers = NULL };
}


void uvr_wc_destroy(struct uvr_wc_destroy *uvrwc) {

  /* Destroy all wayland client surface interfaces/objects */
  if (uvrwc->uvr_wc_surface.xdgToplevel)
    xdg_toplevel_destroy(uvrwc->uvr_wc_surface.xdgToplevel);
  if (uvrwc->uvr_wc_surface.xdgSurface)
    xdg_surface_destroy(uvrwc->uvr_wc_surface.xdgSurface);
  if (uvrwc->uvr_wc_surface.wlSurface)
    wl_surface_destroy(uvrwc->uvr_wc_surface.wlSurface);

  /* Destroy all wayland client buffer interfaces/objects */
  if (uvrwc->uvr_wc_buffer.uvrWcShmBuffers && uvrwc->uvr_wc_buffer.uvrWcWlBuffers) {
    for (int b=0; b < uvrwc->uvr_wc_buffer.bufferCount; b++) {
      if (uvrwc->uvr_wc_buffer.uvrWcShmBuffers[b].shmFd != -1)
        close(uvrwc->uvr_wc_buffer.uvrWcShmBuffers[b].shmFd);
      if (uvrwc->uvr_wc_buffer.uvrWcShmBuffers[b].shmPoolData)
        munmap(uvrwc->uvr_wc_buffer.uvrWcShmBuffers[b].shmPoolData, uvrwc->uvr_wc_buffer.uvrWcShmBuffers[b].shmPoolSize);
      if (uvrwc->uvr_wc_buffer.uvrWcWlBuffers[b].buffer)
        wl_buffer_destroy(uvrwc->uvr_wc_buffer.uvrWcWlBuffers[b].buffer);
    }
    free(uvrwc->uvr_wc_buffer.uvrWcShmBuffers);
    free(uvrwc->uvr_wc_buffer.uvrWcWlBuffers);
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
