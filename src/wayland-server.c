#include <string.h>
#include "wserver.h"


struct uvr_ws_core uvr_ws_core_create(struct uvr_ws_core_create_info *uvrws)
{
  if (uvrws->includeWlrDebugLogs)
    wlr_log_init(WLR_DEBUG, NULL);

  struct uvr_ws_core core;
  memset(&core, 0, sizeof(core));

  core.wlDisplay = wl_display_create();
  if (!core.wlDisplay) {
    uvr_utils_log(UVR_DANGER, "[x] wl_display_create: Failed to create unix socket.");
    goto exit_ws_core;
  }

  core.wlrBackend = wlr_backend_autocreate(core.wlDisplay);
  if (!core.wlrBackend) {
    uvr_utils_log(UVR_DANGER, "[x] wlr_backend_autocreate: Failed to create backend {DRM/KMS, Wayland, X, Headless}");
    goto exit_ws_core_wl_display_destory;
  }

  return core;

// Leave Unreachable for now
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
  if (uvrws->uvr_ws_core.wlrBackend)
    wlr_backend_destroy(uvrws->uvr_ws_core.wlrBackend);
  if (uvrws->uvr_ws_core.wlDisplay)
    wl_display_destroy(uvrws->uvr_ws_core.wlDisplay);
}
