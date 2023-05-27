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

	kmr_utils_log(KMR_DANGER, "[x] retrieve_memory_type: failed to find suitable memory type");

	return UINT32_MAX;
}


VkInstance kmr_vk_instance_create(struct kmr_vk_instance_create_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkInstance instance = VK_NULL_HANDLE;

	/* initialize the VkApplicationInfo structure */
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = NULL;
	appInfo.pApplicationName = kmsvk->appName;
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.pEngineName = kmsvk->engineName;
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
	createInfo.pNext = (kmsvk->enabledLayerNames) ? &features : NULL;
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = kmsvk->enabledLayerCount;
	createInfo.ppEnabledLayerNames = kmsvk->enabledLayerNames;
	createInfo.enabledExtensionCount = kmsvk->enabledExtensionCount;
	createInfo.ppEnabledExtensionNames = kmsvk->enabledExtensionNames;

	/* Create the instance */
	res = vkCreateInstance(&createInfo, NULL, &instance);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkCreateInstance: %s", vkres_msg(res));
		return VK_NULL_HANDLE;
	}

	kmr_utils_log(KMR_SUCCESS, "kmr_vk_instance_create: VkInstance created retval(%p)", instance);

	return instance;
}


VkSurfaceKHR kmr_vk_surface_create(struct kmr_vk_surface_create_info *kmsvk)
{
	VkResult UNUSED res = VK_RESULT_MAX_ENUM;
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	if (!kmsvk->instance) {
		kmr_utils_log(KMR_DANGER, "[x] kmr_vk_surface_create: VkInstance not instantiated");
		kmr_utils_log(KMR_DANGER, "[x] Make a call to kmr_vk_create_instance");
		return VK_NULL_HANDLE;
	}

	if (kmsvk->surfaceType != KMR_SURFACE_WAYLAND_CLIENT && kmsvk->surfaceType != KMR_SURFACE_XCB_CLIENT) {
		kmr_utils_log(KMR_DANGER, "[x] kmr_vk_surface_create: Must specify correct enum kmsvk_surface_type");
		return VK_NULL_HANDLE;
	}

#ifdef INCLUDE_WAYLAND
	if (kmsvk->surfaceType == KMR_SURFACE_WAYLAND_CLIENT) {
		VkWaylandSurfaceCreateInfoKHR waylandSurfaceCreateInfo = {};
		waylandSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
		waylandSurfaceCreateInfo.pNext = NULL;
		waylandSurfaceCreateInfo.flags = 0;
		waylandSurfaceCreateInfo.display = kmsvk->display;
		waylandSurfaceCreateInfo.surface = kmsvk->surface;

		res = vkCreateWaylandSurfaceKHR(kmsvk->instance, &waylandSurfaceCreateInfo, NULL, &surface);
		if (res) {
			kmr_utils_log(KMR_DANGER, "[x] vkCreateWaylandSurfaceKHR: %s", vkres_msg(res));
			return VK_NULL_HANDLE;
		}
	}
#endif

#ifdef INCLUDE_XCB
	if (kmsvk->surfaceType == KMR_SURFACE_XCB_CLIENT) {
		VkXcbSurfaceCreateInfoKHR xcbSurfaceCreateInfo = {};
		xcbSurfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
		xcbSurfaceCreateInfo.pNext = NULL;
		xcbSurfaceCreateInfo.flags = 0;
		xcbSurfaceCreateInfo.connection = kmsvk->display;
		xcbSurfaceCreateInfo.window = kmsvk->window;

		res = vkCreateXcbSurfaceKHR(kmsvk->instance, &xcbSurfaceCreateInfo, NULL, &surface);
		if (res) {
			kmr_utils_log(KMR_DANGER, "[x] vkCreateXcbSurfaceKHR: %s", vkres_msg(res));
			return VK_NULL_HANDLE;
		}
	}
#endif

#if defined(INCLUDE_WAYLAND) || defined(INCLUDE_XCB)
	kmr_utils_log(KMR_SUCCESS, "kmr_vk_surface_create: VkSurfaceKHR created retval(%p)", surface);
#endif

	return surface;
}


struct kmr_vk_phdev kmr_vk_phdev_create(struct kmr_vk_phdev_create_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkPhysicalDevice physDevice = VK_NULL_HANDLE;
	uint32_t deviceCount = 0;
	VkPhysicalDevice *devices = NULL;

	if (!kmsvk->instance) {
		kmr_utils_log(KMR_DANGER, "[x] kmr_vk_create_phdev: VkInstance not instantiated");
		kmr_utils_log(KMR_DANGER, "[x] Make a call to kmr_vk_create_instance");
		goto exit_error_vk_phdev_create;
	}

	res = vkEnumeratePhysicalDevices(kmsvk->instance, &deviceCount, NULL);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkEnumeratePhysicalDevices: %s", vkres_msg(res));
		goto exit_error_vk_phdev_create;
	}

	if (deviceCount == 0) {
		kmr_utils_log(KMR_DANGER, "[x] failed to find GPU with Vulkan support!!!");
		goto exit_error_vk_phdev_create;
	}

	devices = (VkPhysicalDevice *) alloca((deviceCount * sizeof(VkPhysicalDevice)) + 1);

	res = vkEnumeratePhysicalDevices(kmsvk->instance, &deviceCount, devices);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkEnumeratePhysicalDevices: %s", vkres_msg(res));
		goto exit_error_vk_phdev_create;
	}

#ifdef INCLUDE_KMS
	/* Get KMS fd stats */
	struct stat drmStat = {0};
	if (kmsvk->kmsfd != -1) {
		if (fstat(kmsvk->kmsfd, &drmStat) == -1) {
			kmr_utils_log(KMR_DANGER, "[x] fstat('%d'): %s", kmsvk->kmsfd, strerror(errno));
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
		if (kmsvk->kmsfd) {
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
			enter |= ((combinedPrimaryDeviceId == drmStat.st_rdev || combinedRenderDeviceId == drmStat.st_rdev) && physDeviceProperties.deviceType == kmsvk->deviceType);
		}
#endif

		/* Enter will be one if condition succeeds */
		enter |= (physDeviceProperties.deviceType == kmsvk->deviceType);
		if (enter) {
			memmove(&physDevice, &devices[i], sizeof(devices[i]));
			kmr_utils_log(KMR_SUCCESS, "Suitable GPU Found: %s, api version: %u", physDeviceProperties.deviceName, physDeviceProperties.apiVersion);
			break;
		}
	}

	if (physDevice == VK_NULL_HANDLE) {
		kmr_utils_log(KMR_DANGER, "[x] GPU that meets requirement not found!");
		goto exit_error_vk_phdev_create;
	}

	return (struct kmr_vk_phdev) { .instance = kmsvk->instance, .physDevice = physDevice, .physDeviceProperties = physDeviceProperties, .physDeviceFeatures = physDeviceFeatures,
#ifdef INCLUDE_KMS
	                               .kmsfd = kmsvk->kmsfd, .physDeviceDrmProperties = physDeviceDrmProperties
#endif
	};

exit_error_vk_phdev_create:
	memset(&physDeviceFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
	memset(&physDeviceProperties, 0, sizeof(VkPhysicalDeviceProperties));

#ifdef INCLUDE_KMS
	memset(&physDeviceDrmProperties, 0, sizeof(VkPhysicalDeviceDrmPropertiesEXT));
#endif

	return (struct kmr_vk_phdev) { .instance = VK_NULL_HANDLE, .physDevice = VK_NULL_HANDLE, .physDeviceProperties = physDeviceProperties, .physDeviceFeatures = physDeviceFeatures,
#ifdef INCLUDE_KMS
	                               .kmsfd = -1, .physDeviceDrmProperties = physDeviceDrmProperties
#endif
	};
}


struct kmr_vk_queue kmr_vk_queue_create(struct kmr_vk_queue_create_info *kmsvk)
{
	uint8_t flagCount = 0;
	uint32_t queueCount = 0;
	VkQueueFamilyProperties *queueFamilies = NULL;
	struct kmr_vk_queue queue;

	flagCount += (kmsvk->queueFlag & VK_QUEUE_GRAPHICS_BIT) ? 1 : 0;
	flagCount += (kmsvk->queueFlag & VK_QUEUE_COMPUTE_BIT) ? 1 : 0;
	flagCount += (kmsvk->queueFlag & VK_QUEUE_TRANSFER_BIT) ? 1 : 0;
	flagCount += (kmsvk->queueFlag & VK_QUEUE_SPARSE_BINDING_BIT) ? 1 : 0;
	flagCount += (kmsvk->queueFlag & VK_QUEUE_PROTECTED_BIT) ? 1 : 0;

	if (flagCount != 1) {
		kmr_utils_log(KMR_DANGER, "[x] kmr_vk_queue_create: Multiple VkQueueFlags specified, only one allowed per queue");
		goto err_vk_queue_create;
	}

	/*
	 * Almost every operation in Vulkan, requires commands to be submitted to a queue
	 * Find queue family index for a given queue
	 */
	vkGetPhysicalDeviceQueueFamilyProperties(kmsvk->physDevice, &queueCount, NULL);
	queueFamilies = (VkQueueFamilyProperties *) alloca(queueCount * sizeof(VkQueueFamilyProperties));
	vkGetPhysicalDeviceQueueFamilyProperties(kmsvk->physDevice, &queueCount, queueFamilies);

	for (uint32_t i = 0; i < queueCount; i++) {
		queue.queueCount = queueFamilies[i].queueCount;
		if (queueFamilies[i].queueFlags & kmsvk->queueFlag & VK_QUEUE_GRAPHICS_BIT) {
			strncpy(queue.name, "graphics", sizeof(queue.name));
			queue.familyIndex = i; break;
		}

		if (queueFamilies[i].queueFlags & kmsvk->queueFlag & VK_QUEUE_COMPUTE_BIT) {
			strncpy(queue.name, "compute", sizeof(queue.name));
			queue.familyIndex = i; break;
		}

		if (queueFamilies[i].queueFlags & kmsvk->queueFlag & VK_QUEUE_TRANSFER_BIT) {
			strncpy(queue.name, "transfer", sizeof(queue.name));
			queue.familyIndex = i; break;
		}

		if (queueFamilies[i].queueFlags & kmsvk->queueFlag & VK_QUEUE_SPARSE_BINDING_BIT) {
			strncpy(queue.name, "sparse_binding", sizeof(queue.name));
			queue.familyIndex = i; break;
		}

		if (queueFamilies[i].queueFlags & kmsvk->queueFlag & VK_QUEUE_PROTECTED_BIT) {
			strncpy(queue.name, "protected", sizeof(queue.name));
			queue.familyIndex = i; break;
		}
	}

	return queue;

err_vk_queue_create:
	return (struct kmr_vk_queue) { .name[0] = '\0', .queue = VK_NULL_HANDLE, .familyIndex = -1, .queueCount = -1 };
}


struct kmr_vk_lgdev kmr_vk_lgdev_create(struct kmr_vk_lgdev_create_info *kmsvk)
{
	uint32_t qc;
	void *pNext = NULL;
	VkDevice logicalDevice = VK_NULL_HANDLE;
	VkResult res = VK_RESULT_MAX_ENUM;

	VkDeviceQueueCreateInfo *queueCreateInfo = (VkDeviceQueueCreateInfo *) calloc(kmsvk->queueCount, sizeof(VkDeviceQueueCreateInfo));
	if (!queueCreateInfo) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto err_vk_lgdev_create;
	}

	VkPhysicalDeviceSynchronization2FeaturesKHR externalSemaphoreInfo;
	externalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
	externalSemaphoreInfo.pNext = NULL;
	externalSemaphoreInfo.synchronization2 = VK_TRUE;

	VkPhysicalDeviceTimelineSemaphoreFeaturesKHR timelineSemaphoreFeatures;
	timelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR;
	timelineSemaphoreFeatures.pNext = NULL;
	timelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

	for (qc = 0; qc < kmsvk->enabledExtensionCount; qc++) {
		if (!strncmp(kmsvk->enabledExtensionNames[qc], VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, 30)) {
			timelineSemaphoreFeatures.pNext = &externalSemaphoreInfo;
		}
	
		if (!strncmp(kmsvk->enabledExtensionNames[qc], VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME, 30)) {
			pNext = &timelineSemaphoreFeatures;
		}
	}

	/*
	 * https://www.khronos.org/registry/vulkan/specs/1.3-extensions/html/vkspec.html#devsandqueues-priority
	 * set the default priority of all queues to be the highest
	 */
	const float queuePriorities = 1.f;
	for (qc = 0; qc < kmsvk->queueCount; qc++) {
		queueCreateInfo[qc].flags = 0;
		queueCreateInfo[qc].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo[qc].queueFamilyIndex = kmsvk->queues[qc].familyIndex;
		queueCreateInfo[qc].queueCount = kmsvk->queues[qc].queueCount;
		queueCreateInfo[qc].pQueuePriorities = &queuePriorities;
	}

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = pNext;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.queueCreateInfoCount = kmsvk->queueCount;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfo;
	deviceCreateInfo.enabledLayerCount = 0; // Deprecated and ignored
	deviceCreateInfo.ppEnabledLayerNames = NULL; // Deprecated and ignored
	deviceCreateInfo.enabledExtensionCount = kmsvk->enabledExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = kmsvk->enabledExtensionNames;
	deviceCreateInfo.pEnabledFeatures = kmsvk->enabledFeatures;

	/* Create logic device */
	res = vkCreateDevice(kmsvk->physDevice, &deviceCreateInfo, NULL, &logicalDevice);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkCreateDevice: %s", vkres_msg(res));
		goto err_vk_lgdev_free_queueCreateInfo;
	}

	for (qc = 0; qc < kmsvk->queueCount; qc++) {
		vkGetDeviceQueue(logicalDevice, kmsvk->queues[qc].familyIndex, 0, &kmsvk->queues[qc].queue);
		if (!kmsvk->queues[qc].queue)  {
			kmr_utils_log(KMR_DANGER, "[x] vkGetDeviceQueue: Failed to get %s queue handle", kmsvk->queues[qc].name);
			goto err_vk_lgdev_destroy;
		}

		kmr_utils_log(KMR_SUCCESS, "kmr_vk_lgdev_create: '%s' VkQueue successfully created retval(%p)", kmsvk->queues[qc].name, kmsvk->queues[qc].queue);
	}

	kmr_utils_log(KMR_SUCCESS, "kmr_vk_lgdev_create: VkDevice created retval(%p)", logicalDevice);

	free(queueCreateInfo);
	return (struct kmr_vk_lgdev) { .logicalDevice = logicalDevice, .queueCount = kmsvk->queueCount, .queues = kmsvk->queues };

