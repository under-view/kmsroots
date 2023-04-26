#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include "vulkan.h"


/*
 * Taken from lunarG vulkan API
 * https://www.khronos.org/registry/vulkan/specs/1.3-extensions/man/html/VkResult.html
 */
static const char *vkres_msg(int err)
{
	switch (err) {
		case VK_ERROR_OUT_OF_HOST_MEMORY:
			return "host memory allocation has failed";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			return "device memory allocation has failed";
		case VK_ERROR_INITIALIZATION_FAILED:
			return "initialization of an object could not be completed for implementation-specific reasons";
		case VK_ERROR_DEVICE_LOST:
			return "the logical or physical device has been lost";
		case VK_ERROR_MEMORY_MAP_FAILED:
			return "mapping of a memory object has failed";
		case VK_ERROR_LAYER_NOT_PRESENT:
			return "a requested layer is not present or could not be loaded";
		case VK_ERROR_EXTENSION_NOT_PRESENT:
			return "a requested extension is not supported";
		case VK_ERROR_FEATURE_NOT_PRESENT:
			return "a requested feature is not supported";
		case VK_ERROR_INCOMPATIBLE_DRIVER:
			return "the requested version of Vulkan is not supported by the driver or is otherwise "  \
			       "incompatible for implementation-specific reasons";
		case VK_ERROR_TOO_MANY_OBJECTS:
			return "too many objects of the type have already been created";
		case VK_ERROR_FORMAT_NOT_SUPPORTED:
			return "a requested format is not supported on this device";
		case VK_ERROR_FRAGMENTED_POOL:
			return "a pool allocation has failed due to fragmentation of the pool's memory";
		case VK_ERROR_OUT_OF_POOL_MEMORY:
			return "a pool memory allocation has failed";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE:
			return "an external handle is not a valid handle of the specified type";
		case VK_ERROR_FRAGMENTATION:
			return "a descriptor pool creation has failed due to fragmentation";
		case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
			return "a buffer creation or memory allocation failed because the requested address is not available";
		case VK_PIPELINE_COMPILE_REQUIRED:
			return "A requested pipeline creation would have required compilation, but the application requested" \
			       "compilation to not be performed.";
		case VK_ERROR_SURFACE_LOST_KHR:
			return "a surface is no longer available";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			return "the requested window is already in use by Vulkan or another API in a manner which " \
			       "prevents it from being used again";
		case VK_SUBOPTIMAL_KHR:
			return "A swapchain no longer matches the surface properties exactly, but can still be used to present" \
			       "to the surface successfully.";
		case VK_ERROR_OUT_OF_DATE_KHR:
			return "a surface has changed in such a way that it is no longer compatible with the swapchain, " \
			       "any further presentation requests using the swapchain will fail";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
			return "The display used by a swapchain does not use the same presentable image layout, or is " \
			       "incompatible in a way that prevents sharing an image";
		case VK_ERROR_VALIDATION_FAILED_EXT:
			return "VK_ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV:
			return "one or more shaders failed to compile or link";
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
			return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case VK_ERROR_NOT_PERMITTED_KHR:
			return "VK_ERROR_NOT_PERMITTED_KHR";
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
			return "an operation on a swapchain created with VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT failed as " \
			       "it did not have exlusive full-screen access";
		case VK_THREAD_IDLE_KHR:
			return "A deferred operation is not complete but there is currently no work for this thread to do at the time of this call.";
		case VK_THREAD_DONE_KHR:
			return "A deferred operation is not complete but there is no work remaining to assign to additional threads.";
		case VK_OPERATION_DEFERRED_KHR:
			return "A deferred operation was requested and at least some of the work was deferred.";
		case VK_OPERATION_NOT_DEFERRED_KHR:
			return "A deferred operation was requested and no operations were deferred.";
		case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
			return "An image creation failed because internal resources required for compression are exhausted." \
			       "This must only be returned when fixed-rate compression is requested.";
	}

	// VK_ERROR_UNKNOWN
	return "An unknown error has occurred. Either the application has provided invalid input, or an implementation failure has occurred";
}


static uint32_t retrieve_memory_type_index(VkPhysicalDevice physDev, uint32_t memoryType, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physDev, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if (memoryType & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	uvr_utils_log(UVR_DANGER, "[x] retrieve_memory_type: failed to find suitable memory type");

	return UINT32_MAX;
}


VkInstance uvr_vk_instance_create(struct uvr_vk_instance_create_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkInstance instance = VK_NULL_HANDLE;

	/* initialize the VkApplicationInfo structure */
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = NULL;
	appInfo.pApplicationName = uvrvk->appName;
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.pEngineName = uvrvk->engineName;
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.apiVersion = VK_MAKE_VERSION(1, 3, 0);

	/* enable validation layers best practice */
	VkValidationFeatureEnableEXT enables[] = {VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};
	VkValidationFeaturesEXT features = {};
	features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
	features.enabledValidationFeatureCount = ARRAY_LEN(enables);
	features.pEnabledValidationFeatures = enables;

	/*
	 * Tells Vulkan which instance extensions
	 * and validation layers we want to use
	 */
	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = (uvrvk->enabledLayerNames) ? &features : NULL;
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = uvrvk->enabledLayerCount;
	createInfo.ppEnabledLayerNames = uvrvk->enabledLayerNames;
	createInfo.enabledExtensionCount = uvrvk->enabledExtensionCount;
	createInfo.ppEnabledExtensionNames = uvrvk->enabledExtensionNames;

	/* Create the instance */
	res = vkCreateInstance(&createInfo, NULL, &instance);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkCreateInstance: %s", vkres_msg(res));
		return VK_NULL_HANDLE;
	}

	uvr_utils_log(UVR_SUCCESS, "uvr_vk_instance_create: VkInstance created retval(%p)", instance);

	return instance;
}


VkSurfaceKHR uvr_vk_surface_create(struct uvr_vk_surface_create_info *uvrvk)
{
	VkResult UNUSED res = VK_RESULT_MAX_ENUM;
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	if (!uvrvk->instance) {
		uvr_utils_log(UVR_DANGER, "[x] uvr_vk_surface_create: VkInstance not instantiated");
		uvr_utils_log(UVR_DANGER, "[x] Make a call to uvr_vk_create_instance");
		return VK_NULL_HANDLE;
	}

	if (uvrvk->surfaceType != UVR_SURFACE_WAYLAND_CLIENT && uvrvk->surfaceType != UVR_SURFACE_XCB_CLIENT) {
		uvr_utils_log(UVR_DANGER, "[x] uvr_vk_surface_create: Must specify correct enum uvrvk_surface_type");
		return VK_NULL_HANDLE;
	}

#ifdef INCLUDE_WAYLAND
	if (uvrvk->surfaceType == UVR_SURFACE_WAYLAND_CLIENT) {
		VkWaylandSurfaceCreateInfoKHR waylandSurfaceCreateInfo = {};
		waylandSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
		waylandSurfaceCreateInfo.pNext = NULL;
		waylandSurfaceCreateInfo.flags = 0;
		waylandSurfaceCreateInfo.display = uvrvk->display;
		waylandSurfaceCreateInfo.surface = uvrvk->surface;

		res = vkCreateWaylandSurfaceKHR(uvrvk->instance, &waylandSurfaceCreateInfo, NULL, &surface);
		if (res) {
			uvr_utils_log(UVR_DANGER, "[x] vkCreateWaylandSurfaceKHR: %s", vkres_msg(res));
			return VK_NULL_HANDLE;
		}
	}
#endif

#ifdef INCLUDE_XCB
	if (uvrvk->surfaceType == UVR_SURFACE_XCB_CLIENT) {
		VkXcbSurfaceCreateInfoKHR xcbSurfaceCreateInfo = {};
		xcbSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
		xcbSurfaceCreateInfo.pNext = NULL;
		xcbSurfaceCreateInfo.flags = 0;
		xcbSurfaceCreateInfo.connection = uvrvk->display;
		xcbSurfaceCreateInfo.window = uvrvk->window;

		res = vkCreateXcbSurfaceKHR(uvrvk->instance, &xcbSurfaceCreateInfo, NULL, &surface);
		if (res) {
			uvr_utils_log(UVR_DANGER, "[x] vkCreateXcbSurfaceKHR: %s", vkres_msg(res));
			return VK_NULL_HANDLE;
		}
	}
#endif

#if defined(INCLUDE_WAYLAND) || defined(INCLUDE_XCB)
	uvr_utils_log(UVR_SUCCESS, "uvr_vk_surface_create: VkSurfaceKHR created retval(%p)", surface);
#endif

	return surface;
}


