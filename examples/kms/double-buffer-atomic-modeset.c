#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "drm-node.h"
#include "buffer.h"
#include "input.h"

#define MAX_EPOLL_EVENTS 2

/***************************
 * Structs used by example *
 ***************************/
struct app_kms {
	struct kmr_drm_node *kmr_drm_node;
	struct kmr_drm_node_display *kmr_drm_node_display;
	struct kmr_drm_node_atomic_request *kmr_drm_node_atomic_request;
	struct kmr_buffer *kmr_buffer;
	struct kmr_input kmr_input;
#ifdef INCLUDE_LIBSEAT
	struct kmr_session *kmr_session;
#endif
};


struct app_kms_pass {
	unsigned int pixelBufferSize;
	uint8_t *pixelBuffer;
	struct app_kms *app_kms;
};


/***********
 * Globals *
 ***********/
static volatile sig_atomic_t prun = 1;


/***********************
 * Function Prototypes *
 ***********************/
static int
create_kms_instance (struct app_kms *kms);

static int
create_kms_gbm_buffers(struct app_kms *kms);

static int
create_kms_set_crtc(struct app_kms *kms);

static int
create_kms_atomic_request_instance(struct app_kms_pass *passData,
                                   uint8_t *cbuf,
                                   int *fbid,
                                   volatile bool *running);

static int
create_kms_pixel_buffer(struct app_kms_pass *passData);


/************************************
 * Start of function implementation *
 ************************************/

static void
run_stop (int UNUSED signum)
{
	prun = 0;
}


/* https://github.com/dvdhrm/docs/blob/master/drm-howto/modeset-atomic.c#L825 */
static uint8_t
next_color (bool *up, uint8_t cur, unsigned int mod)
{
	uint8_t next;

	next = cur + (*up ? 1 : -1) * (rand() % mod);
	if ((*up && next < cur) || (!*up && next > cur)) {
		*up = !*up;
		next = cur;
	}

	return next;
}


/*
 * Library implementation is as such
 * 1. "render" function implementation is called. Application must update @fbid value (updated number is 103).
 *    This value will be submitted to DRM core.
 * 2. Prepare properties for submitting to DRM core. The new @fbid set by "render" will be used to
 *    when performing a KMS atomic commit (i.e submitting data to DRM core).
 * 3. Prepared properties from step 2 are sent to DRM core and the driver performs an atomic commit.
 *    Which leads to a page-flip.
 * 4. Update choosen buffer and Redo steps 1-3.
 */
static void
render (volatile bool *running, uint8_t *cbuf, int *fbid, void *data)
{
	struct app_kms_pass *passData;
	struct app_kms *kms;

	uint8_t r, g, b;
	bool r_up, g_up, b_up;

	uint8_t *pixelBuffer;
	unsigned int pixelBufferSize;

	unsigned int x, y;
	unsigned int stride;
	unsigned int width, height;
	unsigned int bytesPerPixel, offset;

	passData = (struct app_kms_pass *) data;
	kms = passData->app_kms;

	pixelBuffer = passData->pixelBuffer;
	pixelBufferSize = passData->pixelBufferSize;

	width = kms->kmr_drm_node_display->width;
	height = kms->kmr_drm_node_display->height;

	bytesPerPixel = 4, offset = 0;
	r_up = true, g_up = true, b_up = true;
	stride = width * bytesPerPixel;

	srand(time(NULL));
	r = next_color(&r_up, rand() % 0xff, 20);
	g = next_color(&g_up, rand() % 0xff, 10);
	b = next_color(&b_up, rand() % 0xff, 5);

	for (x = 0; x < width; x++) {
		for (y = 0; y < height; y++) {
			offset = (x * bytesPerPixel) + (y * stride);
			*(uint32_t *) &pixelBuffer[offset] = (r << 16) | (g << 8) | b;
		}
	}

	*cbuf ^= 1;

	// Write to buffer that'll be displayed at function end
	*fbid = kms->kmr_buffer->bufferObjects[*cbuf].fbid;
	gbm_bo_write(kms->kmr_buffer->bufferObjects[*cbuf].bo, pixelBuffer, pixelBufferSize);

	*running = prun;
}


/*
 * Example code demonstrating how to use Vulkan with KMS
 */
