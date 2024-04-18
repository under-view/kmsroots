#include <stdlib.h>
#include <string.h>

#include "vulkan.h"
#include "drm-node.h"
#include "buffer.h"

struct app_vk {
	VkInstance instance;
	struct kmr_vk_phdev *kmr_vk_phdev;
	struct kmr_vk_lgdev kmr_vk_lgdev;
	struct kmr_vk_queue *kmr_vk_queue;
};


struct app_kms {
	struct kmr_drm_node *kmr_drm_node;
	struct kmr_drm_node_display *kmr_drm_node_display;
	struct kmr_buffer *kmr_buffer;
#ifdef INCLUDE_LIBSEAT
	struct kmr_session *kmr_session;
#endif
};


static int
create_kms_instance (struct app_kms *kms);

static int
create_kms_gbm_buffers (struct app_kms *kms);

static int
create_vk_instance (struct app_vk *app);

static int
create_vk_device (struct app_vk *app, struct app_kms *kms);


/*
 * Example code demonstrating how to use Vulkan with KMS
 */
int
main (void)
{
	struct app_vk app;
	struct kmr_vk_destroy appd;
	memset(&app, 0, sizeof(app));
	memset(&appd, 0, sizeof(appd));

	struct app_kms kms;
	memset(&kms, 0, sizeof(kms));

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
	appd.kmr_vk_lgdev_cnt = 1;
	appd.kmr_vk_lgdev = &app.kmr_vk_lgdev;
	kmr_vk_destroy(&appd);
	kmr_vk_queue_destroy(app.kmr_vk_queue);
	kmr_vk_phdev_destroy(app.kmr_vk_phdev);
	kmr_vk_instance_destroy(app.instance);

	kmr_buffer_destroy(kms.kmr_buffer);

	kmr_drm_node_display_destroy(kms.kmr_drm_node_display);
	kmr_drm_node_destroy(kms.kmr_drm_node);
#ifdef INCLUDE_LIBSEAT
	kmr_session_destroy(kms.kmr_session);
#endif
	return 0;
}


static int
create_vk_instance (struct app_vk *app)
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
	struct kmr_vk_instance_create_info instanceCreateInfo;
	instanceCreateInfo.appName = "Example App";
	instanceCreateInfo.engineName = "No Engine";
	instanceCreateInfo.enabledLayerCount = ARRAY_LEN(validationLayers);
	instanceCreateInfo.enabledLayerNames = validationLayers;
	instanceCreateInfo.enabledExtensionCount = ARRAY_LEN(instanceExtensions);
	instanceCreateInfo.enabledExtensionNames = instanceExtensions;

	app->instance = kmr_vk_instance_create(&instanceCreateInfo);
	if (!app->instance)
		return -1;

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
	gbmBufferInfo.gbmBoFlags = GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT;
	gbmBufferInfo.pixelFormat = GBM_BO_FORMAT_XRGB8888;
	gbmBufferInfo.modifiers = NULL;
	gbmBufferInfo.modifierCount = 0;

	kms->kmr_buffer = kmr_buffer_create(&gbmBufferInfo);
	if (!kms->kmr_buffer)
		return -1;

	return 0;
}


static int
create_vk_device(struct app_vk *app, struct app_kms *kms)
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
	struct kmr_vk_phdev_create_info phdevCreateInfo;
	phdevCreateInfo.instance = app->instance;
	phdevCreateInfo.deviceType = VK_PHYSICAL_DEVICE_TYPE;
	phdevCreateInfo.kmsfd = kms->kmr_drm_node->kmsfd;

	app->kmr_vk_phdev = kmr_vk_phdev_create(&phdevCreateInfo);
	if (!app->kmr_vk_phdev)
		return -1;

	struct kmr_vk_queue_create_info queueCreateInfo;
	queueCreateInfo.physDevice = app->kmr_vk_phdev->physDevice;
	queueCreateInfo.queueFlag = VK_QUEUE_GRAPHICS_BIT;

	app->kmr_vk_queue = kmr_vk_queue_create(&queueCreateInfo);
	if (!app->kmr_vk_queue)
		return -1;

	/*
	 * Can Hardset features prior
	 * https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceFeatures.html
	 * app->kmr_vk_phdev.physDeviceFeatures.depthBiasClamp = VK_TRUE;
	 */
	struct kmr_vk_lgdev_create_info lgdevCreateInfo;
	lgdevCreateInfo.instance = app->instance;
	lgdevCreateInfo.physDevice = app->kmr_vk_phdev->physDevice;
	lgdevCreateInfo.enabledFeatures = &app->kmr_vk_phdev->physDeviceFeatures;
	lgdevCreateInfo.enabledExtensionCount = ARRAY_LEN(device_extensions);
	lgdevCreateInfo.enabledExtensionNames = device_extensions;
	lgdevCreateInfo.queueCount = 1;
	lgdevCreateInfo.queues = app->kmr_vk_queue;

	app->kmr_vk_lgdev = kmr_vk_lgdev_create(&lgdevCreateInfo);
	if (!app->kmr_vk_lgdev.logicalDevice)
		return -1;

	return 0;
}