struct uvr_vk_phdev uvr_vk_phdev_create(struct uvr_vk_phdev_create_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkPhysicalDevice physDevice = VK_NULL_HANDLE;
	uint32_t deviceCount = 0;
	VkPhysicalDevice *devices = NULL;

	if (!uvrvk->instance) {
		uvr_utils_log(UVR_DANGER, "[x] uvr_vk_create_phdev: VkInstance not instantiated");
		uvr_utils_log(UVR_DANGER, "[x] Make a call to uvr_vk_create_instance");
		goto exit_error_vk_phdev_create;
	}

	res = vkEnumeratePhysicalDevices(uvrvk->instance, &deviceCount, NULL);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkEnumeratePhysicalDevices: %s", vkres_msg(res));
		goto exit_error_vk_phdev_create;
	}

	if (deviceCount == 0) {
		uvr_utils_log(UVR_DANGER, "[x] failed to find GPU with Vulkan support!!!");
		goto exit_error_vk_phdev_create;
	}

	devices = (VkPhysicalDevice *) alloca((deviceCount * sizeof(VkPhysicalDevice)) + 1);

	res = vkEnumeratePhysicalDevices(uvrvk->instance, &deviceCount, devices);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkEnumeratePhysicalDevices: %s", vkres_msg(res));
		goto exit_error_vk_phdev_create;
	}

#ifdef INCLUDE_KMS
	/* Get KMS fd stats */
	struct stat drmStat = {0};
	if (uvrvk->kmsfd != -1) {
		if (fstat(uvrvk->kmsfd, &drmStat) == -1) {
			uvr_utils_log(UVR_DANGER, "[x] fstat('%d'): %s", uvrvk->kmsfd, strerror(errno));
			goto exit_error_vk_phdev_create;
		}
	}

	/* Taken from wlroots: https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/render/vulkan/vulkan.c#L349 */
	VkPhysicalDeviceProperties2 physDeviceProperties2;
	VkPhysicalDeviceDrmPropertiesEXT physDeviceDrmProperties;
	memset(&physDeviceProperties2, 0, sizeof(VkPhysicalDeviceProperties2));
	memset(&physDeviceDrmProperties, 0, sizeof(VkPhysicalDeviceDrmPropertiesEXT));

	physDeviceDrmProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT;
	physDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
#endif

	bool enter = 0;
	VkPhysicalDeviceFeatures physDeviceFeatures;
	VkPhysicalDeviceProperties physDeviceProperties;

	/*
	 * get a physical device that is suitable
	 * to do the graphics related task that we need
	 */
	for (uint8_t i = 0; i < deviceCount; i++) {
		vkGetPhysicalDeviceFeatures(devices[i], &physDeviceFeatures); /* Query device features */
		vkGetPhysicalDeviceProperties(devices[i], &physDeviceProperties); /* Query device properties */

#ifdef INCLUDE_KMS
		if (uvrvk->kmsfd) {
			/* Taken from wlroots: https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/render/vulkan/vulkan.c#L311 */
			physDeviceProperties2.pNext = &physDeviceDrmProperties;
			vkGetPhysicalDeviceProperties2(devices[i], &physDeviceProperties2);

			physDeviceDrmProperties.pNext = physDeviceProperties2.pNext;
			dev_t combinedPrimaryDeviceId = makedev(physDeviceDrmProperties.primaryMajor, physDeviceDrmProperties.primaryMinor);
			dev_t combinedRenderDeviceId = makedev(physDeviceDrmProperties.renderMajor, physDeviceDrmProperties.renderMinor);

			/*
			 * Enter will be one if condition succeeds
			 * Enter will be zero if condition fails
			 * Need to make sure even if we have a valid DRI device node fd.
			 * That the customer choosen device type is the same as the DRI device node.
			 */
			enter |= ((combinedPrimaryDeviceId == drmStat.st_rdev || combinedRenderDeviceId == drmStat.st_rdev) && physDeviceProperties.deviceType == uvrvk->deviceType);
		}
#endif

		/* Enter will be one if condition succeeds */
		enter |= (physDeviceProperties.deviceType == uvrvk->deviceType);
		if (enter) {
			memmove(&physDevice, &devices[i], sizeof(devices[i]));
			uvr_utils_log(UVR_SUCCESS, "Suitable GPU Found: %s, api version: %u", physDeviceProperties.deviceName, physDeviceProperties.apiVersion);
			break;
		}
	}

	if (physDevice == VK_NULL_HANDLE) {
		uvr_utils_log(UVR_DANGER, "[x] GPU that meets requirement not found!");
		goto exit_error_vk_phdev_create;
	}

	return (struct uvr_vk_phdev) { .instance = uvrvk->instance, .physDevice = physDevice, .physDeviceProperties = physDeviceProperties, .physDeviceFeatures = physDeviceFeatures,
#ifdef INCLUDE_KMS
	                               .kmsfd = uvrvk->kmsfd, .physDeviceDrmProperties = physDeviceDrmProperties
#endif
	};

exit_error_vk_phdev_create:
	memset(&physDeviceFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
	memset(&physDeviceProperties, 0, sizeof(VkPhysicalDeviceProperties));

#ifdef INCLUDE_KMS
	memset(&physDeviceDrmProperties, 0, sizeof(VkPhysicalDeviceDrmPropertiesEXT));
#endif

	return (struct uvr_vk_phdev) { .instance = VK_NULL_HANDLE, .physDevice = VK_NULL_HANDLE, .physDeviceProperties = physDeviceProperties, .physDeviceFeatures = physDeviceFeatures,
#ifdef INCLUDE_KMS
	                               .kmsfd = -1, .physDeviceDrmProperties = physDeviceDrmProperties
#endif
	};
}


struct uvr_vk_queue uvr_vk_queue_create(struct uvr_vk_queue_create_info *uvrvk)
{
	uint32_t queueCount = 0, flagCount = 0;
	VkQueueFamilyProperties *queueFamilies = NULL;
	struct uvr_vk_queue queue;

	flagCount += (uvrvk->queueFlag & VK_QUEUE_GRAPHICS_BIT) ? 1 : 0;
	flagCount += (uvrvk->queueFlag & VK_QUEUE_COMPUTE_BIT) ? 1 : 0;
	flagCount += (uvrvk->queueFlag & VK_QUEUE_TRANSFER_BIT) ? 1 : 0;
	flagCount += (uvrvk->queueFlag & VK_QUEUE_SPARSE_BINDING_BIT) ? 1 : 0;
	flagCount += (uvrvk->queueFlag & VK_QUEUE_PROTECTED_BIT) ? 1 : 0;

	if (flagCount != 1) {
		uvr_utils_log(UVR_DANGER, "[x] uvr_vk_queue_create: Multiple VkQueueFlags specified, only one allowed per queue");
		goto err_vk_queue_create;
	}

	/*
	 * Almost every operation in Vulkan, requires commands to be submitted to a queue
	 * Find queue family index for a given queue
	 */
	vkGetPhysicalDeviceQueueFamilyProperties(uvrvk->physDevice, &queueCount, NULL);
	queueFamilies = (VkQueueFamilyProperties *) alloca(queueCount * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(uvrvk->physDevice, &queueCount, queueFamilies);

	for (uint32_t i = 0; i < queueCount; i++) {
		queue.queueCount = queueFamilies[i].queueCount;
		if (queueFamilies[i].queueFlags & uvrvk->queueFlag & VK_QUEUE_GRAPHICS_BIT) {
			strncpy(queue.name, "graphics", sizeof(queue.name));
			queue.familyIndex = i; break;
		}

		if (queueFamilies[i].queueFlags & uvrvk->queueFlag & VK_QUEUE_COMPUTE_BIT) {
			strncpy(queue.name, "compute", sizeof(queue.name));
			queue.familyIndex = i; break;
		}

		if (queueFamilies[i].queueFlags & uvrvk->queueFlag & VK_QUEUE_TRANSFER_BIT) {
			strncpy(queue.name, "transfer", sizeof(queue.name));
			queue.familyIndex = i; break;
		}

		if (queueFamilies[i].queueFlags & uvrvk->queueFlag & VK_QUEUE_SPARSE_BINDING_BIT) {
			strncpy(queue.name, "sparse_binding", sizeof(queue.name));
			queue.familyIndex = i; break;
		}

		if (queueFamilies[i].queueFlags & uvrvk->queueFlag & VK_QUEUE_PROTECTED_BIT) {
			strncpy(queue.name, "protected", sizeof(queue.name));
			queue.familyIndex = i; break;
		}
	}

	return queue;

err_vk_queue_create:
	return (struct uvr_vk_queue) { .name[0] = '\0', .queue = VK_NULL_HANDLE, .familyIndex = -1, .queueCount = -1 };
}


struct uvr_vk_lgdev uvr_vk_lgdev_create(struct uvr_vk_lgdev_create_info *uvrvk)
{
	VkDevice logicalDevice = VK_NULL_HANDLE;
	VkResult res = VK_RESULT_MAX_ENUM;
	uint32_t qc;

	VkDeviceQueueCreateInfo *queueCreateInfo = (VkDeviceQueueCreateInfo *) calloc(uvrvk->queueCount, sizeof(VkDeviceQueueCreateInfo));
	if (!queueCreateInfo) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto err_vk_lgdev_create;
	}

