#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include "wclient.h"

#define WIDTH 1280
#define HEIGHT 720


struct app_wc {
	struct uvr_wc_core_interface uvr_wc_core_interface;
	struct uvr_wc_surface uvr_wc_surface;
	struct uvr_wc_buffer uvr_wc_buffer;
};


static volatile sig_atomic_t prun = 1;


static void run_stop(int UNUSED signum)
{
	prun = 0;
}


/* https://github.com/dvdhrm/docs/blob/master/drm-howto/modeset-atomic.c#L825 */
static uint8_t next_color(bool *up, uint8_t cur, unsigned int mod)
{
	uint8_t next;

	next = cur + (*up ? 1 : -1) * (rand() % mod);
	if ((*up && next < cur) || (!*up && next > cur)) {
		*up = !*up;
		next = cur;
	}

	return next;
}


void render(bool *running, uint32_t *cbuf, void *data)
{
	struct app_wc *wc = data;

	unsigned int bytes_per_pixel = 4, offset = 0;
	unsigned int stride = WIDTH * bytes_per_pixel;
	bool r_up = true, g_up = true, b_up = true;

	srand(time(NULL));
	uint8_t r = next_color(&r_up, rand() % 0xff, 20);
	uint8_t g = next_color(&g_up, rand() % 0xff, 10);
	uint8_t b = next_color(&b_up, rand() % 0xff, 5);

	for (unsigned int x = 0; x < WIDTH; x++) {
		for (unsigned int y = 0; y < HEIGHT; y++) {
			offset = (x * bytes_per_pixel) + (y * stride);
			if (wc->uvr_wc_buffer.shmBufferObjects[*cbuf].shmPoolSize <= offset) break;
			*(uint32_t *) &wc->uvr_wc_buffer.shmBufferObjects[*cbuf].shmPoolData[offset] = (r << 16) | (g << 8) | b;
		}
	}

	*running = prun;
}


/*
 * Example code demonstrating how to wayland shm buffers
 */
int main(void)
{
	if (signal(SIGINT, run_stop) == SIG_ERR) {
		uvr_utils_log(UVR_DANGER, "[x] signal: Error while installing SIGINT signal handler.");
		return 1;
	}

	if (signal(SIGABRT, run_stop) == SIG_ERR) {
		uvr_utils_log(UVR_DANGER, "[x] signal: Error while installing SIGABRT signal handler.");
		return 1;
	}

	if (signal(SIGTERM, run_stop) == SIG_ERR) {
		uvr_utils_log(UVR_DANGER, "[x] signal: Error while installing SIGTERM signal handler.");
		return 1;
	}

	struct app_wc wc;
	struct uvr_wc_destroy wcd;
	memset(&wcd, 0, sizeof(wcd));
	memset(&wc, 0, sizeof(wc));

	struct uvr_wc_core_interface_create_info wcInterfaceCreateInfo;
	wcInterfaceCreateInfo.displayName = NULL;
	wcInterfaceCreateInfo.iType = UVR_WC_INTERFACE_WL_COMPOSITOR | UVR_WC_INTERFACE_XDG_WM_BASE |
	                              UVR_WC_INTERFACE_WL_SHM| UVR_WC_INTERFACE_WL_SEAT | UVR_WC_INTERFACE_ZWP_FULLSCREEN_SHELL_V1;

	wc.uvr_wc_core_interface = uvr_wc_core_interface_create(&wcInterfaceCreateInfo);
	if (!wc.uvr_wc_core_interface.wlDisplay || !wc.uvr_wc_core_interface.wlRegistry || !wc.uvr_wc_core_interface.wlCompositor)
		goto exit_error;

	struct uvr_wc_buffer_create_info wcBufferCreateInfo;
	wcBufferCreateInfo.coreInterface = &wc.uvr_wc_core_interface;
	wcBufferCreateInfo.bufferCount = 2;
	wcBufferCreateInfo.width = WIDTH;
	wcBufferCreateInfo.height = HEIGHT;
	wcBufferCreateInfo.bytesPerPixel = 4;
	wcBufferCreateInfo.pixelFormat = WL_SHM_FORMAT_XRGB8888;

	wc.uvr_wc_buffer = uvr_wc_buffer_create(&wcBufferCreateInfo);
	if (!wc.uvr_wc_buffer.shmBufferObjects || !wc.uvr_wc_buffer.wlBufferHandles)
		goto exit_error;

	static uint32_t cbuf = 0;
	static bool running = true;

	struct uvr_wc_surface_create_info wcSurfaceCreateInfo;
	wcSurfaceCreateInfo.coreInterface = &wc.uvr_wc_core_interface;
	wcSurfaceCreateInfo.wcBufferObject = &wc.uvr_wc_buffer;
	wcSurfaceCreateInfo.appName = "WL SHM Example Window";
	wcSurfaceCreateInfo.fullscreen = false;
	wcSurfaceCreateInfo.renderer = &render;
	wcSurfaceCreateInfo.rendererData = &wc;
	wcSurfaceCreateInfo.rendererCurrentBuffer = &cbuf;
	wcSurfaceCreateInfo.rendererRunning = &running;

	wc.uvr_wc_surface = uvr_wc_surface_create(&wcSurfaceCreateInfo);
	if (!wc.uvr_wc_surface.wlSurface)
		goto exit_error;

	while (wl_display_dispatch(wc.uvr_wc_core_interface.wlDisplay) != -1 && running) {
		// Leave blank
	}

exit_error:
	wcd.uvr_wc_core_interface = wc.uvr_wc_core_interface;
	wcd.uvr_wc_buffer = wc.uvr_wc_buffer;
	wcd.uvr_wc_surface = wc.uvr_wc_surface;
	uvr_wc_destroy(&wcd);
	return 0;
}
