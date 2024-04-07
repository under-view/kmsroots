#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h> // For offsetof(3)
#include <errno.h>
#include <signal.h>

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/cglm.h>

#include "xclient.h"
#include "vulkan.h"
#include "shader.h"
#include "gltf-loader.h"

//#define WIDTH 3840
//#define HEIGHT 2160
#define WIDTH 1920
#define HEIGHT 1080
#define PRECEIVED_SWAPCHAIN_IMAGE_SIZE 5

/***************************
 * Structs used by example *
 ***************************/

struct app_vk {
	VkInstance instance;
	struct kmr_vk_phdev kmr_vk_phdev;
	struct kmr_vk_lgdev kmr_vk_lgdev;
	struct kmr_vk_queue kmr_vk_queue;

	VkSurfaceKHR surface;
	struct kmr_vk_swapchain kmr_vk_swapchain;

	/*
	 * 0. Swapchain Images
	 * 1. Depth Image
	 * 2. Texture Images
	 */
	struct kmr_vk_image kmr_vk_image[3];

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
	 * 0. CPU visible vertex buffer that stores: (index + vertices) [Used as primary buffer if physical device CPU/INTEGRATED]
	 * 1. GPU visible vertex buffer that stores: (index + vertices)
	 * 2. CPU visible uniform buffer
	 * 3. CPU visible transfer buffer (stores image pixel data)
	 */
	struct kmr_vk_buffer kmr_vk_buffer[4];

	struct kmr_vk_descriptor_set_layout kmr_vk_descriptor_set_layout;
	struct kmr_vk_descriptor_set kmr_vk_descriptor_set;

	/*
	 * Texture samplers
	 */
	struct kmr_vk_sampler kmr_vk_sampler;

	/*
	 * Keep track of data related to FlightHelmet.gltf
	 */
	struct kmr_gltf_loader_mesh *kmr_gltf_loader_mesh;
	struct kmr_gltf_loader_texture_image *kmr_gltf_loader_texture_image;
	struct kmr_gltf_loader_material *kmr_gltf_loader_material;

	/*
	 * Other required data needed for draw operations
	 */
	uint32_t indexBufferOffset;
	struct kmr_utils_aligned_buffer modelTransferSpace;

	// A primitive contains the data for a single draw call
	uint32_t meshCount;
	struct mesh_data {
		mat4 matrix;
		uint32_t firstIndex;
		uint32_t indexCount;
		uint32_t bufferOffset; // Offset in VkBuffer. VkBuffer contains struct app_vertex_data data.
	} *meshData;
};


struct app_vk_xcb {
	struct kmr_xcb_window *kmr_xcb_window;
	struct app_vk *app_vk;
};


// Order inside FlightHelmet.bin
struct app_vertex_data {
	vec3 pos;
	vec3 normal;
	vec2 texCoord;
	vec3 color;
};


struct app_uniform_buffer_scene_model {
	mat4 model;
	int textureIndex;
};


struct app_uniform_buffer_scene {
	mat4 projection;
	mat4 view;
	vec4 lightPosition;
	vec4 viewPosition;
};


/***********
 * Globals *
 ***********/

static volatile sig_atomic_t prun = 1;


/***********************
 * Function Prototypes *
 ***********************/

static int
create_xcb_vk_surface (struct app_vk *app,
                       struct kmr_xcb_window **xc);

static int
create_vk_instance (struct app_vk *app);

static int
create_vk_device (struct app_vk *app);

static int
create_vk_swapchain (struct app_vk *app,
                     VkSurfaceFormatKHR *surfaceFormat,
                     VkExtent2D extent2D);

static int
create_vk_swapchain_images (struct app_vk *app,
                            VkSurfaceFormatKHR *surfaceFormat);

static VkFormat
choose_depth_image_format (struct app_vk *app,
                           VkImageTiling imageTiling,
                           VkFormatFeatureFlags formatFeatureFlags);

static int
create_vk_depth_image (struct app_vk *app);

static int
create_vk_shader_modules (struct app_vk *app);

static int
create_vk_command_buffers (struct app_vk *app);

static int
create_gltf_load_required_data (struct app_vk *app);

static int
create_vk_buffers (struct app_vk *app);

static int
create_vk_texture_images (struct app_vk *app);

static int
create_vk_image_sampler (struct app_vk *app);

static int
create_vk_resource_descriptor_sets (struct app_vk *app);

static int
create_vk_graphics_pipeline (struct app_vk *app,
                             VkSurfaceFormatKHR *surfaceFormat,
                             VkExtent2D extent2D);
static int
create_vk_framebuffers (struct app_vk *app, VkExtent2D extent2D);

static int
create_vk_sync_objs (struct app_vk *app);

static int
record_vk_draw_commands (struct app_vk *app,
                         uint32_t swapchainImageIndex,
                         VkExtent2D extent2D);

