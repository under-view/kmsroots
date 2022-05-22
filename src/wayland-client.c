#include "xdg-shell-client-protocol.h"
#include "wclient.h"


/*
 * Get information about extensions and versions
 * of the Wayland API that the server supports.
 * Allocate other wayland client objects/interfaces
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
      core->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);
    }
  }

  if (core->iType & UVR_WC_XDG_WM_BASE_INTERFACE) {
    if (!strcmp(interface, xdg_wm_base_interface.name)) {
      core->wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
    }
  }

  if (core->iType & UVR_WC_WL_SHM_INTERFACE) {
    if (!strcmp(interface, wl_shm_interface.name)) {
      core->shm = wl_registry_bind(registry, name, &wl_shm_interface, version);
    }
  }

  if (core->iType & UVR_WC_WL_SHM_INTERFACE) {
    if (!strcmp(interface, wl_seat_interface.name)) {
      core->seat = wl_registry_bind(registry, name, &wl_seat_interface, version);
    }
  }
}


static void registry_handle_global_remove(void UNUSED *data,
                                          struct wl_registry UNUSED *registry,
                                          uint32_t UNUSED name)
{
  // This space deliberately left blank
}


static const struct wl_registry_listener registry_listener = {
  .global = registry_handle_global,
  .global_remove = registry_handle_global_remove,
};


struct uvr_wc_core_interface uvr_wc_core_interface_create(struct uvr_wc_core_interface_create_info *uvrwc) {
  static struct uvr_wc_core_interface interfaces;
  memset(&interfaces, 0, sizeof(struct uvr_wc_core_interface));
  interfaces.iType = uvrwc->iType;

  /* Connect to wayland server */
  interfaces.display = wl_display_connect(uvrwc->wl_display_name);
  if (!interfaces.display) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_wc_core_interface_create(wl_display_connect): Failed to connect to Wayland display.");
    goto core_interface_exit;
  }

  uvr_utils_log(UVR_SUCCESS, "Connection to Wayland display established.");

  interfaces.registry = wl_display_get_registry(interfaces.display);
  if (!interfaces.registry) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_wc_core_interface_create(wl_display_get_registry): Failed to set global objects/interfaces");
    goto core_interface_disconnect_display;
  }

  wl_registry_add_listener(interfaces.registry, &registry_listener, &interfaces);

  /* synchronously wait for the server respondes */
  if (!wl_display_roundtrip(interfaces.display)) goto core_interface_exit_destroy;

  return (struct uvr_wc_core_interface) { .iType = uvrwc->iType, .display = interfaces.display, .registry = interfaces.registry,
                                          .compositor = interfaces.compositor, .wm_base = interfaces.wm_base, .shm = interfaces.shm,
                                          .seat = interfaces.seat };

core_interface_exit_destroy:
  if (interfaces.seat)
    wl_seat_destroy(interfaces.seat);
  if (interfaces.shm)
    wl_shm_destroy(interfaces.shm);
  if (interfaces.wm_base)
    xdg_wm_base_destroy(interfaces.wm_base);
  if (interfaces.compositor)
    wl_compositor_destroy(interfaces.compositor);
  if (interfaces.registry)
    wl_registry_destroy(interfaces.registry);
core_interface_disconnect_display:
  if (interfaces.display)
    wl_display_disconnect(interfaces.display);
core_interface_exit:
  return (struct uvr_wc_core_interface) { .iType = UVR_WC_NULL_INTERFACE, .display = NULL, .registry = NULL, .compositor = NULL,
                                          .wm_base = NULL, .shm = NULL, .seat = NULL };
}


struct uvr_wc_buffer uvr_wc_buffer_create(struct uvr_wc_buffer_create_info *uvrwc) {
  struct uvr_wc_buffer uvrwc_buffs;
  memset(&uvrwc_buffs, 0, sizeof(uvrwc_buffs));
  uvrwc_buffs.shm_fd = -1;

