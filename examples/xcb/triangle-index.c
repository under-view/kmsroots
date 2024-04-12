#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h> // For offsetof(3)
#include <signal.h>

#include <cglm/cglm.h>

#include "xclient.h"
#include "vulkan.h"
#include "shader.h"

#define WIDTH 1920
#define HEIGHT 1080
//#define WIDTH 3840
//#define HEIGHT 2160

/***************************
 * Structs used by example *
 ***************************/

struct app_vk {
	VkInstance instance;
	struct kmr_vk_phdev *kmr_vk_phdev;
	struct kmr_vk_lgdev kmr_vk_lgdev;
	struct kmr_vk_queue kmr_vk_queue;

	struct kmr_vk_surface *kmr_vk_surface;
	struct kmr_vk_swapchain kmr_vk_swapchain;

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
	 * 0. CPU visible buffer that stores: (index + vertices) [Used as primary buffer if physical device CPU/INTEGRATED]
	 * 1. GPU visible vertex buffer that stores: (index + vertices)
	 */
	struct kmr_vk_buffer kmr_vk_buffer[2];
};


struct app_vk_xcb {
	struct kmr_xcb_window *kmr_xcb_window;
	struct app_vk *app_vk;
};


struct app_vertex_data {
	vec2 pos;
	vec3 color;
};


/***********
 * Globals *
 ***********/

/*
 * Comments define how to draw rectangle without index buffer
 * Actual array itself draws triangles utilizing index buffers.
 */