	/*
	 * https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/vkspec.html#devsandqueues-priority
	 * set the default priority of all queues to be the highest
	 */
	const float queuePriorities = 1.f;
	for (qc = 0; qc < uvrvk->queueCount; qc++) {
		queueCreateInfo[qc].flags = 0;
		queueCreateInfo[qc].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo[qc].queueFamilyIndex = uvrvk->queues[qc].familyIndex;
		queueCreateInfo[qc].queueCount = uvrvk->queues[qc].queueCount;
		queueCreateInfo[qc].pQueuePriorities = &queuePriorities;
	}

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = NULL;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.queueCreateInfoCount = uvrvk->queueCount;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;
	deviceCreateInfo.enabledLayerCount = 0; // Deprecated and ignored
	deviceCreateInfo.ppEnabledLayerNames = NULL; // Deprecated and ignored
	deviceCreateInfo.enabledExtensionCount = uvrvk->enabledExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = uvrvk->enabledExtensionNames;
	deviceCreateInfo.pEnabledFeatures = uvrvk->enabledFeatures;

	/* Create logic device */
	res = vkCreateDevice(uvrvk->physDevice, &deviceCreateInfo, NULL, &logicalDevice);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkCreateDevice: %s", vkres_msg(res));
		goto err_vk_lgdev_free_queueCreateInfo;
	}

	for (qc = 0; qc < uvrvk->queueCount; qc++) {
		vkGetDeviceQueue(logicalDevice, uvrvk->queues[qc].familyIndex, 0, &uvrvk->queues[qc].queue);
		if (!uvrvk->queues[qc].queue)  {
			uvr_utils_log(UVR_DANGER, "[x] vkGetDeviceQueue: Failed to get %s queue handle", uvrvk->queues[qc].name);
			goto err_vk_lgdev_destroy;
		}

		uvr_utils_log(UVR_SUCCESS, "uvr_vk_lgdev_create: '%s' VkQueue successfully created retval(%p)", uvrvk->queues[qc].name, uvrvk->queues[qc].queue);
	}

	uvr_utils_log(UVR_SUCCESS, "uvr_vk_lgdev_create: VkDevice created retval(%p)", logicalDevice);

	free(queueCreateInfo);
	return (struct uvr_vk_lgdev) { .logicalDevice = logicalDevice, .queueCount = uvrvk->queueCount, .queues = uvrvk->queues };

err_vk_lgdev_destroy:
	if (logicalDevice)
		vkDestroyDevice(logicalDevice, NULL);
err_vk_lgdev_free_queueCreateInfo:
	free(queueCreateInfo);
err_vk_lgdev_create:
	return (struct uvr_vk_lgdev) { .logicalDevice = VK_NULL_HANDLE, .queueCount = -1, .queues = NULL };
}


struct uvr_vk_swapchain uvr_vk_swapchain_create(struct uvr_vk_swapchain_create_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;

	if (uvrvk->surfaceCapabilities.currentExtent.width != UINT32_MAX) {
		uvrvk->extent2D = uvrvk->surfaceCapabilities.currentExtent;
	} else {
		uvrvk->extent2D.width = fmax(uvrvk->surfaceCapabilities.minImageExtent.width, fmin(uvrvk->surfaceCapabilities.maxImageExtent.width, uvrvk->extent2D.width));
		uvrvk->extent2D.height = fmax(uvrvk->surfaceCapabilities.minImageExtent.height, fmin(uvrvk->surfaceCapabilities.maxImageExtent.height, uvrvk->extent2D.height));
	}

	VkSwapchainCreateInfoKHR createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.surface = uvrvk->surface;
	if (uvrvk->surfaceCapabilities.maxImageCount > 0 && (uvrvk->surfaceCapabilities.minImageCount + 1) < uvrvk->surfaceCapabilities.maxImageCount)
		createInfo.minImageCount = uvrvk->surfaceCapabilities.maxImageCount;
	else
		createInfo.minImageCount = uvrvk->surfaceCapabilities.minImageCount + 1;
	createInfo.imageFormat = uvrvk->surfaceFormat.format;
	createInfo.imageColorSpace = uvrvk->surfaceFormat.colorSpace;
	createInfo.imageExtent = uvrvk->extent2D;
	createInfo.imageArrayLayers = uvrvk->imageArrayLayers;
	createInfo.imageUsage = uvrvk->imageUsage;
	createInfo.imageSharingMode = uvrvk->imageSharingMode;
	createInfo.queueFamilyIndexCount = uvrvk->queueFamilyIndexCount;
	createInfo.pQueueFamilyIndices = uvrvk->queueFamilyIndices;
	createInfo.preTransform = uvrvk->surfaceCapabilities.currentTransform;
	createInfo.compositeAlpha = uvrvk->compositeAlpha;
	createInfo.presentMode = uvrvk->presentMode;
	createInfo.clipped = uvrvk->clipped;
	createInfo.oldSwapchain = uvrvk->oldSwapChain;

	res = vkCreateSwapchainKHR(uvrvk->logicalDevice, &createInfo, NULL, &swapchain);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkCreateSwapchainKHR: %s", vkres_msg(res));
		goto exit_vk_swapchain;
	}

	uvr_utils_log(UVR_SUCCESS, "uvr_vk_swapchain_create: VkSwapchainKHR successfully created retval(%p)", swapchain);

	return (struct uvr_vk_swapchain) { .logicalDevice = uvrvk->logicalDevice, .swapchain = swapchain };

exit_vk_swapchain:
	return (struct uvr_vk_swapchain) { .logicalDevice = VK_NULL_HANDLE, .swapchain = VK_NULL_HANDLE };
}