err_vk_lgdev_destroy:
	if (logicalDevice)
		vkDestroyDevice(logicalDevice, NULL);
err_vk_lgdev_free_queueCreateInfo:
	free(queueCreateInfo);
err_vk_lgdev_create:
	return (struct kmr_vk_lgdev) { .logicalDevice = VK_NULL_HANDLE, .queueCount = -1, .queues = NULL };
}


struct kmr_vk_swapchain kmr_vk_swapchain_create(struct kmr_vk_swapchain_create_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;

	if (kmsvk->surfaceCapabilities.currentExtent.width != UINT32_MAX) {
		kmsvk->extent2D = kmsvk->surfaceCapabilities.currentExtent;
	} else {
		kmsvk->extent2D.width = fmax(kmsvk->surfaceCapabilities.minImageExtent.width, fmin(kmsvk->surfaceCapabilities.maxImageExtent.width, kmsvk->extent2D.width));
		kmsvk->extent2D.height = fmax(kmsvk->surfaceCapabilities.minImageExtent.height, fmin(kmsvk->surfaceCapabilities.maxImageExtent.height, kmsvk->extent2D.height));
	}

	VkSwapchainCreateInfoKHR createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.surface = kmsvk->surface;
	if (kmsvk->surfaceCapabilities.maxImageCount > 0 && (kmsvk->surfaceCapabilities.minImageCount + 1) < kmsvk->surfaceCapabilities.maxImageCount)
		createInfo.minImageCount = kmsvk->surfaceCapabilities.maxImageCount;
	else
		createInfo.minImageCount = kmsvk->surfaceCapabilities.minImageCount + 1;
	createInfo.imageFormat = kmsvk->surfaceFormat.format;
	createInfo.imageColorSpace = kmsvk->surfaceFormat.colorSpace;
	createInfo.imageExtent = kmsvk->extent2D;
	createInfo.imageArrayLayers = kmsvk->imageArrayLayers;
	createInfo.imageUsage = kmsvk->imageUsage;
	createInfo.imageSharingMode = kmsvk->imageSharingMode;
	createInfo.queueFamilyIndexCount = kmsvk->queueFamilyIndexCount;
	createInfo.pQueueFamilyIndices = kmsvk->queueFamilyIndices;
	createInfo.preTransform = kmsvk->surfaceCapabilities.currentTransform;
	createInfo.compositeAlpha = kmsvk->compositeAlpha;
	createInfo.presentMode = kmsvk->presentMode;
	createInfo.clipped = kmsvk->clipped;
	createInfo.oldSwapchain = kmsvk->oldSwapChain;

	res = vkCreateSwapchainKHR(kmsvk->logicalDevice, &createInfo, NULL, &swapchain);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkCreateSwapchainKHR: %s", vkres_msg(res));
		goto exit_vk_swapchain;
	}

	kmr_utils_log(KMR_SUCCESS, "kmr_vk_swapchain_create: VkSwapchainKHR successfully created retval(%p)", swapchain);

	return (struct kmr_vk_swapchain) { .logicalDevice = kmsvk->logicalDevice, .swapchain = swapchain };

exit_vk_swapchain:
	return (struct kmr_vk_swapchain) { .logicalDevice = VK_NULL_HANDLE, .swapchain = VK_NULL_HANDLE };
}


#define MAX_DMABUF_PLANES 4


static VkImageAspectFlagBits choose_memory_plane_aspect(uint8_t i) {
	switch (i) {
		case 0: return VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT;
		case 1: return VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT;
		case 2: return VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT;
		case 3: return VK_IMAGE_ASPECT_MEMORY_PLANE_3_BIT_EXT;
		default: return VK_IMAGE_ASPECT_NONE;
	}
}


