#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/epoll.h>

#include "kms-node.h"
#include "buffer.h"
#include "input.h"

#define MAX_EPOLL_EVENTS 2

struct app_kms {
	struct uvr_kms_node uvr_kms_node;
	struct uvr_kms_node_display_output_chain uvr_kms_node_display_output_chain;
	struct uvr_kms_node_atomic_request uvr_kms_node_atomic_request;
	struct uvr_buffer uvr_buffer;
	struct uvr_input uvr_input;
#ifdef INCLUDE_LIBSEAT
	struct uvr_session *uvr_session;
#endif
};


struct app_kms_pass {
	unsigned int pixelBufferSize;
	uint8_t *pixelBuffer;
	struct app_kms *app;
};


int create_kms_instance(struct app_kms *kms);
int create_kms_gbm_buffers(struct app_kms *kms);
int create_kms_set_crtc(struct app_kms *app);
int create_kms_pixel_buffer(struct app_kms_pass *passData);


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


/*
 * Library implementation is as such
 * 1. Prepare properties for submitting to DRM core. The KMS primary plane object property FB_ID value
 *    is set before "render" function is called. @fbid value is initially set to
 *    uvr_buffer.bufferObjects[cbuf=0].fbid (102). This is done when we created the first
 *    KMS atomic request.
 * 2. "render" function implementation is called. Application must update fbid value (updated number is 103).
 *    This value won't be submitted to DRM core until a page-flip event occurs.
 * 3. Prepared properties from step 1 are sent to DRM core and the driver performs an atomic commit.
 *    Which leads to a page-flip. When submitted the @fbid is set to 102.
 * 4. Redo steps 1-3. Using the now rendered into framebuffer.
 */
void render(bool *running, uint8_t *cbuf, int *fbid, void *data)
{
	struct app_kms_pass *passData = (struct app_kms_pass *) data;
	struct app_kms *app = passData->app;

	unsigned int width = app->uvr_kms_node_display_output_chain.width;
	unsigned int height = app->uvr_kms_node_display_output_chain.height;
	unsigned int bytesPerPixel = 4, offset = 0;
	unsigned int stride = width * bytesPerPixel;
	bool r_up = true, g_up = true, b_up = true;

	srand(time(NULL));
	uint8_t r = next_color(&r_up, rand() % 0xff, 20);
	uint8_t g = next_color(&g_up, rand() % 0xff, 10);
	uint8_t b = next_color(&b_up, rand() % 0xff, 5);

	unsigned int pixelBufferSize = passData->pixelBufferSize;
	uint8_t *pixelBuffer = passData->pixelBuffer;

	for (unsigned int x = 0; x < width; x++) {
		for (unsigned int y = 0; y < height; y++) {
			offset = (x * bytesPerPixel) + (y * stride);
			*(uint32_t *) &pixelBuffer[offset] = (r << 16) | (g << 8) | b;
		}
	}

	*cbuf ^= 1;

	// Write to back buffer
	*fbid = app->uvr_buffer.bufferObjects[*cbuf].fbid;
	gbm_bo_write(app->uvr_buffer.bufferObjects[*cbuf].bo, pixelBuffer, pixelBufferSize);

	*running = prun;
}