struct uvr_vk_image uvr_vk_image_create(struct uvr_vk_image_create_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	uint32_t imageCount = 0, i = 0, curImage = 0;
	VkImage *vkImages = NULL;
	VkDeviceMemory *deviceMemories = NULL;

	struct uvr_vk_image_handle *imageHandles = NULL;
	struct uvr_vk_image_view_handle *imageViewHandles = NULL;

	if (uvrvk->swapchain) {
		res = vkGetSwapchainImagesKHR(uvrvk->logicalDevice, uvrvk->swapchain, &imageCount, NULL);
		if (res) {
			uvr_utils_log(UVR_DANGER, "[x] vkGetSwapchainImagesKHR: %s", vkres_msg(res));
			goto exit_vk_image;
		}

		vkImages = alloca(imageCount * sizeof(VkImage));

		res = vkGetSwapchainImagesKHR(uvrvk->logicalDevice, uvrvk->swapchain, &imageCount, vkImages);
		if (res) {
			uvr_utils_log(UVR_DANGER, "[x] vkGetSwapchainImagesKHR: %s", vkres_msg(res));
			goto exit_vk_image;
		}

		uvr_utils_log(UVR_INFO, "uvr_vk_image_create: Total images in swapchain %u", imageCount);
	} else {

		VkImageCreateInfo imageCreateInfo;
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.pNext = NULL;

		imageCount = uvrvk->imageCount;
		vkImages = alloca(imageCount * sizeof(VkImage));
		deviceMemories = alloca(imageCount * sizeof(VkDeviceMemory));

		memset(vkImages, 0, imageCount * sizeof(VkImage));
		memset(deviceMemories, 0, imageCount * sizeof(VkDeviceMemory));

		VkMemoryRequirements memoryRequirements;
		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		for (i = 0; i < imageCount; i++) {
			curImage = (uvrvk->imageCount) ? i : 0;

			imageCreateInfo.flags = uvrvk->imageCreateInfos[curImage].imageflags;
			imageCreateInfo.imageType = uvrvk->imageCreateInfos[curImage].imageType;
			imageCreateInfo.format = uvrvk->imageCreateInfos[curImage].imageFormat;
			imageCreateInfo.extent = uvrvk->imageCreateInfos[curImage].imageExtent3D;
			imageCreateInfo.mipLevels = uvrvk->imageCreateInfos[curImage].imageMipLevels;
			imageCreateInfo.arrayLayers = uvrvk->imageCreateInfos[curImage].imageArrayLayers;
			imageCreateInfo.samples = uvrvk->imageCreateInfos[curImage].imageSamples;
			imageCreateInfo.tiling = uvrvk->imageCreateInfos[curImage].imageTiling;
			imageCreateInfo.usage = uvrvk->imageCreateInfos[curImage].imageUsage;
			imageCreateInfo.sharingMode = uvrvk->imageCreateInfos[curImage].imageSharingMode;
			imageCreateInfo.queueFamilyIndexCount = uvrvk->imageCreateInfos[curImage].imageQueueFamilyIndexCount;
			imageCreateInfo.pQueueFamilyIndices = uvrvk->imageCreateInfos[curImage].imageQueueFamilyIndices;
			imageCreateInfo.initialLayout = uvrvk->imageCreateInfos[curImage].imageInitialLayout;

			res = vkCreateImage(uvrvk->logicalDevice, &imageCreateInfo, NULL, &vkImages[i]);
			if (res) {
				uvr_utils_log(UVR_DANGER, "[x] vkCreateImage: %s", vkres_msg(res));
				goto exit_vk_image_free_images;
			}

			vkGetImageMemoryRequirements(uvrvk->logicalDevice, vkImages[i], &memoryRequirements);

			allocInfo.allocationSize = memoryRequirements.size;
			allocInfo.memoryTypeIndex = retrieve_memory_type_index(uvrvk->physDevice, memoryRequirements.memoryTypeBits, uvrvk->memPropertyFlags);

			res = vkAllocateMemory(uvrvk->logicalDevice, &allocInfo, NULL, &deviceMemories[i]);
			if (res) {
				uvr_utils_log(UVR_DANGER, "[x] vkAllocateMemory: %s", vkres_msg(res));
				goto exit_vk_image_free_images;
			}

			vkBindImageMemory(uvrvk->logicalDevice, vkImages[i], deviceMemories[i], 0);
		}
	}

	imageHandles = calloc(imageCount, sizeof(struct uvr_vk_image_handle));
	if (!imageHandles) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_image_free_images;
	}

	imageViewHandles = calloc(imageCount, sizeof(struct uvr_vk_image_view_handle));
	if (!imageViewHandles) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_image_free_images;
	}

	VkImageViewCreateInfo imageViewCreateInfo;
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = NULL;

	for (i = 0; i < imageCount; i++) {
		curImage = (uvrvk->imageCount) ? i : 0;

		imageViewCreateInfo.flags = uvrvk->imageViewCreateInfos[curImage].imageViewflags;
		imageViewCreateInfo.viewType = uvrvk->imageViewCreateInfos[curImage].imageViewType;
		imageViewCreateInfo.format = uvrvk->imageViewCreateInfos[curImage].imageViewFormat;
		imageViewCreateInfo.components = uvrvk->imageViewCreateInfos[curImage].imageViewComponents;
		imageViewCreateInfo.subresourceRange = uvrvk->imageViewCreateInfos[curImage].imageViewSubresourceRange;

		imageViewCreateInfo.image = imageHandles[i].image = vkImages[i];
		imageHandles[i].deviceMemory = VK_NULL_HANDLE;

		if (deviceMemories) {
			if (deviceMemories[i])
				imageHandles[i].deviceMemory = deviceMemories[i];
		}

		res = vkCreateImageView(uvrvk->logicalDevice, &imageViewCreateInfo, NULL, &imageViewHandles[i].view);
		if (res) {
			uvr_utils_log(UVR_DANGER, "[x] vkCreateImageView: %s", vkres_msg(res));
			goto exit_vk_image_free_image_views;
		}

		uvr_utils_log(UVR_WARNING, "uvr_vk_image_create: VkImage (%p) associate with VkImageView", imageHandles[i].image);
		uvr_utils_log(UVR_SUCCESS, "uvr_vk_image_create: VkImageView successfully created retval(%p)", imageViewHandles[i].view);
	}

	return (struct uvr_vk_image) { .logicalDevice = uvrvk->logicalDevice, .imageCount = imageCount, .imageHandles = imageHandles,
	                               .imageViewHandles = imageViewHandles, .swapchain = uvrvk->swapchain };

exit_vk_image_free_image_views:
	if (imageViewHandles) {
		for (i = 0; i < imageCount; i++) {
			if (imageViewHandles[i].view)
				vkDestroyImageView(uvrvk->logicalDevice, imageViewHandles[i].view, NULL);
		}
	}
	free(imageViewHandles);
exit_vk_image_free_images:
	if (vkImages && deviceMemories && !uvrvk->swapchain) {
		for (i = 0; i < imageCount; i++) {
			if (vkImages[i])
				vkDestroyImage(uvrvk->logicalDevice, vkImages[i], NULL);
			if (deviceMemories[i])
				vkFreeMemory(uvrvk->logicalDevice, deviceMemories[i], NULL);
		}
	}
	free(imageHandles);
exit_vk_image:
	return (struct uvr_vk_image) { .logicalDevice = VK_NULL_HANDLE, .imageCount = 0, .imageHandles = NULL,
	                               .imageViewHandles = NULL, .swapchain = VK_NULL_HANDLE };
}


struct uvr_vk_shader_module uvr_vk_shader_module_create(struct uvr_vk_shader_module_create_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkShaderModule shaderModule = VK_NULL_HANDLE;

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.codeSize = uvrvk->sprivByteSize;
	createInfo.pCode = (const uint32_t *) uvrvk->sprivBytes;

	res = vkCreateShaderModule(uvrvk->logicalDevice, &createInfo, NULL, &shaderModule);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkCreateShaderModule: %s", vkres_msg(res));
		goto exit_vk_shader_module;
	}

	uvr_utils_log(UVR_SUCCESS, "uvr_vk_shader_module_create: '%s' shader VkShaderModule successfully created retval(%p)", uvrvk->shaderName, shaderModule);

	return (struct uvr_vk_shader_module) { .logicalDevice = uvrvk->logicalDevice, .shaderModule = shaderModule, .shaderName = uvrvk->shaderName };

exit_vk_shader_module:
	return (struct uvr_vk_shader_module) { .logicalDevice = VK_NULL_HANDLE, .shaderModule = VK_NULL_HANDLE, .shaderName = NULL };
}


struct uvr_vk_pipeline_layout uvr_vk_pipeline_layout_create(struct uvr_vk_pipeline_layout_create_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.setLayoutCount = uvrvk->descriptorSetLayoutCount;
	createInfo.pSetLayouts = uvrvk->descriptorSetLayouts;
	createInfo.pushConstantRangeCount = uvrvk->pushConstantRangeCount;
	createInfo.pPushConstantRanges = uvrvk->pushConstantRanges;

	res = vkCreatePipelineLayout(uvrvk->logicalDevice, &createInfo, NULL, &pipelineLayout);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkCreatePipelineLayout: %s", vkres_msg(res));
		goto exit_vk_pipeline_layout;
	}

	uvr_utils_log(UVR_SUCCESS, "uvr_vk_pipeline_layout_create: VkPipelineLayout successfully created retval(%p)", pipelineLayout);

	return (struct uvr_vk_pipeline_layout) { .logicalDevice = uvrvk->logicalDevice, .pipelineLayout = pipelineLayout };

exit_vk_pipeline_layout:
	return (struct uvr_vk_pipeline_layout) { .logicalDevice = VK_NULL_HANDLE, .pipelineLayout = VK_NULL_HANDLE };
}


struct uvr_vk_render_pass uvr_vk_render_pass_create(struct uvr_vk_render_pass_create_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkRenderPass renderPass = VK_NULL_HANDLE;

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.attachmentCount = uvrvk->attachmentDescriptionCount;
	createInfo.pAttachments = uvrvk->attachmentDescriptions;
	createInfo.subpassCount = uvrvk->subpassDescriptionCount;
	createInfo.pSubpasses = uvrvk->subpassDescriptions;
	createInfo.dependencyCount = uvrvk->subpassDependencyCount;
	createInfo.pDependencies = uvrvk->subpassDependencies;

	res = vkCreateRenderPass(uvrvk->logicalDevice, &createInfo, NULL, &renderPass);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkCreateRenderPass: %s", vkres_msg(res));
		goto exit_vk_render_pass;
	}

	uvr_utils_log(UVR_SUCCESS, "uvr_vk_render_pass_create: VkRenderPass successfully created retval(%p)", renderPass);

	return (struct uvr_vk_render_pass) { .logicalDevice = uvrvk->logicalDevice, .renderPass = renderPass };

exit_vk_render_pass:
	return (struct uvr_vk_render_pass) { .logicalDevice = VK_NULL_HANDLE, .renderPass = VK_NULL_HANDLE };
}


struct uvr_vk_graphics_pipeline uvr_vk_graphics_pipeline_create(struct uvr_vk_graphics_pipeline_create_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;

	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.stageCount = uvrvk->shaderStageCount;
	createInfo.pStages = uvrvk->shaderStages;
	createInfo.pVertexInputState = uvrvk->vertexInputState;
	createInfo.pInputAssemblyState = uvrvk->inputAssemblyState;
	createInfo.pTessellationState = uvrvk->tessellationState;
	createInfo.pViewportState = uvrvk->viewportState;
	createInfo.pRasterizationState = uvrvk->rasterizationState;
	createInfo.pMultisampleState = uvrvk->multisampleState;
	createInfo.pDepthStencilState = uvrvk->depthStencilState;
	createInfo.pColorBlendState = uvrvk->colorBlendState;
	createInfo.pDynamicState = uvrvk->dynamicState;
	createInfo.layout = uvrvk->pipelineLayout;
	createInfo.renderPass = uvrvk->renderPass;
	createInfo.subpass = uvrvk->subpass;
	// Won't be supporting
	createInfo.basePipelineHandle = VK_NULL_HANDLE;
	createInfo.basePipelineIndex = -1;

	res = vkCreateGraphicsPipelines(uvrvk->logicalDevice, NULL, 1, &createInfo, NULL, &graphicsPipeline);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkCreateGraphicsPipelines: %s", vkres_msg(res));
		goto exit_vk_graphics_pipeline;
	}

	uvr_utils_log(UVR_SUCCESS, "uvr_vk_graphics_pipeline_create: VkPipeline successfully created retval(%p)", graphicsPipeline);

	return (struct uvr_vk_graphics_pipeline) { .logicalDevice = uvrvk->logicalDevice, .graphicsPipeline = graphicsPipeline };