struct kmr_vk_image kmr_vk_image_create(struct kmr_vk_image_create_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	uint32_t imageCount = 0, curImage = 0, i, p;
	VkImage *vkImages = NULL;

	struct _device_memory {
		VkDeviceMemory memory[MAX_DMABUF_PLANES];
		uint8_t        memoryCount;
	} *deviceMemories = NULL;

	struct kmr_vk_image_handle *imageHandles = NULL;
	struct kmr_vk_image_view_handle *imageViewHandles = NULL;

	if (kmsvk->swapchain) {
		res = vkGetSwapchainImagesKHR(kmsvk->logicalDevice, kmsvk->swapchain, &imageCount, NULL);
		if (res) {
			kmr_utils_log(KMR_DANGER, "[x] vkGetSwapchainImagesKHR: %s", vkres_msg(res));
			goto exit_vk_image;
		}

		vkImages = alloca(imageCount * sizeof(VkImage));

		res = vkGetSwapchainImagesKHR(kmsvk->logicalDevice, kmsvk->swapchain, &imageCount, vkImages);
		if (res) {
			kmr_utils_log(KMR_DANGER, "[x] vkGetSwapchainImagesKHR: %s", vkres_msg(res));
			goto exit_vk_image;
		}

		kmr_utils_log(KMR_INFO, "kmr_vk_image_create: Total images in swapchain %u", imageCount);
	} else {
		imageCount = kmsvk->imageCount;
		vkImages = alloca(imageCount * sizeof(VkImage));
		deviceMemories = alloca(imageCount * sizeof(struct _device_memory));

		memset(vkImages, 0, imageCount * sizeof(VkImage));
		memset(deviceMemories, 0, imageCount * sizeof(VkDeviceMemory));

		uint32_t memPropertyFlags = kmsvk->memPropertyFlags, memoryTypeBits = 0;
		int dupFd = -1;

		VkImageDrmFormatModifierExplicitCreateInfoEXT imageModifierInfo;
		imageModifierInfo.sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT;
		imageModifierInfo.pNext = NULL;

		// Specify that an image may be backed by external memory
		VkExternalMemoryImageCreateInfo externalMemImageInfo;
		externalMemImageInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO;
		externalMemImageInfo.pNext = &imageModifierInfo;
		externalMemImageInfo.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

		VkImageCreateInfo imageCreateInfo;
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.pNext = (kmsvk->useExternalDmaBuffer) ? &externalMemImageInfo : NULL;

		VkMemoryRequirements2 memoryRequirements2;
		memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
		memoryRequirements2.pNext = NULL;

		VkImagePlaneMemoryRequirementsInfo planeMemoryRequirementsInfo;
		planeMemoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO;
		planeMemoryRequirementsInfo.pNext = NULL;

		VkImageMemoryRequirementsInfo2 imageMemoryRequirementsInfo;
		imageMemoryRequirementsInfo.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
		imageMemoryRequirementsInfo.pNext = (kmsvk->useExternalDmaBuffer) ? &planeMemoryRequirementsInfo : NULL;

		VkMemoryDedicatedAllocateInfo dedicatedAllocInfo;
		dedicatedAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
		dedicatedAllocInfo.pNext = NULL;
		dedicatedAllocInfo.image = VK_NULL_HANDLE;
		dedicatedAllocInfo.buffer = VK_NULL_HANDLE;

		VkImportMemoryFdInfoKHR importMemoryFdInfo;
		importMemoryFdInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
		importMemoryFdInfo.pNext = &dedicatedAllocInfo;
		importMemoryFdInfo.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;

		VkMemoryAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.pNext = (kmsvk->useExternalDmaBuffer) ? &importMemoryFdInfo : NULL;

		VkBindImageMemoryInfo bindImageMemoryInfos[MAX_DMABUF_PLANES] = {0};
		VkBindImagePlaneMemoryInfo bindPlaneMemoryInfos[MAX_DMABUF_PLANES] = {0};

		for (i = 0; i < imageCount; i++) {
			curImage = (kmsvk->imageCount) ? i : 0;

			if (kmsvk->useExternalDmaBuffer) {
				imageModifierInfo.drmFormatModifier = kmsvk->imageCreateInfos[curImage].imageDmaBufferFormatModifier;
				imageModifierInfo.drmFormatModifierPlaneCount = kmsvk->imageCreateInfos[curImage].imageDmaBufferCount;
				imageModifierInfo.pPlaneLayouts = kmsvk->imageCreateInfos[curImage].imageDmaBufferResourceInfo;
			}

			imageCreateInfo.flags = kmsvk->imageCreateInfos[curImage].imageflags;
			imageCreateInfo.imageType = kmsvk->imageCreateInfos[curImage].imageType;
			imageCreateInfo.format = kmsvk->imageCreateInfos[curImage].imageFormat;
			imageCreateInfo.extent = kmsvk->imageCreateInfos[curImage].imageExtent3D;
			imageCreateInfo.mipLevels = kmsvk->imageCreateInfos[curImage].imageMipLevels;
			imageCreateInfo.arrayLayers = kmsvk->imageCreateInfos[curImage].imageArrayLayers;
			imageCreateInfo.samples = kmsvk->imageCreateInfos[curImage].imageSamples;
			imageCreateInfo.tiling = kmsvk->imageCreateInfos[curImage].imageTiling;
			imageCreateInfo.usage = kmsvk->imageCreateInfos[curImage].imageUsage;
			imageCreateInfo.sharingMode = kmsvk->imageCreateInfos[curImage].imageSharingMode;
			imageCreateInfo.queueFamilyIndexCount = kmsvk->imageCreateInfos[curImage].imageQueueFamilyIndexCount;
			imageCreateInfo.pQueueFamilyIndices = kmsvk->imageCreateInfos[curImage].imageQueueFamilyIndices;
			imageCreateInfo.initialLayout = kmsvk->imageCreateInfos[curImage].imageInitialLayout;

			res = vkCreateImage(kmsvk->logicalDevice, &imageCreateInfo, NULL, &vkImages[i]);
			if (res) {
				kmr_utils_log(KMR_DANGER, "[x] vkCreateImage: %s", vkres_msg(res));
				goto exit_vk_image_free_images;
			}

			dedicatedAllocInfo.image = vkImages[i];
			imageMemoryRequirementsInfo.image = vkImages[i];
			vkGetImageMemoryRequirements2(kmsvk->logicalDevice, &imageMemoryRequirementsInfo, &memoryRequirements2);
			allocInfo.allocationSize = memoryRequirements2.memoryRequirements.size;

			if (kmsvk->useExternalDmaBuffer) {
				deviceMemories[i].memoryCount = imageModifierInfo.drmFormatModifierPlaneCount;
				for (p = 0; p < imageModifierInfo.drmFormatModifierPlaneCount; p++) {
					/*
					 * Taken From: https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/render/vulkan/texture.c
					 * Since importing transfers ownership of the FD to Vulkan, we have
					 * to duplicate it since this operation does not transfer ownership
					 * of the attribs to this texture. Will be closed by Vulkan on
					 * vkFreeMemory.
					 */
					dupFd = fcntl(kmsvk->imageCreateInfos[curImage].imageDmaBufferFds[p], F_DUPFD_CLOEXEC, 0);
					if (dupFd == -1) {
						kmr_utils_log(KMR_DANGER, "[x] fcntl(F_DUPFD_CLOEXEC): %s", strerror(errno));
						goto exit_vk_image_free_images;
					}

					importMemoryFdInfo.fd = dupFd;
					planeMemoryRequirementsInfo.planeAspect = choose_memory_plane_aspect(p);
					memoryTypeBits = memoryRequirements2.memoryRequirements.memoryTypeBits | kmsvk->imageCreateInfos[curImage].imageDmaBufferMemTypeBits[p];
					allocInfo.memoryTypeIndex = retrieve_memory_type_index(kmsvk->physDevice, memoryTypeBits, memPropertyFlags);

					res = vkAllocateMemory(kmsvk->logicalDevice, &allocInfo, NULL, &deviceMemories[i].memory[p]);
					if (res) {
						kmr_utils_log(KMR_DANGER, "[x] vkAllocateMemory: %s", vkres_msg(res));
						goto exit_vk_image_free_images;
					}

					bindPlaneMemoryInfos[p].sType = VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO;
					bindPlaneMemoryInfos[p].pNext = NULL;
					bindPlaneMemoryInfos[p].planeAspect = planeMemoryRequirementsInfo.planeAspect;

					bindImageMemoryInfos[p].sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
					bindImageMemoryInfos[p].pNext = &bindPlaneMemoryInfos[p];
					bindImageMemoryInfos[p].image = vkImages[i];
					bindImageMemoryInfos[p].memory = deviceMemories[i].memory[p];
					bindImageMemoryInfos[p].memoryOffset = 0;
				}
			} else {
				deviceMemories[i].memoryCount = 1;
				memoryTypeBits = memoryRequirements2.memoryRequirements.memoryTypeBits;
				allocInfo.memoryTypeIndex = retrieve_memory_type_index(kmsvk->physDevice, memoryTypeBits, memPropertyFlags);

				res = vkAllocateMemory(kmsvk->logicalDevice, &allocInfo, NULL, &deviceMemories[i].memory[0]);
				if (res) {
					kmr_utils_log(KMR_DANGER, "[x] vkAllocateMemory: %s", vkres_msg(res));
					goto exit_vk_image_free_images;
				}

				bindImageMemoryInfos[0].sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO;
				bindImageMemoryInfos[0].pNext = NULL;
				bindImageMemoryInfos[0].image = vkImages[i];
				bindImageMemoryInfos[0].memory = deviceMemories[i].memory[0];
				bindImageMemoryInfos[0].memoryOffset = 0;
			}

			vkBindImageMemory2(kmsvk->logicalDevice, deviceMemories[i].memoryCount, bindImageMemoryInfos);

			memset(bindImageMemoryInfos, 0, sizeof(bindImageMemoryInfos));
			memset(bindPlaneMemoryInfos, 0, sizeof(bindPlaneMemoryInfos));
		}
	}

	imageHandles = calloc(imageCount, sizeof(struct kmr_vk_image_handle));
	if (!imageHandles) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_image_free_images;
	}

	imageViewHandles = calloc(imageCount, sizeof(struct kmr_vk_image_view_handle));
	if (!imageViewHandles) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_image_free_images;
	}

	VkImageViewCreateInfo imageViewCreateInfo;
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = NULL;

	for (i = 0; i < imageCount; i++) {
		curImage = (kmsvk->imageCount) ? i : 0;

		imageViewCreateInfo.flags = kmsvk->imageViewCreateInfos[curImage].imageViewflags;
		imageViewCreateInfo.viewType = kmsvk->imageViewCreateInfos[curImage].imageViewType;
		imageViewCreateInfo.format = kmsvk->imageViewCreateInfos[curImage].imageViewFormat;
		imageViewCreateInfo.components = kmsvk->imageViewCreateInfos[curImage].imageViewComponents;
		imageViewCreateInfo.subresourceRange = kmsvk->imageViewCreateInfos[curImage].imageViewSubresourceRange;
		imageViewCreateInfo.image = imageHandles[i].image = vkImages[i];

		if (deviceMemories) {
			if (deviceMemories[i].memory[0]) {
				imageHandles[i].deviceMemoryCount = deviceMemories[i].memoryCount;
				for (p = 0; p < imageHandles[i].deviceMemoryCount; p++)
					imageHandles[i].deviceMemory[p] = deviceMemories[i].memory[p];
			}
		}

		res = vkCreateImageView(kmsvk->logicalDevice, &imageViewCreateInfo, NULL, &imageViewHandles[i].view);
		if (res) {
			kmr_utils_log(KMR_DANGER, "[x] vkCreateImageView: %s", vkres_msg(res));
			goto exit_vk_image_free_image_views;
		}

		kmr_utils_log(KMR_WARNING, "kmr_vk_image_create: VkImage (%p) associate with VkImageView", imageHandles[i].image);
		kmr_utils_log(KMR_SUCCESS, "kmr_vk_image_create: VkImageView successfully created retval(%p)", imageViewHandles[i].view);
	}

	return (struct kmr_vk_image) { .logicalDevice = kmsvk->logicalDevice, .imageCount = imageCount, .imageHandles = imageHandles,
	                               .imageViewHandles = imageViewHandles, .swapchain = kmsvk->swapchain };

