#include "wserver.h"


int main(void) {
  struct uvr_ws_core wscore;
  struct uvr_ws_destroy ws;
  memset(&wscore, 0, sizeof(wscore));
  memset(&ws, 0, sizeof(ws));

  struct uvr_ws_core_create_info wscoreinfo;
  wscoreinfo.inc_wlr_debug_logs = true;

  wscore = uvr_ws_core_create(&wscoreinfo);
  if (!wscore.display || !wscore.backend)
    goto exit_ws;

exit_ws:
  ws.uvr_ws_core = wscore;
  uvr_ws_destroy(&ws);

  return 0;
}