exit_vk_graphics_pipeline:
	return (struct uvr_vk_graphics_pipeline) { .logicalDevice = VK_NULL_HANDLE, .graphicsPipeline = VK_NULL_HANDLE };
}


struct uvr_vk_framebuffer uvr_vk_framebuffer_create(struct uvr_vk_framebuffer_create_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	uint8_t currentFrameBuffer = 0;

	struct uvr_vk_framebuffer_handle *framebufferHandles = NULL;

	framebufferHandles = (struct uvr_vk_framebuffer_handle *) calloc(uvrvk->framebufferCount, sizeof(struct uvr_vk_framebuffer_handle));
	if (!framebufferHandles) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_framebuffer;
	}

	VkFramebufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.renderPass = uvrvk->renderPass;
	createInfo.width = uvrvk->width;
	createInfo.height = uvrvk->height;
	createInfo.layers = uvrvk->layers;

	for (currentFrameBuffer = 0; currentFrameBuffer < uvrvk->framebufferCount; currentFrameBuffer++) {
		createInfo.attachmentCount = uvrvk->framebufferImageAttachmentCount;
		createInfo.pAttachments = uvrvk->framebufferImages[currentFrameBuffer].imageAttachments;

		res = vkCreateFramebuffer(uvrvk->logicalDevice, &createInfo, NULL, &framebufferHandles[currentFrameBuffer].framebuffer);
		if (res) {
			uvr_utils_log(UVR_DANGER, "[x] vkCreateFramebuffer: %s", vkres_msg(res));
			goto exit_vk_framebuffer_vk_framebuffer_destroy;
		}

		uvr_utils_log(UVR_SUCCESS, "uvr_vk_framebuffer_create: VkFramebuffer successfully created retval(%p)", framebufferHandles[currentFrameBuffer].framebuffer);
	}

	return (struct uvr_vk_framebuffer) { .logicalDevice = uvrvk->logicalDevice, .framebufferCount = uvrvk->framebufferCount, .framebufferHandles = framebufferHandles };

exit_vk_framebuffer_vk_framebuffer_destroy:
	for (currentFrameBuffer = 0; currentFrameBuffer < uvrvk->framebufferCount; currentFrameBuffer++) {
		if (framebufferHandles[currentFrameBuffer].framebuffer)
			vkDestroyFramebuffer(uvrvk->logicalDevice, framebufferHandles[currentFrameBuffer].framebuffer, NULL);
	}
	free(framebufferHandles);
exit_vk_framebuffer:
	return (struct uvr_vk_framebuffer) { .logicalDevice = VK_NULL_HANDLE, .framebufferCount = 0, .framebufferHandles = NULL };
}


struct uvr_vk_command_buffer uvr_vk_command_buffer_create(struct uvr_vk_command_buffer_create_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkCommandPool commandPool = VK_NULL_HANDLE;
	VkCommandBuffer *commandBuffers = VK_NULL_HANDLE;

	struct uvr_vk_command_buffer_handle *commandBufferHandles = NULL;

	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = uvrvk->queueFamilyIndex;

	res = vkCreateCommandPool(uvrvk->logicalDevice, &createInfo, NULL, &commandPool);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkCreateCommandPool: %s", vkres_msg(res));
		goto exit_vk_command_buffer;
	}

	uvr_utils_log(UVR_SUCCESS, "uvr_vk_command_buffer_create: VkCommandPool successfully created retval(%p)", commandPool);

	commandBuffers = alloca(uvrvk->commandBufferCount * sizeof(VkCommandBuffer));

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = uvrvk->commandBufferCount;

	res = vkAllocateCommandBuffers(uvrvk->logicalDevice, &allocInfo, commandBuffers);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkAllocateCommandBuffers: %s", vkres_msg(res));
		goto exit_vk_command_buffer_destroy_cmd_pool;
	}

	commandBufferHandles = calloc(uvrvk->commandBufferCount, sizeof(struct uvr_vk_command_buffer_handle));
	if (!commandBufferHandles) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_command_buffer_destroy_cmd_pool;
	}

	for (uint32_t i = 0; i < uvrvk->commandBufferCount; i++) {
		commandBufferHandles[i].commandBuffer = commandBuffers[i];
		uvr_utils_log(UVR_WARNING, "uvr_vk_command_buffer_create: VkCommandBuffer successfully created retval(%p)", commandBufferHandles[i].commandBuffer);
	}

	return (struct uvr_vk_command_buffer) { .logicalDevice = uvrvk->logicalDevice, .commandPool = commandPool,
	                                        .commandBufferCount = uvrvk->commandBufferCount,
	                                        .commandBufferHandles = commandBufferHandles };

exit_vk_command_buffer_destroy_cmd_pool:
	vkDestroyCommandPool(uvrvk->logicalDevice, commandPool, NULL);
exit_vk_command_buffer:
	return (struct uvr_vk_command_buffer) { .logicalDevice = VK_NULL_HANDLE, .commandPool = VK_NULL_HANDLE,
	                                        .commandBufferCount = 0, .commandBufferHandles = NULL };
}


int uvr_vk_command_buffer_record_begin(struct uvr_vk_command_buffer_record_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = NULL;
	beginInfo.flags = uvrvk->commandBufferUsageflags;
	// We don't use secondary command buffers in API so set to null
	beginInfo.pInheritanceInfo = NULL;

	for (uint32_t i = 0; i < uvrvk->commandBufferCount; i++) {
		res = vkBeginCommandBuffer(uvrvk->commandBufferHandles[i].commandBuffer, &beginInfo);
		if (res) {
			uvr_utils_log(UVR_DANGER, "[x] vkBeginCommandBuffer: %s", vkres_msg(res));
			return -1;
		}
	}

	return 0;
}


int uvr_vk_command_buffer_record_end(struct uvr_vk_command_buffer_record_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;

	for (uint32_t i = 0; i < uvrvk->commandBufferCount; i++) {
		res = vkEndCommandBuffer(uvrvk->commandBufferHandles[i].commandBuffer);
		if (res) {
			uvr_utils_log(UVR_DANGER, "[x] vkEndCommandBuffer: %s", vkres_msg(res));
			return -1;
		}
	}

	return 0;
}


struct uvr_vk_sync_obj uvr_vk_sync_obj_create(struct uvr_vk_sync_obj_create_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	uint32_t currentSyncObject = 0;

	struct uvr_vk_fence_handle *fenceHandles = NULL;
	struct uvr_vk_semaphore_handle *semaphoreHandles = NULL;

	fenceHandles = calloc(uvrvk->fenceCount, sizeof(struct uvr_vk_fence_handle));
	if (!fenceHandles) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_sync_obj;
	}

	semaphoreHandles = calloc(uvrvk->semaphoreCount, sizeof(struct uvr_vk_semaphore_handle));
	if (!semaphoreHandles) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_sync_obj_free_vk_fence;
	}

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = NULL;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo semphoreCreateInfo = {};
	semphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semphoreCreateInfo.pNext = NULL;
	semphoreCreateInfo.flags = 0;

	for (currentSyncObject = 0; currentSyncObject < uvrvk->fenceCount; currentSyncObject++) {
		res = vkCreateFence(uvrvk->logicalDevice, &fenceCreateInfo, NULL, &fenceHandles[currentSyncObject].fence);
		if (res) {
			uvr_utils_log(UVR_DANGER, "[x] vkCreateFence: %s", vkres_msg(res));
			goto exit_vk_sync_obj_destroy_vk_fence;
		}
	}

	for (currentSyncObject = 0; currentSyncObject < uvrvk->semaphoreCount; currentSyncObject++) {
		res = vkCreateSemaphore(uvrvk->logicalDevice, &semphoreCreateInfo, NULL, &semaphoreHandles[currentSyncObject].semaphore);
		if (res) {
			uvr_utils_log(UVR_DANGER, "[x] vkCreateSemaphore: %s", vkres_msg(res));
			goto exit_vk_sync_obj_destroy_vk_semaphore;
		}
	}

	return (struct uvr_vk_sync_obj) { .logicalDevice = uvrvk->logicalDevice, .fenceCount = uvrvk->fenceCount, .fenceHandles = fenceHandles,
	                                  .semaphoreCount = uvrvk->semaphoreCount, .semaphoreHandles = semaphoreHandles };

