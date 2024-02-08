#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#include "wclient.h"

#define WIDTH 1920
#define HEIGHT 1080


struct app_wc {
	struct kmr_wc_core    *kmr_wc_core;
	struct kmr_wc_surface *kmr_wc_surface;
	struct kmr_wc_buffer  *kmr_wc_buffer;
};


static volatile sig_atomic_t prun = 1;


static void
run_stop (int UNUSED signum)
{
	prun = 0;
}


/* https://github.com/dvdhrm/docs/blob/master/drm-howto/modeset-atomic.c#L825 */
static uint8_t
next_color (uint8_t cur, unsigned int mod)
{
	uint8_t next;

	next = cur + (rand() % mod);
	if ((next < cur) || (next > cur))
		next = cur;

	return next;
}


static void
render (volatile bool *running, uint32_t *cbuf, void *data)
{
	struct app_wc *wc = data;

	uint8_t r, g, b;
	unsigned int x = 0, y = 0;

	unsigned int bytesPerPixel = 4, offset = 0, stride = 0;

	stride = WIDTH * bytesPerPixel;

	srand(time(NULL));
	r = next_color(rand() % 0xff, 20);
	g = next_color(rand() % 0xff, 10);
	b = next_color(rand() % 0xff, 5);

	for (x = 0; x < WIDTH; x++) {
		for (y = 0; y < HEIGHT; y++) {
			offset = (x * bytesPerPixel) + (y * stride);
			if (wc->kmr_wc_buffer->shmBufferObjects[*cbuf].shmPoolSize <= offset) break;
			*(uint32_t *) &wc->kmr_wc_buffer->shmBufferObjects[*cbuf].shmPoolData[offset] = (r << 16) | (g << 8) | b;
		}
	}

	*running = prun;
}


/*
 * Example code demonstrating how to wayland shm buffers
 */
int
main (void)
{
	struct app_wc wc;

	struct kmr_wc_core_create_info coreCreateInfo;
	struct kmr_wc_buffer_create_info bufferCreateInfo;
	struct kmr_wc_surface_create_info surfaceCreateInfo;

	static uint32_t cbuf = 0;
	static volatile bool running = true;

	if (signal(SIGINT, run_stop) == SIG_ERR) {
		kmr_utils_log(KMR_DANGER, "[x] signal: Error while installing SIGINT signal handler.");
		return 1;
	}

	if (signal(SIGABRT, run_stop) == SIG_ERR) {
		kmr_utils_log(KMR_DANGER, "[x] signal: Error while installing SIGABRT signal handler.");
		return 1;
	}

	if (signal(SIGTERM, run_stop) == SIG_ERR) {
		kmr_utils_log(KMR_DANGER, "[x] signal: Error while installing SIGTERM signal handler.");
		return 1;
	}

	kmr_utils_set_log_level(KMR_ALL);

	coreCreateInfo.displayName = NULL;
	coreCreateInfo.iType = KMR_WC_INTERFACE_WL_COMPOSITOR | KMR_WC_INTERFACE_XDG_WM_BASE |
	                              KMR_WC_INTERFACE_WL_SHM| KMR_WC_INTERFACE_WL_SEAT | KMR_WC_INTERFACE_ZWP_FULLSCREEN_SHELL_V1;

	wc.kmr_wc_core = kmr_wc_core_create(&coreCreateInfo);
	if (!wc.kmr_wc_core)
		goto exit_error;

	bufferCreateInfo.core = wc.kmr_wc_core;
	bufferCreateInfo.bufferCount = 2;
	bufferCreateInfo.width = WIDTH;
	bufferCreateInfo.height = HEIGHT;
	bufferCreateInfo.bytesPerPixel = 4;
	bufferCreateInfo.pixelFormat = WL_SHM_FORMAT_XRGB8888;

	wc.kmr_wc_buffer = kmr_wc_buffer_create(&bufferCreateInfo);
	if (!wc.kmr_wc_buffer)
		goto exit_error;

	surfaceCreateInfo.core = wc.kmr_wc_core;
	surfaceCreateInfo.buffer = wc.kmr_wc_buffer;
	surfaceCreateInfo.appName = "WL SHM Example Window";
	surfaceCreateInfo.fullscreen = false;
	surfaceCreateInfo.renderer = &render;
	surfaceCreateInfo.rendererData = &wc;
	surfaceCreateInfo.rendererCurrentBuffer = &cbuf;
	surfaceCreateInfo.rendererRunning = &running;

	wc.kmr_wc_surface = kmr_wc_surface_create(&surfaceCreateInfo);
	if (!wc.kmr_wc_surface)
		goto exit_error;

	while (wl_display_dispatch(wc.kmr_wc_core->wlDisplay) != -1 && running) {
		// Leave blank
	}

exit_error:
	kmr_wc_surface_destroy(wc.kmr_wc_surface);
	kmr_wc_buffer_destroy(wc.kmr_wc_buffer);
	kmr_wc_core_destroy(wc.kmr_wc_core);

	return 0;
}
