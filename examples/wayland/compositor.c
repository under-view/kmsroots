#include "wserver.h"
#include <string.h>

int main(void)
{
  struct uvr_ws_core uvr_ws_core;
  struct uvr_ws_destroy ws;
  memset(&uvr_ws_core, 0, sizeof(uvr_ws_core));
  memset(&ws, 0, sizeof(ws));

  struct uvr_ws_core_create_info wsCoreCreateInfo;
  wsCoreCreateInfo.includeWlrDebugLogs = true;

  uvr_ws_core = uvr_ws_core_create(&wsCoreCreateInfo);
  if (!uvr_ws_core.wlDisplay || !uvr_ws_core.wlrBackend)
    goto exit_ws;

exit_ws:
  ws.uvr_ws_core = uvr_ws_core;
  uvr_ws_destroy(&ws);

  return 0;
}