exit_vk_sync_obj_destroy_vk_semaphore:
	for (currentSyncObject = 0; currentSyncObject < uvrvk->semaphoreCount; currentSyncObject++)
		if (semaphoreHandles[currentSyncObject].semaphore)
			vkDestroySemaphore(uvrvk->logicalDevice, semaphoreHandles[currentSyncObject].semaphore, NULL);
exit_vk_sync_obj_destroy_vk_fence:
	for (currentSyncObject = 0; currentSyncObject < uvrvk->fenceCount; currentSyncObject++)
		if (fenceHandles[currentSyncObject].fence)
			vkDestroyFence(uvrvk->logicalDevice, fenceHandles[currentSyncObject].fence, NULL);
//exit_vk_sync_obj_free_vk_semaphore:
	free(semaphoreHandles);
exit_vk_sync_obj_free_vk_fence:
	free(fenceHandles);
exit_vk_sync_obj:
	return (struct uvr_vk_sync_obj) { .logicalDevice = VK_NULL_HANDLE, .fenceCount = 0, .fenceHandles = NULL, .semaphoreCount = 0, .semaphoreHandles = NULL };
};


struct uvr_vk_buffer uvr_vk_buffer_create(struct uvr_vk_buffer_create_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory deviceMemory = VK_NULL_HANDLE;

	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = uvrvk->bufferFlags;
	createInfo.size  = uvrvk->bufferSize;
	createInfo.usage = uvrvk->bufferUsage;
	createInfo.sharingMode = uvrvk->bufferSharingMode;
	createInfo.queueFamilyIndexCount = uvrvk->queueFamilyIndexCount;
	createInfo.pQueueFamilyIndices = uvrvk->queueFamilyIndices;

	/* Creates underlying buffer header */
	res = vkCreateBuffer(uvrvk->logicalDevice, &createInfo, NULL, &buffer);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkCreateBuffer: %s", vkres_msg(res));
		goto exit_vk_buffer;
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(uvrvk->logicalDevice, buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.memoryTypeIndex = retrieve_memory_type_index(uvrvk->physDevice, memoryRequirements.memoryTypeBits, uvrvk->memPropertyFlags);

	res = vkAllocateMemory(uvrvk->logicalDevice, &allocInfo, NULL, &deviceMemory);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkAllocateMemory: %s", vkres_msg(res));
		goto exit_vk_buffer;
	}

	vkBindBufferMemory(uvrvk->logicalDevice, buffer, deviceMemory, 0);

	return (struct uvr_vk_buffer) { .logicalDevice = uvrvk->logicalDevice, .buffer = buffer, .deviceMemory = deviceMemory };

exit_vk_buffer:
	return (struct uvr_vk_buffer) { .logicalDevice = VK_NULL_HANDLE, .buffer = VK_NULL_HANDLE, .deviceMemory = VK_NULL_HANDLE };
}


struct uvr_vk_descriptor_set_layout uvr_vk_descriptor_set_layout_create(struct uvr_vk_descriptor_set_layout_create_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

	VkDescriptorSetLayoutCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = uvrvk->descriptorSetLayoutCreateflags;
	createInfo.bindingCount = uvrvk->descriptorSetLayoutBindingCount;
	createInfo.pBindings = uvrvk->descriptorSetLayoutBindings;

	res = vkCreateDescriptorSetLayout(uvrvk->logicalDevice, &createInfo, VK_NULL_HANDLE, &descriptorSetLayout);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkCreateDescriptorSetLayout: %s", vkres_msg(res));
		goto exit_vk_descriptor_set_layout;
	}

	return (struct uvr_vk_descriptor_set_layout) { .logicalDevice = uvrvk->logicalDevice, .descriptorSetLayout = descriptorSetLayout };

exit_vk_descriptor_set_layout:
	return (struct uvr_vk_descriptor_set_layout) { .logicalDevice = VK_NULL_HANDLE, .descriptorSetLayout = VK_NULL_HANDLE };
}


struct uvr_vk_descriptor_set uvr_vk_descriptor_set_create(struct uvr_vk_descriptor_set_create_info *uvrvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet *descriptorSets = VK_NULL_HANDLE;

	struct uvr_vk_descriptor_set_handle *descriptorSetHandles = NULL;

	/*
	 * Per my understanding allocate VkDescriptorPool given a certain amount of information. Like the amount of sets,
	 * the layout for a given set, and descriptors. No descriptor is assigned to a set when the pool is created.
	 * Given a descriptor set layout the actual assignment of descriptor to descriptor set happens in the
	 * vkAllocateDescriptorSets function.
	 */
	VkDescriptorPoolCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = uvrvk->descriptorPoolCreateflags;
	createInfo.maxSets = uvrvk->descriptorSetLayoutCount;
	createInfo.poolSizeCount = uvrvk->descriptorPoolInfoCount;
	createInfo.pPoolSizes = uvrvk->descriptorPoolInfos;

	res = vkCreateDescriptorPool(uvrvk->logicalDevice, &createInfo, VK_NULL_HANDLE, &descriptorPool);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkCreateDescriptorSetLayout: %s", vkres_msg(res));
		goto exit_vk_descriptor_set;
	}

	descriptorSets = alloca(uvrvk->descriptorSetLayoutCount * sizeof(VkDescriptorSet));

	VkDescriptorSetAllocateInfo allocInfo;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = NULL;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = uvrvk->descriptorSetLayoutCount;
	allocInfo.pSetLayouts = uvrvk->descriptorSetLayouts;

	res = vkAllocateDescriptorSets(uvrvk->logicalDevice, &allocInfo, descriptorSets);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkAllocateDescriptorSets: %s", vkres_msg(res));
		goto exit_vk_descriptor_set_destroy_pool;
	}

	descriptorSetHandles = calloc(uvrvk->descriptorSetLayoutCount, sizeof(struct uvr_vk_descriptor_set_handle));
	if (!descriptorSetHandles) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_descriptor_set_destroy_pool;
	}

	for (uint32_t i = 0; i < uvrvk->descriptorSetLayoutCount; i++) {
		descriptorSetHandles[i].descriptorSet = descriptorSets[i];
		uvr_utils_log(UVR_WARNING, "uvr_vk_descriptor_set_create: VkDescriptorSet successfully created retval(%p)", descriptorSetHandles[i].descriptorSet);
	}

	return (struct uvr_vk_descriptor_set) { .logicalDevice = uvrvk->logicalDevice, .descriptorPool = descriptorPool,
	                                        .descriptorSetHandles = descriptorSetHandles, .descriptorSetsCount = uvrvk->descriptorSetLayoutCount };

exit_vk_descriptor_set_destroy_pool:
	if (descriptorPool)
		vkDestroyDescriptorPool(uvrvk->logicalDevice, descriptorPool, NULL);
exit_vk_descriptor_set:
	return (struct uvr_vk_descriptor_set) { .logicalDevice = VK_NULL_HANDLE, .descriptorPool = VK_NULL_HANDLE, .descriptorSetHandles = VK_NULL_HANDLE, .descriptorSetsCount = 0 };
}


struct uvr_vk_sampler uvr_vk_sampler_create(struct uvr_vk_sampler_create_info *uvrvk)
{
	VkSampler sampler = VK_NULL_HANDLE;
	VkResult res = VK_RESULT_MAX_ENUM;

	VkSamplerCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = uvrvk->samplerFlags;
	createInfo.magFilter = uvrvk->samplerMagFilter;
	createInfo.minFilter = uvrvk->samplerMinFilter;
	createInfo.mipmapMode = uvrvk->samplerMipmapMode;
	createInfo.addressModeU = uvrvk->samplerAddressModeU;
	createInfo.addressModeV = uvrvk->samplerAddressModeV;
	createInfo.addressModeW = uvrvk->samplerAddressModeW;
	createInfo.mipLodBias = uvrvk->samplerMipLodBias;
	createInfo.anisotropyEnable = uvrvk->samplerAnisotropyEnable;
	createInfo.maxAnisotropy = uvrvk->samplerMaxAnisotropy;
	createInfo.compareEnable = uvrvk->samplerCompareEnable;
	createInfo.compareOp = uvrvk->samplerCompareOp;
	createInfo.minLod = uvrvk->samplerMinLod;
	createInfo.maxLod = uvrvk->samplerMaxLod;
	createInfo.borderColor = uvrvk->samplerBorderColor;
	createInfo.unnormalizedCoordinates = uvrvk->samplerUnnormalizedCoordinates;

	res = vkCreateSampler(uvrvk->logicalDevice, &createInfo, NULL, &sampler);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkCreateSampler: %s", vkres_msg(res));
		goto exit_vk_sampler;
	}

	uvr_utils_log(UVR_SUCCESS, "uvr_vk_sampler_create: VkSampler created retval(%p)", sampler);

	return (struct uvr_vk_sampler) { .logicalDevice = uvrvk->logicalDevice, .sampler = sampler };

