#include "xdg-shell-client-protocol.h"
#include "wclient.h"


struct wl_display *uvr_wclient_display_connect(const char *wl_display_name) {
  struct wl_display *display = NULL;

  /* Connect to wayland server */
  display = wl_display_connect(wl_display_name);
  if (!display) {
    uvr_utils_log(UVR_DANGER, "[x] Failed to connect to Wayland display.");
    return NULL;
  }

  uvr_utils_log(UVR_SUCCESS, "Connection to Wayland display established.");

  return display;
}


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

  struct _uvrwc *wc = (struct _uvrwc *) data;
  if (!strcmp(interface, wl_compositor_interface.name)) {
    wc->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, version);
  } else if (!strcmp(interface, xdg_wm_base_interface.name)) {
    wc->shell = wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
  } else if (!strcmp(interface, wl_shm_interface.name)) {
    wc->shm = wl_registry_bind(registry, name, &wl_shm_interface, version);
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

int uvr_wclient_alloc_interfaces(uvrwc *client) {

  if (!client->display) {
    uvr_utils_log(UVR_DANGER, "[x] Not connected to Wayland display server");
    uvr_utils_log(UVR_DANGER, "[x] Must call uvr_wclient_display_connect()");
    return -1;
  }

  client->registry = wl_display_get_registry(client->display);
  if (!client->registry) {
    uvr_utils_log(UVR_DANGER, "[x] Failed to set global objects/interfaces");
    return -1;
  }

  wl_registry_add_listener(client->registry, &registry_listener, client);

  /* synchronously wait for the server respondes */
  if (!wl_display_roundtrip(client->display)) return -1;

  /* Important globals to create buffers from */
  if (!client->compositor) {
    uvr_utils_log(UVR_DANGER, "[x] Can't find compositor");
    return -1;
  }

  if (!client->shell) {
    uvr_utils_log(UVR_DANGER, "[x] No xdg_wm_base support");
    return -1;
  }

  return 0;
}


int uvr_wclient_process_events(uvrwc *client) {
  return wl_display_dispatch(client->display);
}


int uvr_wclient_flush_request(uvrwc *client) {
  return wl_display_flush(client->display);
}


void uvr_wclient_destory(uvrwc *client) {
  if (client->shm)
    wl_shm_destroy(client->shm);
  if (client->shell)
    xdg_wm_base_destroy(client->shell);
  if (client->compositor)
    wl_compositor_destroy(client->compositor);
  if (client->registry)
    wl_registry_destroy(client->registry);
  if (client->display)
    wl_display_disconnect(client->display);
}
