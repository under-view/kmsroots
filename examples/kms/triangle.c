#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h> // For offsetof(3)
#include <errno.h>
#include <signal.h>
#include <sys/epoll.h>

#include <cglm/cglm.h>

#include "drm-node.h"
#include "buffer.h"
#include "dma-buf.h"
#include "pixel-format.h"
#include "input.h"
#include "vulkan.h"
#include "shader.h"
#include "gltf-loader.h"


#define PRECEIVED_SWAPCHAIN_IMAGE_SIZE 2
#define MAX_EPOLL_EVENTS 2

/***************************
 * Structs used by example *
 ***************************/

struct app_vk {
	VkInstance instance;
	struct kmr_vk_phdev kmr_vk_phdev;
	struct kmr_vk_lgdev kmr_vk_lgdev;
	struct kmr_vk_queue kmr_vk_queue;

	/*
	 * 0. Swapchain Images (DMA-BUF's -> VkImage's->VkDeviceMemory's)
	 */
	struct kmr_vk_image kmr_vk_image;

	/*
	 * 0. Vertex Shader Model
	 * 1. Fragment Shader Model
	 */
	struct kmr_vk_shader_module kmr_vk_shader_module[2];

	struct kmr_vk_pipeline_layout kmr_vk_pipeline_layout;
	struct kmr_vk_render_pass kmr_vk_render_pass;
	struct kmr_vk_graphics_pipeline kmr_vk_graphics_pipeline;
	struct kmr_vk_framebuffer kmr_vk_framebuffer;
	struct kmr_vk_command_buffer kmr_vk_command_buffer;
	struct kmr_vk_sync_obj kmr_vk_sync_obj;

	/*
	 * 1. CPU visible transfer buffer
	 * 2. GPU visible vertex buffer
	 */
	struct kmr_vk_buffer kmr_vk_buffer[2];
};


struct app_kms {
	struct kmr_drm_node *kmr_drm_node;
	struct kmr_drm_node_display *kmr_drm_node_display;
	struct kmr_drm_node_atomic_request *kmr_drm_node_atomic_request;
	struct kmr_buffer *kmr_buffer;
	struct kmr_dma_buf_export_sync_file *kmr_dma_buf_export_sync_file[PRECEIVED_SWAPCHAIN_IMAGE_SIZE];
	struct kmr_input *kmr_input;
#ifdef INCLUDE_LIBSEAT
	struct kmr_session *kmr_session;
#endif
};


struct app_vk_kms {
	struct app_kms *app_kms;
	struct app_vk  *app_vk;
};


struct app_vertex_data {
	vec2 pos;
	vec3 color;
};


/***********
 * Globals *
 ***********/
static volatile sig_atomic_t prun = 1;