exit_vk_sampler:
	return (struct uvr_vk_sampler) { .logicalDevice = VK_NULL_HANDLE, .sampler = VK_NULL_HANDLE };
}


int uvr_vk_resource_copy(struct uvr_vk_resource_copy_info *uvrvk)
{
	struct uvr_vk_command_buffer_handle commandBufferHandle;
	commandBufferHandle.commandBuffer = uvrvk->commandBuffer;

	struct uvr_vk_command_buffer_record_info commandBufferRecordInfo;
	commandBufferRecordInfo.commandBufferCount = 1;
	commandBufferRecordInfo.commandBufferHandles = &commandBufferHandle;
	commandBufferRecordInfo.commandBufferUsageflags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (uvr_vk_command_buffer_record_begin(&commandBufferRecordInfo) == -1)
		return -1;

	VkCommandBuffer commandBuffer = commandBufferHandle.commandBuffer;

	switch (uvrvk->resourceCopyType) {
		case UVR_VK_COPY_VK_BUFFER_TO_VK_BUFFER:
		{
			vkCmdCopyBuffer(commandBuffer, (VkBuffer) uvrvk->srcResource, (VkBuffer) uvrvk->dstResource, 1, uvrvk->bufferCopyInfo);
			break;
		}
		case UVR_VK_COPY_VK_BUFFER_TO_VK_IMAGE:
		{
			vkCmdCopyBufferToImage(commandBuffer, (VkBuffer) uvrvk->srcResource, (VkImage) uvrvk->dstResource, uvrvk->imageLayout, 1, uvrvk->bufferImageCopyInfo);
			break;
		}
		case UVR_VK_COPY_VK_IMAGE_TO_VK_BUFFER:
		{
			vkCmdCopyImageToBuffer(commandBuffer, (VkImage) uvrvk->srcResource, uvrvk->imageLayout, (VkBuffer) uvrvk->dstResource,  1, uvrvk->bufferImageCopyInfo);
			break;
		}
		default:
			uvr_utils_log(UVR_DANGER, "[x] uvr_vk_buffer_copy: Must pass a valid uvr_vk_buffer_copy_type value");
			return -1;
	}

	if (uvr_vk_command_buffer_record_end(&commandBufferRecordInfo) == -1)
		return -1;

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = NULL;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = NULL;
	submitInfo.pWaitDstStageMask = NULL;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = NULL;

	vkQueueSubmit(uvrvk->queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(uvrvk->queue);

	return 0;
}


int uvr_vk_resource_pipeline_barrier(struct uvr_vk_resource_pipeline_barrier_info *uvrvk)
{
	struct uvr_vk_command_buffer_handle commandBufferHandle;
	commandBufferHandle.commandBuffer = uvrvk->commandBuffer;

	struct uvr_vk_command_buffer_record_info commandBufferRecordInfo;
	commandBufferRecordInfo.commandBufferCount = 1;
	commandBufferRecordInfo.commandBufferHandles = &commandBufferHandle;
	commandBufferRecordInfo.commandBufferUsageflags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (uvr_vk_command_buffer_record_begin(&commandBufferRecordInfo) == -1)
		return -1;

	vkCmdPipelineBarrier(commandBufferHandle.commandBuffer,
	                     uvrvk->srcPipelineStage,
	                     uvrvk->dstPipelineStage,
	                     uvrvk->dependencyFlags,
	                     (uvrvk->memoryBarrier      ) ? 1 : 0, uvrvk->memoryBarrier,
	                     (uvrvk->bufferMemoryBarrier) ? 1 : 0, uvrvk->bufferMemoryBarrier,
	                     (uvrvk->imageMemoryBarrier ) ? 1 : 0, uvrvk->imageMemoryBarrier);

	if (uvr_vk_command_buffer_record_end(&commandBufferRecordInfo) == -1)
		return -1;

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = NULL;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = NULL;
	submitInfo.pWaitDstStageMask = NULL;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBufferHandle.commandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = NULL;

	vkQueueSubmit(uvrvk->queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(uvrvk->queue);

	return 0;
}


VkSurfaceCapabilitiesKHR uvr_vk_get_surface_capabilities(VkPhysicalDevice physDev, VkSurfaceKHR surface)
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDev, surface, &surfaceCapabilities);
	return surfaceCapabilities;
}


struct uvr_vk_surface_format uvr_vk_get_surface_formats(VkPhysicalDevice physDev, VkSurfaceKHR surface)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkSurfaceFormatKHR *surfaceFormats = NULL;
	uint32_t surfaceFormatCount = 0;

	res = vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &surfaceFormatCount, NULL);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkGetPhysicalDeviceSurfaceFormatsKHR: %s", vkres_msg(res));
		goto exit_vk_surface_formats;
	}

	surfaceFormats = (VkSurfaceFormatKHR *) calloc(surfaceFormatCount, sizeof(VkSurfaceFormatKHR));
	if (!surfaceFormats) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_surface_formats;
	}

	res = vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &surfaceFormatCount, surfaceFormats);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkGetPhysicalDeviceSurfaceFormatsKHR: %s", vkres_msg(res));
		goto exit_vk_surface_formats_free;
	}

	return (struct uvr_vk_surface_format) { .surfaceFormatCount = surfaceFormatCount, .surfaceFormats = surfaceFormats };

exit_vk_surface_formats_free:
	free(surfaceFormats);
exit_vk_surface_formats:
	return (struct uvr_vk_surface_format) { .surfaceFormatCount = 0, .surfaceFormats = NULL };
}


struct uvr_vk_surface_present_mode uvr_vk_get_surface_present_modes(VkPhysicalDevice physDev, VkSurfaceKHR surface)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkPresentModeKHR *presentModes = NULL;
	uint32_t presentModeCount = 0;

	res = vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &presentModeCount, NULL);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkGetPhysicalDeviceSurfacePresentModesKHR: %s", vkres_msg(res));
		goto exit_vk_surface_present_modes;
	}

	presentModes = (VkPresentModeKHR *) calloc(presentModeCount, sizeof(VkPresentModeKHR));
	if (!presentModes) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_surface_present_modes;
	}

	res = vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &presentModeCount, presentModes);
	if (res) {
		uvr_utils_log(UVR_DANGER, "[x] vkGetPhysicalDeviceSurfacePresentModesKHR: %s", vkres_msg(res));
		goto exit_vk_surface_present_modes_free;
	}

	return (struct uvr_vk_surface_present_mode) { .presentModeCount = presentModeCount, .presentModes = presentModes };

exit_vk_surface_present_modes_free:
	free(presentModes);
exit_vk_surface_present_modes:
	return (struct uvr_vk_surface_present_mode) { .presentModeCount = 0, .presentModes = NULL };
}


struct uvr_vk_phdev_format_prop uvr_vk_get_phdev_format_properties(VkPhysicalDevice physDev,
                                                                   VkFormat *formats,
                                                                   uint32_t formatCount)
{
	VkFormatProperties *formatProperties = NULL;

	formatProperties = (VkFormatProperties *) calloc(formatCount, sizeof(VkFormatProperties));
	if (!formatProperties) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_get_phdev_format_properties;
	}

	for (uint32_t f = 0; f < formatCount; f++) {
		vkGetPhysicalDeviceFormatProperties(physDev, formats[f], &formatProperties[f]);
	}

	return (struct uvr_vk_phdev_format_prop) { .formatProperties = formatProperties, .formatPropertyCount = formatCount };

exit_get_phdev_format_properties:
	free(formatProperties);
	return (struct uvr_vk_phdev_format_prop) { .formatProperties = NULL, .formatPropertyCount = 0 };
}


void uvr_vk_map_memory(struct uvr_vk_map_memory_info *uvrvk)
{
	void *data = NULL;
	vkMapMemory(uvrvk->logicalDevice, uvrvk->deviceMemory, uvrvk->deviceMemoryOffset, uvrvk->memoryBufferSize, 0, &data);
	memcpy(data, uvrvk->bufferData, uvrvk->memoryBufferSize);
	vkUnmapMemory(uvrvk->logicalDevice, uvrvk->deviceMemory);
}