/*
 * Example code demonstrating how to use Vulkan with KMS
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

	struct app_kms kms;
	memset(&kms, 0, sizeof(kms));

	if (create_kms_instance(&kms) == -1)
		goto exit_error;

	if (create_kms_gbm_buffers(&kms) == -1)
		goto exit_error;

	if (create_kms_set_crtc(&kms) == -1)
		goto exit_error;

	struct app_kms_pass passData;
	memset(&passData, 0, sizeof(passData));
	passData.app = &kms;

	if (create_kms_pixel_buffer(&passData) == -1)
		goto exit_error;

	struct epoll_event event, events[MAX_EPOLL_EVENTS];
	int nfds = -1, epollfd = -1, kmsfd = -1, inputfd = -1, n;

	static int fbid = 0;
	static uint8_t cbuf = 0;
	static bool running = true;

	kmsfd = kms.uvr_kms_node_display_output_chain.kmsfd;
	fbid = kms.uvr_buffer.bufferObjects[cbuf].fbid;

	struct uvr_kms_node_atomic_request_create_info atomicRequestInfo;
	atomicRequestInfo.kmsfd = kmsfd;
	atomicRequestInfo.displayOutputChain = &kms.uvr_kms_node_display_output_chain;
	atomicRequestInfo.renderer = &render;
	atomicRequestInfo.rendererRunning = &running;
	atomicRequestInfo.rendererCurrentBuffer = &cbuf;
	atomicRequestInfo.rendererFbId = &fbid;
	atomicRequestInfo.rendererData = &passData;

	kms.uvr_kms_node_atomic_request = uvr_kms_node_atomic_request_create(&atomicRequestInfo);
	if (!kms.uvr_kms_node_atomic_request.atomicRequest)
		goto exit_error;
	
	struct uvr_input_create_info inputInfo;
#ifdef INCLUDE_LIBSEAT
	inputInfo.session = kms.uvr_session;
#endif

	kms.uvr_input = uvr_input_create(&inputInfo);
	if (!kms.uvr_input.input)
		goto exit_error;

	inputfd = kms.uvr_input.inputfd;

	epollfd = epoll_create1(0);
	if (epollfd == -1) {
		uvr_utils_log(UVR_DANGER, "[x] epoll_create1: %s", strerror(errno));
		goto exit_error;
	}

	event.events = EPOLLIN;
	event.data.fd = kmsfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, kmsfd, &event) == -1) {
		uvr_utils_log(UVR_DANGER, "[x] epoll_ctl: %s", strerror(errno));
		goto exit_error;
	}

	event.events = EPOLLIN;
	event.data.fd = inputfd;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, inputfd, &event) == -1) {
		uvr_utils_log(UVR_DANGER, "[x] epoll_ctl: %s", strerror(errno));
		goto exit_error;
	}

	struct uvr_kms_node_handle_drm_event_info drmEventInfo;
	drmEventInfo.kmsfd = kmsfd;
	
	uint64_t inputReturnCode;
	struct uvr_input_handle_input_event_info inputEventInfo;
	inputEventInfo.input = kms.uvr_input;

	while (running) {
		nfds = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, -1);
		if (nfds == -1) {
			uvr_utils_log(UVR_DANGER, "[x] epoll_wait: %s", strerror(errno));
			goto exit_error;
		}

		for (n = 0; n < nfds; n++) {
			if (events[n].data.fd == inputfd) {
				inputReturnCode = uvr_input_handle_input_event(&inputEventInfo);

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
				uvr_kms_node_handle_drm_event(&drmEventInfo);
			}
		}
	}

exit_error:
	struct uvr_kms_node_destroy kmsdevd;
	struct uvr_buffer_destroy kmsbuffsd;
	struct uvr_input_destroy kmsinputd;
	memset(&kmsdevd, 0, sizeof(kmsdevd));
	memset(&kmsbuffsd, 0, sizeof(kmsbuffsd));
	memset(&kmsinputd, 0, sizeof(kmsinputd));

	if (passData.pixelBuffer)
		munmap(passData.pixelBuffer, passData.pixelBufferSize);

	/*
	 * Let the api know of what addresses to free and fd's to close
	 */
	kmsbuffsd.uvr_buffer_cnt = 1;
	kmsbuffsd.uvr_buffer = &kms.uvr_buffer;
	uvr_buffer_destroy(&kmsbuffsd);

	kmsinputd.uvr_input = kms.uvr_input;	
	uvr_input_destroy(&kmsinputd);

	kmsdevd.uvr_kms_node = kms.uvr_kms_node;
	kmsdevd.uvr_kms_node_display_output_chain = kms.uvr_kms_node_display_output_chain;
	kmsdevd.uvr_kms_node_atomic_request = kms.uvr_kms_node_atomic_request;
	uvr_kms_node_destroy(&kmsdevd);