  if (!uvrwc->uvrwccore->shm) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_wc_buffer_create(!uvrwc->uvrwccore->shm): Must bind the wl_shm interface");
    goto error_create_buffer_exit;
  }

  int stride = uvrwc->width * uvrwc->bytes_per_pixel;
  uvrwc_buffs.buffer_count = uvrwc->buffer_count;
  uvrwc_buffs.shm_pool_size = stride * uvrwc->height * uvrwc->buffer_count;

  /* Create POSIX shared memory file of size shm_pool_size */
  uvrwc_buffs.shm_fd = allocate_shm_file(uvrwc_buffs.shm_pool_size);
  if (uvrwc_buffs.shm_fd == -1) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_wc_buffer_create(allocate_shm_file): failed to create file discriptor");
    goto error_create_buffer_exit;
  }

  /* associate shared memory address range with open file */
  uvrwc_buffs.shm_pool_data = mmap(NULL, uvrwc_buffs.shm_pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, uvrwc_buffs.shm_fd, 0);
  if (uvrwc_buffs.shm_pool_data == (void*)-1) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_wc_buffer_create(mmap): %s", strerror(errno));
    goto error_create_buffer_close_shm_fd;
  }

  /* Create pool of memory shared between client and compositor */
  uvrwc_buffs.shm_pool = wl_shm_create_pool(uvrwc->uvrwccore->shm, uvrwc_buffs.shm_fd, uvrwc_buffs.shm_pool_size);
  if (!uvrwc_buffs.shm_pool) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_wc_buffer_create(wl_shm_create_pool): failed to create wl_shm_pool");
    goto error_create_buffer_munmap_shm_pool;
  }

  uvrwc_buffs.buffers = (struct wl_buffer **) calloc(uvrwc_buffs.buffer_count, sizeof(struct wl_buffer *));
  if (!uvrwc_buffs.buffers) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_wc_buffer_create(calloc): failed to allocate wl_buffer memory");
    goto error_create_buffer_shm_pool_destroy;
  }

  int offset = 0;
  for (int c=0; c < uvrwc_buffs.buffer_count; c++) {
    offset = uvrwc->height * stride * c;
    uvrwc_buffs.buffers[c] = wl_shm_pool_create_buffer(uvrwc_buffs.shm_pool, offset, uvrwc->width, uvrwc->height, stride, uvrwc->wl_pix_format);
    if (!uvrwc_buffs.buffers[c]) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_wc_buffer_create(wl_shm_pool_create_buffer): failed to create wl_buffer from a wl_shm_pool");
      goto error_create_buffer_free_wl_buffer;
    }
  }

  /* Can destroy shm pool after creating buffer */
  wl_shm_pool_destroy(uvrwc_buffs.shm_pool);
  return (struct uvr_wc_buffer) { .shm_fd = uvrwc_buffs.shm_fd, .shm_pool_size = uvrwc_buffs.shm_pool_size, .shm_pool_data = uvrwc_buffs.shm_pool_data,
                                  .shm_pool = NULL, .buffer_count = uvrwc_buffs.buffer_count, .buffers = uvrwc_buffs.buffers };

error_create_buffer_free_wl_buffer:
  if (uvrwc_buffs.buffers) {
    for (int c=0; c < uvrwc_buffs.buffer_count; c++)
      if (uvrwc_buffs.buffers[c])
        wl_buffer_destroy(uvrwc_buffs.buffers[c]);
    free(uvrwc_buffs.buffers);
  }
error_create_buffer_shm_pool_destroy:
  if (uvrwc_buffs.shm_pool)
    wl_shm_pool_destroy(uvrwc_buffs.shm_pool);
error_create_buffer_munmap_shm_pool:
  if (uvrwc_buffs.shm_pool_data)
    munmap(uvrwc_buffs.shm_pool_data, uvrwc_buffs.shm_pool_size);
error_create_buffer_close_shm_fd:
  if (uvrwc_buffs.shm_fd != -1)
    close(uvrwc_buffs.shm_fd);
error_create_buffer_exit:
  return (struct uvr_wc_buffer) { .shm_fd = -1, .shm_pool_size = -1, .shm_pool_data = NULL,
                                  .shm_pool = NULL, .buffer_count = -1, .buffers = NULL };
}


static void noop() {
  // This space intentionally left blank
}


static void xdg_surface_handle_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
  struct uvr_wc_surface *surf = (struct uvr_wc_surface *) data;
  static int cur_buff = 0;

  xdg_surface_ack_configure(xdg_surface, serial);
  if (surf->buffers)
    wl_surface_attach(surf->surface, surf->buffers[cur_buff], 0, 0);
  wl_surface_commit(surf->surface);

  if (surf->buffer_count)
    cur_buff = (cur_buff + 1) % surf->buffer_count;
}


static void xdg_toplevel_handle_close(void UNUSED *data, struct xdg_toplevel UNUSED *xdg_toplevel) {
  // This space intentionally left blank
}


static const struct xdg_surface_listener xdg_surface_listener = {
  .configure = xdg_surface_handle_configure,
};


static const struct xdg_toplevel_listener xdg_toplevel_listener = {
  .configure = noop,
  .close = xdg_toplevel_handle_close,
};


struct uvr_wc_surface uvr_wc_surface_create(struct uvr_wc_surface_create_info *uvrwc) {
  static struct uvr_wc_surface uvrwc_surf;
  memset(&uvrwc_surf, 0, sizeof(uvrwc_surf));

  if (uvrwc->uvrwcbuff) {
    uvrwc_surf.buffer_count = uvrwc->uvrwcbuff->buffer_count;
    uvrwc_surf.buffers = uvrwc->uvrwcbuff->buffers;
  }

