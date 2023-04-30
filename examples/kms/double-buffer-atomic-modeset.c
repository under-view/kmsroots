#include "kms.h"
#include "buffer.h"

#define WIDTH 1920
#define HEIGHT 1080

struct app_kms {
	struct uvr_kms_node uvr_kms_node;
	struct uvr_kms_node_display_output_chain uvr_kms_node_display_output_chain;
	struct uvr_buffer uvr_buffer;
#ifdef INCLUDE_SDBUS
	struct uvr_sd_session uvr_sd_session;
#endif
};


int create_kms_node(struct app_kms *kms);
int create_gbm_buffers(struct app_kms *kms);

/*
 * Example code demonstrating how to use Vulkan with KMS
 */
int main(void)
{
	struct app_kms kms;
	struct uvr_kms_node_destroy kmsdevd;
	struct uvr_buffer_destroy kmsbuffsd;
	memset(&kms, 0, sizeof(kms));
	memset(&kmsdevd, 0, sizeof(kmsdevd));
	memset(&kmsbuffsd, 0, sizeof(kmsbuffsd));

	if (create_kms_node(&kms) == -1)
		goto exit_error;

	if (create_gbm_buffers(&kms) == -1)
		goto exit_error;

exit_error:
	/*
	 * Let the api know of what addresses to free and fd's to close
	 */
	kmsbuffsd.uvr_buffer_cnt = 1;
	kmsbuffsd.uvr_buffer = &kms.uvr_buffer;
	uvr_buffer_destroy(&kmsbuffsd);

	kmsdevd.uvr_kms_node = kms.uvr_kms_node;
	kmsdevd.uvr_kms_node_display_output_chain = kms.uvr_kms_node_display_output_chain;
	uvr_kms_node_destroy(&kmsdevd);

#ifdef INCLUDE_SDBUS
	uvr_sd_session_destroy(&kms.uvr_sd_session);
#endif
	return 0;
}


int create_kms_node(struct app_kms *kms)
{
	struct uvr_kms_node_create_info kmsNodeCreateInfo;

#ifdef INCLUDE_SDBUS
	if (uvr_sd_session_create(&(kms->uvr_sd_session)) == -1)
		return -1;

	kmsNodeCreateInfo.systemdSession = &(kms->uvr_sd_session);
	kmsNodeCreateInfo.useLogind = true;
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


int create_gbm_buffers(struct app_kms *kms)
{
	struct uvr_buffer_create_info gbmBufferInfo;
	gbmBufferInfo.bufferType = UVR_BUFFER_GBM_BUFFER;
	gbmBufferInfo.kmsfd = kms->uvr_kms_node.kmsfd;
	gbmBufferInfo.bufferCount = 2;
	gbmBufferInfo.width = WIDTH;
	gbmBufferInfo.height = HEIGHT;
	gbmBufferInfo.bitDepth = 24;
	gbmBufferInfo.bitsPerPixel = 32;
	gbmBufferInfo.gbmBoFlags = GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT;
	gbmBufferInfo.pixelFormat = GBM_BO_FORMAT_XRGB8888;
	gbmBufferInfo.modifiers = NULL;
	gbmBufferInfo.modifierCount = 0;

	kms->uvr_buffer = uvr_buffer_create(&gbmBufferInfo);
	if (!kms->uvr_buffer.gbmDevice)
		return -1;

	return 0;
}