exit_vk_image_free_image_views:
	if (imageViewHandles) {
		for (i = 0; i < imageCount; i++) {
			if (imageViewHandles[i].view)
				vkDestroyImageView(kmsvk->logicalDevice, imageViewHandles[i].view, NULL);
		}
	}
	free(imageViewHandles);
exit_vk_image_free_images:
	if (vkImages && deviceMemories && !kmsvk->swapchain) {
		for (i = 0; i < imageCount; i++) {
			if (vkImages[i])
				vkDestroyImage(kmsvk->logicalDevice, vkImages[i], NULL);
			for (p = 0; p < deviceMemories[i].memoryCount; p++)
				vkFreeMemory(kmsvk->logicalDevice, deviceMemories[i].memory[p], NULL);
		}
	}
	free(imageHandles);
exit_vk_image:
	return (struct kmr_vk_image) { .logicalDevice = VK_NULL_HANDLE, .imageCount = 0, .imageHandles = NULL,
	                               .imageViewHandles = NULL, .swapchain = VK_NULL_HANDLE };
}


struct kmr_vk_shader_module kmr_vk_shader_module_create(struct kmr_vk_shader_module_create_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkShaderModule shaderModule = VK_NULL_HANDLE;

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.codeSize = kmsvk->sprivByteSize;
	createInfo.pCode = (const uint32_t *) kmsvk->sprivBytes;

	res = vkCreateShaderModule(kmsvk->logicalDevice, &createInfo, NULL, &shaderModule);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkCreateShaderModule: %s", vkres_msg(res));
		goto exit_vk_shader_module;
	}

	kmr_utils_log(KMR_SUCCESS, "kmr_vk_shader_module_create: '%s' shader VkShaderModule successfully created retval(%p)", kmsvk->shaderName, shaderModule);

	return (struct kmr_vk_shader_module) { .logicalDevice = kmsvk->logicalDevice, .shaderModule = shaderModule, .shaderName = kmsvk->shaderName };

exit_vk_shader_module:
	return (struct kmr_vk_shader_module) { .logicalDevice = VK_NULL_HANDLE, .shaderModule = VK_NULL_HANDLE, .shaderName = NULL };
}


struct kmr_vk_pipeline_layout kmr_vk_pipeline_layout_create(struct kmr_vk_pipeline_layout_create_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

	VkPipelineLayoutCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.setLayoutCount = kmsvk->descriptorSetLayoutCount;
	createInfo.pSetLayouts = kmsvk->descriptorSetLayouts;
	createInfo.pushConstantRangeCount = kmsvk->pushConstantRangeCount;
	createInfo.pPushConstantRanges = kmsvk->pushConstantRanges;

	res = vkCreatePipelineLayout(kmsvk->logicalDevice, &createInfo, NULL, &pipelineLayout);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkCreatePipelineLayout: %s", vkres_msg(res));
		goto exit_vk_pipeline_layout;
	}

	kmr_utils_log(KMR_SUCCESS, "kmr_vk_pipeline_layout_create: VkPipelineLayout successfully created retval(%p)", pipelineLayout);

	return (struct kmr_vk_pipeline_layout) { .logicalDevice = kmsvk->logicalDevice, .pipelineLayout = pipelineLayout };

exit_vk_pipeline_layout:
	return (struct kmr_vk_pipeline_layout) { .logicalDevice = VK_NULL_HANDLE, .pipelineLayout = VK_NULL_HANDLE };
}


struct kmr_vk_render_pass kmr_vk_render_pass_create(struct kmr_vk_render_pass_create_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkRenderPass renderPass = VK_NULL_HANDLE;

	VkRenderPassCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.attachmentCount = kmsvk->attachmentDescriptionCount;
	createInfo.pAttachments = kmsvk->attachmentDescriptions;
	createInfo.subpassCount = kmsvk->subpassDescriptionCount;
	createInfo.pSubpasses = kmsvk->subpassDescriptions;
	createInfo.dependencyCount = kmsvk->subpassDependencyCount;
	createInfo.pDependencies = kmsvk->subpassDependencies;

	res = vkCreateRenderPass(kmsvk->logicalDevice, &createInfo, NULL, &renderPass);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkCreateRenderPass: %s", vkres_msg(res));
		goto exit_vk_render_pass;
	}

	kmr_utils_log(KMR_SUCCESS, "kmr_vk_render_pass_create: VkRenderPass successfully created retval(%p)", renderPass);

	return (struct kmr_vk_render_pass) { .logicalDevice = kmsvk->logicalDevice, .renderPass = renderPass };

exit_vk_render_pass:
	return (struct kmr_vk_render_pass) { .logicalDevice = VK_NULL_HANDLE, .renderPass = VK_NULL_HANDLE };
}


struct kmr_vk_graphics_pipeline kmr_vk_graphics_pipeline_create(struct kmr_vk_graphics_pipeline_create_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkPipeline graphicsPipeline = VK_NULL_HANDLE;

	VkGraphicsPipelineCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.stageCount = kmsvk->shaderStageCount;
	createInfo.pStages = kmsvk->shaderStages;
	createInfo.pVertexInputState = kmsvk->vertexInputState;
	createInfo.pInputAssemblyState = kmsvk->inputAssemblyState;
	createInfo.pTessellationState = kmsvk->tessellationState;
	createInfo.pViewportState = kmsvk->viewportState;
	createInfo.pRasterizationState = kmsvk->rasterizationState;
	createInfo.pMultisampleState = kmsvk->multisampleState;
	createInfo.pDepthStencilState = kmsvk->depthStencilState;
	createInfo.pColorBlendState = kmsvk->colorBlendState;
	createInfo.pDynamicState = kmsvk->dynamicState;
	createInfo.layout = kmsvk->pipelineLayout;
	createInfo.renderPass = kmsvk->renderPass;
	createInfo.subpass = kmsvk->subpass;
	// Won't be supporting
	createInfo.basePipelineHandle = VK_NULL_HANDLE;
	createInfo.basePipelineIndex = -1;

	res = vkCreateGraphicsPipelines(kmsvk->logicalDevice, NULL, 1, &createInfo, NULL, &graphicsPipeline);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkCreateGraphicsPipelines: %s", vkres_msg(res));
		goto exit_vk_graphics_pipeline;
	}

	kmr_utils_log(KMR_SUCCESS, "kmr_vk_graphics_pipeline_create: VkPipeline successfully created retval(%p)", graphicsPipeline);

	return (struct kmr_vk_graphics_pipeline) { .logicalDevice = kmsvk->logicalDevice, .graphicsPipeline = graphicsPipeline };

exit_vk_graphics_pipeline:
	return (struct kmr_vk_graphics_pipeline) { .logicalDevice = VK_NULL_HANDLE, .graphicsPipeline = VK_NULL_HANDLE };
}


struct kmr_vk_framebuffer kmr_vk_framebuffer_create(struct kmr_vk_framebuffer_create_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	uint8_t currentFrameBuffer = 0;

	struct kmr_vk_framebuffer_handle *framebufferHandles = NULL;

	framebufferHandles = (struct kmr_vk_framebuffer_handle *) calloc(kmsvk->framebufferCount, sizeof(struct kmr_vk_framebuffer_handle));
	if (!framebufferHandles) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_framebuffer;
	}

	VkFramebufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = 0;
	createInfo.renderPass = kmsvk->renderPass;
	createInfo.width = kmsvk->width;
	createInfo.height = kmsvk->height;
	createInfo.layers = kmsvk->layers;

	for (currentFrameBuffer = 0; currentFrameBuffer < kmsvk->framebufferCount; currentFrameBuffer++) {
		createInfo.attachmentCount = kmsvk->framebufferImageAttachmentCount;
		createInfo.pAttachments = kmsvk->framebufferImages[currentFrameBuffer].imageAttachments;

		res = vkCreateFramebuffer(kmsvk->logicalDevice, &createInfo, NULL, &framebufferHandles[currentFrameBuffer].framebuffer);
		if (res) {
			kmr_utils_log(KMR_DANGER, "[x] vkCreateFramebuffer: %s", vkres_msg(res));
			goto exit_vk_framebuffer_vk_framebuffer_destroy;
		}

		kmr_utils_log(KMR_SUCCESS, "kmr_vk_framebuffer_create: VkFramebuffer successfully created retval(%p)", framebufferHandles[currentFrameBuffer].framebuffer);
	}

	return (struct kmr_vk_framebuffer) { .logicalDevice = kmsvk->logicalDevice, .framebufferCount = kmsvk->framebufferCount, .framebufferHandles = framebufferHandles };

exit_vk_framebuffer_vk_framebuffer_destroy:
	for (currentFrameBuffer = 0; currentFrameBuffer < kmsvk->framebufferCount; currentFrameBuffer++) {
		if (framebufferHandles[currentFrameBuffer].framebuffer)
			vkDestroyFramebuffer(kmsvk->logicalDevice, framebufferHandles[currentFrameBuffer].framebuffer, NULL);
	}
	free(framebufferHandles);
exit_vk_framebuffer:
	return (struct kmr_vk_framebuffer) { .logicalDevice = VK_NULL_HANDLE, .framebufferCount = 0, .framebufferHandles = NULL };
}


