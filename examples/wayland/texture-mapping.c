#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h> // For offsetof(3)
#include <stdalign.h> // _Alignas( )
#include <signal.h>

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/cglm.h>

#include "wclient.h"
#include "vulkan.h"
#include "shader.h"

#define WIDTH 1920
#define HEIGHT 1080
#define MAX_SCENE_OBJECTS 2
#define PRECEIVED_SWAPCHAIN_IMAGE_SIZE 5

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
	 * 2. Texture Image
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
         * Other required data needed for draw operations
         */
        struct kmr_utils_aligned_buffer modelTransferSpace;
};


struct app_wc {
	struct kmr_wc_core_interface kmr_wc_core_interface;
	struct kmr_wc_surface kmr_wc_surface;
};


struct app_vk_wc {
	struct app_wc *app_wc;
	struct app_vk *app_vk;
};


struct app_vertex_data {
	vec3 pos;
	vec3 color;
	vec2 texCoord;
};


struct app_uniform_buffer_scene_model {
	mat4 model;
};


struct app_uniform_buffer_scene {
	mat4 view;
	mat4 projection;
};


/*
 * Comments define how to draw rectangle without index buffer
 * Actual array itself draws triangles utilizing index buffers.
 */
const struct app_vertex_data meshData[2][4] = {
	{
		{{-0.1f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // Vertex 0. Top-right- red
		{{-0.1f,0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},   // Vertex 1. Bottom-right - green
		{{-0.9f,0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},   // Vertex 2. Bottom-left- blue
		//{{-0.9f,0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},   // Vertex 2. Bottom-left- blue
		{{-0.9f, -0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}  // Vertex 3. Top-left - yellow
		//{{-0.1f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // Vertex 0. Top-right- red
	},
	{
		{{0.9f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}}, // Vertex 0. Top-right- red
		{{0.9f,0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},   // Vertex 1. Bottom-right - green
		{{0.1f,0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},   // Vertex 2. Bottom-left- blue
		//{{0.1f,0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},   // Vertex 2. Bottom-left- blue
		{{0.1f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}  // Vertex 3. Top-left - yellow
		//{{0.9f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // Vertex 0. Top-right- red
	}
};


/*
 * Defines what vertices in vertex array are reusable
 * inorder to draw a rectangle
 */
const uint16_t indices[] = {
	0, 1, 2, 2, 3, 0
};


static volatile sig_atomic_t prun = 1;


static void run_stop(int UNUSED signum)
{
	prun = 0;
}


int create_wc_vk_surface(struct app_vk *app, struct app_wc *wc, uint32_t *cbuf, bool *running);
int create_vk_instance(struct app_vk *app);
int create_vk_device(struct app_vk *app);
int create_vk_swapchain(struct app_vk *app, VkSurfaceFormatKHR *surfaceFormat, VkExtent2D extent2D);
int create_vk_swapchain_images(struct app_vk *app, VkSurfaceFormatKHR *surfaceFormat);
int create_vk_depth_image(struct app_vk *app);
int create_vk_shader_modules(struct app_vk *app);
int create_vk_command_buffers(struct app_vk *app);
int create_vk_buffers(struct app_vk *app);
int create_vk_texture_image(struct app_vk *app);
int create_vk_image_sampler(struct app_vk *app);
int create_vk_resource_descriptor_sets(struct app_vk *app);
int create_vk_graphics_pipeline(struct app_vk *app, VkSurfaceFormatKHR *surfaceFormat, VkExtent2D extent2D);
int create_vk_framebuffers(struct app_vk *app, VkExtent2D extent2D);
int create_vk_sync_objs(struct app_vk *app);
int record_vk_draw_commands(struct app_vk *app, uint32_t swapchainImageIndex, VkExtent2D extent2D);
void update_uniform_buffer(struct app_vk *app, uint32_t swapchainImageIndex, VkExtent2D extent2D);


void render(bool *running, uint32_t *imageIndex, void *data)
{
	VkExtent2D extent2D = {WIDTH, HEIGHT};
	struct app_vk_wc *vkwc = data;
	struct app_wc UNUSED *wc = vkwc->app_wc;
	struct app_vk *app = vkwc->app_vk;

	if (!app->kmr_vk_sync_obj.fenceHandles || !app->kmr_vk_sync_obj.semaphoreHandles)
		return;

	VkFence imageFence = app->kmr_vk_sync_obj.fenceHandles[0].fence;
	VkSemaphore imageSemaphore = app->kmr_vk_sync_obj.semaphoreHandles[0].semaphore;
	VkSemaphore renderSemaphore = app->kmr_vk_sync_obj.semaphoreHandles[1].semaphore;

	vkWaitForFences(app->kmr_vk_lgdev.logicalDevice, 1, &imageFence, VK_TRUE, UINT64_MAX);

	vkAcquireNextImageKHR(app->kmr_vk_lgdev.logicalDevice, app->kmr_vk_swapchain.swapchain, UINT64_MAX, imageSemaphore, VK_NULL_HANDLE, imageIndex);

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
	presentInfo.pImageIndices = imageIndex;
	presentInfo.pResults = NULL;

	vkQueuePresentKHR(app->kmr_vk_queue.queue, &presentInfo);

	*running = prun;
}


/*
 * Example code demonstrating how use Vulkan with Wayland
 */
int main(void)
{
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

	struct app_vk app;
	struct kmr_vk_destroy appd;
	memset(&app, 0, sizeof(app));
	memset(&appd, 0, sizeof(appd));

	struct app_wc wc;
	struct kmr_wc_destroy wcd;
	memset(&wcd, 0, sizeof(wcd));
	memset(&wc, 0, sizeof(wc));

	static uint32_t cbuf = 0;
	static bool running = true;

	if (create_vk_instance(&app) == -1)
		goto exit_error;

	if (create_wc_vk_surface(&app, &wc, &cbuf, &running) == -1)
		goto exit_error;

	/*
	 * Create Vulkan Physical Device Handle, After Window Surface
	 * so that it doesn't effect VkPhysicalDevice selection
	 */
	if (create_vk_device(&app) == -1)
		goto exit_error;

	VkSurfaceFormatKHR surfaceFormat;
	VkExtent2D extent2D = { .width = WIDTH, .height = HEIGHT };
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

	if (create_vk_buffers(&app) == -1)
		goto exit_error;

	if (create_vk_texture_image(&app) == -1)
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

	while (wl_display_dispatch(wc.kmr_wc_core_interface.wlDisplay) != -1 && running) {
		// Leave blank
	}

exit_error:
	free(app.modelTransferSpace.alignedBufferMemory);

	/*
	 * Let the api know of what addresses to free and fd's to close
	 */
	appd.instance = app.instance;
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

	wcd.kmr_wc_core_interface = wc.kmr_wc_core_interface;
	wcd.kmr_wc_surface = wc.kmr_wc_surface;
	kmr_wc_destroy(&wcd);
	return 0;
}


int create_wc_vk_surface(struct app_vk *app, struct app_wc *wc, uint32_t *cbuf, bool *running)
{
	struct kmr_wc_core_interface_create_info coreInterfacesCreateInfo;
	coreInterfacesCreateInfo.displayName = NULL;
	coreInterfacesCreateInfo.iType = KMR_WC_INTERFACE_WL_COMPOSITOR | KMR_WC_INTERFACE_XDG_WM_BASE |
	                                 KMR_WC_INTERFACE_WL_SEAT | KMR_WC_INTERFACE_ZWP_FULLSCREEN_SHELL_V1;

	wc->kmr_wc_core_interface = kmr_wc_core_interface_create(&coreInterfacesCreateInfo);
	if (!wc->kmr_wc_core_interface.wlDisplay || !wc->kmr_wc_core_interface.wlRegistry || !wc->kmr_wc_core_interface.wlCompositor)
		return -1;

	static struct app_vk_wc vkwc;
	vkwc.app_wc = wc;
	vkwc.app_vk = app;

	struct kmr_wc_surface_create_info wcSurfaceCreateInfo;
	wcSurfaceCreateInfo.coreInterface = &wc->kmr_wc_core_interface;
	wcSurfaceCreateInfo.wcBufferObject = NULL;
	wcSurfaceCreateInfo.bufferCount = 0;
	wcSurfaceCreateInfo.appName = "Texture Mapping Example App";
	wcSurfaceCreateInfo.fullscreen = false;
	wcSurfaceCreateInfo.renderer = render;
	wcSurfaceCreateInfo.rendererData = &vkwc;
	wcSurfaceCreateInfo.rendererCurrentBuffer = cbuf;
	wcSurfaceCreateInfo.rendererRunning = running;

	wc->kmr_wc_surface = kmr_wc_surface_create(&wcSurfaceCreateInfo);
	if (!wc->kmr_wc_surface.wlSurface)
		return -1;

	/*
	 * Create Vulkan Surface
	 */
	struct kmr_vk_surface_create_info vkSurfaceCreateInfo;
	vkSurfaceCreateInfo.surfaceType = KMR_SURFACE_WAYLAND_CLIENT;
	vkSurfaceCreateInfo.instance = app->instance;
	vkSurfaceCreateInfo.display = wc->kmr_wc_core_interface.wlDisplay;
	vkSurfaceCreateInfo.surface = wc->kmr_wc_surface.wlSurface;

	app->surface = kmr_vk_surface_create(&vkSurfaceCreateInfo);
	if (!app->surface)
		return -1;

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
		"VK_KHR_wayland_surface",
		"VK_KHR_surface",
		"VK_KHR_display",
		"VK_EXT_debug_utils"
	};

	struct kmr_vk_instance_create_info instanceCreateInfo;
	instanceCreateInfo.appName = "Texture Mapping Example App";
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


int create_vk_device(struct app_vk *app)
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
int create_vk_swapchain(struct app_vk *app, VkSurfaceFormatKHR *surfaceFormat, VkExtent2D extent2D)
{
	uint32_t i;
	VkPresentModeKHR presentMode;

	VkSurfaceCapabilitiesKHR surfaceCapabilities = kmr_vk_get_surface_capabilities(app->kmr_vk_phdev.physDevice, app->surface);
	struct kmr_vk_surface_format surfaceFormats = kmr_vk_get_surface_formats(app->kmr_vk_phdev.physDevice, app->surface);
	struct kmr_vk_surface_present_mode surfacePresentModes = kmr_vk_get_surface_present_modes(app->kmr_vk_phdev.physDevice, app->surface);

	/* Choose surface format based */
	for (i = 0; i < surfaceFormats.surfaceFormatCount; i++) {
		if (surfaceFormats.surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats.surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
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


int create_vk_swapchain_images(struct app_vk *app, VkSurfaceFormatKHR *surfaceFormat)
{
	struct kmr_vk_image_view_create_info imageViewCreateInfo;
	imageViewCreateInfo.imageViewflags = 0;
	imageViewCreateInfo.imageViewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.imageViewFormat = surfaceFormat->format;
	imageViewCreateInfo.imageViewComponents.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.imageViewComponents.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.imageViewComponents.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.imageViewComponents.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewCreateInfo.imageViewSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Which aspect of image to view (i.e VK_IMAGE_ASPECT_COLOR_BIT view color)
	imageViewCreateInfo.imageViewSubresourceRange.baseMipLevel = 0;                       // Start mipmap level to view from (https://en.wikipedia.org/wiki/Mipmap)
	imageViewCreateInfo.imageViewSubresourceRange.levelCount = 1;                         // Number of mipmap levels to view
	imageViewCreateInfo.imageViewSubresourceRange.baseArrayLayer = 0;                     // Start array level to view from
	imageViewCreateInfo.imageViewSubresourceRange.layerCount = 1;                         // Number of array levels to view

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


VkFormat choose_depth_image_format(struct app_vk *app, VkImageTiling imageTiling, VkFormatFeatureFlags formatFeatureFlags)
{
	VkFormat formats[3] = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };
	VkFormat format = VK_FORMAT_UNDEFINED;

	struct kmr_vk_phdev_format_prop_info formatPropsInfo;
	formatPropsInfo.physDev = app->kmr_vk_phdev.physDevice;
	formatPropsInfo.formats = formats;
	formatPropsInfo.formatCount = ARRAY_LEN(formats);
	formatPropsInfo.modifierProperties = NULL;
	formatPropsInfo.modifierCount = 0;

	struct kmr_vk_phdev_format_prop physDeviceFormatProps = kmr_vk_get_phdev_format_properties(&formatPropsInfo);
	for (uint32_t fp = 0; fp < physDeviceFormatProps.formatPropertyCount; fp++) {
		if (imageTiling == VK_IMAGE_TILING_OPTIMAL && (physDeviceFormatProps.formatProperties[fp].optimalTilingFeatures & formatFeatureFlags) == formatFeatureFlags) {
			format = formats[fp];
			goto exit_choose_depth_image_format;
		} else if (imageTiling == VK_IMAGE_TILING_LINEAR && (physDeviceFormatProps.formatProperties[fp].optimalTilingFeatures & formatFeatureFlags) == formatFeatureFlags) {
			format = formats[fp];
			goto exit_choose_depth_image_format;
		}
	}

exit_choose_depth_image_format:
	free(physDeviceFormatProps.formatProperties);
	return format;
}


int create_vk_depth_image(struct app_vk *app)
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


int create_vk_shader_modules(struct app_vk *app)
{
	int ret = 0;

#ifdef INCLUDE_SHADERC
	const char vertexShader[] =
		"#version 450\n" // GLSL 4.5
		"#extension GL_ARB_separate_shader_objects : enable\n\n"
		"layout(location = 0) out vec3 outColor;\n"
		"layout(location = 1) out vec2 outTexCoord;\n\n"
		"layout(location = 0) in vec3 inPosition;\n"
		"layout(location = 1) in vec3 inColor;\n"
		"layout(location = 2) in vec2 inTexCoord;\n\n"
		"layout(set = 0, binding = 0) uniform uniform_buffer_scene {\n"
		"	mat4 view;\n"
		"	mat4 projection;\n"
		"} uboScene;\n"
		"layout(set = 0, binding = 1) uniform uniform_buffer_scene_model {\n"
		"	mat4 model;\n"
		"} uboModel;\n\n"
		"void main() {\n"
		"	gl_Position = uboScene.projection * uboScene.view * uboModel.model * vec4(inPosition, 1.0);\n"
		"	outColor = inColor;\n"
		"	outTexCoord = inTexCoord;\n"
		"}";

	const char fragmentShader[] =
		"#version 450\n"
		"#extension GL_ARB_separate_shader_objects : enable\n"
		"layout(location = 0) in vec3 inColor;\n"
		"layout(location = 1) in vec2 inTexCoord;\n\n"
		"layout(location = 0) out vec4 outColor;\n"
		"layout(set = 0, binding = 2) uniform sampler2D outTexSampler;\n"
		"void main() { outColor = vec4(texture(outTexSampler, inTexCoord).rgb, 1.0); }";

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
	struct kmr_shader_spirv kmr_shader[2];
	memset(kmr_shader, 0, sizeof(kmr_shader));

	kmr_shader[0] = kmr_shader_compile_buffer_to_spirv(&vertexShaderCreateInfo);
	if (!kmr_shader[0].bytes) { ret = -1 ; goto exit_distroy_shader ; }

	kmr_shader[1] = kmr_shader_compile_buffer_to_spirv(&fragmentShaderCreateInfo);
	if (!kmr_shader[1].bytes) { ret = -1 ; goto exit_distroy_shader ; }

#else
	/*
	 * 0. Vertex Shader
	 * 1. Fragment Shader
	 */
	struct kmr_utils_file kmr_shader[2];
	memset(kmr_shader, 0, sizeof(kmr_shader));

	kmr_shader[0] = kmr_utils_file_load(VERTEX_SHADER_SPIRV);
	if (!kmr_shader[0].bytes) { ret = -1 ; goto exit_distroy_shader ; }

	kmr_shader[1] = kmr_utils_file_load(FRAGMENT_SHADER_SPIRV);
	if (!kmr_shader[1].bytes) { ret = -1 ; goto exit_distroy_shader ; }
#endif

	const char *shaderModuleNames[] = {
		"vertex", "fragment"
	};

	struct kmr_vk_shader_module_create_info shaderModuleCreateInfo;
	for (uint32_t currentShader = 0; currentShader < ARRAY_LEN(kmr_shader); currentShader++) {
		shaderModuleCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
		shaderModuleCreateInfo.sprivByteSize = kmr_shader[currentShader].byteSize;
		shaderModuleCreateInfo.sprivBytes = kmr_shader[currentShader].bytes;
		shaderModuleCreateInfo.shaderName = shaderModuleNames[currentShader];

		app->kmr_vk_shader_module[currentShader] = kmr_vk_shader_module_create(&shaderModuleCreateInfo);
		if (!app->kmr_vk_shader_module[currentShader].shaderModule) { ret = -1 ; goto exit_distroy_shader ; }
	}

exit_distroy_shader:
#ifdef INCLUDE_SHADERC
	struct kmr_shader_destroy shaderd;
	memset(&shaderd, 0, sizeof(shaderd));
	shaderd.kmr_shader_spirv_cnt = ARRAY_LEN(kmr_shader);
	shaderd.kmr_shader_spirv = kmr_shader;
	kmr_shader_destroy(&shaderd);
#else
	free(kmr_shader[0].bytes);
	free(kmr_shader[1].bytes);
#endif
	return ret;
}


int create_vk_command_buffers(struct app_vk *app)
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


int create_vk_buffers(struct app_vk *app)
{
	uint8_t cpuVisibleBuffer = 0, gpuVisibleBuffer = 1;

	size_t singleMeshSize = sizeof(meshData[0]);
	size_t singleIndexBufferSize = sizeof(indices);

	// Create CPU visible vertex + index buffer
	struct kmr_vk_buffer_create_info vkVertexBufferCreateInfo;
	vkVertexBufferCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	vkVertexBufferCreateInfo.physDevice = app->kmr_vk_phdev.physDevice;
	vkVertexBufferCreateInfo.bufferFlags = 0;
	vkVertexBufferCreateInfo.bufferSize = singleIndexBufferSize + sizeof(meshData);
	vkVertexBufferCreateInfo.bufferUsage = (VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU || \
	                                        VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_CPU) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT : VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkVertexBufferCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vkVertexBufferCreateInfo.queueFamilyIndexCount = 0;
	vkVertexBufferCreateInfo.queueFamilyIndices = NULL;
	vkVertexBufferCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	app->kmr_vk_buffer[cpuVisibleBuffer] = kmr_vk_buffer_create(&vkVertexBufferCreateInfo);
	if (!app->kmr_vk_buffer[cpuVisibleBuffer].buffer || !app->kmr_vk_buffer[cpuVisibleBuffer].deviceMemory)
		return -1;

	// Copy index data into CPU visible index buffer
	struct kmr_vk_memory_map_info deviceMemoryCopyInfo;
	deviceMemoryCopyInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	deviceMemoryCopyInfo.deviceMemory = app->kmr_vk_buffer[cpuVisibleBuffer].deviceMemory;
	deviceMemoryCopyInfo.deviceMemoryOffset = 0;
	deviceMemoryCopyInfo.memoryBufferSize = singleIndexBufferSize;
	deviceMemoryCopyInfo.bufferData = indices;
	kmr_vk_memory_map(&deviceMemoryCopyInfo);

	// Copy vertex data into CPU visible vertex buffer
	deviceMemoryCopyInfo.memoryBufferSize = singleMeshSize;
	for (uint32_t currentVertexData = 0; currentVertexData < MAX_SCENE_OBJECTS; currentVertexData++) {
		deviceMemoryCopyInfo.deviceMemoryOffset = singleIndexBufferSize + (singleMeshSize * currentVertexData);
		deviceMemoryCopyInfo.bufferData = meshData[currentVertexData];
		kmr_vk_memory_map(&deviceMemoryCopyInfo);
	}

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

		// Copy contents from CPU visible buffer over to GPU visible buffer
		struct kmr_vk_resource_copy_info bufferCopyInfo;
		bufferCopyInfo.resourceCopyType = KMR_VK_COPY_VK_BUFFER_TO_VK_BUFFER;
		bufferCopyInfo.commandBuffer = app->kmr_vk_command_buffer.commandBufferHandles[0].commandBuffer;
		bufferCopyInfo.queue = app->kmr_vk_queue.queue;
		bufferCopyInfo.srcResource = app->kmr_vk_buffer[cpuVisibleBuffer].buffer;
		bufferCopyInfo.dstResource = app->kmr_vk_buffer[gpuVisibleBuffer].buffer;
		bufferCopyInfo.bufferCopyInfo = &copyRegion;
		bufferCopyInfo.bufferImageCopyInfo = NULL;
		bufferCopyInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		if (kmr_vk_resource_copy(&bufferCopyInfo) == -1)
			return -1;
	}

	cpuVisibleBuffer+=2;
	gpuVisibleBuffer+=2;

	struct kmr_utils_aligned_buffer_create_info modelUniformBufferAlignment;
	modelUniformBufferAlignment.bytesToAlign = sizeof(struct app_uniform_buffer_scene_model);
	modelUniformBufferAlignment.bytesToAlignCount = MAX_SCENE_OBJECTS;
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
	                                       (app->modelTransferSpace.bufferAlignment * MAX_SCENE_OBJECTS);
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


int create_vk_texture_image(struct app_vk *app)
{
	struct kmr_utils_image_buffer imageData;
	uint8_t cpuVisibleImageBuffer = 3, textureImageIndex = 2, imageTransferIndex = 0;

	struct kmr_utils_image_buffer_create_info imageDataCreateInfo;
	imageDataCreateInfo.directory = TEXTURE_IMAGE;
	imageDataCreateInfo.filename = NULL;
	imageDataCreateInfo.maxStrLen = (1<<8);
	imageData = kmr_utils_image_buffer_create(&imageDataCreateInfo);
	if (!imageData.pixels)
		return -1;

	// Create CPU visible buffer to store pixel data
	struct kmr_vk_buffer_create_info vkTextureBufferCreateInfo;
	vkTextureBufferCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	vkTextureBufferCreateInfo.physDevice = app->kmr_vk_phdev.physDevice;
	vkTextureBufferCreateInfo.bufferFlags = 0;
	vkTextureBufferCreateInfo.bufferSize = imageData.imageSize;
	vkTextureBufferCreateInfo.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	vkTextureBufferCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vkTextureBufferCreateInfo.queueFamilyIndexCount = 0;
	vkTextureBufferCreateInfo.queueFamilyIndices = NULL;
	vkTextureBufferCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	app->kmr_vk_buffer[cpuVisibleImageBuffer] = kmr_vk_buffer_create(&vkTextureBufferCreateInfo);
	if (!app->kmr_vk_buffer[cpuVisibleImageBuffer].buffer || !app->kmr_vk_buffer[cpuVisibleImageBuffer].deviceMemory) {
		free(imageData.pixels);
		return -1;
	}

	struct kmr_vk_memory_map_info deviceMemoryCopyInfo;
	deviceMemoryCopyInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	deviceMemoryCopyInfo.deviceMemory = app->kmr_vk_buffer[cpuVisibleImageBuffer].deviceMemory;
	deviceMemoryCopyInfo.deviceMemoryOffset = 0;
	deviceMemoryCopyInfo.memoryBufferSize = imageData.imageSize;
	deviceMemoryCopyInfo.bufferData = imageData.pixels;
	kmr_vk_memory_map(&deviceMemoryCopyInfo);

	free(imageData.pixels);

	struct kmr_vk_image_view_create_info imageViewCreateInfo;
	imageViewCreateInfo.imageViewflags = 0;
	imageViewCreateInfo.imageViewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.imageViewFormat = VK_FORMAT_R8G8B8A8_SRGB;
	imageViewCreateInfo.imageViewComponents = (VkComponentMapping) { .r = 0, .g = 0, .b = 0, .a = 0 };
	imageViewCreateInfo.imageViewSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // Which aspect of image to view (i.e VK_IMAGE_ASPECT_COLOR_BIT view color)
	imageViewCreateInfo.imageViewSubresourceRange.baseMipLevel = 0;                        // Start mipmap level to view from (https://en.wikipedia.org/wiki/Mipmap)
	imageViewCreateInfo.imageViewSubresourceRange.levelCount = 1;                          // Number of mipmap levels to view
	imageViewCreateInfo.imageViewSubresourceRange.baseArrayLayer = 0;                      // Start array level to view from
	imageViewCreateInfo.imageViewSubresourceRange.layerCount = 1;                          // Number of array levels to view

	struct kmr_vk_vimage_create_info vimageCreateInfo;
	vimageCreateInfo.imageflags = 0;
	vimageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	vimageCreateInfo.imageFormat = VK_FORMAT_R8G8B8A8_SRGB;
	vimageCreateInfo.imageExtent3D = (VkExtent3D) { .width = imageData.imageWidth, .height = imageData.imageHeight, .depth = 1 };
	vimageCreateInfo.imageMipLevels = 1;
	vimageCreateInfo.imageArrayLayers = 1;
	vimageCreateInfo.imageSamples = VK_SAMPLE_COUNT_1_BIT;
	vimageCreateInfo.imageTiling = VK_IMAGE_TILING_OPTIMAL;
	vimageCreateInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	vimageCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	vimageCreateInfo.imageQueueFamilyIndexCount = 0;
	vimageCreateInfo.imageQueueFamilyIndices = NULL;
	vimageCreateInfo.imageInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	vimageCreateInfo.imageDmaBufferFormatModifier = 0;
	vimageCreateInfo.imageDmaBufferCount = 0;
	vimageCreateInfo.imageDmaBufferFds = NULL;
	vimageCreateInfo.imageDmaBufferResourceInfo = NULL;
	vimageCreateInfo.imageDmaBufferMemTypeBits = NULL;

	/* Create a VkImage/VkImageView for each texture */
	struct kmr_vk_image_create_info vkImageCreateInfo;
	vkImageCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	vkImageCreateInfo.swapchain = VK_NULL_HANDLE;                         // set VkSwapchainKHR to VK_NULL_HANDLE as we manually create images
	vkImageCreateInfo.imageCount = 1;                                     // Creating 1 VkImage resource's to store pixel data
	vkImageCreateInfo.imageViewCreateInfos = &imageViewCreateInfo;
	vkImageCreateInfo.imageCreateInfos = &vimageCreateInfo;
	vkImageCreateInfo.physDevice = app->kmr_vk_phdev.physDevice;
	vkImageCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	vkImageCreateInfo.useExternalDmaBuffer = false;

	app->kmr_vk_image[textureImageIndex] = kmr_vk_image_create(&vkImageCreateInfo);
	if (!app->kmr_vk_image[textureImageIndex].imageHandles[imageTransferIndex].image && !app->kmr_vk_image[textureImageIndex].imageViewHandles[imageTransferIndex].view)
		return -1;

	VkImageMemoryBarrier imageMemoryBarrier = {};
	imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	imageMemoryBarrier.pNext = NULL;
	imageMemoryBarrier.srcAccessMask = 0;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	imageMemoryBarrier.image = app->kmr_vk_image[textureImageIndex].imageHandles[imageTransferIndex].image;
	imageMemoryBarrier.subresourceRange.aspectMask = imageViewCreateInfo.imageViewSubresourceRange.aspectMask;
	imageMemoryBarrier.subresourceRange.baseMipLevel = imageViewCreateInfo.imageViewSubresourceRange.baseMipLevel;
	imageMemoryBarrier.subresourceRange.levelCount = imageViewCreateInfo.imageViewSubresourceRange.levelCount;
	imageMemoryBarrier.subresourceRange.baseArrayLayer = imageViewCreateInfo.imageViewSubresourceRange.baseArrayLayer;
	imageMemoryBarrier.subresourceRange.layerCount = imageViewCreateInfo.imageViewSubresourceRange.layerCount;

	struct kmr_vk_resource_pipeline_barrier_info pipelineBarrierInfo;
	pipelineBarrierInfo.commandBuffer = app->kmr_vk_command_buffer.commandBufferHandles[0].commandBuffer;
	pipelineBarrierInfo.queue = app->kmr_vk_queue.queue;
	pipelineBarrierInfo.srcPipelineStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	pipelineBarrierInfo.dstPipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	pipelineBarrierInfo.dependencyFlags = 0;
	pipelineBarrierInfo.memoryBarrier = NULL;
	pipelineBarrierInfo.bufferMemoryBarrier = NULL;
	pipelineBarrierInfo.imageMemoryBarrier = &imageMemoryBarrier;

	if (kmr_vk_resource_pipeline_barrier(&pipelineBarrierInfo) == -1)
		return -1;

	/* Copy pixel buffer to VkImage Resource */
	VkBufferImageCopy copyRegion;
	copyRegion.bufferOffset = 0;
	copyRegion.bufferRowLength = 0;
	copyRegion.bufferImageHeight = 0;
	copyRegion.imageSubresource.aspectMask = imageViewCreateInfo.imageViewSubresourceRange.aspectMask;
	copyRegion.imageSubresource.mipLevel = imageViewCreateInfo.imageViewSubresourceRange.baseMipLevel;
	copyRegion.imageSubresource.baseArrayLayer = imageViewCreateInfo.imageViewSubresourceRange.baseArrayLayer;
	copyRegion.imageSubresource.layerCount = imageViewCreateInfo.imageViewSubresourceRange.layerCount;
	copyRegion.imageOffset = (VkOffset3D) { .x = 0, .y = 0, .z = 0 };
	copyRegion.imageExtent = vimageCreateInfo.imageExtent3D;

	/* Copy VkImage resource to VkBuffer resource */
	struct kmr_vk_resource_copy_info bufferCopyInfo;
	bufferCopyInfo.resourceCopyType = KMR_VK_COPY_VK_BUFFER_TO_VK_IMAGE;
	bufferCopyInfo.commandBuffer = app->kmr_vk_command_buffer.commandBufferHandles[0].commandBuffer;
	bufferCopyInfo.queue = app->kmr_vk_queue.queue;
	bufferCopyInfo.srcResource = app->kmr_vk_buffer[cpuVisibleImageBuffer].buffer;
	bufferCopyInfo.dstResource = app->kmr_vk_image[textureImageIndex].imageHandles[imageTransferIndex].image;
	bufferCopyInfo.bufferCopyInfo = NULL;
	bufferCopyInfo.bufferImageCopyInfo = &copyRegion;
	bufferCopyInfo.imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

	if (kmr_vk_resource_copy(&bufferCopyInfo) == -1)
		return -1;

	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	pipelineBarrierInfo.srcPipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	pipelineBarrierInfo.dstPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	pipelineBarrierInfo.imageMemoryBarrier = &imageMemoryBarrier;
	if (kmr_vk_resource_pipeline_barrier(&pipelineBarrierInfo) == -1)
		return -1;

	return 0;
}


int create_vk_image_sampler(struct app_vk *app)
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


int create_vk_resource_descriptor_sets(struct app_vk *app)
{
	uint32_t i;
	VkDescriptorSetLayoutBinding descSetLayoutBindings[3]; // Amount of descriptors in the set
	uint32_t descriptorBindingCount = ARRAY_LEN(descSetLayoutBindings);

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
	descSetLayoutBindings[2].descriptorCount = 1;
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

	// Allocate one pool. With one set containing multiple descriptors
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

	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = app->kmr_vk_image[2].imageViewHandles[0].view; // created in create_vk_texture_image
	imageInfo.sampler = app->kmr_vk_sampler.sampler;                     // created in create_vk_image_sampler

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
		descriptorWrites[i].pImageInfo = (i == 2) ? &imageInfo : NULL;
		descriptorWrites[i].pTexelBufferView = NULL;
	}

	vkUpdateDescriptorSets(app->kmr_vk_lgdev.logicalDevice, descriptorBindingCount, descriptorWrites, 0, NULL);

	return 0;
}


int create_vk_graphics_pipeline(struct app_vk *app, VkSurfaceFormatKHR *surfaceFormat, VkExtent2D extent2D)
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

	// Three incomming vertex atributes in the vertex shader.
	VkVertexInputAttributeDescription vertexAttributeDescriptions[3];

	// position attribute
	vertexAttributeDescriptions[0].location = 0;
	vertexAttributeDescriptions[0].binding = 0;
	// defines the byte size of attribute data
	vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 - RGB color channels match size of vec3
	// specifies the byte where struct app_vertex_data member vec3 pos is located
	vertexAttributeDescriptions[0].offset = offsetof(struct app_vertex_data, pos);

	// color attribute
	vertexAttributeDescriptions[1].location = 1;
	vertexAttributeDescriptions[1].binding = 0;
	vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 - RGB color channels match size of vec3
	// specifies the byte where struct app_vertex_data member vec3 color is located
	vertexAttributeDescriptions[1].offset = offsetof(struct app_vertex_data, color);

	// texture attribute
	vertexAttributeDescriptions[2].location = 2;
	vertexAttributeDescriptions[2].binding = 0;
	vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // vec2 - RG color channels match size of vec2
	// specifies the byte where struct app_vertex_data member vec3 color is located
	vertexAttributeDescriptions[2].offset = offsetof(struct app_vertex_data, texCoord);

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
	 * Attachments
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


int create_vk_framebuffers(struct app_vk *app, VkExtent2D extent2D)
{
	uint8_t framebufferCount = app->kmr_vk_image[0].imageCount;
	struct kmr_vk_framebuffer_images framebufferImages[framebufferCount];

	for (uint8_t i = 0; i < framebufferCount; i++) {
		framebufferImages[i].imageAttachments[0] = app->kmr_vk_image[0].imageViewHandles[i].view; // VkImageView->VkImage for color image attachment
		framebufferImages[i].imageAttachments[1] = app->kmr_vk_image[1].imageViewHandles[0].view; // VkImageView->VkImage for depth buffer image attachment
	}

	struct kmr_vk_framebuffer_create_info framebufferInfo;
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


int create_vk_sync_objs(struct app_vk *app)
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


int record_vk_draw_commands(struct app_vk *app, uint32_t swapchainImageIndex, VkExtent2D extent2D)
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
	VkDeviceSize offsets[] = {sizeof(indices), sizeof(indices) + sizeof(meshData[0]) };
	uint32_t dynamicUniformBufferOffset = 0;

	vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->kmr_vk_graphics_pipeline.graphicsPipeline);
	vkCmdBindIndexBuffer(cmdBuffer, vertexBuffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
	vkCmdSetScissor(cmdBuffer, 0, 1, &renderArea);

	for (uint32_t mesh = 0; mesh < MAX_SCENE_OBJECTS; mesh++) {
		dynamicUniformBufferOffset = mesh * app->modelTransferSpace.bufferAlignment;
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->kmr_vk_pipeline_layout.pipelineLayout, 0, 1,
		                        &app->kmr_vk_descriptor_set.descriptorSetHandles[0].descriptorSet, 1, &dynamicUniformBufferOffset);
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, &offsets[mesh]);
		vkCmdDrawIndexed(cmdBuffer, ARRAY_LEN(indices), 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(cmdBuffer);

	if (kmr_vk_command_buffer_record_end(&commandBufferRecordInfo) == -1)
		return -1;

	return 0;
}


void update_uniform_buffer(struct app_vk *app, uint32_t swapchainImageIndex, VkExtent2D extent2D)
{
	VkDeviceMemory uniformBufferDeviceMemory = app->kmr_vk_buffer[2].deviceMemory;
	uint32_t uboSize = sizeof(struct app_uniform_buffer_scene);
	struct app_uniform_buffer_scene uboScene = {};
	struct app_uniform_buffer_scene_model uboModels[MAX_SCENE_OBJECTS];

	// Update view matrix
	vec3 eye = {2.0f, 2.0f, 2.0f};
	vec3 center = {0.0f, 0.0f, 0.0f};
	vec3 up = {0.0f, 0.0f, 1.0f};
	glm_lookat(eye, center, up, uboScene.view);

	// Update projection matrix
	float fovy = glm_rad(45.0f); // Field of view
	float aspect = (float) extent2D.width / (float) extent2D.height;
	float nearPlane = 0.1f;
	float farPlane = 10.0f;
	glm_perspective(fovy, aspect, nearPlane, farPlane, uboScene.projection);

	// invert y - coordinate on projection matrix
	uboScene.projection[1][1] *= -1;

	// Copy VP data
	struct kmr_vk_memory_map_info deviceMemoryCopyInfo;
	deviceMemoryCopyInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	deviceMemoryCopyInfo.deviceMemory = uniformBufferDeviceMemory;
	deviceMemoryCopyInfo.deviceMemoryOffset = swapchainImageIndex * uboSize;
	deviceMemoryCopyInfo.memoryBufferSize = uboSize;
	deviceMemoryCopyInfo.bufferData = &uboScene;
	kmr_vk_memory_map(&deviceMemoryCopyInfo);

	static float lastTime = 0;
	float now = (float) (kmr_utils_nanosecond() / 10000000ULL);
	float deltaTime = now - lastTime;
	lastTime = now;

	// Update model matrix
	static float angle = 0.0f;

	angle += deltaTime * glm_rad(10.0f);
	if (angle > 360.0f) angle -= 360.0f;

	vec3 axis = {0.0f, 0.0f, 1.0f};
	glm_mat4_identity(uboModels[0].model);
	glm_mat4_identity(uboModels[1].model);

	glm_rotate(uboModels[0].model, glm_rad(angle), axis);
	glm_rotate(uboModels[1].model, glm_rad(-angle * 4.0f), axis);
	glm_translate(uboModels[0].model, (vec3){ 1.0f,0.0f, 1.0f });
	glm_translate(uboModels[1].model, (vec3){ 1.0f, 1.0f, 1.0f });
	glm_rotate(uboModels[0].model, glm_rad(angle), axis);
	glm_rotate(uboModels[1].model, glm_rad(-angle * 4.0f), axis);

	// Copy Model data
	for (uint32_t mesh = 0; mesh < MAX_SCENE_OBJECTS; mesh++) {
		struct app_uniform_buffer_scene_model *model = (struct app_uniform_buffer_scene_model *) ((uint64_t) app->modelTransferSpace.alignedBufferMemory + (mesh * app->modelTransferSpace.bufferAlignment));
		memcpy(model, &uboModels[mesh], app->modelTransferSpace.bufferAlignment);
	}

	// Map all Model data
	deviceMemoryCopyInfo.deviceMemoryOffset = uboSize * PRECEIVED_SWAPCHAIN_IMAGE_SIZE;
	deviceMemoryCopyInfo.memoryBufferSize = app->modelTransferSpace.bufferAlignment * MAX_SCENE_OBJECTS;
	deviceMemoryCopyInfo.bufferData = app->modelTransferSpace.alignedBufferMemory;
	kmr_vk_memory_map(&deviceMemoryCopyInfo);
}