int
main (void)
{
	static int fbid = 0;
	static uint8_t cbuf = 0;
	static volatile bool running = true;

	struct app_kms kms;
	static struct app_kms_pass passData;

	struct epoll_event event, events[MAX_EPOLL_EVENTS];
	int nfds = -1, epollfd = -1, kmsfd = -1, inputfd = -1, n;

	uint64_t inputReturnCode;
	struct kmr_input_create_info inputInfo;
	struct kmr_input_handle_input_event_info inputEventInfo;
	struct kmr_drm_node_handle_drm_event_info drmEventInfo;

	memset(&kms, 0, sizeof(kms));
	memset(&passData, 0, sizeof(passData));
	memset(&inputInfo, 0, sizeof(inputInfo));
	passData.app_kms = &kms;

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

	if (create_kms_instance(&kms) == -1)
		goto exit_error;

	if (create_kms_gbm_buffers(&kms) == -1)
		goto exit_error;

	if (create_kms_set_crtc(&kms) == -1)
		goto exit_error;

	if (create_kms_pixel_buffer(&passData) == -1)
		goto exit_error;

	if (create_kms_atomic_request_instance(&passData, &cbuf, &fbid, &running) == -1)
		goto exit_error;

#ifdef INCLUDE_LIBSEAT
	inputInfo.session = kms.kmr_session;
#endif
	kms.kmr_input = kmr_input_create(&inputInfo);
	if (!kms.kmr_input.input)
		goto exit_error;

	inputfd = kms.kmr_input.inputfd;
	kmsfd = kms.kmr_drm_node->kmsfd;

	epollfd = epoll_create1(0);
	if (epollfd == -1) {
		kmr_utils_log(KMR_DANGER, "[x] epoll_create1: %s", strerror(errno));
		goto exit_error;
	}

	event.events = EPOLLIN;
	event.data.fd = kmsfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, kmsfd, &event) == -1) {
		kmr_utils_log(KMR_DANGER, "[x] epoll_ctl: %s", strerror(errno));
		goto exit_error;
	}

	event.events = EPOLLIN;
	event.data.fd = inputfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, inputfd, &event) == -1) {
		kmr_utils_log(KMR_DANGER, "[x] epoll_ctl: %s", strerror(errno));
		goto exit_error;
	}

	drmEventInfo.kmsfd = kmsfd;
	inputEventInfo.input = kms.kmr_input;

	while (running) {
		nfds = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, -1);
		if (nfds == -1) {
			kmr_utils_log(KMR_DANGER, "[x] epoll_wait: %s", strerror(errno));
			goto exit_error;
		}

		for (n = 0; n < nfds; n++) {
			if (events[n].data.fd == inputfd) {
				inputReturnCode = kmr_input_handle_input_event(&inputEventInfo);

				/*
				 * input-event-codes.h
				 * 1 == KEY_ESC
				 * 16 = KEY_Q
				 */
				if (inputReturnCode == 1 || inputReturnCode == 16) {
					goto exit_error;
				}
			}

			if (events[n].data.fd == kmsfd) {
				kmr_drm_node_handle_drm_event(&drmEventInfo);
			}
		}
	}

exit_error:
	struct kmr_input_destroy kmsinputd;
	memset(&kmsinputd, 0, sizeof(kmsinputd));

	if (passData.pixelBuffer)
		munmap(passData.pixelBuffer, passData.pixelBufferSize);

	/*
	 * Let the api know of what addresses to free and fd's to close
	 */
	kmr_buffer_destroy(kms.kmr_buffer);

	kmsinputd.kmr_input = kms.kmr_input;
	kmr_input_destroy(&kmsinputd);

	kmr_drm_node_atomic_request_destroy(kms.kmr_drm_node_atomic_request);
	kmr_drm_node_display_destroy(kms.kmr_drm_node_display);
	kmr_drm_node_destroy(kms.kmr_drm_node);

#ifdef INCLUDE_LIBSEAT
	kmr_session_destroy(kms.kmr_session);
#endif
	return 0;
}


