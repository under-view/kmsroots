#include <stdlib.h>
#include <string.h>

#include "vulkan.h"
#include "kms.h"
#include "buffer.h"

#define WIDTH 1920
#define HEIGHT 1080

struct app_vk {
	VkInstance instance;
	struct uvr_vk_phdev uvr_vk_phdev;
	struct uvr_vk_lgdev uvr_vk_lgdev;
	struct uvr_vk_queue uvr_vk_queue;
};


struct app_kms {
	struct uvr_kms_node uvr_kms_node;
	struct uvr_kms_node_display_output_chain uvr_kms_node_display_output_chain;
	struct uvr_buffer uvr_buffer;
#ifdef INCLUDE_LIBSEAT
	struct uvr_session *uvr_session;
#endif
};


int create_kms_instance(struct app_kms *kms);
int create_kms_gbm_buffers(struct app_kms *kms);
int create_vk_instance(struct app_vk *app);
int create_vk_device(struct app_vk *app, struct app_kms *kms);


/*
 * Example code demonstrating how to use Vulkan with KMS
 */
int main(void)
{
	struct app_vk app;
	struct uvr_vk_destroy appd;
	memset(&app, 0, sizeof(app));
	memset(&appd, 0, sizeof(appd));

	struct app_kms kms;
	struct uvr_kms_node_destroy kmsdevd;
	struct uvr_buffer_destroy kmsbuffsd;
	memset(&kms, 0, sizeof(kms));
	memset(&kmsdevd, 0, sizeof(kmsdevd));
	memset(&kmsbuffsd, 0, sizeof(kmsbuffsd));

	if (create_vk_instance(&app) == -1)
		goto exit_error;

	if (create_kms_instance(&kms) == -1)
		goto exit_error;

	if (create_kms_gbm_buffers(&kms) == -1)
		goto exit_error;

	if (create_vk_device(&app, &kms) == -1)
		goto exit_error;

exit_error:
	/*
	 * Let the api know of what addresses to free and fd's to close
	 */
	appd.instance = app.instance;
	appd.uvr_vk_lgdev_cnt = 1;
	appd.uvr_vk_lgdev = &app.uvr_vk_lgdev;
	uvr_vk_destroy(&appd);

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


int create_vk_instance(struct app_vk *app)
{
	/*
	 * "VK_LAYER_KHRONOS_validation"
	 * All of the useful standard validation is
	 * bundled into a layer included in the SDK
	 */
	const char *validationLayers[] = {
#ifdef INCLUDE_VULKAN_VALIDATION_LAYERS
		"VK_LAYER_KHRONOS_validation"
#endif
	};

	const char *instanceExtensions[] = {
		"VK_EXT_debug_utils"
	};

	/*
	 * Create Vulkan Instance
	 */
	struct uvr_vk_instance_create_info instanceCreateInfo;
	instanceCreateInfo.appName = "Example App";
	instanceCreateInfo.engineName = "No Engine";
	instanceCreateInfo.enabledLayerCount = ARRAY_LEN(validationLayers);
	instanceCreateInfo.enabledLayerNames = validationLayers;
	instanceCreateInfo.enabledExtensionCount = ARRAY_LEN(instanceExtensions);
	instanceCreateInfo.enabledExtensionNames = instanceExtensions;

	app->instance = uvr_vk_instance_create(&instanceCreateInfo);
	if (!app->instance)
		return -1;

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


int create_vk_device(struct app_vk *app, struct app_kms *kms)
{
	const char *device_extensions[] = {
		"VK_KHR_timeline_semaphore",
		"VK_KHR_external_memory_fd",
		"VK_KHR_external_semaphore_fd",
		"VK_KHR_synchronization2",
		"VK_KHR_image_format_list",
		"VK_EXT_external_memory_dma_buf",
		"VK_EXT_queue_family_foreign",
		"VK_EXT_image_drm_format_modifier",
	};

	/*
	 * Create Vulkan Physical Device Handle, After buffer creation
	 * as it can actually effect VkPhysicalDevice creation
	 */
	struct uvr_vk_phdev_create_info phdevCreateInfo;
	phdevCreateInfo.instance = app->instance;
	phdevCreateInfo.deviceType = VK_PHYSICAL_DEVICE_TYPE;
	phdevCreateInfo.kmsfd = kms->uvr_kms_node.kmsfd;

	app->uvr_vk_phdev = uvr_vk_phdev_create(&phdevCreateInfo);
	if (!app->uvr_vk_phdev.physDevice)
		return -1;

	struct uvr_vk_queue_create_info queueCreateInfo;
	queueCreateInfo.physDevice = app->uvr_vk_phdev.physDevice;
	queueCreateInfo.queueFlag = VK_QUEUE_GRAPHICS_BIT;

	app->uvr_vk_queue = uvr_vk_queue_create(&queueCreateInfo);
	if (app->uvr_vk_queue.familyIndex == -1)
		return -1;

	/*
	 * Can Hardset features prior
	 * https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceFeatures.html
	 * app->uvr_vk_phdev.physDeviceFeatures.depthBiasClamp = VK_TRUE;
	 */
	struct uvr_vk_lgdev_create_info lgdevCreateInfo;
	lgdevCreateInfo.instance = app->instance;
	lgdevCreateInfo.physDevice = app->uvr_vk_phdev.physDevice;
	lgdevCreateInfo.enabledFeatures = &app->uvr_vk_phdev.physDeviceFeatures;
	lgdevCreateInfo.enabledExtensionCount = ARRAY_LEN(device_extensions);
	lgdevCreateInfo.enabledExtensionNames = device_extensions;
	lgdevCreateInfo.queueCount = 1;
	lgdevCreateInfo.queues = &app->uvr_vk_queue;

	app->uvr_vk_lgdev = uvr_vk_lgdev_create(&lgdevCreateInfo);
	if (!app->uvr_vk_lgdev.logicalDevice)
		return -1;

	return 0;
}
