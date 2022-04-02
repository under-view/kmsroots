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


int uvr_wclient_alloc_shm_buffers(uvrwc *client,
                                  const int buffer_count,
                                  const int width,
                                  const int height,
                                  const int bytes_per_pixel,
                                  uint64_t format)
{

  int stride = width * bytes_per_pixel;
  client->buffer_count = buffer_count;
  client->shm_pool_size = height * stride * client->buffer_count;

  /* Create POSIX shared memory file of size shm_pool_size */
  client->shm_fd = allocate_shm_file(client->shm_pool_size);
  if (client->shm_fd == -1) {
    uvr_utils_log(UVR_DANGER, "[x] allocate_shm_file: failed to create file discriptor");
    return -1;
  }

  /* associate shared memory address range with open file */
  client->shm_pool_data = mmap(NULL, client->shm_pool_size, PROT_READ | PROT_WRITE, MAP_SHARED, client->shm_fd, 0);
  if (client->shm_pool_data == (void*)-1) {
    uvr_utils_log(UVR_DANGER, "[x] mmap: %s", strerror(errno));
    return -1;
  }

  /* Create pool of memory shared between client and compositor */
  client->shm_pool = wl_shm_create_pool(client->shm, client->shm_fd, client->shm_pool_size);
  if (!client->shm_pool) {
    uvr_utils_log(UVR_DANGER, "[x] wl_shm_create_pool: failed to create wl_shm_pool");
    return -1;
  }

  client->buffers = (struct wl_buffer **) calloc(client->buffer_count, sizeof(struct wl_buffer *));
  if (!client->buffers) {
    uvr_utils_log(UVR_DANGER, "[x] calloc: failed to allocate wl_buffer memory");
    return -1;
  }

  int offset = 0;
  /* Can destroy shm pool after creating buffer */
  for (int c=0; c < client->buffer_count; c++) {
    offset = height * stride * c;
    client->buffers[c] = wl_shm_pool_create_buffer(client->shm_pool, offset, width, height, stride, format);
    if (!client->buffers[c]) {
      uvr_utils_log(UVR_DANGER, "[x] wl_shm_pool_create_buffer: failed to create wl_buffer from a wl_shm_pool");
      return -1;
    }
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
  if (client->shm_fd)
    close(client->shm_fd);
  if (client->shm_pool_data)
    munmap(client->shm_pool_data, client->shm_pool_size);
  if (client->buffers) {
    for (int c=0; c < client->buffer_count; c++)
      if (client->buffers[c])
        wl_buffer_destroy(client->buffers[c]);
    free(client->buffers);
  }
  if (client->shm_pool)
    wl_shm_pool_destroy(client->shm_pool);
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