#ifdef INCLUDE_LIBSEAT
	uvr_session_destroy(kms.uvr_session);
#endif
	return 0;
}


int create_kms_instance(struct app_kms *kms)
{
	struct uvr_kms_node_create_info kmsNodeCreateInfo;

#ifdef INCLUDE_LIBSEAT
	kms->uvr_session = uvr_session_create();
	if (!kms->uvr_session->seat)
		return -1;

	kmsNodeCreateInfo.session = kms->uvr_session;
#endif

	kmsNodeCreateInfo.kmsNode = NULL;
	kms->uvr_kms_node = uvr_kms_node_create(&kmsNodeCreateInfo);
	if (kms->uvr_kms_node.kmsfd == -1)
		return -1;

	struct uvr_kms_node_display_output_chain_create_info dochainCreateInfo;
	dochainCreateInfo.kmsfd = kms->uvr_kms_node.kmsfd;

	kms->uvr_kms_node_display_output_chain = uvr_kms_node_display_output_chain_create(&dochainCreateInfo);
	if (!kms->uvr_kms_node_display_output_chain.connector.propsData ||
	    !kms->uvr_kms_node_display_output_chain.crtc.propsData      ||
	    !kms->uvr_kms_node_display_output_chain.plane.propsData)
	{
		return -1;
	}

	return 0;
}


int create_kms_gbm_buffers(struct app_kms *kms)
{
	struct uvr_buffer_create_info gbmBufferInfo;
	gbmBufferInfo.bufferType = UVR_BUFFER_GBM_BUFFER;
	gbmBufferInfo.kmsfd = kms->uvr_kms_node.kmsfd;
	gbmBufferInfo.bufferCount = 2;
	gbmBufferInfo.width = kms->uvr_kms_node_display_output_chain.width;
	gbmBufferInfo.height = kms->uvr_kms_node_display_output_chain.height;
	gbmBufferInfo.bitDepth = 24;
	gbmBufferInfo.bitsPerPixel = 32;
	gbmBufferInfo.gbmBoFlags = GBM_BO_USE_SCANOUT | GBM_BO_USE_WRITE;
	gbmBufferInfo.pixelFormat = GBM_BO_FORMAT_XRGB8888;
	gbmBufferInfo.modifiers = NULL;
	gbmBufferInfo.modifierCount = 0;

	kms->uvr_buffer = uvr_buffer_create(&gbmBufferInfo);
	if (!kms->uvr_buffer.gbmDevice)
		return -1;

	return 0;
}


int create_kms_set_crtc(struct app_kms *app)
{
	struct uvr_kms_node_display_mode_info nextImageInfo;

	for (uint8_t i = 0; i < app->uvr_buffer.bufferCount; i++) {
		nextImageInfo.fbid = app->uvr_buffer.bufferObjects[0].fbid;
		nextImageInfo.displayChain = &app->uvr_kms_node_display_output_chain;
		if (uvr_kms_node_display_mode_set(&nextImageInfo))
			return -1;
	}

	return 0;
}


int create_kms_pixel_buffer(struct app_kms_pass *passData)
{
	struct app_kms *app = passData->app;

	unsigned int width = app->uvr_kms_node_display_output_chain.width;
	unsigned int height = app->uvr_kms_node_display_output_chain.height;
	unsigned int bytesPerPixel = 4;
	unsigned int pixelBufferSize = width * height * bytesPerPixel;

	uint8_t *pixelBuffer = mmap(NULL, pixelBufferSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, app->uvr_buffer.bufferObjects[0].offsets[0]);
	if (pixelBuffer == MAP_FAILED) {
		uvr_utils_log(UVR_DANGER, "[x] mmap: %s", strerror(errno));
		return -1;
	}

	passData->pixelBuffer = pixelBuffer;
	passData->pixelBufferSize = pixelBufferSize;

	return 0;
}