struct kmr_vk_command_buffer kmr_vk_command_buffer_create(struct kmr_vk_command_buffer_create_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkCommandPool commandPool = VK_NULL_HANDLE;
	VkCommandBuffer *commandBuffers = VK_NULL_HANDLE;

	struct kmr_vk_command_buffer_handle *commandBufferHandles = NULL;

	VkCommandPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	createInfo.queueFamilyIndex = kmsvk->queueFamilyIndex;

	res = vkCreateCommandPool(kmsvk->logicalDevice, &createInfo, NULL, &commandPool);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkCreateCommandPool: %s", vkres_msg(res));
		goto exit_vk_command_buffer;
	}

	kmr_utils_log(KMR_SUCCESS, "kmr_vk_command_buffer_create: VkCommandPool successfully created retval(%p)", commandPool);

	commandBuffers = alloca(kmsvk->commandBufferCount * sizeof(VkCommandBuffer));

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = kmsvk->commandBufferCount;

	res = vkAllocateCommandBuffers(kmsvk->logicalDevice, &allocInfo, commandBuffers);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkAllocateCommandBuffers: %s", vkres_msg(res));
		goto exit_vk_command_buffer_destroy_cmd_pool;
	}

	commandBufferHandles = calloc(kmsvk->commandBufferCount, sizeof(struct kmr_vk_command_buffer_handle));
	if (!commandBufferHandles) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_command_buffer_destroy_cmd_pool;
	}

	for (uint32_t i = 0; i < kmsvk->commandBufferCount; i++) {
		commandBufferHandles[i].commandBuffer = commandBuffers[i];
		kmr_utils_log(KMR_WARNING, "kmr_vk_command_buffer_create: VkCommandBuffer successfully created retval(%p)", commandBufferHandles[i].commandBuffer);
	}

	return (struct kmr_vk_command_buffer) { .logicalDevice = kmsvk->logicalDevice, .commandPool = commandPool,
	                                        .commandBufferCount = kmsvk->commandBufferCount,
	                                        .commandBufferHandles = commandBufferHandles };

exit_vk_command_buffer_destroy_cmd_pool:
	vkDestroyCommandPool(kmsvk->logicalDevice, commandPool, NULL);
exit_vk_command_buffer:
	return (struct kmr_vk_command_buffer) { .logicalDevice = VK_NULL_HANDLE, .commandPool = VK_NULL_HANDLE,
	                                        .commandBufferCount = 0, .commandBufferHandles = NULL };
}


int kmr_vk_command_buffer_record_begin(struct kmr_vk_command_buffer_record_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.pNext = NULL;
	beginInfo.flags = kmsvk->commandBufferUsageflags;
	// We don't use secondary command buffers in API so set to null
	beginInfo.pInheritanceInfo = NULL;

	for (uint32_t i = 0; i < kmsvk->commandBufferCount; i++) {
		res = vkBeginCommandBuffer(kmsvk->commandBufferHandles[i].commandBuffer, &beginInfo);
		if (res) {
			kmr_utils_log(KMR_DANGER, "[x] vkBeginCommandBuffer: %s", vkres_msg(res));
			return -1;
		}
	}

	return 0;
}


int kmr_vk_command_buffer_record_end(struct kmr_vk_command_buffer_record_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;

	for (uint32_t i = 0; i < kmsvk->commandBufferCount; i++) {
		res = vkEndCommandBuffer(kmsvk->commandBufferHandles[i].commandBuffer);
		if (res) {
			kmr_utils_log(KMR_DANGER, "[x] vkEndCommandBuffer: %s", vkres_msg(res));
			return -1;
		}
	}

	return 0;
}


struct kmr_vk_sync_obj kmr_vk_sync_obj_create(struct kmr_vk_sync_obj_create_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	uint8_t currentSyncObject = 0;

	struct kmr_vk_fence_handle *fenceHandles = NULL;
	struct kmr_vk_semaphore_handle *semaphoreHandles = NULL;

	fenceHandles = calloc(kmsvk->fenceCount, sizeof(struct kmr_vk_fence_handle));
	if (!fenceHandles) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_sync_obj;
	}

	semaphoreHandles = calloc(kmsvk->semaphoreCount, sizeof(struct kmr_vk_semaphore_handle));
	if (!semaphoreHandles) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_sync_obj_free_vk_sync_handles;
	}

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = NULL;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo;
	semaphoreTypeCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	semaphoreTypeCreateInfo.pNext = NULL;
	semaphoreTypeCreateInfo.semaphoreType = kmsvk->semaphoreType;
	semaphoreTypeCreateInfo.initialValue = 0;

	VkSemaphoreCreateInfo semphoreCreateInfo = {};
	semphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semphoreCreateInfo.pNext = &semaphoreTypeCreateInfo;
	semphoreCreateInfo.flags = 0;

	for (currentSyncObject = 0; currentSyncObject < kmsvk->fenceCount; currentSyncObject++) {
		res = vkCreateFence(kmsvk->logicalDevice, &fenceCreateInfo, NULL, &fenceHandles[currentSyncObject].fence);
		if (res) {
			kmr_utils_log(KMR_DANGER, "[x] vkCreateFence: %s", vkres_msg(res));
			goto exit_vk_sync_obj_destroy_vk_fence;
		}
	}

	for (currentSyncObject = 0; currentSyncObject < kmsvk->semaphoreCount; currentSyncObject++) {
		res = vkCreateSemaphore(kmsvk->logicalDevice, &semphoreCreateInfo, NULL, &semaphoreHandles[currentSyncObject].semaphore);
		if (res) {
			kmr_utils_log(KMR_DANGER, "[x] vkCreateSemaphore: %s", vkres_msg(res));
			goto exit_vk_sync_obj_destroy_vk_semaphore;
		}
	}

	return (struct kmr_vk_sync_obj) { .logicalDevice = kmsvk->logicalDevice, .fenceCount = kmsvk->fenceCount, .fenceHandles = fenceHandles,
	                                  .semaphoreCount = kmsvk->semaphoreCount, .semaphoreHandles = semaphoreHandles };

exit_vk_sync_obj_destroy_vk_semaphore:
	for (currentSyncObject = 0; currentSyncObject < kmsvk->semaphoreCount; currentSyncObject++)
		if (semaphoreHandles[currentSyncObject].semaphore)
			vkDestroySemaphore(kmsvk->logicalDevice, semaphoreHandles[currentSyncObject].semaphore, NULL);
exit_vk_sync_obj_destroy_vk_fence:
	for (currentSyncObject = 0; currentSyncObject < kmsvk->fenceCount; currentSyncObject++)
		if (fenceHandles[currentSyncObject].fence)
			vkDestroyFence(kmsvk->logicalDevice, fenceHandles[currentSyncObject].fence, NULL);
exit_vk_sync_obj_free_vk_sync_handles:
	free(semaphoreHandles);
	free(fenceHandles);
exit_vk_sync_obj:
	return (struct kmr_vk_sync_obj) { .logicalDevice = VK_NULL_HANDLE, .fenceCount = 0, .fenceHandles = NULL, .semaphoreCount = 0, .semaphoreHandles = NULL };
};


int kmr_vk_sync_obj_import_external_sync_fd(struct kmr_vk_sync_obj_import_external_sync_fd_info *kmrvk)
{
	VkResult res;

	switch (kmrvk->syncType) {
		case KMR_VK_SYNC_OBJ_SEMAPHORE:
		{
			VkImportSemaphoreFdInfoKHR importSyncInfo;
			importSyncInfo.sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_FD_INFO_KHR;
			importSyncInfo.pNext = NULL;
			importSyncInfo.semaphore = kmrvk->syncHandle.semaphore;
			importSyncInfo.flags = VK_SEMAPHORE_IMPORT_TEMPORARY_BIT;
			importSyncInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
			importSyncInfo.fd = kmrvk->syncFd;

			PFN_vkImportSemaphoreFdKHR vkImportSemaphoreFdKHR;
			KMR_VK_DEVICE_PROC_ADDR(kmrvk->logicalDevice, vkImportSemaphoreFdKHR, ImportSemaphoreFdKHR);

			res = vkImportSemaphoreFdKHR(kmrvk->logicalDevice, &importSyncInfo);
			if (res) {
				kmr_utils_log(KMR_DANGER, "[x] vkImportSemaphoreFdKHR: %s", vkres_msg(res));
				return -1;
			}

		}
			break;
		case KMR_VK_SYNC_OBJ_FENCE:
		{
			VkImportFenceFdInfoKHR importSyncInfo;
			importSyncInfo.sType = VK_STRUCTURE_TYPE_IMPORT_FENCE_FD_INFO_KHR;
			importSyncInfo.pNext = NULL;
			importSyncInfo.fence = kmrvk->syncHandle.fence;
			importSyncInfo.flags = VK_FENCE_IMPORT_TEMPORARY_BIT;
			importSyncInfo.handleType = VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT;
			importSyncInfo.fd = kmrvk->syncFd;

			PFN_vkImportFenceFdKHR vkImportFenceFdKHR;
			KMR_VK_DEVICE_PROC_ADDR(kmrvk->logicalDevice, vkImportFenceFdKHR, ImportFenceFdKHR);

			res = vkImportFenceFdKHR(kmrvk->logicalDevice, &importSyncInfo);
			if (res) {
				kmr_utils_log(KMR_DANGER, "[x] vkImportFenceFdKHR: %s", vkres_msg(res));
				return -1;
			}
		}
			break;
		default:
			kmr_utils_log(KMR_DANGER, "[x] kmr_vk_sync_obj_import_external_sync_fd: Did you correctly specify @syncType");
			return -1;
	}
	return 0;
}