const struct app_vertex_data meshData[2][4] = {
	{
		{{-0.1f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // Vertex 0. Top-right- red
		{{-0.1f,0.5f}, {0.0f, 1.0f, 0.0f}},   // Vertex 1. Bottom-right - green
		{{-0.9f,0.5f}, {0.0f, 0.0f, 1.0f}},   // Vertex 2. Bottom-left- blue
		//{{-0.9f,0.5f}, {0.0f, 0.0f, 1.0f}},   // Vertex 2. Bottom-left- blue
		{{-0.9f, -0.5f}, {1.0f, 1.0f, 0.0f}}  // Vertex 3. Top-left - yellow
		//{{-0.1f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // Vertex 0. Top-right- red
	},
	{
		{{0.9f, -0.5f}, {1.0f, 0.0f, 0.0f}}, // Vertex 0. Top-right- red
		{{0.9f,0.5f}, {0.0f, 1.0f, 0.0f}},   // Vertex 1. Bottom-right - green
		{{0.1f,0.5f}, {0.0f, 0.0f, 1.0f}},   // Vertex 2. Bottom-left- blue
		//{{0.1f,0.5f}, {0.0f, 0.0f, 1.0f}},   // Vertex 2. Bottom-left- blue
		{{0.1f, -0.5f}, {1.0f, 1.0f, 0.0f}}  // Vertex 3. Top-left - yellow
		//{{0.9f, -0.5f, {1.0f, 0.0f, 0.0f}},  // Vertex 0. Top-right- red
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


static void
render (volatile bool *running, uint8_t *imageIndex, void *data)
{
	VkExtent2D extent2D = { .width = WIDTH, .height = HEIGHT };
	struct app_vk_xcb *vkxcb = (struct app_vk_xcb *) data;
	struct kmr_xcb_window UNUSED *xc = vkxcb->kmr_xcb_window;
	struct app_vk *app = vkxcb->app_vk;

	if (!app->kmr_vk_sync_obj.fenceHandles || !app->kmr_vk_sync_obj.semaphoreHandles)
		return;

	VkFence imageFence = app->kmr_vk_sync_obj.fenceHandles[0].fence;
	VkSemaphore imageSemaphore = app->kmr_vk_sync_obj.semaphoreHandles[0].semaphore;
	VkSemaphore renderSemaphore = app->kmr_vk_sync_obj.semaphoreHandles[1].semaphore;

	vkWaitForFences(app->kmr_vk_lgdev.logicalDevice, 1, &imageFence, VK_TRUE, UINT64_MAX);

	vkAcquireNextImageKHR(app->kmr_vk_lgdev.logicalDevice, app->kmr_vk_swapchain.swapchain,
	                      UINT64_MAX, imageSemaphore, VK_NULL_HANDLE, (uint32_t*) imageIndex);

	record_vk_draw_commands(app, *imageIndex, extent2D);

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

	/*
	 * Let the api know of what addresses to free and fd's to close
	 */
	appd.kmr_vk_lgdev_cnt = 1;
	appd.kmr_vk_lgdev = &app.kmr_vk_lgdev;
	appd.kmr_vk_swapchain_cnt = 1;
	appd.kmr_vk_swapchain = &app.kmr_vk_swapchain;
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
	kmr_vk_surface_destroy(app.kmr_vk_surface);
	kmr_vk_phdev_destroy(app.kmr_vk_phdev);
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
	xcbWindowCreateInfo.appName = "Triangle Index Buffer Example App";
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

	app->kmr_vk_surface = kmr_vk_surface_create(&vkSurfaceCreateInfo);
	if (!app->kmr_vk_surface)
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
	instanceCreateInfo.appName = "Triangle Index Buffer Example App";
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
	if (!app->kmr_vk_phdev)
		return -1;

	struct kmr_vk_queue_create_info queueCreateInfo;
	queueCreateInfo.physDevice = app->kmr_vk_phdev->physDevice;
	queueCreateInfo.queueFlag = VK_QUEUE_GRAPHICS_BIT;

	app->kmr_vk_queue = kmr_vk_queue_create(&queueCreateInfo);
	if (app->kmr_vk_queue.familyIndex == -1)
		return -1;

	/*
	 * Can Hardset features prior
	 * https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceFeatures.html
	 * app->kmr_vk_phdev->physDeviceFeatures.depthBiasClamp = VK_TRUE;
	 */
	struct kmr_vk_lgdev_create_info lgdevCreateInfo;
	lgdevCreateInfo.instance = app->instance;
	lgdevCreateInfo.physDevice = app->kmr_vk_phdev->physDevice;
	lgdevCreateInfo.enabledFeatures = &app->kmr_vk_phdev->physDeviceFeatures;
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

	VkSurfaceCapabilitiesKHR surfaceCapabilities = kmr_vk_get_surface_capabilities(app->kmr_vk_phdev->physDevice, app->kmr_vk_surface->surface);
	struct kmr_vk_surface_format surfaceFormats = kmr_vk_get_surface_formats(app->kmr_vk_phdev->physDevice, app->kmr_vk_surface->surface);
	struct kmr_vk_surface_present_mode surfacePresentModes = kmr_vk_get_surface_present_modes(app->kmr_vk_phdev->physDevice, app->kmr_vk_surface->surface);

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
	swapchainCreateInfo.surface = app->kmr_vk_surface->surface;
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
create_vk_swapchain_images(struct app_vk *app,
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
	imageViewCreateInfo.imageViewSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Which aspect of image to view (i.e VK_IMAGE_ASPECT_COLOR_BIT view color)
	imageViewCreateInfo.imageViewSubresourceRange.baseMipLevel = 0;                       // Start mipmap level to view from (https://en.wikipedia.org/wiki/Mipmap)
	imageViewCreateInfo.imageViewSubresourceRange.levelCount = 1;                         // Number of mipmap levels to view
	imageViewCreateInfo.imageViewSubresourceRange.baseArrayLayer = 0;                     // Start array level to view from
	imageViewCreateInfo.imageViewSubresourceRange.layerCount = 1;                         // Number of array levels to view

	struct kmr_vk_image_create_info swapchainImagesInfo;
	swapchainImagesInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	swapchainImagesInfo.swapchain = app->kmr_vk_swapchain.swapchain;
	swapchainImagesInfo.imageCount = 0;                                                   // set to zero as VkSwapchainKHR != VK_NULL_HANDLE
	swapchainImagesInfo.imageViewCreateInfos = &imageViewCreateInfo;
	/* Not creating images manually so rest of struct members can be safely ignored */
	swapchainImagesInfo.physDevice = VK_NULL_HANDLE;
	swapchainImagesInfo.imageCreateInfos = NULL;
	swapchainImagesInfo.memPropertyFlags = 0;
	swapchainImagesInfo.useExternalDmaBuffer = false;

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

	size_t singleMeshSize = sizeof(meshData[0]);
	size_t singleIndexBufferSize = sizeof(indices);

	// Create CPU visible vertex + index buffer
	struct kmr_vk_buffer_create_info vkVertexBufferCreateInfo;
	vkVertexBufferCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
	vkVertexBufferCreateInfo.physDevice = app->kmr_vk_phdev->physDevice;
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
	for (uint32_t currentVertexData = 0; currentVertexData < 2; currentVertexData++) {
		deviceMemoryCopyInfo.deviceMemoryOffset = singleIndexBufferSize + (singleMeshSize * currentVertexData);
		deviceMemoryCopyInfo.bufferData = meshData[currentVertexData];
		kmr_vk_memory_map(&deviceMemoryCopyInfo);
	}

	if (VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
		// Create GPU visible vertex buffer
		struct kmr_vk_buffer_create_info vkVertexBufferGPUCreateInfo;
		vkVertexBufferGPUCreateInfo.logicalDevice = app->kmr_vk_lgdev.logicalDevice;
		vkVertexBufferGPUCreateInfo.physDevice = app->kmr_vk_phdev->physDevice;
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
	VkDeviceSize offsets[] = {sizeof(indices), sizeof(indices) + sizeof(meshData[0]) };

	vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->kmr_vk_graphics_pipeline.graphicsPipeline);
	vkCmdBindIndexBuffer(cmdBuffer, vertexBuffer, 0, VK_INDEX_TYPE_UINT16);
	vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
	vkCmdSetScissor(cmdBuffer, 0, 1, &renderArea);

	for (uint32_t mesh = 0; mesh < ARRAY_LEN(offsets); mesh++) {
		vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, &offsets[mesh]);
		vkCmdDrawIndexed(cmdBuffer, ARRAY_LEN(indices), 1, 0, 0, 0);
	}

	vkCmdEndRenderPass(cmdBuffer);

	if (kmr_vk_command_buffer_record_end(&commandBufferRecordInfo) == -1)
		return -1;

	return 0;
}