static int
create_kms_instance (struct app_kms *kms)
{
	struct kmr_drm_node_create_info kmsNodeCreateInfo;

#ifdef INCLUDE_LIBSEAT
	kms->kmr_session = kmr_session_create();
	if (!kms->kmr_session)
		return -1;

	kmsNodeCreateInfo.session = kms->kmr_session;
#endif

	kmsNodeCreateInfo.kmsNode = NULL;
	kms->kmr_drm_node = kmr_drm_node_create(&kmsNodeCreateInfo);
	if (!kms->kmr_drm_node)
		return -1;

	struct kmr_drm_node_display_create_info displayCreateInfo;
	displayCreateInfo.kmsfd = kms->kmr_drm_node->kmsfd;

	kms->kmr_drm_node_display = kmr_drm_node_display_create(&displayCreateInfo);
	if (!kms->kmr_drm_node_display)
		return -1;

	return 0;
}


static int
create_kms_gbm_buffers (struct app_kms *kms)
{
	struct kmr_buffer_create_info gbmBufferInfo;
	gbmBufferInfo.bufferType = KMR_BUFFER_GBM_BUFFER;
	gbmBufferInfo.kmsfd = kms->kmr_drm_node->kmsfd;
	gbmBufferInfo.bufferCount = 2;
	gbmBufferInfo.width = kms->kmr_drm_node_display->width;
	gbmBufferInfo.height = kms->kmr_drm_node_display->height;
	gbmBufferInfo.bitDepth = 24;
	gbmBufferInfo.bitsPerPixel = 32;
	gbmBufferInfo.gbmBoFlags = GBM_BO_USE_SCANOUT | GBM_BO_USE_WRITE;
	gbmBufferInfo.pixelFormat = GBM_BO_FORMAT_XRGB8888;
	gbmBufferInfo.modifiers = NULL;
	gbmBufferInfo.modifierCount = 0;

	kms->kmr_buffer = kmr_buffer_create(&gbmBufferInfo);
	if (!kms->kmr_buffer)
		return -1;

	return 0;
}


static int
create_kms_set_crtc (struct app_kms *kms)
{
	uint8_t i;
	struct kmr_drm_node_display_mode_info nextImageInfo;

	for (i = 0; i < kms->kmr_buffer->bufferCount; i++) {
		nextImageInfo.fbid = kms->kmr_buffer->bufferObjects[i].fbid;
		nextImageInfo.display = kms->kmr_drm_node_display;
		if (kmr_drm_node_display_mode_set(&nextImageInfo))
			return -1;
	}

	return 0;
}


static int
create_kms_atomic_request_instance (struct app_kms_pass *passData,
                                    uint8_t *cbuf,
                                    int *fbid,
                                    volatile bool *running)
{
	struct app_kms *kms = passData->app_kms;
	struct kmr_drm_node_atomic_request_create_info atomicRequestInfo;

	*fbid = kms->kmr_buffer->bufferObjects[*cbuf].fbid;

	atomicRequestInfo.kmsfd = kms->kmr_drm_node_display->kmsfd;
	atomicRequestInfo.display = kms->kmr_drm_node_display;
	atomicRequestInfo.renderer = &render;
	atomicRequestInfo.rendererRunning = running;
	atomicRequestInfo.rendererCurrentBuffer = cbuf;
	atomicRequestInfo.rendererFbId = fbid;
	atomicRequestInfo.rendererData = passData;

	kms->kmr_drm_node_atomic_request = kmr_drm_node_atomic_request_create(&atomicRequestInfo);
	if (!kms->kmr_drm_node_atomic_request)
		return -1;

	return 0;
}


static int
create_kms_pixel_buffer (struct app_kms_pass *passData)
{
	struct app_kms *kms = NULL;

	uint8_t *pixelBuffer = NULL; 
	unsigned int width = 0, height = 0;
	unsigned int bytesPerPixel = 0, pixelBufferSize = 0;

	kms = passData->app_kms;
	width = kms->kmr_drm_node_display->width;
	height = kms->kmr_drm_node_display->height;

	bytesPerPixel = 4;
	pixelBufferSize = width * height * bytesPerPixel;

	pixelBuffer = mmap(NULL, pixelBufferSize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON,
                           -1, kms->kmr_buffer->bufferObjects[0].offsets[0]);
	if (pixelBuffer == MAP_FAILED) {
		kmr_utils_log(KMR_DANGER, "[x] mmap: %s", strerror(errno));
		return -1;
	}

	passData->pixelBuffer = pixelBuffer;
	passData->pixelBufferSize = pixelBufferSize;

	return 0;
}