int kmr_vk_sync_obj_export_external_sync_fd(struct kmr_vk_sync_obj_export_external_sync_fd_info *kmrvk)
{
	VkResult res;
	int fd = -1;

	switch (kmrvk->syncType) {
		case KMR_VK_SYNC_OBJ_SEMAPHORE:
		{
			VkSemaphoreGetFdInfoKHR exportSyncInfo;
			exportSyncInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR;
			exportSyncInfo.pNext = NULL;
			exportSyncInfo.semaphore = kmrvk->syncHandle.semaphore;
			exportSyncInfo.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;

			PFN_vkGetSemaphoreFdKHR vkGetSemaphoreFdKHR;
			KMR_VK_DEVICE_PROC_ADDR(kmrvk->logicalDevice, vkGetSemaphoreFdKHR, GetSemaphoreFdKHR);

			// Note: vkGetSemaphoreFdKHR implicitly resets the semaphore
			res = vkGetSemaphoreFdKHR(kmrvk->logicalDevice, &exportSyncInfo, &fd);
			if (res) {
				kmr_utils_log(KMR_DANGER, "[x] vkGetSemaphoreFdKHR: %s", vkres_msg(res));
				return -1;
			}
		}
			break;
		case KMR_VK_SYNC_OBJ_FENCE:
		{
			VkFenceGetFdInfoKHR exportSyncInfo;
			exportSyncInfo.sType = VK_STRUCTURE_TYPE_FENCE_GET_FD_INFO_KHR;
			exportSyncInfo.pNext = NULL;
			exportSyncInfo.fence = kmrvk->syncHandle.fence;
			exportSyncInfo.handleType = VK_EXTERNAL_FENCE_HANDLE_TYPE_SYNC_FD_BIT;

			PFN_vkGetFenceFdKHR vkGetFenceFdKHR;
			KMR_VK_DEVICE_PROC_ADDR(kmrvk->logicalDevice, vkGetFenceFdKHR, GetFenceFdKHR);

			res = vkGetFenceFdKHR(kmrvk->logicalDevice, &exportSyncInfo, &fd);
			if (res) {
				kmr_utils_log(KMR_DANGER, "[x] vkGetFenceFdKHR: %s", vkres_msg(res));
				return -1;
			}
		}
			break;
		default:
			kmr_utils_log(KMR_DANGER, "[x] kmr_vk_sync_obj_export_external_sync_fd: Did you correctly specify @syncType");
			return -1;
	}

	return fd;
}


struct kmr_vk_buffer kmr_vk_buffer_create(struct kmr_vk_buffer_create_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory deviceMemory = VK_NULL_HANDLE;

	VkBufferCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = kmsvk->bufferFlags;
	createInfo.size  = kmsvk->bufferSize;
	createInfo.usage = kmsvk->bufferUsage;
	createInfo.sharingMode = kmsvk->bufferSharingMode;
	createInfo.queueFamilyIndexCount = kmsvk->queueFamilyIndexCount;
	createInfo.pQueueFamilyIndices = kmsvk->queueFamilyIndices;

	/* Creates underlying buffer header */
	res = vkCreateBuffer(kmsvk->logicalDevice, &createInfo, NULL, &buffer);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkCreateBuffer: %s", vkres_msg(res));
		goto exit_vk_buffer;
	}

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(kmsvk->logicalDevice, buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.memoryTypeIndex = retrieve_memory_type_index(kmsvk->physDevice, memoryRequirements.memoryTypeBits, kmsvk->memPropertyFlags);

	res = vkAllocateMemory(kmsvk->logicalDevice, &allocInfo, NULL, &deviceMemory);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkAllocateMemory: %s", vkres_msg(res));
		goto exit_vk_buffer;
	}

	vkBindBufferMemory(kmsvk->logicalDevice, buffer, deviceMemory, 0);

	return (struct kmr_vk_buffer) { .logicalDevice = kmsvk->logicalDevice, .buffer = buffer, .deviceMemory = deviceMemory };

exit_vk_buffer:
	return (struct kmr_vk_buffer) { .logicalDevice = VK_NULL_HANDLE, .buffer = VK_NULL_HANDLE, .deviceMemory = VK_NULL_HANDLE };
}


struct kmr_vk_descriptor_set_layout kmr_vk_descriptor_set_layout_create(struct kmr_vk_descriptor_set_layout_create_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

	VkDescriptorSetLayoutCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = kmsvk->descriptorSetLayoutCreateflags;
	createInfo.bindingCount = kmsvk->descriptorSetLayoutBindingCount;
	createInfo.pBindings = kmsvk->descriptorSetLayoutBindings;

	res = vkCreateDescriptorSetLayout(kmsvk->logicalDevice, &createInfo, VK_NULL_HANDLE, &descriptorSetLayout);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkCreateDescriptorSetLayout: %s", vkres_msg(res));
		goto exit_vk_descriptor_set_layout;
	}

	return (struct kmr_vk_descriptor_set_layout) { .logicalDevice = kmsvk->logicalDevice, .descriptorSetLayout = descriptorSetLayout };

exit_vk_descriptor_set_layout:
	return (struct kmr_vk_descriptor_set_layout) { .logicalDevice = VK_NULL_HANDLE, .descriptorSetLayout = VK_NULL_HANDLE };
}


struct kmr_vk_descriptor_set kmr_vk_descriptor_set_create(struct kmr_vk_descriptor_set_create_info *kmsvk)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet *descriptorSets = VK_NULL_HANDLE;

	struct kmr_vk_descriptor_set_handle *descriptorSetHandles = NULL;

	/*
	 * Per my understanding allocate VkDescriptorPool given a certain amount of information. Like the amount of sets,
	 * the layout for a given set, and descriptors. No descriptor is assigned to a set when the pool is created.
	 * Given a descriptor set layout the actual assignment of descriptor to descriptor set happens in the
	 * vkAllocateDescriptorSets function.
	 */
	VkDescriptorPoolCreateInfo createInfo;
	createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = kmsvk->descriptorPoolCreateflags;
	createInfo.maxSets = kmsvk->descriptorSetLayoutCount;
	createInfo.poolSizeCount = kmsvk->descriptorPoolInfoCount;
	createInfo.pPoolSizes = kmsvk->descriptorPoolInfos;

	res = vkCreateDescriptorPool(kmsvk->logicalDevice, &createInfo, VK_NULL_HANDLE, &descriptorPool);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkCreateDescriptorSetLayout: %s", vkres_msg(res));
		goto exit_vk_descriptor_set;
	}

	descriptorSets = alloca(kmsvk->descriptorSetLayoutCount * sizeof(VkDescriptorSet));

	VkDescriptorSetAllocateInfo allocInfo;
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.pNext = NULL;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = kmsvk->descriptorSetLayoutCount;
	allocInfo.pSetLayouts = kmsvk->descriptorSetLayouts;

	res = vkAllocateDescriptorSets(kmsvk->logicalDevice, &allocInfo, descriptorSets);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkAllocateDescriptorSets: %s", vkres_msg(res));
		goto exit_vk_descriptor_set_destroy_pool;
	}

	descriptorSetHandles = calloc(kmsvk->descriptorSetLayoutCount, sizeof(struct kmr_vk_descriptor_set_handle));
	if (!descriptorSetHandles) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_descriptor_set_destroy_pool;
	}

	for (uint32_t i = 0; i < kmsvk->descriptorSetLayoutCount; i++) {
		descriptorSetHandles[i].descriptorSet = descriptorSets[i];
		kmr_utils_log(KMR_WARNING, "kmr_vk_descriptor_set_create: VkDescriptorSet successfully created retval(%p)", descriptorSetHandles[i].descriptorSet);
	}

	return (struct kmr_vk_descriptor_set) { .logicalDevice = kmsvk->logicalDevice, .descriptorPool = descriptorPool,
	                                        .descriptorSetHandles = descriptorSetHandles, .descriptorSetsCount = kmsvk->descriptorSetLayoutCount };

exit_vk_descriptor_set_destroy_pool:
	if (descriptorPool)
		vkDestroyDescriptorPool(kmsvk->logicalDevice, descriptorPool, NULL);
exit_vk_descriptor_set:
	return (struct kmr_vk_descriptor_set) { .logicalDevice = VK_NULL_HANDLE, .descriptorPool = VK_NULL_HANDLE, .descriptorSetHandles = VK_NULL_HANDLE, .descriptorSetsCount = 0 };
}


struct kmr_vk_sampler kmr_vk_sampler_create(struct kmr_vk_sampler_create_info *kmsvk)
{
	VkSampler sampler = VK_NULL_HANDLE;
	VkResult res = VK_RESULT_MAX_ENUM;

	VkSamplerCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	createInfo.pNext = NULL;
	createInfo.flags = kmsvk->samplerFlags;
	createInfo.magFilter = kmsvk->samplerMagFilter;
	createInfo.minFilter = kmsvk->samplerMinFilter;
	createInfo.mipmapMode = kmsvk->samplerMipmapMode;
	createInfo.addressModeU = kmsvk->samplerAddressModeU;
	createInfo.addressModeV = kmsvk->samplerAddressModeV;
	createInfo.addressModeW = kmsvk->samplerAddressModeW;
	createInfo.mipLodBias = kmsvk->samplerMipLodBias;
	createInfo.anisotropyEnable = kmsvk->samplerAnisotropyEnable;
	createInfo.maxAnisotropy = kmsvk->samplerMaxAnisotropy;
	createInfo.compareEnable = kmsvk->samplerCompareEnable;
	createInfo.compareOp = kmsvk->samplerCompareOp;
	createInfo.minLod = kmsvk->samplerMinLod;
	createInfo.maxLod = kmsvk->samplerMaxLod;
	createInfo.borderColor = kmsvk->samplerBorderColor;
	createInfo.unnormalizedCoordinates = kmsvk->samplerUnnormalizedCoordinates;

	res = vkCreateSampler(kmsvk->logicalDevice, &createInfo, NULL, &sampler);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkCreateSampler: %s", vkres_msg(res));
		goto exit_vk_sampler;
	}

	kmr_utils_log(KMR_SUCCESS, "kmr_vk_sampler_create: VkSampler created retval(%p)", sampler);

	return (struct kmr_vk_sampler) { .logicalDevice = kmsvk->logicalDevice, .sampler = sampler };

exit_vk_sampler:
	return (struct kmr_vk_sampler) { .logicalDevice = VK_NULL_HANDLE, .sampler = VK_NULL_HANDLE };
}


