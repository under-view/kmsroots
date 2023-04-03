#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include "wserver.h"

static struct uvr_ws_destroy ws;


static void run_stop(int UNUSED _)
{
	uvr_ws_destroy(&ws);
}


int main(void)
{
	if (signal(SIGINT, run_stop) == SIG_ERR) {
		fprintf(stderr, "[x] signal: Error while installing SIGINT signal handler.\n");
		return 1;
	}

	if (signal(SIGABRT, run_stop) == SIG_ERR) {
		fprintf(stderr, "[x] signal: Error while installing SIGABRT signal handler.\n");
		return 1;
	}

	if (signal(SIGTERM, run_stop) == SIG_ERR) {
		fprintf(stderr, "[x] signal: Error while installing SIGTERM signal handler.\n");
		return 1;
	}

	struct uvr_ws_core uvr_ws_core;
	memset(&uvr_ws_core, 0, sizeof(uvr_ws_core));
	memset(&ws, 0, sizeof(ws));

	struct uvr_ws_core_create_info wsCoreCreateInfo;
	wsCoreCreateInfo.includeWlrDebugLogs = true;
	wsCoreCreateInfo.unixSockName = "underview-comp-0";

	uvr_ws_core = uvr_ws_core_create(&wsCoreCreateInfo);
	if (!uvr_ws_core.wlDisplay || !uvr_ws_core.wlrBackend)
	{ raise(SIGTERM); return 1; }

	ws.uvr_ws_core = uvr_ws_core;

	wl_display_run(uvr_ws_core.wlDisplay);

	return 0;
}