void uvr_vk_destroy(struct uvr_vk_destroy *uvrvk)
{
	uint32_t i, j;

	for (i = 0; i < uvrvk->uvr_vk_lgdev_cnt; i++) {
		if (uvrvk->uvr_vk_lgdev[i].logicalDevice) {
			vkDeviceWaitIdle(uvrvk->uvr_vk_lgdev[i].logicalDevice);
		}
	}

	if (uvrvk->uvr_vk_sync_obj) {
		for (i = 0; i < uvrvk->uvr_vk_sync_obj_cnt; i++) {
			for (j = 0; j < uvrvk->uvr_vk_sync_obj[i].fenceCount; j++) {
				if (uvrvk->uvr_vk_sync_obj[i].fenceHandles[j].fence) {
					vkDestroyFence(uvrvk->uvr_vk_sync_obj[i].logicalDevice, uvrvk->uvr_vk_sync_obj[i].fenceHandles[j].fence, NULL);
				}
			}
			for (j = 0; j < uvrvk->uvr_vk_sync_obj[i].semaphoreCount; j++) {
				if (uvrvk->uvr_vk_sync_obj[i].semaphoreHandles[j].semaphore) {
					vkDestroySemaphore(uvrvk->uvr_vk_sync_obj[i].logicalDevice, uvrvk->uvr_vk_sync_obj[i].semaphoreHandles[j].semaphore, NULL);
				}
			}
			free(uvrvk->uvr_vk_sync_obj[i].fenceHandles);
			free(uvrvk->uvr_vk_sync_obj[i].semaphoreHandles);
		}
	}

	if (uvrvk->uvr_vk_command_buffer) {
		for (i = 0; i < uvrvk->uvr_vk_command_buffer_cnt; i++) {
			if (uvrvk->uvr_vk_command_buffer[i].logicalDevice && uvrvk->uvr_vk_command_buffer[i].commandPool)
				vkDestroyCommandPool(uvrvk->uvr_vk_command_buffer[i].logicalDevice, uvrvk->uvr_vk_command_buffer[i].commandPool, NULL);
			free(uvrvk->uvr_vk_command_buffer[i].commandBufferHandles);
		}
	}

	if (uvrvk->uvr_vk_buffer) {
		for (i = 0; i < uvrvk->uvr_vk_buffer_cnt; i++) {
			if (uvrvk->uvr_vk_buffer[i].buffer)
				vkDestroyBuffer(uvrvk->uvr_vk_buffer[i].logicalDevice, uvrvk->uvr_vk_buffer[i].buffer, NULL);
			if (uvrvk->uvr_vk_buffer[i].deviceMemory)
				vkFreeMemory(uvrvk->uvr_vk_buffer[i].logicalDevice, uvrvk->uvr_vk_buffer[i].deviceMemory, NULL);
		}
	}

	if (uvrvk->uvr_vk_descriptor_set) {
		for (i = 0; i < uvrvk->uvr_vk_descriptor_set_cnt; i++) {
			if (uvrvk->uvr_vk_descriptor_set[i].logicalDevice && uvrvk->uvr_vk_descriptor_set[i].descriptorPool)
				vkDestroyDescriptorPool(uvrvk->uvr_vk_descriptor_set[i].logicalDevice, uvrvk->uvr_vk_descriptor_set[i].descriptorPool, NULL);
			free(uvrvk->uvr_vk_descriptor_set[i].descriptorSetHandles);
		}
	}

	if (uvrvk->uvr_vk_descriptor_set_layout) {
		for (i = 0; i < uvrvk->uvr_vk_descriptor_set_layout_cnt; i++) {
			if (uvrvk->uvr_vk_descriptor_set_layout[i].logicalDevice && uvrvk->uvr_vk_descriptor_set_layout[i].descriptorSetLayout)
				vkDestroyDescriptorSetLayout(uvrvk->uvr_vk_descriptor_set_layout[i].logicalDevice, uvrvk->uvr_vk_descriptor_set_layout[i].descriptorSetLayout, NULL);
		}
	}

	if (uvrvk->uvr_vk_framebuffer) {
		for (i = 0; i < uvrvk->uvr_vk_framebuffer_cnt; i++) {
			for (j = 0; j < uvrvk->uvr_vk_framebuffer[i].framebufferCount; j++) {
				if (uvrvk->uvr_vk_framebuffer[i].logicalDevice && uvrvk->uvr_vk_framebuffer[i].framebufferHandles[j].framebuffer)
					vkDestroyFramebuffer(uvrvk->uvr_vk_framebuffer[i].logicalDevice, uvrvk->uvr_vk_framebuffer[i].framebufferHandles[j].framebuffer, NULL);
			}
			free(uvrvk->uvr_vk_framebuffer[i].framebufferHandles);
		}
	}

	if (uvrvk->uvr_vk_graphics_pipeline) {
		for (i = 0; i < uvrvk->uvr_vk_graphics_pipeline_cnt; i++) {
			if (uvrvk->uvr_vk_graphics_pipeline[i].logicalDevice && uvrvk->uvr_vk_graphics_pipeline[i].graphicsPipeline)
				vkDestroyPipeline(uvrvk->uvr_vk_graphics_pipeline[i].logicalDevice, uvrvk->uvr_vk_graphics_pipeline[i].graphicsPipeline, NULL);
		}
	}

	if (uvrvk->uvr_vk_pipeline_layout) {
		for (i = 0; i < uvrvk->uvr_vk_pipeline_layout_cnt; i++) {
			if (uvrvk->uvr_vk_pipeline_layout[i].logicalDevice && uvrvk->uvr_vk_pipeline_layout[i].pipelineLayout)
				vkDestroyPipelineLayout(uvrvk->uvr_vk_pipeline_layout[i].logicalDevice, uvrvk->uvr_vk_pipeline_layout[i].pipelineLayout, NULL);
		}
	}

	if (uvrvk->uvr_vk_render_pass) {
		for (i = 0; i < uvrvk->uvr_vk_render_pass_cnt; i++) {
			if (uvrvk->uvr_vk_render_pass[i].logicalDevice && uvrvk->uvr_vk_render_pass[i].renderPass)
				vkDestroyRenderPass(uvrvk->uvr_vk_render_pass[i].logicalDevice, uvrvk->uvr_vk_render_pass[i].renderPass, NULL);
		}
	}

	if (uvrvk->uvr_vk_shader_module) {
		for (i = 0; i < uvrvk->uvr_vk_shader_module_cnt; i++) {
			if (uvrvk->uvr_vk_shader_module[i].logicalDevice && uvrvk->uvr_vk_shader_module[i].shaderModule)
				vkDestroyShaderModule(uvrvk->uvr_vk_shader_module[i].logicalDevice, uvrvk->uvr_vk_shader_module[i].shaderModule, NULL);
		}
	}

	if (uvrvk->uvr_vk_sampler) {
		for (i = 0; i < uvrvk->uvr_vk_sampler_cnt; i++) {
			if (uvrvk->uvr_vk_sampler[i].logicalDevice && uvrvk->uvr_vk_sampler[i].sampler)
			vkDestroySampler(uvrvk->uvr_vk_sampler[i].logicalDevice, uvrvk->uvr_vk_sampler[i].sampler, NULL);
		}
	}

	if (uvrvk->uvr_vk_image) {
		for (i = 0; i < uvrvk->uvr_vk_image_cnt; i++) {
			for (j = 0; j < uvrvk->uvr_vk_image[i].imageCount; j++) {
				if (uvrvk->uvr_vk_image[i].logicalDevice && uvrvk->uvr_vk_image[i].imageHandles[j].image && !uvrvk->uvr_vk_image[i].swapchain)
					vkDestroyImage(uvrvk->uvr_vk_image[i].logicalDevice, uvrvk->uvr_vk_image[i].imageHandles[j].image, NULL);
				if (uvrvk->uvr_vk_image[i].logicalDevice && uvrvk->uvr_vk_image[i].imageHandles[j].deviceMemory && !uvrvk->uvr_vk_image[i].swapchain)
					vkFreeMemory(uvrvk->uvr_vk_image[i].logicalDevice, uvrvk->uvr_vk_image[i].imageHandles[j].deviceMemory, NULL);
				if (uvrvk->uvr_vk_image[i].logicalDevice && uvrvk->uvr_vk_image[i].imageViewHandles[j].view)
					vkDestroyImageView(uvrvk->uvr_vk_image[i].logicalDevice, uvrvk->uvr_vk_image[i].imageViewHandles[j].view, NULL);
			}
			free(uvrvk->uvr_vk_image[i].imageHandles);
			free(uvrvk->uvr_vk_image[i].imageViewHandles);
		}
	}

	if (uvrvk->uvr_vk_swapchain) {
		for (i = 0; i < uvrvk->uvr_vk_swapchain_cnt; i++) {
			if (uvrvk->uvr_vk_swapchain[i].logicalDevice && uvrvk->uvr_vk_swapchain[i].swapchain)
				vkDestroySwapchainKHR(uvrvk->uvr_vk_swapchain[i].logicalDevice, uvrvk->uvr_vk_swapchain[i].swapchain, NULL);
		}
	}

	for (i = 0; i < uvrvk->uvr_vk_lgdev_cnt; i++) {
		if (uvrvk->uvr_vk_lgdev[i].logicalDevice) {
			vkDestroyDevice(uvrvk->uvr_vk_lgdev[i].logicalDevice, NULL);
		}
	}

	if (uvrvk->surface)
		vkDestroySurfaceKHR(uvrvk->instance, uvrvk->surface, NULL);
	if (uvrvk->instance)
		vkDestroyInstance(uvrvk->instance, NULL);
}