int kmr_vk_resource_copy(struct kmr_vk_resource_copy_info *kmsvk)
{
	struct kmr_vk_command_buffer_handle commandBufferHandle;
	commandBufferHandle.commandBuffer = kmsvk->commandBuffer;

	struct kmr_vk_command_buffer_record_info commandBufferRecordInfo;
	commandBufferRecordInfo.commandBufferCount = 1;
	commandBufferRecordInfo.commandBufferHandles = &commandBufferHandle;
	commandBufferRecordInfo.commandBufferUsageflags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (kmr_vk_command_buffer_record_begin(&commandBufferRecordInfo) == -1)
		return -1;

	VkCommandBuffer commandBuffer = commandBufferHandle.commandBuffer;

	switch (kmsvk->resourceCopyType) {
		case KMR_VK_COPY_VK_BUFFER_TO_VK_BUFFER:
		{
			vkCmdCopyBuffer(commandBuffer, (VkBuffer) kmsvk->srcResource, (VkBuffer) kmsvk->dstResource, 1, kmsvk->bufferCopyInfo);
			break;
		}
		case KMR_VK_COPY_VK_BUFFER_TO_VK_IMAGE:
		{
			vkCmdCopyBufferToImage(commandBuffer, (VkBuffer) kmsvk->srcResource, (VkImage) kmsvk->dstResource, kmsvk->imageLayout, 1, kmsvk->bufferImageCopyInfo);
			break;
		}
		case KMR_VK_COPY_VK_IMAGE_TO_VK_BUFFER:
		{
			vkCmdCopyImageToBuffer(commandBuffer, (VkImage) kmsvk->srcResource, kmsvk->imageLayout, (VkBuffer) kmsvk->dstResource,  1, kmsvk->bufferImageCopyInfo);
			break;
		}
		default:
			kmr_utils_log(KMR_DANGER, "[x] kmr_vk_buffer_copy: Must pass a valid kmr_vk_buffer_copy_type value");
			return -1;
	}

	if (kmr_vk_command_buffer_record_end(&commandBufferRecordInfo) == -1)
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

	vkQueueSubmit(kmsvk->queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(kmsvk->queue);

	return 0;
}


int kmr_vk_resource_pipeline_barrier(struct kmr_vk_resource_pipeline_barrier_info *kmsvk)
{
	struct kmr_vk_command_buffer_handle commandBufferHandle;
	commandBufferHandle.commandBuffer = kmsvk->commandBuffer;

	struct kmr_vk_command_buffer_record_info commandBufferRecordInfo;
	commandBufferRecordInfo.commandBufferCount = 1;
	commandBufferRecordInfo.commandBufferHandles = &commandBufferHandle;
	commandBufferRecordInfo.commandBufferUsageflags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (kmr_vk_command_buffer_record_begin(&commandBufferRecordInfo) == -1)
		return -1;

	vkCmdPipelineBarrier(commandBufferHandle.commandBuffer,
	                     kmsvk->srcPipelineStage,
	                     kmsvk->dstPipelineStage,
	                     kmsvk->dependencyFlags,
	                     (kmsvk->memoryBarrier      ) ? 1 : 0, kmsvk->memoryBarrier,
	                     (kmsvk->bufferMemoryBarrier) ? 1 : 0, kmsvk->bufferMemoryBarrier,
	                     (kmsvk->imageMemoryBarrier ) ? 1 : 0, kmsvk->imageMemoryBarrier);

	if (kmr_vk_command_buffer_record_end(&commandBufferRecordInfo) == -1)
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

	vkQueueSubmit(kmsvk->queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(kmsvk->queue);

	return 0;
}


VkSurfaceCapabilitiesKHR kmr_vk_get_surface_capabilities(VkPhysicalDevice physDev, VkSurfaceKHR surface)
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDev, surface, &surfaceCapabilities);
	return surfaceCapabilities;
}


struct kmr_vk_surface_format kmr_vk_get_surface_formats(VkPhysicalDevice physDev, VkSurfaceKHR surface)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkSurfaceFormatKHR *surfaceFormats = NULL;
	uint32_t surfaceFormatCount = 0;

	res = vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &surfaceFormatCount, NULL);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkGetPhysicalDeviceSurfaceFormatsKHR: %s", vkres_msg(res));
		goto exit_vk_surface_formats;
	}

	surfaceFormats = (VkSurfaceFormatKHR *) calloc(surfaceFormatCount, sizeof(VkSurfaceFormatKHR));
	if (!surfaceFormats) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_surface_formats;
	}

	res = vkGetPhysicalDeviceSurfaceFormatsKHR(physDev, surface, &surfaceFormatCount, surfaceFormats);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkGetPhysicalDeviceSurfaceFormatsKHR: %s", vkres_msg(res));
		goto exit_vk_surface_formats_free;
	}

	return (struct kmr_vk_surface_format) { .surfaceFormatCount = surfaceFormatCount, .surfaceFormats = surfaceFormats };

exit_vk_surface_formats_free:
	free(surfaceFormats);
exit_vk_surface_formats:
	return (struct kmr_vk_surface_format) { .surfaceFormatCount = 0, .surfaceFormats = NULL };
}


struct kmr_vk_surface_present_mode kmr_vk_get_surface_present_modes(VkPhysicalDevice physDev, VkSurfaceKHR surface)
{
	VkResult res = VK_RESULT_MAX_ENUM;
	VkPresentModeKHR *presentModes = NULL;
	uint32_t presentModeCount = 0;

	res = vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &presentModeCount, NULL);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkGetPhysicalDeviceSurfacePresentModesKHR: %s", vkres_msg(res));
		goto exit_vk_surface_present_modes;
	}

	presentModes = (VkPresentModeKHR *) calloc(presentModeCount, sizeof(VkPresentModeKHR));
	if (!presentModes) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_vk_surface_present_modes;
	}

	res = vkGetPhysicalDeviceSurfacePresentModesKHR(physDev, surface, &presentModeCount, presentModes);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkGetPhysicalDeviceSurfacePresentModesKHR: %s", vkres_msg(res));
		goto exit_vk_surface_present_modes_free;
	}

	return (struct kmr_vk_surface_present_mode) { .presentModeCount = presentModeCount, .presentModes = presentModes };

exit_vk_surface_present_modes_free:
	free(presentModes);
exit_vk_surface_present_modes:
	return (struct kmr_vk_surface_present_mode) { .presentModeCount = 0, .presentModes = NULL };
}


struct kmr_vk_phdev_format_prop kmr_vk_get_phdev_format_properties(struct kmr_vk_phdev_format_prop_info *kmrvk)
{
	VkFormatProperties *formatProperties = NULL;

	VkDrmFormatModifierPropertiesListEXT modPropsList;
	modPropsList.sType = VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT;
	modPropsList.pNext = NULL;
	modPropsList.drmFormatModifierCount = kmrvk->modifierCount;
	modPropsList.pDrmFormatModifierProperties = kmrvk->modifierProperties;

	VkFormatProperties2 formatProps2;
	formatProps2.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;
	formatProps2.pNext = (kmrvk->modifierCount) ? &modPropsList : NULL;

	formatProperties = (VkFormatProperties *) calloc(kmrvk->formatCount, sizeof(VkFormatProperties));
	if (!formatProperties) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_get_phdev_format_properties;
	}

	for (uint32_t f = 0; f < kmrvk->formatCount; f++) {
		vkGetPhysicalDeviceFormatProperties2(kmrvk->physDev, kmrvk->formats[f], &formatProps2);
		memcpy(&formatProperties[f], &formatProps2.formatProperties, sizeof(formatProps2.formatProperties));
		memset(&formatProps2.formatProperties, 0, sizeof(formatProps2.formatProperties));
	}

	return (struct kmr_vk_phdev_format_prop) { .formatProperties = formatProperties, .formatPropertyCount = kmrvk->formatCount };

exit_get_phdev_format_properties:
	free(formatProperties);
	return (struct kmr_vk_phdev_format_prop) { .formatProperties = NULL, .formatPropertyCount = 0 };
}


VkExternalSemaphoreProperties kmr_vk_get_external_semaphore_properties(VkPhysicalDevice physDev, VkExternalSemaphoreHandleTypeFlagBits handleType) {
	VkPhysicalDeviceExternalSemaphoreInfo externalSemaphoreInfo;
	externalSemaphoreInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO;
	externalSemaphoreInfo.pNext = NULL;
	externalSemaphoreInfo.handleType = handleType;

	VkExternalSemaphoreProperties semaphoreProperties;
	memset(&semaphoreProperties, 0, sizeof(semaphoreProperties));
	semaphoreProperties.sType = VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES;
	semaphoreProperties.pNext = NULL;

	vkGetPhysicalDeviceExternalSemaphoreProperties(physDev, &externalSemaphoreInfo, &semaphoreProperties);

	return semaphoreProperties;
}


uint32_t kmr_vk_get_external_fd_memory_properties(VkDevice logicalDevice,
                                                  int externalMemoryFd,
                                                  VkExternalMemoryHandleTypeFlagBits handleType)
{
	VkResult res;

	VkMemoryFdPropertiesKHR memoryPropsInfo;
	memset(&memoryPropsInfo, 0, sizeof(memoryPropsInfo));
	memoryPropsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR;

	PFN_vkGetMemoryFdPropertiesKHR vkGetMemoryFdPropertiesKHR;
	KMR_VK_DEVICE_PROC_ADDR(logicalDevice, vkGetMemoryFdPropertiesKHR, GetMemoryFdPropertiesKHR);

	res = vkGetMemoryFdPropertiesKHR(logicalDevice, handleType, externalMemoryFd, &memoryPropsInfo);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkGetMemoryFdPropertiesKHR: %s", vkres_msg(res));
		return UINT32_MAX;
	}

	return memoryPropsInfo.memoryTypeBits;
}


int kmr_vk_memory_export_external_fd(struct kmr_vk_memory_export_external_fd_info *kmrvk)
{
	VkResult res;
	int fd = -1;

	VkMemoryGetFdInfoKHR exportMemoryInfo;
	exportMemoryInfo.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR;
	exportMemoryInfo.pNext = NULL;
	exportMemoryInfo.memory = kmrvk->deviceMemory;
	exportMemoryInfo.handleType = kmrvk->handleType;

	PFN_vkGetMemoryFdKHR vkGetMemoryFdKHR;
	KMR_VK_DEVICE_PROC_ADDR(kmrvk->logicalDevice, vkGetMemoryFdKHR, GetMemoryFdKHR);

	res = vkGetMemoryFdKHR(kmrvk->logicalDevice, &exportMemoryInfo, &fd);
	if (res) {
		kmr_utils_log(KMR_DANGER, "[x] vkGetMemoryFdKHR: %s", vkres_msg(res));
		return -1;
	}

	return fd;
}


