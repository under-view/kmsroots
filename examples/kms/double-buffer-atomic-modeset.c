#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <sys/mman.h>

#include "kms.h"
#include "buffer.h"

struct app_kms {
	struct uvr_kms_node uvr_kms_node;
	struct uvr_kms_node_display_output_chain uvr_kms_node_display_output_chain;
	struct uvr_buffer uvr_buffer;
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


void render(bool UNUSED *running, uint32_t *cbuf, void *data)
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

	// Write to back buffer
	gbm_bo_write(app->uvr_buffer.bufferObjects[*cbuf^1].bo, pixelBuffer, pixelBufferSize);

	// Display front buffer
	struct uvr_kms_display_mode_info nextImageInfo;
	nextImageInfo.fbid = app->uvr_buffer.bufferObjects[*cbuf].fbid;
	nextImageInfo.displayChain = &app->uvr_kms_node_display_output_chain;
	if (uvr_kms_set_display_mode(&nextImageInfo) == 0)
		*cbuf ^= 1;

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
	struct uvr_kms_node_destroy kmsdevd;
	struct uvr_buffer_destroy kmsbuffsd;
	memset(&kms, 0, sizeof(kms));
	memset(&kmsdevd, 0, sizeof(kmsdevd));
	memset(&kmsbuffsd, 0, sizeof(kmsbuffsd));

	if (create_kms_instance(&kms) == -1)
		goto exit_error;

	if (create_kms_gbm_buffers(&kms) == -1)
		goto exit_error;

	if (create_kms_set_crtc(&kms) == -1)
		goto exit_error;

	static uint32_t cbuf = 0;
	static bool running = true;

	struct app_kms_pass passData;
	memset(&passData, 0, sizeof(passData));
	passData.app = &kms;

	if (create_kms_pixel_buffer(&passData) == -1)
		goto exit_error;

	while (running) {
		render(&running, &cbuf, &passData);
	}

exit_error:
	if (passData.pixelBuffer)
		munmap(passData.pixelBuffer, passData.pixelBufferSize);

	/*
	 * Let the api know of what addresses to free and fd's to close
	 */
	kmsbuffsd.uvr_buffer_cnt = 1;
	kmsbuffsd.uvr_buffer = &kms.uvr_buffer;
	uvr_buffer_destroy(&kmsbuffsd);

	kmsdevd.uvr_kms_node = kms.uvr_kms_node;
	kmsdevd.uvr_kms_node_display_output_chain = kms.uvr_kms_node_display_output_chain;
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
	if (!kms->uvr_kms_node_display_output_chain.connector ||
	    !kms->uvr_kms_node_display_output_chain.encoder   ||
	    !kms->uvr_kms_node_display_output_chain.crtc      ||
	    !kms->uvr_kms_node_display_output_chain.plane)
	{
		return -1;
	}

	struct uvr_kms_node_device_capabilites UNUSED kmsNodeDeviceCapabilites;
	kmsNodeDeviceCapabilites = uvr_kms_node_get_device_capabilities(kms->uvr_kms_node.kmsfd);

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
	struct uvr_kms_display_mode_info nextImageInfo;

	for (uint8_t i = 0; i < app->uvr_buffer.bufferCount; i++) {
		nextImageInfo.fbid = app->uvr_buffer.bufferObjects[0].fbid;
		nextImageInfo.displayChain = &app->uvr_kms_node_display_output_chain;
		if (uvr_kms_set_display_mode(&nextImageInfo))
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