  /* Use wl_compositor interface to create a wl_surface */
  uvrwc_surf.surface = wl_compositor_create_surface(uvrwc->uvrwccore->compositor);
  if (!uvrwc_surf.surface) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_wc_surface_create(wl_compositor_create_surface): Can't create wl_surface interface");
    goto error_create_surf_exit;
  }

  /* Use xdg_wm_base interface and wl_surface just created to create an xdg_surface */
  uvrwc_surf.xdg_surface = xdg_wm_base_get_xdg_surface(uvrwc->uvrwccore->wm_base, uvrwc_surf.surface);
  if (!uvrwc_surf.xdg_surface) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_wc_surface_create(xdg_wm_base_get_xdg_surface): Can't create xdg_surface interface");
    goto error_create_surf_wl_surface_destroy;
  }

  /* Create XDG xdg_toplevel interface from which we manage application window from */
  uvrwc_surf.xdg_toplevel = xdg_surface_get_toplevel(uvrwc_surf.xdg_surface);
  if (!uvrwc_surf.xdg_toplevel) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_wc_surface_create(xdg_surface_get_toplevel): Can't create xdg_toplevel interface");
    goto error_create_surf_xdg_surface_destroy;
  }

  if (xdg_surface_add_listener(uvrwc_surf.xdg_surface, &xdg_surface_listener, &uvrwc_surf)) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_wc_surface_create(xdg_surface_add_listener): Failed");
    goto error_create_surf_xdg_toplevel_destroy;
  }

  if (xdg_toplevel_add_listener(uvrwc_surf.xdg_toplevel, &xdg_toplevel_listener, &uvrwc_surf)) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_wc_surface_create(xdg_toplevel_add_listener): Failed");
    goto error_create_surf_xdg_toplevel_destroy;
  }

  xdg_toplevel_set_title(uvrwc_surf.xdg_toplevel, uvrwc->appname);

  /*
  TODO
  if (fullscreen)
    xdg_toplevel_set_fullscreen(client->xdg_toplevel, client->output);
  */

  wl_surface_commit(uvrwc_surf.surface);

  return (struct uvr_wc_surface) { .xdg_toplevel = uvrwc_surf.xdg_toplevel, .xdg_surface = uvrwc_surf.xdg_surface,
                                   .surface = uvrwc_surf.surface, .buffer_count = uvrwc_surf.buffer_count,
                                   .buffers = uvrwc_surf.buffers };

error_create_surf_xdg_toplevel_destroy:
  if (uvrwc_surf.xdg_toplevel)
    xdg_toplevel_destroy(uvrwc_surf.xdg_toplevel);
error_create_surf_xdg_surface_destroy:
  if (uvrwc_surf.xdg_surface)
    xdg_surface_destroy(uvrwc_surf.xdg_surface);
error_create_surf_wl_surface_destroy:
  if (uvrwc_surf.surface)
    wl_surface_destroy(uvrwc_surf.surface);
error_create_surf_exit:
  return (struct uvr_wc_surface) { .xdg_toplevel = NULL, .xdg_surface = NULL, .surface = NULL,
                                   .buffer_count = -1, .buffers = NULL };
}


int uvr_wc_process_events(struct uvr_wc_core_interface *uvrwc) {
  return wl_display_dispatch(uvrwc->display);
}


int uvr_wc_flush_request(struct uvr_wc_core_interface *uvrwc) {
  return wl_display_flush(uvrwc->display);
}


void uvr_wc_destory(struct uvr_wc_destory *uvrwc) {

  /* Destroy all wayland client surface interfaces/objects */
  if (uvrwc->wcsurface) {
    if (uvrwc->wcsurface->xdg_toplevel)
      xdg_toplevel_destroy(uvrwc->wcsurface->xdg_toplevel);
    if (uvrwc->wcsurface->xdg_surface)
      xdg_surface_destroy(uvrwc->wcsurface->xdg_surface);
    if (uvrwc->wcsurface->surface)
      wl_surface_destroy(uvrwc->wcsurface->surface);
  }

  /* Destroy all wayland client buffer interfaces/objects */
  if (uvrwc->wcbuff) {
    if (uvrwc->wcbuff->shm_fd != -1)
      close(uvrwc->wcbuff->shm_fd);
    if (uvrwc->wcbuff->shm_pool_data)
      munmap(uvrwc->wcbuff->shm_pool_data, uvrwc->wcbuff->shm_pool_size);
    if (uvrwc->wcbuff->buffers) {
      for (int b=0; b < uvrwc->wcbuff->buffer_count; b++)
        if (uvrwc->wcbuff->buffers[b])
          wl_buffer_destroy(uvrwc->wcbuff->buffers[b]);
      free(uvrwc->wcbuff->buffers);
    }
    if (uvrwc->wcbuff->shm_pool)
      wl_shm_pool_destroy(uvrwc->wcbuff->shm_pool);
  }

  /* Destroy core wayland interfaces if allocated */
  if (uvrwc->wccinterface) {
    if (uvrwc->wccinterface->seat)
      wl_seat_destroy(uvrwc->wccinterface->seat);
    if (uvrwc->wccinterface->shm)
      wl_shm_destroy(uvrwc->wccinterface->shm);
    if (uvrwc->wccinterface->wm_base)
      xdg_wm_base_destroy(uvrwc->wccinterface->wm_base);
    if (uvrwc->wccinterface->compositor)
      wl_compositor_destroy(uvrwc->wccinterface->compositor);
    if (uvrwc->wccinterface->registry)
      wl_registry_destroy(uvrwc->wccinterface->registry);
    if (uvrwc->wccinterface->display)
      wl_display_disconnect(uvrwc->wccinterface->display);
  }
}