static const struct app_vertex_data meshData[3] = {
	{{ 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{ 0.5f,0.5f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f,0.5f}, {0.0f, 0.0f, 1.0f}}
};


/***********************
 * Function Prototypes *
 ***********************/

static int
create_kms_instance (struct app_kms *kms);

static int
create_kms_gbm_buffers (struct app_kms *kms);

static int
create_kms_set_crtc (struct app_kms *kms);

static int
create_kms_atomic_request_instance (struct app_vk_kms *passData,
                                    uint8_t *cbuf,
                                    int *fbid,
                                    volatile bool *running);

static int
create_vk_instance (struct app_vk *app);

static int
create_vk_device (struct app_vk *app, struct app_kms *kms);

static int
create_vk_swapchain_images (struct app_vk *app,
                            struct app_kms *kms,
                            VkSurfaceFormatKHR *surfaceFormat);

static int
create_vk_shader_modules (struct app_vk *app);

static int
create_vk_buffers (struct app_vk *app);

static int
create_vk_graphics_pipeline (struct app_vk *app,
                             VkSurfaceFormatKHR *surfaceFormat,
                             VkExtent2D extent2D);

static int
create_vk_framebuffers (struct app_vk *app, VkExtent2D extent2D);

static int
create_vk_command_buffers (struct app_vk *app);

static int
create_vk_sync_objs (struct app_vk *app);

static int
record_vk_draw_commands (struct app_vk *app,
                         uint32_t swapchainImageIndex,
                         VkExtent2D extent2D);

/************************************
 * Start of function implementation *
 ************************************/

static void
run_stop (int UNUSED signum)
{
	prun = 0;
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
render (volatile bool *running, uint8_t *imageIndex, int *fbid, void *data)
{
	struct app_vk_kms *passData = (struct app_vk_kms *) data;
	struct app_vk *app = passData->app_vk;
	struct app_kms *kms = passData->app_kms;

	if (!app->kmr_vk_sync_obj.semaphoreHandles)
		return;

	VkExtent2D extent2D;
	extent2D.width = kms->kmr_drm_node_display->width;
	extent2D.height = kms->kmr_drm_node_display->height;

	// Write to buffer that'll be displayed at function end
	// acquire Next Image (TODO: Implement own version)
	*imageIndex = (*imageIndex + 1) % kms->kmr_buffer->bufferCount;
	*fbid = kms->kmr_buffer->bufferObjects[*imageIndex].fbid;

	record_vk_draw_commands(app, *((uint32_t*)imageIndex), extent2D);

	static uint64_t signalValue = 1;

	VkSemaphore timelineSemaphore = app->kmr_vk_sync_obj.semaphoreHandles[0].semaphore;
	VkCommandBuffer commandBuffer = app->kmr_vk_command_buffer.commandBufferHandles[0].commandBuffer;

	VkPipelineStageFlags waitStages[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore waitSemaphores[1] = { timelineSemaphore };
	VkSemaphore signalSemaphores[1] = { timelineSemaphore };

	VkTimelineSemaphoreSubmitInfo timelineInfo;
	timelineInfo.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
	timelineInfo.pNext = NULL;
	timelineInfo.waitSemaphoreValueCount = 0;
	timelineInfo.pWaitSemaphoreValues = NULL;
	timelineInfo.signalSemaphoreValueCount = 1;
	timelineInfo.pSignalSemaphoreValues = &signalValue;

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = &timelineInfo;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = NULL;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = ARRAY_LEN(signalSemaphores);
	submitInfo.pSignalSemaphores = signalSemaphores;

	/* Submit draw command */
	vkQueueSubmit(app->kmr_vk_queue.queue, 1, &submitInfo, VK_NULL_HANDLE);

	/*
	 * Synchronous wait to ensure DMA-BUF is populated before
	 * submitting associated KMS fbid to DRM core.
	 */
	VkSemaphoreWaitInfo waitInfo;
	waitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	waitInfo.pNext = NULL;
	waitInfo.flags = 0;
	waitInfo.semaphoreCount = ARRAY_LEN(waitSemaphores);
	waitInfo.pSemaphores = waitSemaphores;
	waitInfo.pValues = &signalValue;

	vkWaitSemaphores(app->kmr_vk_lgdev.logicalDevice, &waitInfo, UINT64_MAX);

	*running = prun;
	signalValue++;
}


/*
 * Example code demonstrating how use Vulkan with KMS
 */
int
main (void)
{
	VkExtent2D extent2D;
	VkSurfaceFormatKHR surfaceFormat;

	uint64_t inputReturnCode = 0;
	int kmsfd = -1, inputfd = -1;
	int nfds = -1, epollfd = -1, n;
	enum libinput_event_type eventType;
	struct libinput *input = NULL;
	struct libinput_event *inputEvent = NULL;
	struct libinput_event_keyboard *keyEvent = NULL;
	struct epoll_event event, events[MAX_EPOLL_EVENTS];

	struct app_kms kms;
	struct app_vk app;
	struct kmr_vk_destroy appd;

	struct kmr_input_create_info inputInfo;
	struct kmr_drm_node_handle_drm_event_info drmEventInfo;

	static int fbid = 0;
	static uint8_t cbuf = 0;
	static volatile bool running = true;
	static struct app_vk_kms passData;

	memset(&app, 0, sizeof(app));
	memset(&appd, 0, sizeof(appd));
	memset(&kms, 0, sizeof(kms));

	passData.app_kms = &kms;
	passData.app_vk = &app;

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

	if (create_vk_instance(&app) == -1)
		goto exit_error;

	if (create_kms_instance(&kms) == -1)
		goto exit_error;

	if (create_kms_gbm_buffers(&kms) == -1)
		goto exit_error;

	if (create_kms_set_crtc(&kms) == -1)
		goto exit_error;

	extent2D.width = kms.kmr_drm_node_display->width;
	extent2D.height = kms.kmr_drm_node_display->height;

	/*
	 * Create Vulkan Physical Device Handle, After Window Surface
	 * so that it doesn't effect VkPhysicalDevice selection
	 */
	if (create_vk_device(&app, &kms) == -1)
		goto exit_error;

	if (create_vk_swapchain_images(&app, &kms, &surfaceFormat) == -1)
		goto exit_error;

	if (create_vk_shader_modules(&app) == -1)
		goto exit_error;

	if (create_vk_graphics_pipeline(&app, &surfaceFormat, extent2D) == -1)
		goto exit_error;

	if (create_vk_framebuffers(&app, extent2D) == -1)
		goto exit_error;

	if (create_vk_command_buffers(&app) == -1)
		goto exit_error;

	if (create_vk_sync_objs(&app) == -1)
		goto exit_error;

	if (create_vk_buffers(&app) == -1)
		goto exit_error;

	if (create_kms_atomic_request_instance(&passData, &cbuf, &fbid, &running) == -1)
		goto exit_error;

#ifdef INCLUDE_LIBSEAT
	inputInfo.session = kms.kmr_session;
#endif
	kms.kmr_input = kmr_input_create(&inputInfo);
	if (!kms.kmr_input)
		goto exit_error;

	input = kms.kmr_input->inputInst;
	inputfd = kms.kmr_input->inputfd;
	kmsfd = kms.kmr_drm_node->kmsfd;
	drmEventInfo.kmsfd = kmsfd;

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

	while (running) {
		nfds = epoll_wait(epollfd, events, MAX_EPOLL_EVENTS, -1);
		if (nfds == -1) {
			kmr_utils_log(KMR_DANGER, "[x] epoll_wait: %s", strerror(errno));
			goto exit_error;
		}

		for (n = 0; n < nfds; n++) {
			if (events[n].data.fd == inputfd) {
				libinput_dispatch(input);

				inputEvent = libinput_get_event(input);
				if (!inputEvent)
					continue;

				eventType = libinput_event_get_type(inputEvent);

				switch (eventType) {
					case LIBINPUT_EVENT_KEYBOARD_KEY:
						keyEvent = libinput_event_get_keyboard_event(inputEvent);
						inputReturnCode = libinput_event_keyboard_get_key(keyEvent);
						break;
					default:
						break;
				}

				libinput_event_destroy(inputEvent);
				libinput_dispatch(input);

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
	unsigned int destroyLoop;

	/*
	 * Let the api know of what addresses to free and fd's to close
	 */
	appd.kmr_vk_lgdev_cnt = 1;
	appd.kmr_vk_lgdev = &app.kmr_vk_lgdev;
	appd.kmr_vk_image_cnt = 1;
	appd.kmr_vk_image = &app.kmr_vk_image;
	appd.kmr_vk_shader_module_cnt = ARRAY_LEN(app.kmr_vk_shader_module);
	appd.kmr_vk_shader_module = app.kmr_vk_shader_module;
	appd.kmr_vk_pipeline_layout_cnt = 1;
	appd.kmr_vk_pipeline_layout = &app.kmr_vk_pipeline_layout;
	appd.kmr_vk_render_pass_cnt = 1;
	appd.kmr_vk_render_pass = &app.kmr_vk_render_pass;
	appd.kmr_vk_graphics_pipeline_cnt = 1;
	appd.kmr_vk_graphics_pipeline = &app.kmr_vk_graphics_pipeline;
	appd.kmr_vk_framebuffer_cnt = 1;
	appd.kmr_vk_framebuffer = &app.kmr_vk_framebuffer;
	appd.kmr_vk_command_buffer_cnt = 1;
	appd.kmr_vk_command_buffer = &app.kmr_vk_command_buffer;
	appd.kmr_vk_sync_obj_cnt = 1;
	appd.kmr_vk_sync_obj = &app.kmr_vk_sync_obj;
	appd.kmr_vk_buffer_cnt = ARRAY_LEN(app.kmr_vk_buffer);
	appd.kmr_vk_buffer = app.kmr_vk_buffer;
	kmr_vk_destroy(&appd);
	kmr_vk_instance_destroy(app.instance);

	for (destroyLoop = 0; destroyLoop < ARRAY_LEN(kms.kmr_dma_buf_export_sync_file); destroyLoop++)
		kmr_dma_buf_export_sync_file_destroy(kms.kmr_dma_buf_export_sync_file[destroyLoop]);

	kmr_buffer_destroy(kms.kmr_buffer);

	kmr_input_destroy(kms.kmr_input);

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
	gbmBufferInfo.bufferCount = PRECEIVED_SWAPCHAIN_IMAGE_SIZE;
	gbmBufferInfo.width = kms->kmr_drm_node_display->width;
	gbmBufferInfo.height = kms->kmr_drm_node_display->height;
	gbmBufferInfo.bitDepth = 24;
	gbmBufferInfo.bitsPerPixel = 32;
	gbmBufferInfo.gbmBoFlags = GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT | GBM_BO_USE_WRITE;
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
	struct kmr_drm_node_display_mode_info nextImageInfo;

	for (uint8_t i = 0; i < kms->kmr_buffer->bufferCount; i++) {
		nextImageInfo.fbid = kms->kmr_buffer->bufferObjects[i].fbid;
		nextImageInfo.display = kms->kmr_drm_node_display;
		if (kmr_drm_node_display_mode_set(&nextImageInfo))
			return -1;
	}

	return 0;
}


static int
create_kms_atomic_request_instance (struct app_vk_kms *passData,
                                    uint8_t *cbuf,
                                    int *fbid,
                                    volatile bool *running)
{
	struct app_kms *kms = passData->app_kms;

	*fbid = kms->kmr_buffer->bufferObjects[*cbuf].fbid;

	struct kmr_drm_node_atomic_request_create_info atomicRequestInfo;
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

	struct kmr_vk_instance_create_info instanceCreateInfo;
	instanceCreateInfo.appName = "Triangle Example App";
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
create_vk_device (struct app_vk *app, struct app_kms *kms)
{
	const char *deviceExtensions[] = {
		"VK_KHR_timeline_semaphore",
		"VK_KHR_external_memory_fd",
		"VK_KHR_external_semaphore_fd",
		"VK_KHR_synchronization2",
		"VK_KHR_image_format_list",
		"VK_EXT_external_memory_dma_buf",
		"VK_EXT_queue_family_foreign",
		"VK_EXT_image_drm_format_modifier",
	};

	struct kmr_vk_phdev_create_info phdevCreateInfo;
	phdevCreateInfo.instance = app->instance;
	phdevCreateInfo.deviceType = VK_PHYSICAL_DEVICE_TYPE;
	phdevCreateInfo.kmsfd = kms->kmr_drm_node->kmsfd;

	app->kmr_vk_phdev = kmr_vk_phdev_create(&phdevCreateInfo);
	if (!app->kmr_vk_phdev.physDevice)
		return -1;

	struct kmr_vk_queue_create_info queueCreateInfo;
	queueCreateInfo.physDevice = app->kmr_vk_phdev.physDevice;
	queueCreateInfo.queueFlag = VK_QUEUE_GRAPHICS_BIT;

	app->kmr_vk_queue = kmr_vk_queue_create(&queueCreateInfo);
	if (app->kmr_vk_queue.familyIndex == -1)
		return -1;

	/*
	 * Can Hardset features prior
	 * https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceFeatures.html
	 * app->kmr_vk_phdev.physDeviceFeatures.depthBiasClamp = VK_TRUE;
	 */
	struct kmr_vk_lgdev_create_info lgdevCreateInfo;
	lgdevCreateInfo.instance = app->instance;
	lgdevCreateInfo.physDevice = app->kmr_vk_phdev.physDevice;
	lgdevCreateInfo.enabledFeatures = &app->kmr_vk_phdev.physDeviceFeatures;
	lgdevCreateInfo.enabledExtensionCount = ARRAY_LEN(deviceExtensions);
	lgdevCreateInfo.enabledExtensionNames = deviceExtensions;
	lgdevCreateInfo.queueCount = 1;
	lgdevCreateInfo.queues = &app->kmr_vk_queue;

	app->kmr_vk_lgdev = kmr_vk_lgdev_create(&lgdevCreateInfo);
	if (!app->kmr_vk_lgdev.logicalDevice)
		return -1;

	return 0;
}


static int
create_vk_swapchain_images (struct app_vk *app,
                            struct app_kms *kms,
                            VkSurfaceFormatKHR *surfaceFormat)
{
	uint16_t width, height;
	struct kmr_buffer *bufferHandle = kms->kmr_buffer;
	struct kmr_drm_node_display *display = kms->kmr_drm_node_display;

	uint8_t curImage, plane, imageCount = bufferHandle->bufferCount;
	VkSubresourceLayout *imageDmaBufferResourceInfos = NULL;
	uint32_t *imageDmaBufferMemTypeBits = NULL;

	width = display->width;
	height = display->height;
	surfaceFormat->format = kmr_pixel_format_convert_name(KMR_PIXEL_FORMAT_CONV_GBM_TO_VK, bufferHandle->bufferObjects[0].format);
	if (surfaceFormat->format == UINT32_MAX)
		return -1;

	kmr_utils_log(KMR_SUCCESS, "Vulkan format %s", kmr_pixel_format_get_name(KMR_PIXEL_FORMAT_VK, surfaceFormat->format));

	struct kmr_vk_image_view_create_info imageViewCreateInfos[imageCount];
	struct kmr_vk_vimage_create_info imageCreateInfos[imageCount];
	for (curImage = 0; curImage < imageCount; curImage++) {
		imageViewCreateInfos[curImage].imageViewflags = 0;
		imageViewCreateInfos[curImage].imageViewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfos[curImage].imageViewFormat = surfaceFormat->format;
		imageViewCreateInfos[curImage].imageViewComponents = (VkComponentMapping) { .r = 0, .g = 0, .b = 0, .a = 0 };
		// Which aspect of image to view (i.e VK_IMAGE_ASPECT_COLOR_BIT view color)
		imageViewCreateInfos[curImage].imageViewSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// Start mipmap level to view from (https://en.wikipedia.org/wiki/Mipmap)
		imageViewCreateInfos[curImage].imageViewSubresourceRange.baseMipLevel = 0;
		imageViewCreateInfos[curImage].imageViewSubresourceRange.levelCount = 1;      // Number of mipmap levels to view
		imageViewCreateInfos[curImage].imageViewSubresourceRange.baseArrayLayer = 0;  // Start array level to view from
		imageViewCreateInfos[curImage].imageViewSubresourceRange.layerCount = 1;      // Number of array levels to view

		imageCreateInfos[curImage].imageflags = 0;
		imageCreateInfos[curImage].imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfos[curImage].imageFormat = surfaceFormat->format;
		imageCreateInfos[curImage].imageExtent3D = (VkExtent3D) { .width = width, .height = height, .depth = 1 };
		imageCreateInfos[curImage].imageMipLevels = 1;
		imageCreateInfos[curImage].imageArrayLayers = 1;
		imageCreateInfos[curImage].imageSamples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfos[curImage].imageTiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
		imageCreateInfos[curImage].imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		imageCreateInfos[curImage].imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfos[curImage].imageQueueFamilyIndexCount = 0;
		imageCreateInfos[curImage].imageQueueFamilyIndices = NULL;
		imageCreateInfos[curImage].imageInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfos[curImage].imageDmaBufferFormatModifier = bufferHandle->bufferObjects[curImage].modifier;
		imageCreateInfos[curImage].imageDmaBufferCount = bufferHandle->bufferObjects[curImage].planeCount;
		imageCreateInfos[curImage].imageDmaBufferFds = bufferHandle->bufferObjects[curImage].dmaBufferFds;

		imageDmaBufferMemTypeBits = alloca(imageCreateInfos[curImage].imageDmaBufferCount * sizeof(uint32_t));
		imageDmaBufferResourceInfos = alloca(imageCreateInfos[curImage].imageDmaBufferCount * sizeof(VkSubresourceLayout));

		for (plane = 0; plane < imageCreateInfos[curImage].imageDmaBufferCount; plane++) {
			imageDmaBufferMemTypeBits[plane] = \
				kmr_vk_get_external_fd_memory_properties(app->kmr_vk_lgdev.logicalDevice,
				                                         bufferHandle->bufferObjects[curImage].dmaBufferFds[plane],
				                                         VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT);

			imageDmaBufferResourceInfos[plane].offset = bufferHandle->bufferObjects[curImage].offsets[plane];
			imageDmaBufferResourceInfos[plane].rowPitch = bufferHandle->bufferObjects[curImage].pitches[plane];
			imageDmaBufferResourceInfos[plane].size = 0;
			imageDmaBufferResourceInfos[plane].arrayPitch = 0;
			imageDmaBufferResourceInfos[plane].depthPitch = 0;
		}

		imageCreateInfos[curImage].imageDmaBufferResourceInfo = imageDmaBufferResourceInfos;
		imageCreateInfos[curImage].imageDmaBufferMemTypeBits = imageDmaBufferMemTypeBits;
	}

	struct kmr_vk_image_create_info swapchainImagesInfo;
	swapchainImagesInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	swapchainImagesInfo.swapchain = VK_NULL_HANDLE;
	swapchainImagesInfo.imageCount = imageCount;
	swapchainImagesInfo.imageViewCreateInfos = imageViewCreateInfos;
	swapchainImagesInfo.imageCreateInfos = imageCreateInfos;
	swapchainImagesInfo.physDevice = app->kmr_vk_phdev.physDevice;
	swapchainImagesInfo.memPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	swapchainImagesInfo.useExternalDmaBuffer = true;

	app->kmr_vk_image = kmr_vk_image_create(&swapchainImagesInfo);
	if (!app->kmr_vk_image.imageViewHandles[0].view)
		return -1;

	return 0;
}


static int
create_vk_shader_modules (struct app_vk *app)
{
	int ret = 0;

	uint32_t currentShader;

	struct kmr_vk_shader_module_create_info shaderModuleCreateInfo;

	const char *shaderModuleNames[] = {
		"vertex", "fragment"
	};

#ifdef INCLUDE_SHADERC
	const char vertexShader[] =
		"#version 450\n"// GLSL 4.5
		"#extension GL_ARB_separate_shader_objects : enable\n"
		"layout(location = 0) out vec3 outColor;\n\n"
		"layout(location = 0) in vec2 inPosition;\n"
		"layout(location = 1) in vec3 inColor;\n"
		"void main() {\n"
		"	gl_Position = vec4(inPosition, 0.0, 1.0);\n"
		"	outColor = inColor;\n"
		"}";

	const char fragmentShader[] =
		"#version 450\n"
		"#extension GL_ARB_separate_shader_objects : enable\n"
		"layout(location = 0) in vec3 inColor;\n"
		"layout(location = 0) out vec4 outColor;\n"
		"void main() { outColor = vec4(inColor, 1.0); }";

	struct kmr_shader_spirv_create_info vertexShaderCreateInfo;
	vertexShaderCreateInfo.kind = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderCreateInfo.source = vertexShader;
	vertexShaderCreateInfo.filename = "vert.spv";
	vertexShaderCreateInfo.entryPoint = "main";

	struct kmr_shader_spirv_create_info fragmentShaderCreateInfo;
	fragmentShaderCreateInfo.kind = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderCreateInfo.source = fragmentShader;
	fragmentShaderCreateInfo.filename = "frag.spv";
	fragmentShaderCreateInfo.entryPoint = "main";

	/*
	 * 0. Vertex Shader
	 * 1. Fragment Shader
	 */
	struct kmr_shader_spirv *kmr_shader[2];
	memset(kmr_shader, 0, sizeof(kmr_shader));

	kmr_shader[0] = kmr_shader_spirv_create(&vertexShaderCreateInfo);
	if (!kmr_shader[0]) { ret = -1 ; goto exit_destroy_shader ; }

	kmr_shader[1] = kmr_shader_spirv_create(&fragmentShaderCreateInfo);
	if (!kmr_shader[1]) { ret = -1 ; goto exit_destroy_shader ; }

#else
	/*
	 * 0. Vertex Shader
	 * 1. Fragment Shader
	 */
	struct kmr_utils_file kmr_shader[2];
	memset(kmr_shader, 0, sizeof(kmr_shader));

	kmr_shader[0] = kmr_utils_file_load(VERTEX_SHADER_SPIRV);
	if (!kmr_shader[0].bytes) { ret = -1 ; goto exit_destroy_shader ; }

	kmr_shader[1] = kmr_utils_file_load(FRAGMENT_SHADER_SPIRV);
	if (!kmr_shader[1].bytes) { ret = -1 ; goto exit_destroy_shader ; }

#endif

	for (currentShader = 0; currentShader < ARRAY_LEN(kmr_shader); currentShader++) {
		shaderModuleCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
#ifdef INCLUDE_SHADERC
		shaderModuleCreateInfo.sprivByteSize = kmr_shader[currentShader]->byteSize;
		shaderModuleCreateInfo.sprivBytes = kmr_shader[currentShader]->bytes;
#else
		shaderModuleCreateInfo.sprivByteSize = kmr_shader[currentShader].byteSize;
		shaderModuleCreateInfo.sprivBytes = kmr_shader[currentShader].bytes;
#endif
		shaderModuleCreateInfo.shaderName = shaderModuleNames[currentShader];

		app->kmr_vk_shader_module[currentShader] = kmr_vk_shader_module_create(&shaderModuleCreateInfo);
		if (!app->kmr_vk_shader_module[currentShader].shaderModule) { ret = -1 ; goto exit_destroy_shader ; }
	}

exit_destroy_shader:
	for (currentShader = 0; currentShader < ARRAY_LEN(kmr_shader); currentShader++) {
#ifdef INCLUDE_SHADERC
		kmr_shader_spirv_destroy(kmr_shader[currentShader]);
#else
		free(kmr_shader[currentShader].bytes);
#endif
	}
	return ret;
}


static int
create_vk_buffers (struct app_vk *app)
{
	uint8_t cpuVisibleBuffer = 0, gpuVisibleBuffer = 1;

	// Create CPU visible vertex buffer
	struct kmr_vk_buffer_create_info vkVertexBufferCreateInfo;
	vkVertexBufferCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	vkVertexBufferCreateInfo.physDevice = app->kmr_vk_phdev.physDevice;
	vkVertexBufferCreateInfo.bufferFlags = 0;
	vkVertexBufferCreateInfo.bufferSize = sizeof(meshData);
	vkVertexBufferCreateInfo.bufferUsage = (VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU || \
	                                        VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_CPU) ? \
	                                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkVertexBufferCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vkVertexBufferCreateInfo.queueFamilyIndexCount = 0;
	vkVertexBufferCreateInfo.queueFamilyIndices = NULL;
	vkVertexBufferCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	app->kmr_vk_buffer[cpuVisibleBuffer] = kmr_vk_buffer_create(&vkVertexBufferCreateInfo);
	if (!app->kmr_vk_buffer[cpuVisibleBuffer].buffer || !app->kmr_vk_buffer[0].deviceMemory)
		return -1;

	// Copy vertex data into CPU visible vertex
	struct kmr_vk_memory_map_info deviceMemoryCopyInfo;
	deviceMemoryCopyInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	deviceMemoryCopyInfo.deviceMemory = app->kmr_vk_buffer[cpuVisibleBuffer].deviceMemory;
	deviceMemoryCopyInfo.deviceMemoryOffset = 0;
	deviceMemoryCopyInfo.memoryBufferSize = vkVertexBufferCreateInfo.bufferSize;
	deviceMemoryCopyInfo.bufferData = meshData;
	kmr_vk_memory_map(&deviceMemoryCopyInfo);

	if (VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		// Create GPU visible vertex buffer
		struct kmr_vk_buffer_create_info vkVertexBufferGPUCreateInfo;
		vkVertexBufferGPUCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
		vkVertexBufferGPUCreateInfo.physDevice = app->kmr_vk_phdev.physDevice;
		vkVertexBufferGPUCreateInfo.bufferFlags = 0;
		vkVertexBufferGPUCreateInfo.bufferSize = vkVertexBufferCreateInfo.bufferSize;
		vkVertexBufferGPUCreateInfo.bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		vkVertexBufferGPUCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vkVertexBufferGPUCreateInfo.queueFamilyIndexCount = 0;
		vkVertexBufferGPUCreateInfo.queueFamilyIndices = NULL;
		vkVertexBufferGPUCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		app->kmr_vk_buffer[gpuVisibleBuffer] = kmr_vk_buffer_create(&vkVertexBufferGPUCreateInfo);
		if (!app->kmr_vk_buffer[gpuVisibleBuffer].buffer || !app->kmr_vk_buffer[1].deviceMemory)
			return -1;

		VkBufferCopy copyRegion;
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = vkVertexBufferCreateInfo.bufferSize;

		struct kmr_vk_resource_copy_buffer_to_buffer_info bufferToBufferCopyInfo;
		bufferToBufferCopyInfo.copyRegion = &copyRegion;

		// Copy contents from CPU visible buffer over to GPU visible vertex buffer
		struct kmr_vk_resource_copy_info bufferCopyInfo;
		bufferCopyInfo.resourceCopyType = KMR_VK_RESOURCE_COPY_VK_BUFFER_TO_VK_BUFFER;
		bufferCopyInfo.resourceCopyInfo = &bufferToBufferCopyInfo;
		bufferCopyInfo.commandBuffer = app->kmr_vk_command_buffer.commandBufferHandles[0].commandBuffer;
		bufferCopyInfo.queue = app->kmr_vk_queue.queue;
		bufferCopyInfo.srcResource = app->kmr_vk_buffer[cpuVisibleBuffer].buffer;
		bufferCopyInfo.dstResource = app->kmr_vk_buffer[gpuVisibleBuffer].buffer;

		if (kmr_vk_resource_copy(&bufferCopyInfo) == -1)
			return -1;
	}

	return 0;
}


static int
create_vk_graphics_pipeline (struct app_vk *app,
                             VkSurfaceFormatKHR *surfaceFormat,
                             VkExtent2D extent2D)
{
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = app->kmr_vk_shader_module[0].shaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = app->kmr_vk_shader_module[1].shaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[2] = {vertShaderStageInfo, fragShaderStageInfo};

	VkVertexInputBindingDescription vertexInputBindingDescription = {};
	vertexInputBindingDescription.binding = 0;
	vertexInputBindingDescription.stride = sizeof(struct app_vertex_data); // Number of bytes from one entry to another
	vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Move to next data entry after each vertex

	// Two incomming vertex atributes in the vertex shader.
	VkVertexInputAttributeDescription vertexAttributeDescriptions[2];

	// position attribute
	vertexAttributeDescriptions[0].location = 0;
	vertexAttributeDescriptions[0].binding = 0;
	// defines the byte size of attribute data
	vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // vec2 - RG color channels match size of vec2
	// specifies the byte where struct app_vertex_data member vec2 pos is located
	vertexAttributeDescriptions[0].offset = offsetof(struct app_vertex_data, pos);

	// color attribute
	vertexAttributeDescriptions[1].location = 1;
	vertexAttributeDescriptions[1].binding = 0;
	vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 - RGB color channels match size of vec3
	// specifies the byte where struct app_vertex_data member vec3 color is located
	vertexAttributeDescriptions[1].offset = offsetof(struct app_vertex_data, color);

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.pVertexBindingDescriptions = &vertexInputBindingDescription;
	vertexInputInfo.vertexAttributeDescriptionCount = ARRAY_LEN(vertexAttributeDescriptions);
	vertexInputInfo.pVertexAttributeDescriptions = vertexAttributeDescriptions;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) extent2D.width;
	viewport.height = (float) extent2D.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = extent2D;

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = NULL;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;

	/*
	 * Blending uses equation (srcColorBlendFactor * new color) colorBlendOp (dstColorBlendFactor * old color)
	 * (VK_BLEND_FACTOR_SRC_ALPHA * new color) VK_BLEND_OP_ADD (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old color)
	 * (new color alpha channel * new color) + ((1 - new color alpha channel) * old color)
	 */
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // multiply new color by its alpha value
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	// (1 * new color alpha channel) + (0 * old color alpha channel) = new alpha
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkDynamicState dynamicStates[2];
	dynamicStates[0] = VK_DYNAMIC_STATE_VIEWPORT;
	dynamicStates[1] = VK_DYNAMIC_STATE_SCISSOR;

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = ARRAY_LEN(dynamicStates);
	dynamicState.pDynamicStates = dynamicStates;

	struct kmr_vk_pipeline_layout_create_info graphicsPipelineLayoutCreateInfo;
	graphicsPipelineLayoutCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	graphicsPipelineLayoutCreateInfo.descriptorSetLayoutCount = 0;
	graphicsPipelineLayoutCreateInfo.descriptorSetLayouts = NULL;
	graphicsPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	graphicsPipelineLayoutCreateInfo.pushConstantRanges = NULL;

	app->kmr_vk_pipeline_layout = kmr_vk_pipeline_layout_create(&graphicsPipelineLayoutCreateInfo);
	if (!app->kmr_vk_pipeline_layout.pipelineLayout)
		return -1;

	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = surfaceFormat->format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency subpassDependencies[2];
	// Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[0].dstSubpass = 0;
	subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
	subpassDependencies[1].srcSubpass = 0;
	subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	struct kmr_vk_render_pass_create_info renderPassInfo;
	renderPassInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	renderPassInfo.attachmentDescriptionCount = 1;
	renderPassInfo.attachmentDescriptions = &colorAttachment;
	renderPassInfo.subpassDescriptionCount = 1;
	renderPassInfo.subpassDescriptions = &subpass;
	renderPassInfo.subpassDependencyCount = ARRAY_LEN(subpassDependencies);
	renderPassInfo.subpassDependencies = subpassDependencies;

	app->kmr_vk_render_pass = kmr_vk_render_pass_create(&renderPassInfo);
	if (!app->kmr_vk_render_pass.renderPass)
		return -1;

	struct kmr_vk_graphics_pipeline_create_info graphicsPipelineInfo;
	graphicsPipelineInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	graphicsPipelineInfo.shaderStageCount = ARRAY_LEN(shaderStages);
	graphicsPipelineInfo.shaderStages = shaderStages;
	graphicsPipelineInfo.vertexInputState = &vertexInputInfo;
	graphicsPipelineInfo.inputAssemblyState = &inputAssembly;
	graphicsPipelineInfo.tessellationState = NULL;
	graphicsPipelineInfo.viewportState = &viewportState;
	graphicsPipelineInfo.rasterizationState = &rasterizer;
	graphicsPipelineInfo.multisampleState = &multisampling;
	graphicsPipelineInfo.depthStencilState = NULL;
	graphicsPipelineInfo.colorBlendState = &colorBlending;
	graphicsPipelineInfo.dynamicState = &dynamicState;
	graphicsPipelineInfo.pipelineLayout = app->kmr_vk_pipeline_layout.pipelineLayout;
	graphicsPipelineInfo.renderPass = app->kmr_vk_render_pass.renderPass;
	graphicsPipelineInfo.subpass = 0;

	app->kmr_vk_graphics_pipeline = kmr_vk_graphics_pipeline_create(&graphicsPipelineInfo);
	if (!app->kmr_vk_graphics_pipeline.graphicsPipeline)
		return -1;

	return 0;
}


static int
create_vk_framebuffers (struct app_vk *app, VkExtent2D extent2D)
{
	uint8_t framebufferCount, i;

	struct kmr_vk_framebuffer_create_info framebufferInfo;
	struct kmr_vk_framebuffer_images *framebufferImages = NULL;

	framebufferCount = app->kmr_vk_image.imageCount;
	framebufferImages = alloca(framebufferCount * sizeof(struct kmr_vk_framebuffer_images));
	for (i = 0; i < framebufferCount; i++) {
		// VkImageView->VkImage for color image attachment
		framebufferImages[i].imageAttachments[0] = app->kmr_vk_image.imageViewHandles[i].view;
	}

	framebufferInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	framebufferInfo.framebufferCount = framebufferCount;     // Amount of framebuffers to create
	framebufferInfo.framebufferImageAttachmentCount = 1;
	framebufferInfo.framebufferImages = framebufferImages;   // image attachments per framebuffer
	framebufferInfo.renderPass = app->kmr_vk_render_pass.renderPass;
	framebufferInfo.width = extent2D.width;
	framebufferInfo.height = extent2D.height;
	framebufferInfo.layers = 1;

	app->kmr_vk_framebuffer = kmr_vk_framebuffer_create(&framebufferInfo);
	if (!app->kmr_vk_framebuffer.framebufferHandles[0].framebuffer)
		return -1;

	return 0;
}


static int
create_vk_command_buffers (struct app_vk *app)
{
	struct kmr_vk_command_buffer_create_info commandBufferCreateInfo;
	commandBufferCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	commandBufferCreateInfo.queueFamilyIndex = app->kmr_vk_queue.familyIndex;
	commandBufferCreateInfo.commandBufferCount = 1;

	app->kmr_vk_command_buffer = kmr_vk_command_buffer_create(&commandBufferCreateInfo);
	if (!app->kmr_vk_command_buffer.commandPool)
		return -1;

	return 0;
}


static int
create_vk_sync_objs (struct app_vk *app)
{
	struct kmr_vk_sync_obj_create_info syncObjsCreateInfo;
	syncObjsCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	syncObjsCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	syncObjsCreateInfo.semaphoreCount = 1;
	syncObjsCreateInfo.fenceCount = 0;

	app->kmr_vk_sync_obj = kmr_vk_sync_obj_create(&syncObjsCreateInfo);
	if (!app->kmr_vk_sync_obj.semaphoreHandles[0].semaphore)
		return -1;

/*
	struct kmr_buffer bufferHandle = kms->kmr_buffer;
	struct kmr_dma_buf_export_sync_file_create_info syncFileCreateInfo;
	struct kmr_vk_sync_obj_import_external_sync_fd_info importSyncFileInfo;

	for (uint8_t b = 0; b < bufferHandle.bufferCount; b++) {
		syncFileCreateInfo.dmaBufferFdsCount = bufferHandle.bufferObjects[b].planeCount;
		syncFileCreateInfo.dmaBufferFds = bufferHandle.bufferObjects[b].dmaBufferFds;
		syncFileCreateInfo.syncFlags = KMR_DMA_BUF_SYNC_RW;

		kms->kmr_dma_buf_export_sync_file[b] = kmr_dma_buf_export_sync_file_create(&syncFileCreateInfo);
		if (!kms->kmr_dma_buf_export_sync_file[b].syncFileFds)
			return -1;

		importSyncFileInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
		importSyncFileInfo.syncFd = kms->kmr_dma_buf_export_sync_file[b].syncFileFds[0];
		importSyncFileInfo.syncType = KMR_VK_SYNC_OBJ_SEMAPHORE;
		importSyncFileInfo.syncHandle.semaphore = app->kmr_vk_sync_obj[0].semaphoreHandles[0].semaphore;
		if (kmr_vk_sync_obj_import_external_sync_fd(&importSyncFileInfo) == -1)
			return -1;
	}
*/

	return 0;
}


static int
record_vk_draw_commands (struct app_vk *app,
                         uint32_t swapchainImageIndex,
                         VkExtent2D extent2D)
{
	struct kmr_vk_command_buffer_record_info commandBufferRecordInfo;
	commandBufferRecordInfo.commandBufferCount = app->kmr_vk_command_buffer.commandBufferCount;
	commandBufferRecordInfo.commandBufferHandles = app->kmr_vk_command_buffer.commandBufferHandles;
	commandBufferRecordInfo.commandBufferUsageflags = 0;

	if (kmr_vk_command_buffer_record_begin(&commandBufferRecordInfo) == -1)
		return -1;

	VkCommandBuffer cmdBuffer = app->kmr_vk_command_buffer.commandBufferHandles[0].commandBuffer;

	VkRect2D renderArea = {};
	renderArea.offset.x = 0;
	renderArea.offset.y = 0;
	renderArea.extent = extent2D;

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) extent2D.width;
	viewport.height = (float) extent2D.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	/*
	 * Values used by VK_ATTACHMENT_LOAD_OP_CLEAR to reset
	 * framebuffer attachments with.
	 */
	float float32[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	int32_t int32[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	uint32_t uint32[4] = {0.0f, 0.0f, 0.0f, 1.0f};

	VkClearValue clearColor[1];
	memcpy(clearColor[0].color.float32, float32, ARRAY_LEN(float32));
	memcpy(clearColor[0].color.int32, int32, ARRAY_LEN(int32));
	memcpy(clearColor[0].color.int32, uint32, ARRAY_LEN(uint32));
	clearColor[0].depthStencil.depth = 0.0f;
	clearColor[0].depthStencil.stencil = 0;

	VkRenderPassBeginInfo renderPassInfo;
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.pNext = NULL;
	renderPassInfo.renderPass = app->kmr_vk_render_pass.renderPass;
	renderPassInfo.framebuffer = app->kmr_vk_framebuffer.framebufferHandles[swapchainImageIndex].framebuffer;
	renderPassInfo.renderArea = renderArea;
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = clearColor;

	/*
	 * 0. CPU visible vertex buffer
	 * 1. GPU visible vertex buffer
	 */
	VkBuffer vertexBuffer = app->kmr_vk_buffer[(VK_PHYSICAL_DEVICE_TYPE != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? 0 : 1].buffer;
	VkDeviceSize offsets[] = {0};

	vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->kmr_vk_graphics_pipeline.graphicsPipeline);
	vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
	vkCmdSetScissor(cmdBuffer, 0, 1, &renderArea);
	vkCmdDraw(cmdBuffer, ARRAY_LEN(meshData), 1, 0, 0);
	vkCmdEndRenderPass(cmdBuffer);

	if (kmr_vk_command_buffer_record_end(&commandBufferRecordInfo) == -1)
		return -1;

	return 0;
}
