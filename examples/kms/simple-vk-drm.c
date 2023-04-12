#include "vulkan.h"
#include "kms.h"
#include "buffer.h"


struct uvr_vk {
	VkInstance instance;
	struct uvr_vk_phdev uvr_vk_phdev;
	struct uvr_vk_lgdev uvr_vk_lgdev;
	struct uvr_vk_queue uvr_vk_queue;
};


struct uvr_kms {
	struct uvr_kms_node uvr_kms_node;
	struct uvr_kms_node_display_output_chain uvr_kms_node_display_output_chain;
	struct uvr_buffer uvr_buffer;
#ifdef INCLUDE_SDBUS
	struct uvr_sd_session uvr_sd_session;
#endif
};


int create_vk_instance(struct uvr_vk *app);
int create_kms_node(struct uvr_kms *kms);
int create_gbm_buffers(struct uvr_kms *kms);
int create_vk_device(struct uvr_vk *app, struct uvr_kms *kms);


/*
 * Example code demonstrating how to use Vulkan with KMS
 */
int main(void)
{
	struct uvr_vk app;
	struct uvr_vk_destroy appd;
	memset(&app, 0, sizeof(app));
	memset(&appd, 0, sizeof(appd));

	struct uvr_kms kms;
	struct uvr_kms_node_destroy kmsdevd;
	struct uvr_buffer_destroy kmsbuffsd;
	memset(&kms, 0, sizeof(kms));
	memset(&kmsdevd, 0, sizeof(kmsdevd));
	memset(&kmsbuffsd, 0, sizeof(kmsbuffsd));

	if (create_vk_instance(&app) == -1)
		goto exit_error;

	if (create_kms_node(&kms) == -1)
		goto exit_error;

	if (create_gbm_buffers(&kms) == -1)
		goto exit_error;

	if (create_vk_device(&app, &kms) == -1)
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

	appd.instance = app.instance;
	appd.uvr_vk_lgdev_cnt = 1;
	appd.uvr_vk_lgdev = &app.uvr_vk_lgdev;
	uvr_vk_destroy(&appd);
#ifdef INCLUDE_SDBUS
	uvr_sd_session_destroy(&kms.uvr_sd_session);
#endif
	return 0;
}


int create_vk_instance(struct uvr_vk *app)
{
	/*
	 * "VK_LAYER_KHRONOS_validation"
	 * All of the useful standard validation is
	 * bundled into a layer included in the SDK
	 */
	const char *validationLayers[] = {
		"VK_LAYER_KHRONOS_validation"
	};


	const char *instanceExtensions[] = {
		"VK_KHR_get_physical_device_properties2",
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


int create_kms_node(struct uvr_kms *kms)
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


int create_gbm_buffers(struct uvr_kms *kms)
{
	struct uvr_buffer_create_info gbmBufferInfo;
	gbmBufferInfo.bufferType = UVR_BUFFER_GBM_BUFFER;
	gbmBufferInfo.kmsfd = kms->uvr_kms_node.kmsfd;
	gbmBufferInfo.bufferCount = 2;
	gbmBufferInfo.width = 3840;
	gbmBufferInfo.height = 2160;
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


int create_vk_device(struct uvr_vk *app, struct uvr_kms *kms)
{
	const char *device_extensions[] = {
		"VK_EXT_image_drm_format_modifier",
		"VK_KHR_image_format_list",
		"VK_KHR_bind_memory2",
		"VK_KHR_sampler_ycbcr_conversion",
		"VK_KHR_maintenance1",
		"VK_KHR_get_memory_requirements2",
		"VK_KHR_driver_properties"
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