void kmr_vk_memory_map(struct kmr_vk_memory_map_info *kmsvk)
{
	void *data = NULL;
	vkMapMemory(kmsvk->logicalDevice, kmsvk->deviceMemory, kmsvk->deviceMemoryOffset, kmsvk->memoryBufferSize, 0, &data);
	memcpy(data, kmsvk->bufferData, kmsvk->memoryBufferSize);
	vkUnmapMemory(kmsvk->logicalDevice, kmsvk->deviceMemory);
}


void kmr_vk_destroy(struct kmr_vk_destroy *kmsvk)
{
	uint32_t i, j, p;

	for (i = 0; i < kmsvk->kmr_vk_lgdev_cnt; i++) {
		if (kmsvk->kmr_vk_lgdev[i].logicalDevice) {
			vkDeviceWaitIdle(kmsvk->kmr_vk_lgdev[i].logicalDevice);
		}
	}

	if (kmsvk->kmr_vk_sync_obj) {
		for (i = 0; i < kmsvk->kmr_vk_sync_obj_cnt; i++) {
			for (j = 0; j < kmsvk->kmr_vk_sync_obj[i].fenceCount; j++) {
				if (kmsvk->kmr_vk_sync_obj[i].fenceHandles[j].fence) {
					vkDestroyFence(kmsvk->kmr_vk_sync_obj[i].logicalDevice, kmsvk->kmr_vk_sync_obj[i].fenceHandles[j].fence, NULL);
				}
			}
			for (j = 0; j < kmsvk->kmr_vk_sync_obj[i].semaphoreCount; j++) {
				if (kmsvk->kmr_vk_sync_obj[i].semaphoreHandles[j].semaphore) {
					vkDestroySemaphore(kmsvk->kmr_vk_sync_obj[i].logicalDevice, kmsvk->kmr_vk_sync_obj[i].semaphoreHandles[j].semaphore, NULL);
				}
			}
			free(kmsvk->kmr_vk_sync_obj[i].fenceHandles);
			free(kmsvk->kmr_vk_sync_obj[i].semaphoreHandles);
		}
	}

	if (kmsvk->kmr_vk_command_buffer) {
		for (i = 0; i < kmsvk->kmr_vk_command_buffer_cnt; i++) {
			if (kmsvk->kmr_vk_command_buffer[i].logicalDevice && kmsvk->kmr_vk_command_buffer[i].commandPool)
				vkDestroyCommandPool(kmsvk->kmr_vk_command_buffer[i].logicalDevice, kmsvk->kmr_vk_command_buffer[i].commandPool, NULL);
			free(kmsvk->kmr_vk_command_buffer[i].commandBufferHandles);
		}
	}

	if (kmsvk->kmr_vk_buffer) {
		for (i = 0; i < kmsvk->kmr_vk_buffer_cnt; i++) {
			if (kmsvk->kmr_vk_buffer[i].buffer)
				vkDestroyBuffer(kmsvk->kmr_vk_buffer[i].logicalDevice, kmsvk->kmr_vk_buffer[i].buffer, NULL);
			if (kmsvk->kmr_vk_buffer[i].deviceMemory)
				vkFreeMemory(kmsvk->kmr_vk_buffer[i].logicalDevice, kmsvk->kmr_vk_buffer[i].deviceMemory, NULL);
		}
	}

	if (kmsvk->kmr_vk_descriptor_set) {
		for (i = 0; i < kmsvk->kmr_vk_descriptor_set_cnt; i++) {
			if (kmsvk->kmr_vk_descriptor_set[i].logicalDevice && kmsvk->kmr_vk_descriptor_set[i].descriptorPool)
				vkDestroyDescriptorPool(kmsvk->kmr_vk_descriptor_set[i].logicalDevice, kmsvk->kmr_vk_descriptor_set[i].descriptorPool, NULL);
			free(kmsvk->kmr_vk_descriptor_set[i].descriptorSetHandles);
		}
	}

	if (kmsvk->kmr_vk_descriptor_set_layout) {
		for (i = 0; i < kmsvk->kmr_vk_descriptor_set_layout_cnt; i++) {
			if (kmsvk->kmr_vk_descriptor_set_layout[i].logicalDevice && kmsvk->kmr_vk_descriptor_set_layout[i].descriptorSetLayout)
				vkDestroyDescriptorSetLayout(kmsvk->kmr_vk_descriptor_set_layout[i].logicalDevice, kmsvk->kmr_vk_descriptor_set_layout[i].descriptorSetLayout, NULL);
		}
	}

	if (kmsvk->kmr_vk_framebuffer) {
		for (i = 0; i < kmsvk->kmr_vk_framebuffer_cnt; i++) {
			for (j = 0; j < kmsvk->kmr_vk_framebuffer[i].framebufferCount; j++) {
				if (kmsvk->kmr_vk_framebuffer[i].logicalDevice && kmsvk->kmr_vk_framebuffer[i].framebufferHandles[j].framebuffer)
					vkDestroyFramebuffer(kmsvk->kmr_vk_framebuffer[i].logicalDevice, kmsvk->kmr_vk_framebuffer[i].framebufferHandles[j].framebuffer, NULL);
			}
			free(kmsvk->kmr_vk_framebuffer[i].framebufferHandles);
		}
	}

	if (kmsvk->kmr_vk_graphics_pipeline) {
		for (i = 0; i < kmsvk->kmr_vk_graphics_pipeline_cnt; i++) {
			if (kmsvk->kmr_vk_graphics_pipeline[i].logicalDevice && kmsvk->kmr_vk_graphics_pipeline[i].graphicsPipeline)
				vkDestroyPipeline(kmsvk->kmr_vk_graphics_pipeline[i].logicalDevice, kmsvk->kmr_vk_graphics_pipeline[i].graphicsPipeline, NULL);
		}
	}

	if (kmsvk->kmr_vk_pipeline_layout) {
		for (i = 0; i < kmsvk->kmr_vk_pipeline_layout_cnt; i++) {
			if (kmsvk->kmr_vk_pipeline_layout[i].logicalDevice && kmsvk->kmr_vk_pipeline_layout[i].pipelineLayout)
				vkDestroyPipelineLayout(kmsvk->kmr_vk_pipeline_layout[i].logicalDevice, kmsvk->kmr_vk_pipeline_layout[i].pipelineLayout, NULL);
		}
	}

	if (kmsvk->kmr_vk_render_pass) {
		for (i = 0; i < kmsvk->kmr_vk_render_pass_cnt; i++) {
			if (kmsvk->kmr_vk_render_pass[i].logicalDevice && kmsvk->kmr_vk_render_pass[i].renderPass)
				vkDestroyRenderPass(kmsvk->kmr_vk_render_pass[i].logicalDevice, kmsvk->kmr_vk_render_pass[i].renderPass, NULL);
		}
	}

	if (kmsvk->kmr_vk_shader_module) {
		for (i = 0; i < kmsvk->kmr_vk_shader_module_cnt; i++) {
			if (kmsvk->kmr_vk_shader_module[i].logicalDevice && kmsvk->kmr_vk_shader_module[i].shaderModule)
				vkDestroyShaderModule(kmsvk->kmr_vk_shader_module[i].logicalDevice, kmsvk->kmr_vk_shader_module[i].shaderModule, NULL);
		}
	}

	if (kmsvk->kmr_vk_sampler) {
		for (i = 0; i < kmsvk->kmr_vk_sampler_cnt; i++) {
			if (kmsvk->kmr_vk_sampler[i].logicalDevice && kmsvk->kmr_vk_sampler[i].sampler)
			vkDestroySampler(kmsvk->kmr_vk_sampler[i].logicalDevice, kmsvk->kmr_vk_sampler[i].sampler, NULL);
		}
	}

	if (kmsvk->kmr_vk_image) {
		for (i = 0; i < kmsvk->kmr_vk_image_cnt; i++) {
			for (j = 0; j < kmsvk->kmr_vk_image[i].imageCount; j++) {
				if (!kmsvk->kmr_vk_image[i].swapchain) {
					if (kmsvk->kmr_vk_image[i].logicalDevice && kmsvk->kmr_vk_image[i].imageHandles[j].image)
						vkDestroyImage(kmsvk->kmr_vk_image[i].logicalDevice, kmsvk->kmr_vk_image[i].imageHandles[j].image, NULL);
					for (p = 0; p < kmsvk->kmr_vk_image[i].imageHandles[j].deviceMemoryCount; p++) {
						if (kmsvk->kmr_vk_image[i].logicalDevice && kmsvk->kmr_vk_image[i].imageHandles[j].deviceMemory[p])
							vkFreeMemory(kmsvk->kmr_vk_image[i].logicalDevice, kmsvk->kmr_vk_image[i].imageHandles[j].deviceMemory[p], NULL);
					}
				}
				if (kmsvk->kmr_vk_image[i].logicalDevice && kmsvk->kmr_vk_image[i].imageViewHandles[j].view)
					vkDestroyImageView(kmsvk->kmr_vk_image[i].logicalDevice, kmsvk->kmr_vk_image[i].imageViewHandles[j].view, NULL);
			}
			free(kmsvk->kmr_vk_image[i].imageHandles);
			free(kmsvk->kmr_vk_image[i].imageViewHandles);
		}
	}

	if (kmsvk->kmr_vk_swapchain) {
		for (i = 0; i < kmsvk->kmr_vk_swapchain_cnt; i++) {
			if (kmsvk->kmr_vk_swapchain[i].logicalDevice && kmsvk->kmr_vk_swapchain[i].swapchain)
				vkDestroySwapchainKHR(kmsvk->kmr_vk_swapchain[i].logicalDevice, kmsvk->kmr_vk_swapchain[i].swapchain, NULL);
		}
	}

	for (i = 0; i < kmsvk->kmr_vk_lgdev_cnt; i++) {
		if (kmsvk->kmr_vk_lgdev[i].logicalDevice) {
			vkDestroyDevice(kmsvk->kmr_vk_lgdev[i].logicalDevice, NULL);
		}
	}

	if (kmsvk->surface)
		vkDestroySurfaceKHR(kmsvk->instance, kmsvk->surface, NULL);
	if (kmsvk->instance)
		vkDestroyInstance(kmsvk->instance, NULL);
}
