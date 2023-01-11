#include <string.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/allocator.h>
#include <wlr/util/log.h>

#include "wserver.h"


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
    goto exit_ws_core_wl_display_destory;
  }

  // TODO: Implement custom renderer
  core.wlrRenderer = wlr_renderer_autocreate(core.wlrBackend);
  if (!core.wlrRenderer) {
    wlr_log(WLR_ERROR, "[X] wlr_renderer_autocreate: failed to create wlr_renderer");
    goto exit_ws_core_wlr_backend_destroy;
  }

  wlr_renderer_init_wl_display(core.wlrRenderer, core.wlDisplay);

  /* Bridge between the renderer and the backend that handles the buffer creation */
  core.wlrRendererAllocator = wlr_allocator_autocreate(core.wlrBackend, core.wlrRenderer);
  if (!core.wlrRendererAllocator) {
    wlr_log(WLR_ERROR, "[X] wlr_allocator_autocreate: failed to create wlr_allocator");
    goto exit_ws_core_wlr_backend_destroy;
  }

  return core;

exit_ws_core_wlr_backend_destroy:
  if (core.wlrBackend)
    wlr_backend_destroy(core.wlrBackend);
exit_ws_core_wl_display_destory:
  if (core.wlDisplay)
    wl_display_destroy(core.wlDisplay);
exit_ws_core:
  return (struct uvr_ws_core) { .wlDisplay = NULL, .wlrBackend = NULL };
}


void uvr_ws_destroy(struct uvr_ws_destroy *uvrws)
{
  if (uvrws->uvr_ws_core.wlDisplay)
    wl_display_destroy_clients(uvrws->uvr_ws_core.wlDisplay);
  if (uvrws->uvr_ws_core.wlrBackend)
    wlr_backend_destroy(uvrws->uvr_ws_core.wlrBackend);
  if (uvrws->uvr_ws_core.wlDisplay)
    wl_display_destroy(uvrws->uvr_ws_core.wlDisplay);
}