static void
update_uniform_buffer (struct app_vk *app,
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


static void
render (volatile bool *running, uint8_t *imageIndex, void *data)
{
	VkExtent2D extent2D = { .width = WIDTH, .height = HEIGHT };
	struct app_vk_xcb *vkxcb = (struct app_vk_xcb *) data;
	struct app_vk *app = vkxcb->app_vk;
	struct kmr_xcb_window UNUSED *xc = vkxcb->kmr_xcb_window;

	if (!app->kmr_vk_sync_obj.fenceHandles || !app->kmr_vk_sync_obj.semaphoreHandles)
		return;

	VkFence imageFence = app->kmr_vk_sync_obj.fenceHandles[0].fence;
	VkSemaphore imageSemaphore = app->kmr_vk_sync_obj.semaphoreHandles[0].semaphore;
	VkSemaphore renderSemaphore = app->kmr_vk_sync_obj.semaphoreHandles[1].semaphore;

	vkWaitForFences(app->kmr_vk_lgdev.logicalDevice, 1, &imageFence, VK_TRUE, UINT64_MAX);

	vkAcquireNextImageKHR(app->kmr_vk_lgdev.logicalDevice, app->kmr_vk_swapchain.swapchain,
	                      UINT64_MAX, imageSemaphore, VK_NULL_HANDLE, (uint32_t*) imageIndex);

	record_vk_draw_commands(app, *imageIndex, extent2D);
	update_uniform_buffer(app, *imageIndex, extent2D);

	VkSemaphore waitSemaphores[1] = { imageSemaphore };
	VkSemaphore signalSemaphores[1] = { renderSemaphore };
	VkPipelineStageFlags waitStages[1] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo;
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = NULL;
	submitInfo.waitSemaphoreCount = ARRAY_LEN(waitSemaphores);
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &app->kmr_vk_command_buffer.commandBufferHandles[0].commandBuffer;
	submitInfo.signalSemaphoreCount = ARRAY_LEN(signalSemaphores);
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(app->kmr_vk_lgdev.logicalDevice, 1, &imageFence);

	/* Submit draw command */
	vkQueueSubmit(app->kmr_vk_queue.queue, 1, &submitInfo, imageFence);

	VkPresentInfoKHR presentInfo;
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = NULL;
	presentInfo.waitSemaphoreCount = ARRAY_LEN(signalSemaphores);
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &app->kmr_vk_swapchain.swapchain;
	presentInfo.pImageIndices = (uint32_t*) imageIndex;
	presentInfo.pResults = NULL;

	vkQueuePresentKHR(app->kmr_vk_queue.queue, &presentInfo);

	*running = prun;
}


/*
 * Example code demonstrating how use Vulkan with XCB
 */
int
main (void)
{
	VkExtent2D extent2D = { };
	VkSurfaceFormatKHR surfaceFormat;

	struct app_vk app;
	struct kmr_vk_destroy appd;

	struct kmr_xcb_window *xc = NULL;

	struct kmr_xcb_window_handle_event_info eventInfo;

	static uint8_t cbuf = 0;
	static volatile bool running = true;

	static struct app_vk_xcb vkxc;

	memset(&app, 0, sizeof(app));
	memset(&appd, 0, sizeof(appd));

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

	if (create_xcb_vk_surface(&app, &xc) == -1)
		goto exit_error;

	/*
	 * Create Vulkan Physical Device Handle, After Window Surface
	 * so that it doesn't effect VkPhysicalDevice selection
	 */
	if (create_vk_device(&app) == -1)
		goto exit_error;

	extent2D.width = WIDTH;
	extent2D.height = HEIGHT;

	if (create_vk_swapchain(&app, &surfaceFormat, extent2D) == -1)
		goto exit_error;

	if (create_vk_swapchain_images(&app, &surfaceFormat) == -1)
		goto exit_error;

	if (create_vk_depth_image(&app) == -1)
		goto exit_error;

	if (create_vk_shader_modules(&app) == -1)
		goto exit_error;

	if (create_vk_command_buffers(&app) == -1)
		goto exit_error;

	if (create_gltf_load_required_data(&app) == -1)
		goto exit_error;

	if (create_vk_buffers(&app) == -1)
		goto exit_error;

	if (create_vk_texture_images(&app) == -1)
		goto exit_error;

	if (create_vk_image_sampler(&app) == -1)
		goto exit_error;

	if (create_vk_resource_descriptor_sets(&app) == -1)
		goto exit_error;

	if (create_vk_graphics_pipeline(&app, &surfaceFormat, extent2D) == -1)
		goto exit_error;

	if (create_vk_framebuffers(&app, extent2D) == -1)
		goto exit_error;

	if (create_vk_sync_objs(&app) == -1)
		goto exit_error;

	/* Map the window to the screen */
	kmr_xcb_window_make_visible(xc);

	vkxc.kmr_xcb_window = xc;
	vkxc.app_vk = &app;

	eventInfo.xcbWindowObject = xc;
	eventInfo.renderer = render;
	eventInfo.rendererData = &vkxc;
	eventInfo.rendererCurrentBuffer = &cbuf;
	eventInfo.rendererRunning = &running;

	while (kmr_xcb_window_handle_event(&eventInfo) && running) {
		// Initentionally left blank
	}


exit_error:
	free(app.modelTransferSpace.alignedBufferMemory);
	free(app.meshData);

	/*
	 * Let the api know of what addresses to free and fd's to close
	 */
	appd.surface = app.surface;
	appd.kmr_vk_lgdev_cnt = 1;
	appd.kmr_vk_lgdev = &app.kmr_vk_lgdev;
	appd.kmr_vk_swapchain_cnt = 1;
	appd.kmr_vk_swapchain = &app.kmr_vk_swapchain;
	appd.kmr_vk_image_cnt = ARRAY_LEN(app.kmr_vk_image);
	appd.kmr_vk_image = app.kmr_vk_image;
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
	appd.kmr_vk_descriptor_set_layout_cnt = 1;
	appd.kmr_vk_descriptor_set_layout = &app.kmr_vk_descriptor_set_layout;
	appd.kmr_vk_descriptor_set_cnt = 1;
	appd.kmr_vk_descriptor_set = &app.kmr_vk_descriptor_set;
	appd.kmr_vk_sampler_cnt = 1;
	appd.kmr_vk_sampler = &app.kmr_vk_sampler;
	kmr_vk_destroy(&appd);
	kmr_vk_instance_destroy(app.instance);

	kmr_xcb_window_destroy(xc);

	return 0;
}


static int
create_xcb_vk_surface (struct app_vk *app,
                       struct kmr_xcb_window **xc)
{
	/*
	 * Create xcb client
	 */
	struct kmr_xcb_window_create_info xcbWindowCreateInfo;
	xcbWindowCreateInfo.display = NULL;
	xcbWindowCreateInfo.screen = NULL;
	xcbWindowCreateInfo.appName = "GLTF Model Loading Example App";
	xcbWindowCreateInfo.width = WIDTH;
	xcbWindowCreateInfo.height = HEIGHT;
	xcbWindowCreateInfo.fullscreen = false;
	xcbWindowCreateInfo.transparent = false;

	*xc = kmr_xcb_window_create(&xcbWindowCreateInfo);
	if (!(*xc))
		return -1;

	/*
	 * Create Vulkan Surface
	 */
	struct kmr_vk_surface_create_info vkSurfaceCreateInfo;
	vkSurfaceCreateInfo.surfaceType = KMR_SURFACE_XCB_CLIENT;
	vkSurfaceCreateInfo.instance = app->instance;
	vkSurfaceCreateInfo.display = (*xc)->conn;
	vkSurfaceCreateInfo.window = (*xc)->window;

	app->surface = kmr_vk_surface_create(&vkSurfaceCreateInfo);
	if (!app->surface)
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
		"VK_KHR_xcb_surface",
		"VK_KHR_surface",
		"VK_KHR_display",
		"VK_EXT_debug_utils"
	};

	struct kmr_vk_instance_create_info instanceCreateInfo;
	instanceCreateInfo.appName = "GLTF Model Loading Example App";
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
create_vk_device (struct app_vk *app)
{
	const char *deviceExtensions[] = {
		"VK_KHR_swapchain"
	};

	struct kmr_vk_phdev_create_info phdevCreateInfo;
	phdevCreateInfo.instance = app->instance;
	phdevCreateInfo.deviceType = VK_PHYSICAL_DEVICE_TYPE;
#ifdef INCLUDE_KMS
	phdevCreateInfo.kmsfd = -1;
#endif

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
	app->kmr_vk_phdev.physDeviceFeatures.samplerAnisotropy = VK_TRUE;

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


/* choose swap chain surface format & present mode */
static int
create_vk_swapchain (struct app_vk *app,
                     VkSurfaceFormatKHR *surfaceFormat,
                     VkExtent2D extent2D)
{
	uint32_t i;
	VkPresentModeKHR presentMode;

	VkSurfaceCapabilitiesKHR surfaceCapabilities = kmr_vk_get_surface_capabilities(app->kmr_vk_phdev.physDevice, app->surface);
	struct kmr_vk_surface_format surfaceFormats = kmr_vk_get_surface_formats(app->kmr_vk_phdev.physDevice, app->surface);
	struct kmr_vk_surface_present_mode surfacePresentModes = kmr_vk_get_surface_present_modes(app->kmr_vk_phdev.physDevice, app->surface);

	/* Choose surface format based */
	for (i = 0; i < surfaceFormats.surfaceFormatCount; i++) {
		if (surfaceFormats.surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && \
                    surfaceFormats.surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			*surfaceFormat = surfaceFormats.surfaceFormats[i];
		}
	}

	for (i = 0; i < surfacePresentModes.presentModeCount; i++) {
		if (surfacePresentModes.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentMode = surfacePresentModes.presentModes[i];
		}
	}

	free(surfaceFormats.surfaceFormats); surfaceFormats.surfaceFormats = NULL;
	free(surfacePresentModes.presentModes); surfacePresentModes.presentModes = NULL;

	struct kmr_vk_swapchain_create_info swapchainCreateInfo;
	swapchainCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	swapchainCreateInfo.surface = app->surface;
	swapchainCreateInfo.surfaceCapabilities = surfaceCapabilities;
	swapchainCreateInfo.surfaceFormat = *surfaceFormat;
	swapchainCreateInfo.extent2D = extent2D;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.queueFamilyIndexCount = 0;
	swapchainCreateInfo.queueFamilyIndices = NULL;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.oldSwapChain = VK_NULL_HANDLE;

	app->kmr_vk_swapchain = kmr_vk_swapchain_create(&swapchainCreateInfo);
	if (!app->kmr_vk_swapchain.swapchain)
		return -1;

	return 0;
}


static int
create_vk_swapchain_images (struct app_vk *app,
                            VkSurfaceFormatKHR *surfaceFormat)
{
	struct kmr_vk_image_view_create_info imageViewCreateInfo;
	imageViewCreateInfo.imageViewflags = 0;
	imageViewCreateInfo.imageViewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.imageViewFormat = surfaceFormat->format;
	imageViewCreateInfo.imageViewComponents.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.imageViewComponents.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.imageViewComponents.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.imageViewComponents.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.imageViewSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // Which aspect of image to view (i.e VK_IMAGE_ASPECT_COLOR_BIT view color)
	imageViewCreateInfo.imageViewSubresourceRange.baseMipLevel = 0;                        // Start mipmap level to view from (https://en.wikipedia.org/wiki/Mipmap)
	imageViewCreateInfo.imageViewSubresourceRange.levelCount = 1;                          // Number of mipmap levels to view
	imageViewCreateInfo.imageViewSubresourceRange.baseArrayLayer = 0;                      // Start array level to view from
	imageViewCreateInfo.imageViewSubresourceRange.layerCount = 1;                          // Number of array levels to view

	struct kmr_vk_image_create_info swapchainImagesInfo;
	swapchainImagesInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	swapchainImagesInfo.swapchain = app->kmr_vk_swapchain.swapchain;
	swapchainImagesInfo.imageCount = 0;                                                    // set to zero as VkSwapchainKHR != VK_NULL_HANDLE
	swapchainImagesInfo.imageViewCreateInfos = &imageViewCreateInfo;
	/* Not creating images manually so rest of struct members can be safely ignored */
	swapchainImagesInfo.physDevice = VK_NULL_HANDLE;
	swapchainImagesInfo.imageCreateInfos = NULL;
	swapchainImagesInfo.memPropertyFlags = 0;
	swapchainImagesInfo.useExternalDmaBuffer = false;

	app->kmr_vk_image[0] = kmr_vk_image_create(&swapchainImagesInfo);
	if (!app->kmr_vk_image[0].imageViewHandles[0].view)
		return -1;

	return 0;
}


static VkFormat
choose_depth_image_format (struct app_vk *app,
                           VkImageTiling imageTiling,
                           VkFormatFeatureFlags formatFeatureFlags)
{
	uint32_t fp;

	VkFormat format = VK_FORMAT_UNDEFINED;
	VkFormat formats[3] = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };

	struct kmr_vk_phdev_format_prop physDeviceFormatProps;
	struct kmr_vk_phdev_format_prop_info formatPropsInfo;

	formatPropsInfo.physDev = app->kmr_vk_phdev.physDevice;
	formatPropsInfo.formats = formats;
	formatPropsInfo.formatCount = ARRAY_LEN(formats);
	formatPropsInfo.modifierProperties = NULL;
	formatPropsInfo.modifierCount = 0;

	physDeviceFormatProps = kmr_vk_get_phdev_format_properties(&formatPropsInfo);
	for (fp = 0; fp < physDeviceFormatProps.formatPropertyCount; fp++) {
		if (imageTiling == VK_IMAGE_TILING_OPTIMAL && \
		   (physDeviceFormatProps.formatProperties[fp].optimalTilingFeatures & formatFeatureFlags) == formatFeatureFlags) {
			format = formats[fp];
			break;
		} else if (imageTiling == VK_IMAGE_TILING_LINEAR && \
		          (physDeviceFormatProps.formatProperties[fp].optimalTilingFeatures & formatFeatureFlags) == formatFeatureFlags) {
			format = formats[fp];
			break;
		}
	}

	free(physDeviceFormatProps.formatProperties);
	return format;
}


static int
create_vk_depth_image (struct app_vk *app)
{
	VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL;

	VkFormat imageFormat = choose_depth_image_format(app, imageTiling, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	if (imageFormat == VK_FORMAT_UNDEFINED)
		return -1;

	struct kmr_vk_image_view_create_info imageViewCreateInfo;
	imageViewCreateInfo.imageViewflags = 0;
	imageViewCreateInfo.imageViewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.imageViewFormat = imageFormat;
	imageViewCreateInfo.imageViewComponents = (VkComponentMapping) { .r = 0, .g = 0, .b = 0, .a = 0 };
	imageViewCreateInfo.imageViewSubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	imageViewCreateInfo.imageViewSubresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.imageViewSubresourceRange.levelCount = 1;
	imageViewCreateInfo.imageViewSubresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.imageViewSubresourceRange.layerCount = 1;

	struct kmr_vk_vimage_create_info vimageCreateInfo;
	vimageCreateInfo.imageflags = 0;
	vimageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	vimageCreateInfo.imageFormat = imageFormat;
	vimageCreateInfo.imageExtent3D = (VkExtent3D) { .width = WIDTH, .height = HEIGHT, .depth = 1 }; // depth describes how deep the image goes (1 because no 3D aspect)
	vimageCreateInfo.imageMipLevels = 1;
	vimageCreateInfo.imageArrayLayers = 1;
	vimageCreateInfo.imageSamples = VK_SAMPLE_COUNT_1_BIT;
	vimageCreateInfo.imageTiling = imageTiling;
	vimageCreateInfo.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	vimageCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vimageCreateInfo.imageQueueFamilyIndexCount = 0;
	vimageCreateInfo.imageQueueFamilyIndices = NULL;
	vimageCreateInfo.imageInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vimageCreateInfo.imageDmaBufferFormatModifier = 0;
	vimageCreateInfo.imageDmaBufferCount = 0;
	vimageCreateInfo.imageDmaBufferFds = NULL;
	vimageCreateInfo.imageDmaBufferResourceInfo = NULL;
	vimageCreateInfo.imageDmaBufferMemTypeBits = NULL;

	struct kmr_vk_image_create_info imageCreateInfo;
	imageCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	imageCreateInfo.swapchain = VK_NULL_HANDLE;
	imageCreateInfo.imageCount = 1;
	imageCreateInfo.imageViewCreateInfos = &imageViewCreateInfo;
	imageCreateInfo.imageCreateInfos = &vimageCreateInfo;
	imageCreateInfo.physDevice = app->kmr_vk_phdev.physDevice;
	imageCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	imageCreateInfo.useExternalDmaBuffer = false;

	app->kmr_vk_image[1] = kmr_vk_image_create(&imageCreateInfo);
	if (!app->kmr_vk_image[1].imageHandles[0].image && !app->kmr_vk_image[1].imageViewHandles[0].view)
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
	const char vertexShader[] = \
		"#version 450\n" // GLSL 4.5
		"#extension GL_ARB_separate_shader_objects : enable\n\n"
		"layout (location = 0) out vec3 outNormal;\n"
		"layout (location = 1) out vec3 outColor;\n"
		"layout (location = 2) out vec2 outTexCoord;\n"
		"layout (location = 3) out vec3 outViewVec;\n"
		"layout (location = 4) out vec3 outLightVec;\n"
		"layout (location = 5) out flat int outTextureIndex;\n\n"
		"layout (location = 0) in vec3 inPosition;\n"
		"layout (location = 1) in vec3 inNormal;\n"
		"layout (location = 2) in vec2 inTexCoord;\n"
		"layout (location = 3) in vec3 inColor;\n\n"
		"layout (set = 0, binding = 0) uniform uniform_buffer_scene {\n"
		"	mat4 projection;\n"
		"	mat4 view;\n"
		"	vec4 lightPosition;\n"
		"	vec4 viewPosition;\n"
		"} uboScene;\n\n"
		"layout(set = 0, binding = 1) uniform uniform_buffer_scene_model {\n"
		"	mat4 model;\n"
		"	int textureIndex;\n"
		"} uboModel;\n"
		"void main() {\n"
		"	outNormal = inNormal;\n"
		"	outColor = inColor;\n"
		"	outTexCoord = inTexCoord;\n"
		"	outTextureIndex = uboModel.textureIndex;\n"
		"	gl_Position = uboScene.projection * uboScene.view * uboModel.model * vec4(inPosition.xyz, 1.0);\n"
		"	vec4 pos = uboScene.view * vec4(inPosition, 1.0);\n"
		"	outNormal = mat3(uboScene.view) * inNormal;\n"
		"	outLightVec = uboScene.lightPosition.xyz - pos.xyz;\n"
		"	outViewVec = uboScene.viewPosition.xyz - pos.xyz;\n"
		"}\n";

	const char fragmentShader[] = \
		"#version 450\n"
		"#extension GL_ARB_separate_shader_objects : enable\n\n"
		"layout (set = 0, binding = 2) uniform sampler2D samplerColorMap[5];\n\n"
		"layout (location = 0) in vec3 inNormal;\n"
		"layout (location = 1) in vec3 inColor;\n"
		"layout (location = 2) in vec2 inTexCoord;\n"
		"layout (location = 3) in vec3 inViewVec;\n"
		"layout (location = 4) in vec3 inLightVec;\n"
		"layout (location = 5) in flat int inTextureIndex;\n\n"
		"layout (location = 0) out vec4 outColor;\n"
		"void main() {\n"
		"	vec4 color = texture(samplerColorMap[inTextureIndex], inTexCoord) * vec4(inColor, 1.0);\n\n"
		"	vec3 N = normalize(inNormal);\n"
		"	vec3 L = normalize(inLightVec);\n"
		"	vec3 V = normalize(inViewVec);\n"
		"	vec3 R = reflect(L, N);\n"
		"	vec3 diffuse = max(dot(N, L), 0.15) * inColor;\n"
		"	vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.75);\n"
		"	outColor = vec4(diffuse * color.rgb + specular, 1.0);\n"
		"}";

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
create_vk_buffers (struct app_vk *app)
{
	uint8_t cpuVisibleBuffer = 0, gpuVisibleBuffer = 1;

	uint32_t m, v;
	uint16_t *indexBufferData = NULL;
	struct app_vertex_data *vertexBufferData = NULL;	
	uint32_t vertexBufferDataSize = 0, indexBufferDataSize = 0;
	uint32_t curVertexBufferIndex = 0, curIndexBufferIndex = 0, index;

	for (m = 0; m < app->kmr_gltf_loader_mesh->meshDataCount; m++) {
		app->meshData[m].bufferOffset += vertexBufferDataSize;
		vertexBufferDataSize += app->kmr_gltf_loader_mesh->meshData[m].vertexBufferDataSize;
		indexBufferDataSize += app->kmr_gltf_loader_mesh->meshData[m].indexBufferDataSize;
	}

	vertexBufferData = alloca(vertexBufferDataSize);		
	indexBufferData = alloca(indexBufferDataSize);
	app->indexBufferOffset = vertexBufferDataSize;

	/*
	 * Application must take vertex + index buffer arrays created in kmr_gltf_loader_mesh_create(3)
	 * and populate it's local array before creating and copying buffer into VkBuffer
	 */
	for (m = 0; m < app->kmr_gltf_loader_mesh->meshDataCount; m++) {
		// kmr_utils_log(KMR_INFO, "curVertexBufferIndex: %u, curIndexBufferIndex: %u", curVertexBufferIndex, curIndexBufferIndex);
		for (v = 0; v < app->kmr_gltf_loader_mesh->meshData[m].vertexBufferDataCount; v++) {
			index = curVertexBufferIndex + v;
			vertexBufferData[index].pos[0] = app->kmr_gltf_loader_mesh->meshData[m].vertexBufferData[v].position[0];
			vertexBufferData[index].pos[1] = app->kmr_gltf_loader_mesh->meshData[m].vertexBufferData[v].position[1];
			vertexBufferData[index].pos[2] = app->kmr_gltf_loader_mesh->meshData[m].vertexBufferData[v].position[2];
			
			vertexBufferData[index].color[0] = app->kmr_gltf_loader_mesh->meshData[m].vertexBufferData[v].color[0];
			vertexBufferData[index].color[1] = app->kmr_gltf_loader_mesh->meshData[m].vertexBufferData[v].color[1];
			vertexBufferData[index].color[2] = app->kmr_gltf_loader_mesh->meshData[m].vertexBufferData[v].color[2];
			
			vertexBufferData[index].normal[0] = app->kmr_gltf_loader_mesh->meshData[m].vertexBufferData[v].normal[0];
			vertexBufferData[index].normal[1] = app->kmr_gltf_loader_mesh->meshData[m].vertexBufferData[v].normal[1];
			vertexBufferData[index].normal[2] = app->kmr_gltf_loader_mesh->meshData[m].vertexBufferData[v].normal[2];
			
			vertexBufferData[index].texCoord[0] = app->kmr_gltf_loader_mesh->meshData[m].vertexBufferData[v].texCoord[0];
			vertexBufferData[index].texCoord[1] = app->kmr_gltf_loader_mesh->meshData[m].vertexBufferData[v].texCoord[1];
		}

		for (v = 0; v < app->kmr_gltf_loader_mesh->meshData[m].indexBufferDataCount; v++) {
			indexBufferData[curIndexBufferIndex + v] = app->kmr_gltf_loader_mesh->meshData[m].indexBufferData[v];
		}

		curIndexBufferIndex += app->kmr_gltf_loader_mesh->meshData[m].indexBufferDataCount;
		curVertexBufferIndex += app->kmr_gltf_loader_mesh->meshData[m].vertexBufferDataCount;
		// kmr_utils_log(KMR_WARNING, "curVertexBufferIndex: %u, curIndexBufferIndex: %u", curVertexBufferIndex, curIndexBufferIndex);
	}

	// Free'ing kmr_gltf_loader_mesh no longer required
	kmr_gltf_loader_mesh_destroy(app->kmr_gltf_loader_mesh);
	app->kmr_gltf_loader_mesh = NULL;

	/*
	 * Create CPU visible buffer [vertex + index]
	 */
	struct kmr_vk_buffer_create_info vkVertexBufferCreateInfo;
	vkVertexBufferCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	vkVertexBufferCreateInfo.physDevice = app->kmr_vk_phdev.physDevice;
	vkVertexBufferCreateInfo.bufferFlags = 0;
	vkVertexBufferCreateInfo.bufferSize = vertexBufferDataSize + indexBufferDataSize;
	vkVertexBufferCreateInfo.bufferUsage = (VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU || \
	                                        VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_CPU) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT : VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkVertexBufferCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vkVertexBufferCreateInfo.queueFamilyIndexCount = 0;
	vkVertexBufferCreateInfo.queueFamilyIndices = NULL;
	vkVertexBufferCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	app->kmr_vk_buffer[cpuVisibleBuffer] = kmr_vk_buffer_create(&vkVertexBufferCreateInfo);
	if (!app->kmr_vk_buffer[cpuVisibleBuffer].buffer || !app->kmr_vk_buffer[cpuVisibleBuffer].deviceMemory)
		return -1;

	// Copy GLTF buffer into Vulkan API created CPU visible buffer memory
	struct kmr_vk_memory_map_info deviceMemoryCopyInfo;
	deviceMemoryCopyInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	deviceMemoryCopyInfo.deviceMemory = app->kmr_vk_buffer[cpuVisibleBuffer].deviceMemory;
	deviceMemoryCopyInfo.deviceMemoryOffset = 0;
	deviceMemoryCopyInfo.memoryBufferSize = vertexBufferDataSize;
	deviceMemoryCopyInfo.bufferData = vertexBufferData;
	kmr_vk_memory_map(&deviceMemoryCopyInfo);

	deviceMemoryCopyInfo.deviceMemoryOffset = app->indexBufferOffset;
	deviceMemoryCopyInfo.memoryBufferSize = indexBufferDataSize;
	deviceMemoryCopyInfo.bufferData = indexBufferData;
	kmr_vk_memory_map(&deviceMemoryCopyInfo);

	if (VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		// Create GPU visible vertex buffer
		struct kmr_vk_buffer_create_info vkVertexBufferGPUCreateInfo;
		vkVertexBufferGPUCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
		vkVertexBufferGPUCreateInfo.physDevice = app->kmr_vk_phdev.physDevice;
		vkVertexBufferGPUCreateInfo.bufferFlags = 0;
		vkVertexBufferGPUCreateInfo.bufferSize = vkVertexBufferCreateInfo.bufferSize;
		vkVertexBufferGPUCreateInfo.bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
		vkVertexBufferGPUCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vkVertexBufferGPUCreateInfo.queueFamilyIndexCount = 0;
		vkVertexBufferGPUCreateInfo.queueFamilyIndices = NULL;
		vkVertexBufferGPUCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

		app->kmr_vk_buffer[gpuVisibleBuffer] = kmr_vk_buffer_create(&vkVertexBufferGPUCreateInfo);
		if (!app->kmr_vk_buffer[gpuVisibleBuffer].buffer || !app->kmr_vk_buffer[gpuVisibleBuffer].deviceMemory)
			return -1;

		VkBufferCopy copyRegion;
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = vkVertexBufferCreateInfo.bufferSize;

		struct kmr_vk_resource_copy_buffer_to_buffer_info bufferToBufferCopyInfo;
		bufferToBufferCopyInfo.copyRegion = &copyRegion;

		// Copy contents from CPU visible buffer over to GPU visible buffer
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

	cpuVisibleBuffer+=2;
	gpuVisibleBuffer+=2;

	struct kmr_utils_aligned_buffer_create_info modelUniformBufferAlignment;
	modelUniformBufferAlignment.bytesToAlign = sizeof(struct app_uniform_buffer_scene_model);
	modelUniformBufferAlignment.bytesToAlignCount = app->meshCount;
	modelUniformBufferAlignment.bufferAlignment = app->kmr_vk_phdev.physDeviceProperties.limits.minUniformBufferOffsetAlignment;

	app->modelTransferSpace = kmr_utils_aligned_buffer_create(&modelUniformBufferAlignment);
	if (!app->modelTransferSpace.alignedBufferMemory)
		return -1;

	// Create CPU visible uniform buffer to store (view projection matrices in first have) (Dynamic uniform buffer (model matrix) in second half)
	struct kmr_vk_buffer_create_info vkUniformBufferCreateInfo;
	vkUniformBufferCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	vkUniformBufferCreateInfo.physDevice = app->kmr_vk_phdev.physDevice;
	vkUniformBufferCreateInfo.bufferFlags = 0;
	vkUniformBufferCreateInfo.bufferSize = (sizeof(struct app_uniform_buffer_scene) * PRECEIVED_SWAPCHAIN_IMAGE_SIZE) + \
	                                       (app->modelTransferSpace.bufferAlignment * app->meshCount);
	vkUniformBufferCreateInfo.bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	vkUniformBufferCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vkUniformBufferCreateInfo.queueFamilyIndexCount = 0;
	vkUniformBufferCreateInfo.queueFamilyIndices = NULL;
	vkUniformBufferCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	app->kmr_vk_buffer[cpuVisibleBuffer] = kmr_vk_buffer_create(&vkUniformBufferCreateInfo);
	if (!app->kmr_vk_buffer[cpuVisibleBuffer].buffer || !app->kmr_vk_buffer[cpuVisibleBuffer].deviceMemory)
		return -1;

	return 0;
}


static int
create_vk_texture_images (struct app_vk *app)
{
	struct kmr_utils_image_buffer *imageData = NULL;

	uint32_t curImage, imageCount = 0;
	uint8_t textureImageIndex = 2, cpuVisibleImageBuffer = 3;

	imageCount = app->kmr_gltf_loader_texture_image->imageCount;
	imageData = app->kmr_gltf_loader_texture_image->imageData;

	// Create CPU visible buffer to store pixel data
	struct kmr_vk_buffer_create_info vkTextureBufferCreateInfo;
	vkTextureBufferCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	vkTextureBufferCreateInfo.physDevice = app->kmr_vk_phdev.physDevice;
	vkTextureBufferCreateInfo.bufferFlags = 0;
	vkTextureBufferCreateInfo.bufferSize = app->kmr_gltf_loader_texture_image->totalBufferSize;
	vkTextureBufferCreateInfo.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkTextureBufferCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vkTextureBufferCreateInfo.queueFamilyIndexCount = 0;
	vkTextureBufferCreateInfo.queueFamilyIndices = NULL;
	vkTextureBufferCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	app->kmr_vk_buffer[cpuVisibleImageBuffer] = kmr_vk_buffer_create(&vkTextureBufferCreateInfo);
	if (!app->kmr_vk_buffer[cpuVisibleImageBuffer].buffer || !app->kmr_vk_buffer[cpuVisibleImageBuffer].deviceMemory) {
		kmr_gltf_loader_texture_image_destroy(app->kmr_gltf_loader_texture_image);
		app->kmr_gltf_loader_texture_image = NULL;
		return -1;
	}

	/* Map all GLTF texture images into CPU Visible buffer memory */
	struct kmr_vk_memory_map_info deviceMemoryCopyInfo;
	deviceMemoryCopyInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	deviceMemoryCopyInfo.deviceMemory = app->kmr_vk_buffer[cpuVisibleImageBuffer].deviceMemory;

	struct kmr_vk_image_view_create_info imageViewCreateInfos[imageCount];
	struct kmr_vk_vimage_create_info vimageCreateInfos[imageCount];
	for (curImage = 0; curImage < imageCount; curImage++) {
		deviceMemoryCopyInfo.deviceMemoryOffset = imageData[curImage].imageBufferOffset;
		deviceMemoryCopyInfo.memoryBufferSize = imageData[curImage].imageSize;
		deviceMemoryCopyInfo.bufferData = imageData[curImage].pixels;
		kmr_vk_memory_map(&deviceMemoryCopyInfo);

		imageViewCreateInfos[curImage].imageViewflags = 0;
		imageViewCreateInfos[curImage].imageViewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfos[curImage].imageViewFormat = VK_FORMAT_R8G8B8A8_SRGB;
		imageViewCreateInfos[curImage].imageViewComponents = (VkComponentMapping) { .r = 0, .g = 0, .b = 0, .a = 0 };
		imageViewCreateInfos[curImage].imageViewSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Which aspect of image to view (i.e VK_IMAGE_ASPECT_COLOR_BIT view color)
		imageViewCreateInfos[curImage].imageViewSubresourceRange.baseMipLevel = 0;                       // Start mipmap level to view from (https://en.wikipedia.org/wiki/Mipmap)
		imageViewCreateInfos[curImage].imageViewSubresourceRange.levelCount = 1;                         // Number of mipmap levels to view
		imageViewCreateInfos[curImage].imageViewSubresourceRange.baseArrayLayer = 0;                     // Start array level to view from
		imageViewCreateInfos[curImage].imageViewSubresourceRange.layerCount = 1;                         // Number of array levels to view

		vimageCreateInfos[curImage].imageflags = 0;
		vimageCreateInfos[curImage].imageType = VK_IMAGE_TYPE_2D;
		vimageCreateInfos[curImage].imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
		vimageCreateInfos[curImage].imageExtent3D = (VkExtent3D) { .width = imageData[curImage].imageWidth, .height = imageData[curImage].imageHeight, .depth = 1 };
		vimageCreateInfos[curImage].imageMipLevels = 1;
		vimageCreateInfos[curImage].imageArrayLayers = 1;
		vimageCreateInfos[curImage].imageSamples = VK_SAMPLE_COUNT_1_BIT;
		vimageCreateInfos[curImage].imageTiling = VK_IMAGE_TILING_OPTIMAL;
		vimageCreateInfos[curImage].imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		vimageCreateInfos[curImage].imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		vimageCreateInfos[curImage].imageQueueFamilyIndexCount = 0;
		vimageCreateInfos[curImage].imageQueueFamilyIndices = NULL;
		vimageCreateInfos[curImage].imageInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vimageCreateInfos[curImage].imageDmaBufferFormatModifier = 0;
		vimageCreateInfos[curImage].imageDmaBufferCount = 0;
		vimageCreateInfos[curImage].imageDmaBufferFds = NULL;
		vimageCreateInfos[curImage].imageDmaBufferResourceInfo = NULL;
		vimageCreateInfos[curImage].imageDmaBufferMemTypeBits = NULL;
	}

	/* Create a VkImage/VkImageView for each texture */
	struct kmr_vk_image_create_info vkImageCreateInfo;
	vkImageCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	vkImageCreateInfo.swapchain = VK_NULL_HANDLE;                      // set VkSwapchainKHR to VK_NULL_HANDLE as we manually create images
	vkImageCreateInfo.imageCount = imageCount;                         // Creating @imageCount amount of VkImage(VkDeviceMemory)/VkImageView resource's to store pixel data
	vkImageCreateInfo.imageViewCreateInfos = imageViewCreateInfos;
	vkImageCreateInfo.imageCreateInfos = vimageCreateInfos;
	vkImageCreateInfo.physDevice = app->kmr_vk_phdev.physDevice;
	vkImageCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	vkImageCreateInfo.useExternalDmaBuffer = false;

	kmr_utils_log(KMR_INFO, "Creating VkImage's/VkImageView's for textures [total amount: %u]", imageCount);
	app->kmr_vk_image[textureImageIndex] = kmr_vk_image_create(&vkImageCreateInfo);
	if (!app->kmr_vk_image[textureImageIndex].imageHandles[curImage].image && !app->kmr_vk_image[textureImageIndex].imageViewHandles[curImage].view) {
		kmr_gltf_loader_texture_image_destroy(app->kmr_gltf_loader_texture_image);
		app->kmr_gltf_loader_texture_image = NULL;
		return -1;
	}

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.pNext = NULL;

	struct kmr_vk_resource_pipeline_barrier_info pipelineBarrierInfo;
	pipelineBarrierInfo.commandBuffer = app->kmr_vk_command_buffer.commandBufferHandles[0].commandBuffer;
	pipelineBarrierInfo.queue = app->kmr_vk_queue.queue;

	VkBufferImageCopy copyRegion;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;

	struct kmr_vk_resource_copy_buffer_to_image_info bufferToImageCopyInfo;
	bufferToImageCopyInfo.copyRegion = &copyRegion;
	bufferToImageCopyInfo.imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	/* Copy VkImage resource to VkBuffer resource */
	struct kmr_vk_resource_copy_info bufferCopyInfo;
	bufferCopyInfo.resourceCopyType = KMR_VK_RESOURCE_COPY_VK_BUFFER_TO_VK_IMAGE;
	bufferCopyInfo.resourceCopyInfo = &bufferToImageCopyInfo;
	bufferCopyInfo.commandBuffer = app->kmr_vk_command_buffer.commandBufferHandles[0].commandBuffer;
	bufferCopyInfo.queue = app->kmr_vk_queue.queue;
	bufferCopyInfo.srcResource = app->kmr_vk_buffer[cpuVisibleImageBuffer].buffer;

	for (curImage = 0; curImage < imageCount; curImage++) {
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = app->kmr_vk_image[textureImageIndex].imageHandles[curImage].image;
		imageMemoryBarrier.subresourceRange.aspectMask = imageViewCreateInfos[curImage].imageViewSubresourceRange.aspectMask;
		imageMemoryBarrier.subresourceRange.baseMipLevel = imageViewCreateInfos[curImage].imageViewSubresourceRange.baseMipLevel;
		imageMemoryBarrier.subresourceRange.levelCount = imageViewCreateInfos[curImage].imageViewSubresourceRange.levelCount;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = imageViewCreateInfos[curImage].imageViewSubresourceRange.baseArrayLayer;
		imageMemoryBarrier.subresourceRange.layerCount = imageViewCreateInfos[curImage].imageViewSubresourceRange.layerCount;

		pipelineBarrierInfo.srcPipelineStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		pipelineBarrierInfo.dstPipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		pipelineBarrierInfo.dependencyFlags = 0;
		pipelineBarrierInfo.memoryBarrier = NULL;
		pipelineBarrierInfo.bufferMemoryBarrier = NULL;
		pipelineBarrierInfo.imageMemoryBarrier = &imageMemoryBarrier;

		if (kmr_vk_resource_pipeline_barrier(&pipelineBarrierInfo) == -1) {
			kmr_gltf_loader_texture_image_destroy(app->kmr_gltf_loader_texture_image);
			app->kmr_gltf_loader_texture_image = NULL;
			return -1;
		}

		/* Copy pixel buffer to VkImage Resource */
		copyRegion.imageSubresource.aspectMask = imageViewCreateInfos[curImage].imageViewSubresourceRange.aspectMask;
		copyRegion.imageSubresource.mipLevel = imageViewCreateInfos[curImage].imageViewSubresourceRange.baseMipLevel;
		copyRegion.imageSubresource.baseArrayLayer = imageViewCreateInfos[curImage].imageViewSubresourceRange.baseArrayLayer;
		copyRegion.imageSubresource.layerCount = imageViewCreateInfos[curImage].imageViewSubresourceRange.layerCount;
		copyRegion.imageOffset = (VkOffset3D) { .x = 0, .y = 0, .z = 0 };
		copyRegion.imageExtent = vimageCreateInfos[curImage].imageExtent3D;
		copyRegion.bufferOffset = imageData[curImage].imageBufferOffset;

		bufferCopyInfo.dstResource = app->kmr_vk_image[textureImageIndex].imageHandles[curImage].image;

		if (kmr_vk_resource_copy(&bufferCopyInfo) == -1) {
			kmr_gltf_loader_texture_image_destroy(app->kmr_gltf_loader_texture_image);
			app->kmr_gltf_loader_texture_image = NULL;
			return -1;
		}

		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		pipelineBarrierInfo.srcPipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		pipelineBarrierInfo.dstPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		pipelineBarrierInfo.imageMemoryBarrier = &imageMemoryBarrier;
		if (kmr_vk_resource_pipeline_barrier(&pipelineBarrierInfo) == -1) {
			kmr_gltf_loader_texture_image_destroy(app->kmr_gltf_loader_texture_image);
			app->kmr_gltf_loader_texture_image = NULL;
			return -1;
		}
	}

	/* Free up memory after everything is copied */
	kmr_gltf_loader_texture_image_destroy(app->kmr_gltf_loader_texture_image);
	app->kmr_gltf_loader_texture_image = NULL;
	vkDestroyBuffer(app->kmr_vk_buffer[cpuVisibleImageBuffer].logicalDevice, app->kmr_vk_buffer[cpuVisibleImageBuffer].buffer, NULL);
	vkFreeMemory(app->kmr_vk_buffer[cpuVisibleImageBuffer].logicalDevice, app->kmr_vk_buffer[cpuVisibleImageBuffer].deviceMemory, NULL);
	app->kmr_vk_buffer[cpuVisibleImageBuffer].buffer = VK_NULL_HANDLE;
	app->kmr_vk_buffer[cpuVisibleImageBuffer].deviceMemory = VK_NULL_HANDLE;

	kmr_utils_log(KMR_SUCCESS, "Successfully created VkImage objects for GLTF texture assets");

	return 0;
}


static int
create_gltf_load_required_data (struct app_vk *app)
{
	uint32_t meshIndex;

	struct kmr_gltf_loader_file *gltfLoaderFile = NULL;
	struct kmr_gltf_loader_node *gltfLoaderFileNodes = NULL;

	struct kmr_gltf_loader_file_create_info gltfLoaderFileCreateInfo;
	struct kmr_gltf_loader_mesh_create_info gltfMeshInfo;
	struct kmr_gltf_loader_texture_image_create_info gltfTextureImagesInfo;
	struct kmr_gltf_loader_node_create_info gltfLoaderFileNodeInfo;
	struct kmr_gltf_loader_material_create_info gltfLoaderMaterialInfo;

	gltfLoaderFileCreateInfo.fileName = GLTF_MODEL;
	gltfLoaderFile = kmr_gltf_loader_file_create(&gltfLoaderFileCreateInfo);
	if (!gltfLoaderFile->gltfData)
		return -1;

	gltfMeshInfo.gltfFile = gltfLoaderFile;
	gltfMeshInfo.bufferIndex = 0;
	app->kmr_gltf_loader_mesh = kmr_gltf_loader_mesh_create(&gltfMeshInfo);
	if (!app->kmr_gltf_loader_mesh)
		goto exit_error_create_gltf_load_required_data;

	gltfTextureImagesInfo.gltfFile = gltfLoaderFile;
	gltfTextureImagesInfo.directory = gltfLoaderFileCreateInfo.fileName;
	app->kmr_gltf_loader_texture_image = kmr_gltf_loader_texture_image_create(&gltfTextureImagesInfo);
	if (!app->kmr_gltf_loader_texture_image)
		goto exit_error_create_gltf_load_required_data;

	gltfLoaderFileNodeInfo.gltfFile = gltfLoaderFile;
	gltfLoaderFileNodeInfo.sceneIndex = 0;
	gltfLoaderFileNodes = kmr_gltf_loader_node_create(&gltfLoaderFileNodeInfo);
	if (!gltfLoaderFileNodes->nodeData)
		goto exit_error_create_gltf_load_required_data;

	gltfLoaderMaterialInfo.gltfFile = gltfLoaderFile;
	app->kmr_gltf_loader_material = kmr_gltf_loader_material_create(&gltfLoaderMaterialInfo);
	if (!app->kmr_gltf_loader_material)
		goto exit_error_create_gltf_load_required_data;

	app->meshCount = app->kmr_gltf_loader_mesh->meshDataCount;
	app->meshData = calloc(app->meshCount, sizeof(struct mesh_data));
	if (!app->meshData) {
		kmr_utils_log(KMR_DANGER, "[x] calloc(app->meshData): %s", strerror(errno));
		goto exit_error_create_gltf_load_required_data;
	}

	// Copy TRS matrix transform data to the passable buffer used during draw operations
	for (meshIndex = 0; meshIndex < gltfLoaderFileNodes->nodeDataCount; meshIndex++) {
		// glm_mat4_copy(gltfLoaderFileNodes.nodeData[meshIndex].matrixTransform, app->meshData[gltfLoaderFileNodes.nodeData[meshIndex].objectIndex].matrix);
		memcpy(app->meshData[gltfLoaderFileNodes->nodeData[meshIndex].objectIndex].matrix, \
		       gltfLoaderFileNodes->nodeData[meshIndex].matrixTransform, \
		       sizeof(mat4));
		app->meshData[meshIndex].firstIndex = app->kmr_gltf_loader_mesh->meshData[meshIndex].firstIndex;
		app->meshData[meshIndex].indexCount = app->kmr_gltf_loader_mesh->meshData[meshIndex].indexBufferDataCount;
	}

	// Have everything we need free memory created
	kmr_gltf_loader_node_destroy(gltfLoaderFileNodes);
	kmr_gltf_loader_file_destroy(gltfLoaderFile);
	return 0;

exit_error_create_gltf_load_required_data:
	kmr_gltf_loader_node_destroy(gltfLoaderFileNodes);
	kmr_gltf_loader_material_destroy(app->kmr_gltf_loader_material);
	kmr_gltf_loader_texture_image_destroy(app->kmr_gltf_loader_texture_image);
	kmr_gltf_loader_mesh_destroy(app->kmr_gltf_loader_mesh);
	kmr_gltf_loader_file_destroy(gltfLoaderFile);
	app->kmr_gltf_loader_material = NULL;
	app->kmr_gltf_loader_texture_image = NULL;
	app->kmr_gltf_loader_mesh = NULL;
	return -1;
}


static int
create_vk_image_sampler (struct app_vk *app)
{
	struct kmr_vk_sampler_create_info vkSamplerCreateInfo;
	vkSamplerCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	vkSamplerCreateInfo.samplerFlags = 0;
	vkSamplerCreateInfo.samplerMagFilter = VK_FILTER_LINEAR;                  // Close to texture: 50/50 sample of two pixels if camera inbetween to pixels
	vkSamplerCreateInfo.samplerMinFilter = VK_FILTER_LINEAR;                  // Away from texture
	vkSamplerCreateInfo.samplerAddressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	vkSamplerCreateInfo.samplerAddressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	vkSamplerCreateInfo.samplerAddressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	vkSamplerCreateInfo.samplerCompareOp = VK_COMPARE_OP_ALWAYS;
	vkSamplerCreateInfo.samplerMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	vkSamplerCreateInfo.samplerMipLodBias = 0.0f;
	vkSamplerCreateInfo.samplerMinLod = 0.0f;
	vkSamplerCreateInfo.samplerMaxLod = 0.0f;
	vkSamplerCreateInfo.samplerUnnormalizedCoordinates = VK_FALSE;
	vkSamplerCreateInfo.samplerBorderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	vkSamplerCreateInfo.samplerAnisotropyEnable = VK_TRUE;
	vkSamplerCreateInfo.samplerMaxAnisotropy = app->kmr_vk_phdev.physDeviceProperties.limits.maxSamplerAnisotropy;
	vkSamplerCreateInfo.samplerCompareEnable = VK_FALSE;

	app->kmr_vk_sampler = kmr_vk_sampler_create(&vkSamplerCreateInfo);
	if (!app->kmr_vk_sampler.sampler)
		return -1;

	return 0;
}


static int
create_vk_resource_descriptor_sets (struct app_vk *app)
{
	uint32_t i;
	VkDescriptorSetLayoutBinding descSetLayoutBindings[3]; // Amount of descriptors in the set
	uint32_t descriptorBindingCount = ARRAY_LEN(descSetLayoutBindings);

	uint32_t materialCount = app->kmr_gltf_loader_material->materialDataCount;
	struct kmr_gltf_loader_material_data *materialData = app->kmr_gltf_loader_material->materialData;

	// Uniform descriptor
	descSetLayoutBindings[0].binding = 0;
	descSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	// See struct kmr_vk_descriptor_set_handle for more information in this
	descSetLayoutBindings[0].descriptorCount = 1;
	descSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descSetLayoutBindings[0].pImmutableSamplers = NULL;

	// Dynamic Uniform descriptor
	descSetLayoutBindings[1].binding = 1;
	descSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
	// See struct kmr_vk_descriptor_set_handle for more information in this
	descSetLayoutBindings[1].descriptorCount = 1;
	descSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	descSetLayoutBindings[1].pImmutableSamplers = NULL;

	// Combinded Image Sampler descriptor
	descSetLayoutBindings[2].binding = 2;
	descSetLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	// See struct kmr_vk_descriptor_set_handle for more information in this
	descSetLayoutBindings[2].descriptorCount = materialCount;
	descSetLayoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	descSetLayoutBindings[2].pImmutableSamplers = NULL;

	struct kmr_vk_descriptor_set_layout_create_info descriptorCreateInfo;
	descriptorCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	descriptorCreateInfo.descriptorSetLayoutCreateflags = 0;
	descriptorCreateInfo.descriptorSetLayoutBindingCount = ARRAY_LEN(descSetLayoutBindings);
	descriptorCreateInfo.descriptorSetLayoutBindings = descSetLayoutBindings;

	app->kmr_vk_descriptor_set_layout = kmr_vk_descriptor_set_layout_create(&descriptorCreateInfo);
	if (!app->kmr_vk_descriptor_set_layout.descriptorSetLayout)
		return -1;

	/*
	 * Per my understanding this is just so the VkDescriptorPool knows what to preallocate. No descriptor is
	 * assigned to a set when the pool is created. Given an array of descriptor set layouts the actual assignment
	 * of descriptor to descriptor set happens in the vkAllocateDescriptorSets function.
	 */
	VkDescriptorPoolSize descriptorPoolInfos[descriptorBindingCount];
	for (i = 0; i < descriptorBindingCount; i++) {
		descriptorPoolInfos[i].type = descSetLayoutBindings[i].descriptorType;
		descriptorPoolInfos[i].descriptorCount = descSetLayoutBindings[i].descriptorCount;
	}

	// Should allocate one pool. With one set containing multiple descriptors
	struct kmr_vk_descriptor_set_create_info descriptorSetsCreateInfo;
	descriptorSetsCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	descriptorSetsCreateInfo.descriptorPoolInfos = descriptorPoolInfos;
	descriptorSetsCreateInfo.descriptorPoolInfoCount = ARRAY_LEN(descriptorPoolInfos);
	descriptorSetsCreateInfo.descriptorSetLayouts = &app->kmr_vk_descriptor_set_layout.descriptorSetLayout;
	descriptorSetsCreateInfo.descriptorSetLayoutCount = 1;
	descriptorSetsCreateInfo.descriptorPoolCreateflags = 0;

	app->kmr_vk_descriptor_set = kmr_vk_descriptor_set_create(&descriptorSetsCreateInfo);
	if (!app->kmr_vk_descriptor_set.descriptorPool || !app->kmr_vk_descriptor_set.descriptorSetHandles[0].descriptorSet)
		return -1;

	VkDescriptorBufferInfo bufferInfos[descriptorBindingCount];
	bufferInfos[0].buffer = app->kmr_vk_buffer[2].buffer; // CPU visible uniform buffer
	bufferInfos[0].offset = 0;
	bufferInfos[0].range = sizeof(struct app_uniform_buffer_scene);

	bufferInfos[1].buffer = app->kmr_vk_buffer[2].buffer; // CPU visible uniform buffer dynamic buffer
	bufferInfos[1].offset = sizeof(struct app_uniform_buffer_scene) * PRECEIVED_SWAPCHAIN_IMAGE_SIZE;
	bufferInfos[1].range = app->modelTransferSpace.bufferAlignment;

	// Texture images from GLTF file
	uint8_t baseColorTextureImageIndex;
	VkDescriptorImageInfo imageInfos[materialCount];
	for (i = 0; i < materialCount; i++) {
		baseColorTextureImageIndex = materialData[i].pbrMetallicRoughness.baseColorTexture.imageIndex;
		imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfos[i].imageView = app->kmr_vk_image[2].imageViewHandles[baseColorTextureImageIndex].view; // created in create_vk_texture_image
		imageInfos[i].sampler = app->kmr_vk_sampler.sampler;                                              // created in create_vk_image_sampler
	}

	// Remove material data no longer required
	kmr_gltf_loader_material_destroy(app->kmr_gltf_loader_material);
	app->kmr_gltf_loader_material = NULL;

	/* Binds multiple descriptors and their objects to the same set */
	VkWriteDescriptorSet descriptorWrites[descriptorBindingCount];
	for (i = 0; i < ARRAY_LEN(descriptorWrites); i++) {
		descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[i].pNext = NULL;
		descriptorWrites[i].dstSet = app->kmr_vk_descriptor_set.descriptorSetHandles[0].descriptorSet;
		descriptorWrites[i].dstBinding = descSetLayoutBindings[i].binding;
		descriptorWrites[i].dstArrayElement = 0;
		descriptorWrites[i].descriptorType = descSetLayoutBindings[i].descriptorType;
		descriptorWrites[i].descriptorCount = descSetLayoutBindings[i].descriptorCount;
		descriptorWrites[i].pBufferInfo = (i < 2) ? &bufferInfos[i] : NULL;
		descriptorWrites[i].pImageInfo = (i == 2) ? imageInfos : NULL;
		descriptorWrites[i].pTexelBufferView = NULL;
	}

	vkUpdateDescriptorSets(app->kmr_vk_lgdev.logicalDevice, descriptorBindingCount, descriptorWrites, 0, NULL);

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

	// Four incomming vertex atributes for the vertex shader.
	VkVertexInputAttributeDescription vertexAttributeDescriptions[4];

	// position attribute
	vertexAttributeDescriptions[0].location = 0;
	vertexAttributeDescriptions[0].binding = 0;
	// defines the byte size of attribute data
	vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 - RGB color channels match size of vec3
	// specifies the byte where struct app_vertex_data member vec3 pos is located
	vertexAttributeDescriptions[0].offset = offsetof(struct app_vertex_data, pos);

	// normal attribute
	vertexAttributeDescriptions[1].location = 1;
	vertexAttributeDescriptions[1].binding = 0;
	vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 - RGB color channels match size of vec3
	// specifies the byte where struct app_vertex_data member normal color is located
	vertexAttributeDescriptions[1].offset = offsetof(struct app_vertex_data, normal);

	// texture attribute
	vertexAttributeDescriptions[2].location = 2;
	vertexAttributeDescriptions[2].binding = 0;
	vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // vec2 - RG color channels match size of vec2
	// specifies the byte where struct app_vertex_data member vec2 texCoord is located
	vertexAttributeDescriptions[2].offset = offsetof(struct app_vertex_data, texCoord);

	// color attribute
	vertexAttributeDescriptions[3].location = 3;
	vertexAttributeDescriptions[3].binding = 0;
	vertexAttributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 - RGB color channels match size of vec3
	// specifies the byte where struct app_vertex_data member vec3 color is located
	vertexAttributeDescriptions[3].offset = offsetof(struct app_vertex_data, color);

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
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

	VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
	depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilInfo.pNext = NULL;
	depthStencilInfo.flags = 0;
	depthStencilInfo.depthTestEnable = VK_TRUE;           // Enable checking depth to determine fragment write
	depthStencilInfo.depthWriteEnable = VK_TRUE;          // Enable depth buffer writes inorder to replace old values
	depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS; // Comparison operation that allows an overwrite (if is in front)
	depthStencilInfo.depthBoundsTestEnable = VK_FALSE;    // Enable if depth test should be enabled between @minDepthBounds & @maxDepthBounds
	// There isn't any depth stencil in this example
	depthStencilInfo.stencilTestEnable = VK_FALSE;
	depthStencilInfo.front = (VkStencilOpState) { .failOp = VK_STENCIL_OP_KEEP, .passOp = VK_STENCIL_OP_KEEP, .depthFailOp = VK_STENCIL_OP_KEEP,
	                                              .compareOp = VK_COMPARE_OP_NEVER, .compareMask = 0, .writeMask = 0, .reference = 0 };
	depthStencilInfo.back = (VkStencilOpState) { .failOp = VK_STENCIL_OP_KEEP, .passOp = VK_STENCIL_OP_KEEP, .depthFailOp = VK_STENCIL_OP_KEEP,
	                                             .compareOp = VK_COMPARE_OP_NEVER, .compareMask = 0, .writeMask = 0, .reference = 0 };
	depthStencilInfo.minDepthBounds = 0.0f;
	depthStencilInfo.maxDepthBounds = 0.0f;

	/*
	VkPushConstantRange pushConstantRange;
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(uniform_buffer_model); // sizeof data to pass
	*/

	struct kmr_vk_pipeline_layout_create_info graphicsPipelineLayoutCreateInfo;
	graphicsPipelineLayoutCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	graphicsPipelineLayoutCreateInfo.descriptorSetLayoutCount = 1;
	graphicsPipelineLayoutCreateInfo.descriptorSetLayouts = &app->kmr_vk_descriptor_set_layout.descriptorSetLayout;
	graphicsPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	graphicsPipelineLayoutCreateInfo.pushConstantRanges = NULL;
	// graphicsPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	// graphicsPipelineLayoutCreateInfo.pushConstantRanges = &pushConstantRange;

	app->kmr_vk_pipeline_layout = kmr_vk_pipeline_layout_create(&graphicsPipelineLayoutCreateInfo);
	if (!app->kmr_vk_pipeline_layout.pipelineLayout)
		return -1;

	/*
	 * Attachments
	 * 0. Color Attachment
	 * 1. Depth Attachment
	 */
	VkAttachmentDescription attachmentDescriptions[2];
	attachmentDescriptions[0].flags = 0;
	attachmentDescriptions[0].format = surfaceFormat->format;
	attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Load data into attachment be sure to clear first
	attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	attachmentDescriptions[1].flags = 0;
	attachmentDescriptions[1].format = choose_depth_image_format(app, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	/*
	 * Attachment References
	 * 0. Color Attachment Reference
	 * 1. Depth Attachment Reference
	 */
	VkAttachmentReference attachmentReferences[2] = {};
	attachmentReferences[0].attachment = 0;
	attachmentReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachmentReferences[1].attachment = 1;
	attachmentReferences[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = NULL;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentReferences[0];
	subpass.pResolveAttachments = NULL;
	subpass.pDepthStencilAttachment = &attachmentReferences[1];
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = NULL;

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
	renderPassInfo.attachmentDescriptionCount = ARRAY_LEN(attachmentDescriptions);
	renderPassInfo.attachmentDescriptions = attachmentDescriptions;
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
	graphicsPipelineInfo.depthStencilState = &depthStencilInfo;
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

	framebufferCount = app->kmr_vk_image[0].imageCount;
	framebufferImages = alloca(framebufferCount * sizeof(struct kmr_vk_framebuffer_images));
	for (i = 0; i < framebufferCount; i++) {
		// VkImageView->VkImage for color image attachment
		framebufferImages[i].imageAttachments[0] = app->kmr_vk_image[0].imageViewHandles[i].view;
		// VkImageView->VkImage for depth buffer image attachment
		framebufferImages[i].imageAttachments[1] = app->kmr_vk_image[1].imageViewHandles[0].view;
	}

	framebufferInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	framebufferInfo.framebufferCount = framebufferCount;      // Amount of framebuffers to create
	framebufferInfo.framebufferImageAttachmentCount = 2;
	framebufferInfo.framebufferImages = framebufferImages;    // image attachments per framebuffer
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
create_vk_sync_objs (struct app_vk *app)
{
	struct kmr_vk_sync_obj_create_info syncObjsCreateInfo;
	syncObjsCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	syncObjsCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_BINARY;
	syncObjsCreateInfo.semaphoreCount = 2;
	syncObjsCreateInfo.fenceCount = 1;

	app->kmr_vk_sync_obj = kmr_vk_sync_obj_create(&syncObjsCreateInfo);
	if (!app->kmr_vk_sync_obj.fenceHandles[0].fence && !app->kmr_vk_sync_obj.semaphoreHandles[0].semaphore)
		return -1;

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
	 * Values used by VK_ATTACHMENT_LOAD_OP_CLEAR to clear
	 * framebuffer attachments with.
	 */
	float float32[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	int32_t int32[4] = {0.0f, 0.0f, 0.0f, 1.0f};
	uint32_t uint32[4] = {0.0f, 0.0f, 0.0f, 1.0f};

	VkClearValue clearColors[2];
	memcpy(clearColors[0].color.float32, float32, ARRAY_LEN(float32));
	memcpy(clearColors[0].color.int32, int32, ARRAY_LEN(int32));
	memcpy(clearColors[0].color.int32, uint32, ARRAY_LEN(uint32));
	clearColors[0].depthStencil.depth = 0.0f;
	clearColors[0].depthStencil.stencil = 0;

	memcpy(clearColors[1].color.float32, float32, ARRAY_LEN(float32));
	memcpy(clearColors[1].color.int32, int32, ARRAY_LEN(int32));
	memcpy(clearColors[1].color.int32, uint32, ARRAY_LEN(uint32));
	clearColors[1].depthStencil.depth = 1.0f;
	clearColors[1].depthStencil.stencil = 0;

	VkRenderPassBeginInfo renderPassInfo;
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.pNext = NULL;
	renderPassInfo.renderPass = app->kmr_vk_render_pass.renderPass;
	renderPassInfo.framebuffer = app->kmr_vk_framebuffer.framebufferHandles[swapchainImageIndex].framebuffer;
	renderPassInfo.renderArea = renderArea;
	renderPassInfo.clearValueCount = ARRAY_LEN(clearColors);
	renderPassInfo.pClearValues = clearColors;

	/*
	 * 0. CPU visible vertex buffer
	 * 1. GPU visible vertex buffer
	 */
	VkBuffer vertexBuffer = app->kmr_vk_buffer[(VK_PHYSICAL_DEVICE_TYPE != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? 0 : 1].buffer;

	vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->kmr_vk_graphics_pipeline.graphicsPipeline);
	vkCmdBindIndexBuffer(cmdBuffer, vertexBuffer, app->indexBufferOffset, VK_INDEX_TYPE_UINT16);

	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
	vkCmdSetScissor(cmdBuffer, 0, 1, &renderArea);

	VkDeviceSize offset;
	uint32_t dynamicUniformBufferOffset = 0;
	for (uint32_t mesh = 0; mesh < app->meshCount; mesh++) {
		offset = app->meshData[mesh].bufferOffset;
		dynamicUniformBufferOffset = mesh * app->modelTransferSpace.bufferAlignment;
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->kmr_vk_pipeline_layout.pipelineLayout, 0, 1,
		                        &app->kmr_vk_descriptor_set.descriptorSetHandles[0].descriptorSet, 1, &dynamicUniformBufferOffset);
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, &offset);
		vkCmdDrawIndexed(cmdBuffer, app->meshData[mesh].indexCount, 1, app->meshData[mesh].firstIndex, 0, 0);
	}

	vkCmdEndRenderPass(cmdBuffer);

	if (kmr_vk_command_buffer_record_end(&commandBufferRecordInfo) == -1)
		return -1;

	return 0;
}


static void
update_uniform_buffer (struct app_vk *app,
                       uint32_t swapchainImageIndex,
                       VkExtent2D extent2D)
{
	VkDeviceMemory uniformBufferDeviceMemory = app->kmr_vk_buffer[2].deviceMemory;

	struct app_uniform_buffer_scene ubo = {};
	uint32_t uboSize = sizeof(ubo);

	vec4 lightPosition = {5.0f, 5.0f, -5.0f, 1.0f};
	glm_vec4_copy(lightPosition, ubo.lightPosition);

	vec4 positionVec4;
	vec3 position = { 0.0f, -0.1f, -1.0f };
	vec4 multiplyVec = { -1.0f, 1.0f, -1.0f, 1.0f };

	glm_vec4(position, 0.0f, positionVec4);
	glm_vec4_mul(positionVec4, multiplyVec, ubo.viewPosition);

	// Update view/camera matrix
	vec3 eye = {0.3f, -0.1f, 1.0f};
	vec3 center = {0.0f, -0.1f, 0.0f};
	vec3 up = {0.0f, 1.0f, 0.0f};
	glm_lookat(eye, center, up, ubo.view);

	// Update projection matrix
	float fovy = glm_rad(60.0f); // Field of view
	float aspect = (float) extent2D.width / (float) extent2D.height;
	float nearPlane = 0.1f;
	float farPlane = 256.0f;
	glm_perspective(fovy, aspect, nearPlane, farPlane, ubo.projection);

	// invert y - coordinate on projection matrix
	ubo.projection[1][1] *= -1;

	// Copy UBO Scene data
	struct kmr_vk_memory_map_info deviceMemoryCopyInfo;
	deviceMemoryCopyInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	deviceMemoryCopyInfo.deviceMemory = uniformBufferDeviceMemory;
	deviceMemoryCopyInfo.deviceMemoryOffset = swapchainImageIndex * uboSize;
	deviceMemoryCopyInfo.memoryBufferSize = uboSize;
	deviceMemoryCopyInfo.bufferData = &ubo;
	kmr_vk_memory_map(&deviceMemoryCopyInfo);

	// Spin model about the X-axis
	const float angle = 0.0025f;
	vec3 axis = {0.0f, 1.0f, 0.0f};

	// Copy Model data
	struct app_uniform_buffer_scene_model *sceneModel = NULL;
	for (uint32_t mesh = 0; mesh < app->meshCount; mesh++) {
		sceneModel = (struct app_uniform_buffer_scene_model *) ((uint64_t) app->modelTransferSpace.alignedBufferMemory + (mesh * app->modelTransferSpace.bufferAlignment));
		glm_rotate(app->meshData[mesh].matrix, glm_rad(angle), axis);
		memcpy(sceneModel->model, app->meshData[mesh].matrix, sizeof(mat4));
		sceneModel->textureIndex = mesh;
	}

	// Map all Model data
	deviceMemoryCopyInfo.deviceMemoryOffset = uboSize * PRECEIVED_SWAPCHAIN_IMAGE_SIZE;
	deviceMemoryCopyInfo.memoryBufferSize = app->modelTransferSpace.bufferAlignment * app->meshCount;
	deviceMemoryCopyInfo.bufferData = app->modelTransferSpace.alignedBufferMemory;
	kmr_vk_memory_map(&deviceMemoryCopyInfo);
}
