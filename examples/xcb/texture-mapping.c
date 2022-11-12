#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>   // For offsetof(3)
#include <stdalign.h> // _Alignas( )

#define CGLM_FORCE_DEPTH_ZERO_TO_ONE
#include <cglm/cglm.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "xclient.h"
#include "vulkan.h"
#include "shader.h"

//#define WIDTH 3840
//#define HEIGHT 2160
#define WIDTH 1920
#define HEIGHT 1080
#define MAX_SCENE_OBJECTS 2
#define PRECEIVED_SWAPCHAIN_IMAGE_SIZE 5

struct uvr_vk {
  VkInstance instance;
  struct uvr_vk_phdev uvr_vk_phdev;
  struct uvr_vk_lgdev uvr_vk_lgdev;
  struct uvr_vk_queue uvr_vk_queue;

  VkSurfaceKHR surface;
  struct uvr_vk_swapchain uvr_vk_swapchain;

  /*
   * 0. Swapchain Images
   * 1. Depth Image
   * 2. Texture Image
   */
  struct uvr_vk_image uvr_vk_image[3];

  /*
   * 0. Vertex Shader Model
   * 1. Fragment Shader Model
   */
  struct uvr_vk_shader_module uvr_vk_shader_module[2];

  struct uvr_vk_pipeline_layout uvr_vk_pipeline_layout;
  struct uvr_vk_render_pass uvr_vk_render_pass;
  struct uvr_vk_graphics_pipeline uvr_vk_graphics_pipeline;
  struct uvr_vk_framebuffer uvr_vk_framebuffer;
  struct uvr_vk_command_buffer uvr_vk_command_buffer;
  struct uvr_vk_sync_obj uvr_vk_sync_obj;

  /*
   * 0. CPU visible transfer buffer that stores: (index + vertices)
   * 1. GPU visible vertex buffer that stores: (index + vertices)
   * 2. CPU visible uniform buffer
   * 3. CPU visible transfer buffer (stores image pixel data)
   */
  struct uvr_vk_buffer uvr_vk_buffer[4];

  struct uvr_vk_descriptor_set_layout uvr_vk_descriptor_set_layout;
  struct uvr_vk_descriptor_set uvr_vk_descriptor_set;

  /*
   * Texture samplers
   */
  struct uvr_vk_sampler uvr_vk_sampler;
};

struct uvr_vk_xcb {
  struct uvr_utils_aligned_buffer *model_transfer_space;
  struct uvr_xcb_window *uvr_xcb_window;
  struct uvr_vk *uvr_vk;
};


struct uvr_vertex_data {
  vec3 pos;
  vec3 color;
  vec2 texCoord;
};


struct uvr_uniform_buffer_model {
  mat4 model;
};


struct uvr_uniform_buffer {
  mat4 view;
  mat4 proj;
};


/*
 * Comments define how to draw rectangle without index buffer
 * Actual array itself draws triangles utilizing index buffers.
 */
const struct uvr_vertex_data meshData[2][4] = {
  {
    {{-0.1f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},   // Vertex 0. Top-right    - red
    {{-0.1f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},   // Vertex 1. Bottom-right - green
    {{-0.9f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},   // Vertex 2. Bottom-left  - blue
    //{{-0.9f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // Vertex 2. Bottom-left  - blue
    {{-0.9f, -0.5f, 0.0f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}    // Vertex 3. Top-left     - yellow
    //{{-0.1f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}}, // Vertex 0. Top-right    - red
  },
  {
    {{0.9f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},   // Vertex 0. Top-right    - red
    {{0.9f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},   // Vertex 1. Bottom-right - green
    {{0.1f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},   // Vertex 2. Bottom-left  - blue
    //{{0.1f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}, // Vertex 2. Bottom-left  - blue
    {{0.1f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}, {0.0f, 0.0f}}    // Vertex 3. Top-left     - yellow
    //{{0.9f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // Vertex 0. Top-right    - red
    }
};


/*
 * Defines what vertices in vertex array are reusable
 * inorder to draw a rectangle
 */
const uint16_t indices[] = {
  0, 1, 2, 2, 3, 0
};


int create_xcb_vk_surface(struct uvr_vk *app, struct uvr_xcb_window *xc);
int create_vk_instance(struct uvr_vk *uvrvk);
int create_vk_device(struct uvr_vk *app);
int create_vk_swapchain(struct uvr_vk *app, VkSurfaceFormatKHR *surfaceFormat, VkExtent2D extent2D);
int create_vk_swapchain_images(struct uvr_vk *app, VkSurfaceFormatKHR *surfaceFormat);
int create_vk_depth_image(struct uvr_vk *app);
int create_vk_images(struct uvr_vk *app, VkSurfaceFormatKHR *surfaceFormat);
int create_vk_shader_modules(struct uvr_vk *app);
int create_vk_command_buffers(struct uvr_vk *app);
int create_vk_buffers(struct uvr_vk *app, struct uvr_utils_aligned_buffer *modelTransferSpace);
int create_vk_texture_image(struct uvr_vk *app, VkSurfaceFormatKHR *surfaceFormat);
int create_vk_image_sampler(struct uvr_vk *app);
int create_vk_resource_descriptor_sets(struct uvr_vk *app, struct uvr_utils_aligned_buffer *modelTransferSpace);
int create_vk_graphics_pipeline(struct uvr_vk *app, VkSurfaceFormatKHR *surfaceFormat, VkExtent2D extent2D);
int create_vk_framebuffers(struct uvr_vk *app, VkExtent2D extent2D);
int create_vk_sync_objs(struct uvr_vk *app);
int record_vk_draw_commands(struct uvr_vk *app, struct uvr_utils_aligned_buffer *modelTransferSpace, uint32_t swapchainImageIndex, VkExtent2D extent2D);
void update_uniform_buffer(struct uvr_vk *app, struct uvr_utils_aligned_buffer *modelTransferSpace, uint32_t swapchainImageIndex, VkExtent2D extent2D);


void render(bool UNUSED *running, uint32_t *imageIndex, void *data)
{
  VkExtent2D extent2D = { .width = WIDTH, .height = HEIGHT };
  struct uvr_vk_xcb *vkxcb = (struct uvr_vk_xcb *) data;
  struct uvr_utils_aligned_buffer *modelTransferSpace = vkxcb->model_transfer_space;
  struct uvr_xcb_window UNUSED *xc = vkxcb->uvr_xcb_window;
  struct uvr_vk *app = vkxcb->uvr_vk;

  if (!app->uvr_vk_sync_obj.fenceHandles || !app->uvr_vk_sync_obj.semaphoreHandles)
    return;

  VkFence imageFence = app->uvr_vk_sync_obj.fenceHandles[0].fence;
  VkSemaphore imageSemaphore = app->uvr_vk_sync_obj.semaphoreHandles[0].semaphore;
  VkSemaphore renderSemaphore = app->uvr_vk_sync_obj.semaphoreHandles[1].semaphore;

  vkWaitForFences(app->uvr_vk_lgdev.logicalDevice, 1, &imageFence, VK_TRUE, UINT64_MAX);

  vkAcquireNextImageKHR(app->uvr_vk_lgdev.logicalDevice, app->uvr_vk_swapchain.swapchain, UINT64_MAX, imageSemaphore, VK_NULL_HANDLE, imageIndex);

  record_vk_draw_commands(app, modelTransferSpace, *imageIndex, extent2D);
  update_uniform_buffer(app, modelTransferSpace, *imageIndex, extent2D);

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
  submitInfo.pCommandBuffers = &app->uvr_vk_command_buffer.commandBufferHandles[0].commandBuffer;
  submitInfo.signalSemaphoreCount = ARRAY_LEN(signalSemaphores);
  submitInfo.pSignalSemaphores = signalSemaphores;

  vkResetFences(app->uvr_vk_lgdev.logicalDevice, 1, &imageFence);

  /* Submit draw command */
  vkQueueSubmit(app->uvr_vk_queue.queue, 1, &submitInfo, imageFence);

  VkPresentInfoKHR presentInfo;
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = NULL;
  presentInfo.waitSemaphoreCount = ARRAY_LEN(signalSemaphores);
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &app->uvr_vk_swapchain.swapchain;
  presentInfo.pImageIndices = imageIndex;
  presentInfo.pResults = NULL;

  vkQueuePresentKHR(app->uvr_vk_queue.queue, &presentInfo);
}


/*
 * Example code demonstrating how use Vulkan with Wayland
 */
int main(void)
{
  struct uvr_vk app;
  struct uvr_vk_destroy appd;
  memset(&app, 0, sizeof(app));
  memset(&appd, 0, sizeof(appd));

  struct uvr_xcb_window xc;
  struct uvr_xcb_destroy xcd;
  memset(&xc, 0, sizeof(xc));
  memset(&xcd, 0, sizeof(xcd));

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

  struct uvr_utils_aligned_buffer modelTransferSpace;
  memset(&modelTransferSpace, 0, sizeof(struct uvr_utils_aligned_buffer));

  if (create_vk_buffers(&app, &modelTransferSpace) == -1)
    goto exit_error;

  if (create_vk_texture_image(&app, &surfaceFormat) == -1)
    goto exit_error;

  if (create_vk_image_sampler(&app) == -1)
    goto exit_error;

  if (create_vk_resource_descriptor_sets(&app, &modelTransferSpace) == -1)
    goto exit_error;

  if (create_vk_graphics_pipeline(&app, &surfaceFormat, extent2D) == -1)
    goto exit_error;

  if (create_vk_framebuffers(&app, extent2D) == -1)
    goto exit_error;

  if (create_vk_sync_objs(&app) == -1)
    goto exit_error;

  /* Map the window to the screen */
  uvr_xcb_window_make_visible(&xc);

  static uint32_t cbuf = 0;
  static bool running = true;

  static struct uvr_vk_xcb vkxc;
  vkxc.uvr_xcb_window = &xc;
  vkxc.uvr_vk = &app;
  vkxc.model_transfer_space = &modelTransferSpace;

  struct uvr_xcb_window_handle_event_info eventInfo;
  eventInfo.xcbWindowObject = &xc;
  eventInfo.renderer = render;
  eventInfo.rendererData = &vkxc;
  eventInfo.rendererCurrentBuffer = &cbuf;
  eventInfo.rendererRunning = &running;

  while (uvr_xcb_window_handle_event(&eventInfo) && running) {
    // Initentionally left blank
  }


exit_error:
  free(modelTransferSpace.alignedBufferMemory);

  /*
   * Let the api know of what addresses to free and fd's to close
   */
  appd.instance = app.instance;
  appd.surface = app.surface;
  appd.uvr_vk_lgdev_cnt = 1;
  appd.uvr_vk_lgdev = &app.uvr_vk_lgdev;
  appd.uvr_vk_swapchain_cnt = 1;
  appd.uvr_vk_swapchain = &app.uvr_vk_swapchain;
  appd.uvr_vk_image_cnt = ARRAY_LEN(app.uvr_vk_image);
  appd.uvr_vk_image = app.uvr_vk_image;
  appd.uvr_vk_shader_module_cnt = ARRAY_LEN(app.uvr_vk_shader_module);
  appd.uvr_vk_shader_module = app.uvr_vk_shader_module;
  appd.uvr_vk_pipeline_layout_cnt = 1;
  appd.uvr_vk_pipeline_layout = &app.uvr_vk_pipeline_layout;
  appd.uvr_vk_render_pass_cnt = 1;
  appd.uvr_vk_render_pass = &app.uvr_vk_render_pass;
  appd.uvr_vk_graphics_pipeline_cnt = 1;
  appd.uvr_vk_graphics_pipeline = &app.uvr_vk_graphics_pipeline;
  appd.uvr_vk_framebuffer_cnt = 1;
  appd.uvr_vk_framebuffer = &app.uvr_vk_framebuffer;
  appd.uvr_vk_command_buffer_cnt = 1;
  appd.uvr_vk_command_buffer = &app.uvr_vk_command_buffer;
  appd.uvr_vk_sync_obj_cnt = 1;
  appd.uvr_vk_sync_obj = &app.uvr_vk_sync_obj;
  appd.uvr_vk_buffer_cnt = ARRAY_LEN(app.uvr_vk_buffer);
  appd.uvr_vk_buffer = app.uvr_vk_buffer;
  appd.uvr_vk_descriptor_set_layout_cnt = 1;
  appd.uvr_vk_descriptor_set_layout = &app.uvr_vk_descriptor_set_layout;
  appd.uvr_vk_descriptor_set_cnt = 1;
  appd.uvr_vk_descriptor_set = &app.uvr_vk_descriptor_set;
  appd.uvr_vk_sampler_cnt = 1;
  appd.uvr_vk_sampler = &app.uvr_vk_sampler;
  uvr_vk_destory(&appd);

  xcd.uvr_xcb_window = xc;
  uvr_xcb_destory(&xcd);
  return 0;
}


int create_xcb_vk_surface(struct uvr_vk *app, struct uvr_xcb_window *xc)
{
  /*
   * Create xcb client
   */
  struct uvr_xcb_window_create_info xcbWindowCreateInfo;
  xcbWindowCreateInfo.display = NULL;
  xcbWindowCreateInfo.screen = NULL;
  xcbWindowCreateInfo.appName = "Texture Mapping Example App";
  xcbWindowCreateInfo.width = WIDTH;
  xcbWindowCreateInfo.height = HEIGHT;
  xcbWindowCreateInfo.fullscreen = false;
  xcbWindowCreateInfo.transparent = false;

  *xc = uvr_xcb_window_create(&xcbWindowCreateInfo);
  if (!xc->conn)
    return -1;

  /*
   * Create Vulkan Surface
   */
  struct uvr_vk_surface_create_info vkSurfaceCreateInfo;
  vkSurfaceCreateInfo.surfaceType = UVR_SURFACE_XCB_CLIENT;
  vkSurfaceCreateInfo.instance = app->instance;
  vkSurfaceCreateInfo.display = xc->conn;
  vkSurfaceCreateInfo.window = xc->window;

  app->surface = uvr_vk_surface_create(&vkSurfaceCreateInfo);
  if (!app->surface)
    return -1;

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

  struct uvr_vk_instance_create_info instanceCreateInfo;
  instanceCreateInfo.appName = "Texture Mapping Example App";
  instanceCreateInfo.engineName = "No Engine";
  instanceCreateInfo.enabledLayerCount = ARRAY_LEN(validationLayers);
  instanceCreateInfo.enabledLayerNames = validationLayers;
  instanceCreateInfo.enabledExtensionCount = ARRAY_LEN(instanceExtensions);
  instanceCreateInfo.enabledExtensionNames = instanceExtensions;

  app->instance = uvr_vk_instance_create(&instanceCreateInfo);
  if (!app->instance) return -1;

  return 0;
}


int create_vk_device(struct uvr_vk *app)
{
  const char *deviceExtensions[] = {
    "VK_KHR_swapchain"
  };

  struct uvr_vk_phdev_create_info phdevCreateInfo;
  phdevCreateInfo.instance = app->instance;
  phdevCreateInfo.deviceType = VK_PHYSICAL_DEVICE_TYPE;
#ifdef INCLUDE_KMS
  phdevCreateInfo.kmsfd = -1;
#endif

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
  app->uvr_vk_phdev.physDeviceFeatures.samplerAnisotropy = VK_TRUE;

  struct uvr_vk_lgdev_create_info lgdevCreateInfo;
  lgdevCreateInfo.instance = app->instance;
  lgdevCreateInfo.physDevice = app->uvr_vk_phdev.physDevice;
  lgdevCreateInfo.enabledFeatures = &app->uvr_vk_phdev.physDeviceFeatures;
  lgdevCreateInfo.enabledExtensionCount = ARRAY_LEN(deviceExtensions);
  lgdevCreateInfo.enabledExtensionNames = deviceExtensions;
  lgdevCreateInfo.queueCount = 1;
  lgdevCreateInfo.queues = &app->uvr_vk_queue;

  app->uvr_vk_lgdev = uvr_vk_lgdev_create(&lgdevCreateInfo);
  if (!app->uvr_vk_lgdev.logicalDevice)
    return -1;

  return 0;
}


/* choose swap chain surface format & present mode */
int create_vk_swapchain(struct uvr_vk *app, VkSurfaceFormatKHR *surfaceFormat, VkExtent2D extent2D)
{
  VkPresentModeKHR presentMode;

  VkSurfaceCapabilitiesKHR surfaceCapabilities = uvr_vk_get_surface_capabilities(app->uvr_vk_phdev.physDevice, app->surface);
  struct uvr_vk_surface_format surfaceFormats = uvr_vk_get_surface_formats(app->uvr_vk_phdev.physDevice, app->surface);
  struct uvr_vk_surface_present_mode surfacePresentModes = uvr_vk_get_surface_present_modes(app->uvr_vk_phdev.physDevice, app->surface);

  /* Choose surface format based */
  for (uint32_t s = 0; s < surfaceFormats.surfaceFormatCount; s++) {
    if (surfaceFormats.surfaceFormats[s].format == VK_FORMAT_B8G8R8A8_SRGB && surfaceFormats.surfaceFormats[s].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      *surfaceFormat = surfaceFormats.surfaceFormats[s];
    }
  }

  for (uint32_t p = 0; p < surfacePresentModes.presentModeCount; p++) {
    if (surfacePresentModes.presentModes[p] == VK_PRESENT_MODE_MAILBOX_KHR) {
      presentMode = surfacePresentModes.presentModes[p];
    }
  }

  free(surfaceFormats.surfaceFormats); surfaceFormats.surfaceFormats = NULL;
  free(surfacePresentModes.presentModes); surfacePresentModes.presentModes = NULL;

  struct uvr_vk_swapchain_create_info swapchainCreateInfo;
  swapchainCreateInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
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

  app->uvr_vk_swapchain = uvr_vk_swapchain_create(&swapchainCreateInfo);
  if (!app->uvr_vk_swapchain.swapchain)
    return -1;

  return 0;
}


int create_vk_swapchain_images(struct uvr_vk *app, VkSurfaceFormatKHR *surfaceFormat)
{
  struct uvr_vk_image_create_info swapchainImagesInfo;
  swapchainImagesInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
  swapchainImagesInfo.swapchain = app->uvr_vk_swapchain.swapchain;
  swapchainImagesInfo.imageCount = 0;                                                // set to zero as VkSwapchainKHR != VK_NULL_HANDLE
  swapchainImagesInfo.imageViewflags = 0;
  swapchainImagesInfo.imageViewType = VK_IMAGE_VIEW_TYPE_2D;
  swapchainImagesInfo.imageViewFormat = surfaceFormat->format;
  swapchainImagesInfo.imageViewComponents.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  swapchainImagesInfo.imageViewComponents.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  swapchainImagesInfo.imageViewComponents.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  swapchainImagesInfo.imageViewComponents.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  swapchainImagesInfo.imageViewSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // Which aspect of image to view (i.e VK_IMAGE_ASPECT_COLOR_BIT view color)
  swapchainImagesInfo.imageViewSubresourceRange.baseMipLevel = 0;                        // Start mipmap level to view from (https://en.wikipedia.org/wiki/Mipmap)
  swapchainImagesInfo.imageViewSubresourceRange.levelCount = 1;                          // Number of mipmap levels to view
  swapchainImagesInfo.imageViewSubresourceRange.baseArrayLayer = 0;                      // Start array level to view from
  swapchainImagesInfo.imageViewSubresourceRange.layerCount = 1;                          // Number of array levels to view
  /* Not create images manually so rest of struct members can be safely ignored */
  swapchainImagesInfo.physDevice = VK_NULL_HANDLE;
  swapchainImagesInfo.memPropertyFlags = 0;
  swapchainImagesInfo.imageflags = 0;
  swapchainImagesInfo.imageType = 0;
  swapchainImagesInfo.imageExtent3D = (VkExtent3D) { 0, 0, 0 };
  swapchainImagesInfo.imageMipLevels = 0;
  swapchainImagesInfo.imageArrayLayers = 0;
  swapchainImagesInfo.imageSamples = 0;
  swapchainImagesInfo.imageTiling = 0;
  swapchainImagesInfo.imageUsage = 0;
  swapchainImagesInfo.imageSharingMode = 0;
  swapchainImagesInfo.imageQueueFamilyIndexCount = 0;
  swapchainImagesInfo.imageQueueFamilyIndices = NULL;
  swapchainImagesInfo.imageInitialLayout = 0;

  app->uvr_vk_image[0] = uvr_vk_image_create(&swapchainImagesInfo);
  if (!app->uvr_vk_image[0].imageViewHandles[0].view)
    return -1;

  return 0;
}


VkFormat choose_depth_image_format(struct uvr_vk *app, VkImageTiling imageTiling, VkFormatFeatureFlags formatFeatureFlags)
{
  VkFormat formats[3] = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT };
  VkFormat format = VK_FORMAT_UNDEFINED;

  struct uvr_vk_phdev_format_prop physDeviceFormatProps = uvr_vk_get_phdev_format_properties(app->uvr_vk_phdev.physDevice, formats, ARRAY_LEN(formats));
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


int create_vk_depth_image(struct uvr_vk *app)
{
  VkImageTiling imageTiling = VK_IMAGE_TILING_OPTIMAL;

  VkFormat imageFormat = choose_depth_image_format(app, imageTiling, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
  if (imageFormat == VK_FORMAT_UNDEFINED)
    return -1;

  struct uvr_vk_image_create_info imageCreateInfo;
  imageCreateInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
  imageCreateInfo.swapchain = VK_NULL_HANDLE;
  imageCreateInfo.imageCount = 1;
  imageCreateInfo.imageViewflags = 0;
  imageCreateInfo.imageViewType = VK_IMAGE_VIEW_TYPE_2D;
  imageCreateInfo.imageViewFormat = imageFormat;
  imageCreateInfo.imageViewComponents = (VkComponentMapping) { .r = 0, .g = 0, .b = 0, .a = 0 };
  imageCreateInfo.imageViewSubresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  imageCreateInfo.imageViewSubresourceRange.baseMipLevel = 0;
  imageCreateInfo.imageViewSubresourceRange.levelCount = 1;
  imageCreateInfo.imageViewSubresourceRange.baseArrayLayer = 0;
  imageCreateInfo.imageViewSubresourceRange.layerCount = 1;
  imageCreateInfo.physDevice = app->uvr_vk_phdev.physDevice;
  imageCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  imageCreateInfo.imageflags = 0;
  imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imageCreateInfo.imageExtent3D = (VkExtent3D) { .width = WIDTH, .height = HEIGHT, .depth = 1 }; // depth describes how deep the image goes (1 because no 3D aspect)
  imageCreateInfo.imageMipLevels = 1;
  imageCreateInfo.imageArrayLayers = 1;
  imageCreateInfo.imageSamples = VK_SAMPLE_COUNT_1_BIT;
  imageCreateInfo.imageTiling = imageTiling;
  imageCreateInfo.imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageCreateInfo.imageQueueFamilyIndexCount = 0;
  imageCreateInfo.imageQueueFamilyIndices = NULL;
  imageCreateInfo.imageInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  app->uvr_vk_image[1] = uvr_vk_image_create(&imageCreateInfo);
  if (!app->uvr_vk_image[1].imageHandles[0].image && !app->uvr_vk_image[1].imageViewHandles[0].view)
    return -1;

  return 0;
}


int create_vk_shader_modules(struct uvr_vk *app)
{
  int ret = 0;

  struct uvr_shader_destroy shaderd;
  memset(&shaderd, 0, sizeof(shaderd));

#ifdef INCLUDE_SHADERC
  const char vertexShader[] =
    "#version 450\n" // GLSL 4.5
    "#extension GL_ARB_separate_shader_objects : enable\n\n"
    "layout(location = 0) out vec3 outColor;\n"
    "layout(location = 1) out vec2 outTexCoord;\n\n"
    "layout(location = 0) in vec3 inPosition;\n"
    "layout(location = 1) in vec3 inColor;\n"
    "layout(location = 2) in vec2 inTexCoord;\n\n"
    "layout(set = 0, binding = 0) uniform uniform_buffer {\n"
    "  mat4 view;\n"
    "  mat4 proj;\n"
    "} uboViewProjection;\n"
    "layout(set = 0, binding = 1) uniform uniform_buffer_model {\n"
    "  mat4 model;\n"
    "} uboModel;\n\n"
    "void main() {\n"
    "  gl_Position = uboViewProjection.proj * uboViewProjection.view * uboModel.model * vec4(inPosition, 1.0);\n"
    "  outColor = inColor;\n"
    "  outTexCoord = inTexCoord;\n"
    "}";

  const char fragmentShader[] =
    "#version 450\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "layout(location = 0) in vec3 inColor;\n"
    "layout(location = 1) in vec2 inTexCoord;\n\n"
    "layout(location = 0) out vec4 outColor;\n"
    "layout(set = 0, binding = 2) uniform sampler2D outTexSampler;\n"
    "void main() { outColor = vec4(inColor * texture(outTexSampler, inTexCoord).rgb, 1.0); }";

  struct uvr_shader_spirv_create_info vertexShaderCreateInfo;
  vertexShaderCreateInfo.kind = VK_SHADER_STAGE_VERTEX_BIT;
  vertexShaderCreateInfo.source = vertexShader;
  vertexShaderCreateInfo.filename = "vert.spv";
  vertexShaderCreateInfo.entryPoint = "main";

  struct uvr_shader_spirv_create_info fragmentShaderCreateInfo;
  fragmentShaderCreateInfo.kind = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragmentShaderCreateInfo.source = fragmentShader;
  fragmentShaderCreateInfo.filename = "frag.spv";
  fragmentShaderCreateInfo.entryPoint = "main";

  /*
   * 0. Vertex Shader
   * 1. Fragment Shader
   */
  struct uvr_shader_spirv uvr_shader[2];

  uvr_shader[0] = uvr_shader_compile_buffer_to_spirv(&vertexShaderCreateInfo);
  if (!uvr_shader[0].bytes) { ret = -1 ; goto exit_distroy_shader ; }

  uvr_shader[1] = uvr_shader_compile_buffer_to_spirv(&fragmentShaderCreateInfo);
  if (!uvr_shader[1].bytes) { ret = -1 ; goto exit_distroy_shader ; }

#else
  /*
   * 0. Vertex Shader
   * 1. Fragment Shader
   */
  struct uvr_shader_file uvr_shader[2];

  uvr_shader[0] = uvr_shader_file_load(VERTEX_SHADER_SPIRV);
  if (!uvr_shader[0].bytes) { ret = -1 ; goto exit_distroy_shader ; }

  uvr_shader[1] = uvr_shader_file_load(FRAGMENT_SHADER_SPIRV);
  if (!uvr_shader[1].bytes) { ret = -1 ; goto exit_distroy_shader ; }

#endif

  const char *shaderModuleNames[] = {
    "vertex", "fragment"
  };

  struct uvr_vk_shader_module_create_info shaderModuleCreateInfo;
  for (uint32_t currentShader = 0; currentShader < ARRAY_LEN(uvr_shader); currentShader++) {
    shaderModuleCreateInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
    shaderModuleCreateInfo.sprivByteSize = uvr_shader[currentShader].byteSize;
    shaderModuleCreateInfo.sprivBytes = uvr_shader[currentShader].bytes;
    shaderModuleCreateInfo.shaderName = shaderModuleNames[currentShader];

    app->uvr_vk_shader_module[currentShader] = uvr_vk_shader_module_create(&shaderModuleCreateInfo);
    if (!app->uvr_vk_shader_module[currentShader].shaderModule) { ret = -1 ; goto exit_distroy_shader ; }
  }

exit_distroy_shader:
#ifdef INCLUDE_SHADERC
  shaderd.uvr_shader_spirv_cnt = ARRAY_LEN(uvr_shader);
  shaderd.uvr_shader_spirv = uvr_shader;
#else
  shaderd.uvr_shader_file_cnt = ARRAY_LEN(uvr_shader);
  shaderd.uvr_shader_file = uvr_shader;
#endif
  uvr_shader_destroy(&shaderd);

  return ret;
}


int create_vk_command_buffers(struct uvr_vk *app)
{
  struct uvr_vk_command_buffer_create_info commandBufferCreateInfo;
  commandBufferCreateInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
  commandBufferCreateInfo.queueFamilyIndex = app->uvr_vk_queue.familyIndex;
  commandBufferCreateInfo.commandBufferCount = 1;

  app->uvr_vk_command_buffer = uvr_vk_command_buffer_create(&commandBufferCreateInfo);
  if (!app->uvr_vk_command_buffer.commandPool)
    return -1;

  return 0;
}


int create_vk_buffers(struct uvr_vk *app, struct uvr_utils_aligned_buffer *modelTransferSpace)
{
  uint32_t cpuVisibleBuffer = 0, gpuVisibleBuffer = 1;
  void *data = NULL;

  size_t singleMeshSize = sizeof(meshData[0]);
  size_t singleIndexBufferSize = sizeof(indices);

  // Create CPU visible vertex + index buffer
  struct uvr_vk_buffer_create_info vkVertexBufferCreateInfo;
  vkVertexBufferCreateInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
  vkVertexBufferCreateInfo.physDevice = app->uvr_vk_phdev.physDevice;
  vkVertexBufferCreateInfo.bufferFlags = 0;
  vkVertexBufferCreateInfo.bufferSize = singleIndexBufferSize + sizeof(meshData);
  vkVertexBufferCreateInfo.bufferUsage = (VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU || \
                                          VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_CPU) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT : VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  vkVertexBufferCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkVertexBufferCreateInfo.queueFamilyIndexCount = 0;
  vkVertexBufferCreateInfo.queueFamilyIndices = NULL;
  vkVertexBufferCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  app->uvr_vk_buffer[cpuVisibleBuffer] = uvr_vk_buffer_create(&vkVertexBufferCreateInfo);
  if (!app->uvr_vk_buffer[cpuVisibleBuffer].buffer || !app->uvr_vk_buffer[cpuVisibleBuffer].deviceMemory)
    return -1;

  // Copy index data into CPU visible index buffer
  vkMapMemory(app->uvr_vk_lgdev.logicalDevice, app->uvr_vk_buffer[cpuVisibleBuffer].deviceMemory, 0, singleIndexBufferSize, 0, &data);
  memcpy(data, indices, singleIndexBufferSize);
  vkUnmapMemory(app->uvr_vk_lgdev.logicalDevice, app->uvr_vk_buffer[cpuVisibleBuffer].deviceMemory);
  data = NULL;

  // Copy vertex data into CPU visible vertex buffer
  for (uint32_t currentVertexData = 0; currentVertexData < 2; currentVertexData++) {
    vkMapMemory(app->uvr_vk_lgdev.logicalDevice, app->uvr_vk_buffer[cpuVisibleBuffer].deviceMemory, singleIndexBufferSize + (singleMeshSize * currentVertexData), singleMeshSize, 0, &data);
    memcpy(data, meshData[currentVertexData], singleMeshSize);
    vkUnmapMemory(app->uvr_vk_lgdev.logicalDevice, app->uvr_vk_buffer[cpuVisibleBuffer].deviceMemory);
    data = NULL;
  }

  if (VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    // Create GPU visible vertex buffer
    struct uvr_vk_buffer_create_info vkVertexBufferGPUCreateInfo;
    vkVertexBufferGPUCreateInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
    vkVertexBufferGPUCreateInfo.physDevice = app->uvr_vk_phdev.physDevice;
    vkVertexBufferGPUCreateInfo.bufferFlags = 0;
    vkVertexBufferGPUCreateInfo.bufferSize = vkVertexBufferCreateInfo.bufferSize;
    vkVertexBufferGPUCreateInfo.bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    vkVertexBufferGPUCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkVertexBufferGPUCreateInfo.queueFamilyIndexCount = 0;
    vkVertexBufferGPUCreateInfo.queueFamilyIndices = NULL;
    vkVertexBufferGPUCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    app->uvr_vk_buffer[gpuVisibleBuffer] = uvr_vk_buffer_create(&vkVertexBufferGPUCreateInfo);
    if (!app->uvr_vk_buffer[gpuVisibleBuffer].buffer || !app->uvr_vk_buffer[gpuVisibleBuffer].deviceMemory)
      return -1;

    VkBufferCopy copyRegion;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = vkVertexBufferCreateInfo.bufferSize;

    // Copy contents from CPU visible buffer over to GPU visible buffer
    struct uvr_vk_resource_copy_info bufferCopyInfo;
    bufferCopyInfo.resourceCopyType = UVR_VK_COPY_VK_BUFFER_TO_VK_BUFFER;
    bufferCopyInfo.commandBuffer = app->uvr_vk_command_buffer.commandBufferHandles[0].commandBuffer;
    bufferCopyInfo.queue = app->uvr_vk_queue.queue;
    bufferCopyInfo.srcResource = app->uvr_vk_buffer[cpuVisibleBuffer].buffer;
    bufferCopyInfo.dstResource = app->uvr_vk_buffer[gpuVisibleBuffer].buffer;
    bufferCopyInfo.bufferCopyInfo = &copyRegion;
    bufferCopyInfo.bufferImageCopyInfo = NULL;
    bufferCopyInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (uvr_vk_resource_copy(&bufferCopyInfo) == -1)
      return -1;
  }

  cpuVisibleBuffer+=2;
  gpuVisibleBuffer+=2;

  struct uvr_utils_aligned_buffer_create_info modelUniformBufferAlignment;
  modelUniformBufferAlignment.bytesToAlign = sizeof(struct uvr_uniform_buffer_model);
  modelUniformBufferAlignment.bytesToAlignCount = MAX_SCENE_OBJECTS;
  modelUniformBufferAlignment.bufferAlignment = app->uvr_vk_phdev.physDeviceProperties.limits.minUniformBufferOffsetAlignment;

  *modelTransferSpace = uvr_utils_aligned_buffer_create(&modelUniformBufferAlignment);
  if (!modelTransferSpace->alignedBufferMemory)
    return -1;

  // Create CPU visible uniform buffer to store (view projection matrices in first have) (Dynamic uniform buffer (model matrix) in second half)
  struct uvr_vk_buffer_create_info vkUniformBufferCreateInfo;
  vkUniformBufferCreateInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
  vkUniformBufferCreateInfo.physDevice = app->uvr_vk_phdev.physDevice;
  vkUniformBufferCreateInfo.bufferFlags = 0;
  vkUniformBufferCreateInfo.bufferSize = (sizeof(struct uvr_uniform_buffer) * PRECEIVED_SWAPCHAIN_IMAGE_SIZE) + \
                                         (modelTransferSpace->bufferAlignment * MAX_SCENE_OBJECTS);
  vkUniformBufferCreateInfo.bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  vkUniformBufferCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkUniformBufferCreateInfo.queueFamilyIndexCount = 0;
  vkUniformBufferCreateInfo.queueFamilyIndices = NULL;
  vkUniformBufferCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  app->uvr_vk_buffer[cpuVisibleBuffer] = uvr_vk_buffer_create(&vkUniformBufferCreateInfo);
  if (!app->uvr_vk_buffer[cpuVisibleBuffer].buffer || !app->uvr_vk_buffer[cpuVisibleBuffer].deviceMemory)
    return -1;

  return 0;
}


int create_vk_texture_image(struct uvr_vk *app, VkSurfaceFormatKHR *surfaceFormat)
{
  int textureWidth, textureHeight, textureChannels, textureImageIndex = 2, imageTransferIndex = 0;
  stbi_uc *pixels = stbi_load(TEXTURE_IMAGE, &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
  if (!pixels) {
    uvr_utils_log(UVR_DANGER, "[x] stbi_load: failed to load %s", TEXTURE_IMAGE);
    return -1;
  }

  VkDeviceSize imageSize = textureWidth * textureHeight * STBI_rgb_alpha;
  uint32_t cpuVisibleImageBuffer = 3;

  // Create CPU visible buffer to store pixel data
  struct uvr_vk_buffer_create_info vkTextureBufferCreateInfo;
  vkTextureBufferCreateInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
  vkTextureBufferCreateInfo.physDevice = app->uvr_vk_phdev.physDevice;
  vkTextureBufferCreateInfo.bufferFlags = 0;
  vkTextureBufferCreateInfo.bufferSize = imageSize;
  vkTextureBufferCreateInfo.bufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  vkTextureBufferCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkTextureBufferCreateInfo.queueFamilyIndexCount = 0;
  vkTextureBufferCreateInfo.queueFamilyIndices = NULL;
  vkTextureBufferCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  app->uvr_vk_buffer[cpuVisibleImageBuffer] = uvr_vk_buffer_create(&vkTextureBufferCreateInfo);
  if (!app->uvr_vk_buffer[cpuVisibleImageBuffer].buffer || !app->uvr_vk_buffer[cpuVisibleImageBuffer].deviceMemory) {
    stbi_image_free(pixels);
    return -1;
  }

  void *data = NULL;
  vkMapMemory(app->uvr_vk_lgdev.logicalDevice, app->uvr_vk_buffer[cpuVisibleImageBuffer].deviceMemory, 0, imageSize, 0, &data);
  memcpy(data, pixels, imageSize);
  vkUnmapMemory(app->uvr_vk_lgdev.logicalDevice, app->uvr_vk_buffer[cpuVisibleImageBuffer].deviceMemory);

  stbi_image_free(pixels);

  struct uvr_vk_image_create_info vkImageCreateInfo;
  vkImageCreateInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
  vkImageCreateInfo.swapchain = VK_NULL_HANDLE;                                     // set VkSwapchainKHR to VK_NULL_HANDLE as we manually create images
  vkImageCreateInfo.imageCount = 1;                                                 // Only creating 1 VkImage resource to store pixel data
  vkImageCreateInfo.imageViewflags = 0;
  vkImageCreateInfo.imageViewType = VK_IMAGE_VIEW_TYPE_2D;
  vkImageCreateInfo.imageViewFormat = surfaceFormat->format;
  vkImageCreateInfo.imageViewComponents = (VkComponentMapping) { .r = 0, .g = 0, .b = 0, .a = 0 };
  vkImageCreateInfo.imageViewSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // Which aspect of image to view (i.e VK_IMAGE_ASPECT_COLOR_BIT view color)
  vkImageCreateInfo.imageViewSubresourceRange.baseMipLevel = 0;                        // Start mipmap level to view from (https://en.wikipedia.org/wiki/Mipmap)
  vkImageCreateInfo.imageViewSubresourceRange.levelCount = 1;                          // Number of mipmap levels to view
  vkImageCreateInfo.imageViewSubresourceRange.baseArrayLayer = 0;                      // Start array level to view from
  vkImageCreateInfo.imageViewSubresourceRange.layerCount = 1;                          // Number of array levels to view
  vkImageCreateInfo.physDevice = app->uvr_vk_phdev.physDevice;
  vkImageCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
  vkImageCreateInfo.imageflags = 0;
  vkImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  vkImageCreateInfo.imageExtent3D = (VkExtent3D) { .width = textureWidth, .height = textureHeight, .depth = 1 };
  vkImageCreateInfo.imageMipLevels = 1;
  vkImageCreateInfo.imageArrayLayers = 1;
  vkImageCreateInfo.imageSamples = VK_SAMPLE_COUNT_1_BIT;
  vkImageCreateInfo.imageTiling = VK_IMAGE_TILING_OPTIMAL;
  vkImageCreateInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  vkImageCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkImageCreateInfo.imageQueueFamilyIndexCount = 0;
  vkImageCreateInfo.imageQueueFamilyIndices = NULL;
  vkImageCreateInfo.imageInitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  app->uvr_vk_image[textureImageIndex] = uvr_vk_image_create(&vkImageCreateInfo);
  if (!app->uvr_vk_image[textureImageIndex].imageHandles[imageTransferIndex].image && !app->uvr_vk_image[textureImageIndex].imageViewHandles[imageTransferIndex].view)
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
  imageMemoryBarrier.image = app->uvr_vk_image[textureImageIndex].imageHandles[imageTransferIndex].image;
  imageMemoryBarrier.subresourceRange.aspectMask = vkImageCreateInfo.imageViewSubresourceRange.aspectMask;
  imageMemoryBarrier.subresourceRange.baseMipLevel = vkImageCreateInfo.imageViewSubresourceRange.baseMipLevel;
  imageMemoryBarrier.subresourceRange.levelCount = vkImageCreateInfo.imageViewSubresourceRange.levelCount;
  imageMemoryBarrier.subresourceRange.baseArrayLayer = vkImageCreateInfo.imageViewSubresourceRange.baseArrayLayer;
  imageMemoryBarrier.subresourceRange.layerCount = vkImageCreateInfo.imageViewSubresourceRange.layerCount;

  struct uvr_vk_resource_pipeline_barrier_info pipelineBarrierInfo;
  pipelineBarrierInfo.commandBuffer = app->uvr_vk_command_buffer.commandBufferHandles[0].commandBuffer;
  pipelineBarrierInfo.queue = app->uvr_vk_queue.queue;
  pipelineBarrierInfo.srcPipelineStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  pipelineBarrierInfo.dstPipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  pipelineBarrierInfo.dependencyFlags = 0;
  pipelineBarrierInfo.memoryBarrier = NULL;
  pipelineBarrierInfo.bufferMemoryBarrier = NULL;
  pipelineBarrierInfo.imageMemoryBarrier = &imageMemoryBarrier;

  if (uvr_vk_resource_pipeline_barrier(&pipelineBarrierInfo) == -1)
    return -1;

  /* Copy pixel buffer to VkImage Resource */
  VkBufferImageCopy copyRegion;
  copyRegion.bufferOffset = 0;
  copyRegion.bufferRowLength = 0;
  copyRegion.bufferImageHeight = 0;
  copyRegion.imageSubresource.aspectMask = vkImageCreateInfo.imageViewSubresourceRange.aspectMask;
  copyRegion.imageSubresource.mipLevel = vkImageCreateInfo.imageViewSubresourceRange.baseMipLevel;
  copyRegion.imageSubresource.baseArrayLayer = vkImageCreateInfo.imageViewSubresourceRange.baseArrayLayer;
  copyRegion.imageSubresource.layerCount = vkImageCreateInfo.imageViewSubresourceRange.layerCount;
  copyRegion.imageOffset = (VkOffset3D) { .x = 0, .y = 0, .z = 0 };
  copyRegion.imageExtent = vkImageCreateInfo.imageExtent3D;

  /* Copy VkImage resource to VkBuffer resource */
  struct uvr_vk_resource_copy_info bufferCopyInfo;
  bufferCopyInfo.resourceCopyType = UVR_VK_COPY_VK_BUFFER_TO_VK_IMAGE;
  bufferCopyInfo.commandBuffer = app->uvr_vk_command_buffer.commandBufferHandles[0].commandBuffer;
  bufferCopyInfo.queue = app->uvr_vk_queue.queue;
  bufferCopyInfo.srcResource = app->uvr_vk_buffer[cpuVisibleImageBuffer].buffer;
  bufferCopyInfo.dstResource = app->uvr_vk_image[textureImageIndex].imageHandles[imageTransferIndex].image;
  bufferCopyInfo.bufferCopyInfo = NULL;
  bufferCopyInfo.bufferImageCopyInfo = &copyRegion;
  bufferCopyInfo.imageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

  if (uvr_vk_resource_copy(&bufferCopyInfo) == -1)
    return -1;

  imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  pipelineBarrierInfo.srcPipelineStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  pipelineBarrierInfo.dstPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  pipelineBarrierInfo.imageMemoryBarrier = &imageMemoryBarrier;
  if (uvr_vk_resource_pipeline_barrier(&pipelineBarrierInfo) == -1)
    return -1;

  return 0;
}


int create_vk_image_sampler(struct uvr_vk *app)
{
  struct uvr_vk_sampler_create_info vkSamplerCreateInfo;
  vkSamplerCreateInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
  vkSamplerCreateInfo.samplerFlags = 0;
  vkSamplerCreateInfo.samplerMagFilter = VK_FILTER_LINEAR;
  vkSamplerCreateInfo.samplerMinFilter = VK_FILTER_LINEAR;
  vkSamplerCreateInfo.samplerMipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  vkSamplerCreateInfo.samplerAddressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  vkSamplerCreateInfo.samplerAddressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  vkSamplerCreateInfo.samplerAddressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  vkSamplerCreateInfo.samplerMipLodBias = 0.0f;
  vkSamplerCreateInfo.samplerAnisotropyEnable = VK_TRUE;
  vkSamplerCreateInfo.samplerMaxAnisotropy = app->uvr_vk_phdev.physDeviceProperties.limits.maxSamplerAnisotropy;
  vkSamplerCreateInfo.samplerCompareEnable = VK_FALSE;
  vkSamplerCreateInfo.samplerCompareOp = VK_COMPARE_OP_ALWAYS;
  vkSamplerCreateInfo.samplerMinLod = 0.0f;
  vkSamplerCreateInfo.samplerMaxLod = 0.0f;
  vkSamplerCreateInfo.samplerBorderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  vkSamplerCreateInfo.samplerUnnormalizedCoordinates = VK_FALSE;

  app->uvr_vk_sampler = uvr_vk_sampler_create(&vkSamplerCreateInfo);
  if (!app->uvr_vk_sampler.sampler)
    return -1;

  return 0;
}


int create_vk_resource_descriptor_sets(struct uvr_vk *app, struct uvr_utils_aligned_buffer *modelTransferSpace)
{
  VkDescriptorSetLayoutBinding descSetLayoutBindings[3]; // Amount of descriptors in the set
  uint32_t descriptorBindingCount = ARRAY_LEN(descSetLayoutBindings);

  // Uniform descriptor
  descSetLayoutBindings[0].binding = 0;
  descSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  // See struct uvr_vk_descriptor_set_handle for more information in this
  descSetLayoutBindings[0].descriptorCount = 1;
  descSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  descSetLayoutBindings[0].pImmutableSamplers = NULL;

  // Dynamic Uniform descriptor
  descSetLayoutBindings[1].binding = 1;
  descSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  // See struct uvr_vk_descriptor_set_handle for more information in this
  descSetLayoutBindings[1].descriptorCount = 1;
  descSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  descSetLayoutBindings[1].pImmutableSamplers = NULL;

  // Combinded Image Sampler descriptor
  descSetLayoutBindings[2].binding = 2;
  descSetLayoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  // See struct uvr_vk_descriptor_set_handle for more information in this
  descSetLayoutBindings[2].descriptorCount = 1;
  descSetLayoutBindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  descSetLayoutBindings[2].pImmutableSamplers = NULL;

  struct uvr_vk_descriptor_set_layout_create_info descriptorCreateInfo;
  descriptorCreateInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
  descriptorCreateInfo.descriptorSetLayoutCreateflags = 0;
  descriptorCreateInfo.descriptorSetLayoutBindingCount = ARRAY_LEN(descSetLayoutBindings);
  descriptorCreateInfo.descriptorSetLayoutBindings = descSetLayoutBindings;

  app->uvr_vk_descriptor_set_layout = uvr_vk_descriptor_set_layout_create(&descriptorCreateInfo);
  if (!app->uvr_vk_descriptor_set_layout.descriptorSetLayout)
    return -1;

  /*
   * Per my understanding this is just so the VkDescriptorPool knows what to preallocate. No descriptor is
   * assigned to a set when the pool is created. Given an array of descriptor set layouts the actual assignment
   * of descriptor to descriptor set happens in the vkAllocateDescriptorSets function.
   */
  VkDescriptorPoolSize descriptorPoolInfos[descriptorBindingCount];
  for (uint32_t desc = 0; desc < descriptorBindingCount; desc++) {
    descriptorPoolInfos[desc].type = descSetLayoutBindings[desc].descriptorType;
    descriptorPoolInfos[desc].descriptorCount = descSetLayoutBindings[desc].descriptorCount;
  }

  // Should allocate one pool. With one set containing multiple descriptors
  struct uvr_vk_descriptor_set_create_info descriptorSetsCreateInfo;
  descriptorSetsCreateInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
  descriptorSetsCreateInfo.descriptorPoolInfos = descriptorPoolInfos;
  descriptorSetsCreateInfo.descriptorPoolInfoCount = ARRAY_LEN(descriptorPoolInfos);
  descriptorSetsCreateInfo.descriptorSetLayouts = &app->uvr_vk_descriptor_set_layout.descriptorSetLayout;
  descriptorSetsCreateInfo.descriptorSetLayoutCount = 1;
  descriptorSetsCreateInfo.descriptorPoolCreateflags = 0;

  app->uvr_vk_descriptor_set = uvr_vk_descriptor_set_create(&descriptorSetsCreateInfo);
  if (!app->uvr_vk_descriptor_set.descriptorPool || !app->uvr_vk_descriptor_set.descriptorSetHandles[0].descriptorSet)
    return -1;

  VkDescriptorBufferInfo bufferInfos[descriptorBindingCount];
  bufferInfos[0].buffer = app->uvr_vk_buffer[2].buffer; // CPU visible uniform buffer
  bufferInfos[0].offset = 0;
  bufferInfos[0].range = sizeof(struct uvr_uniform_buffer);

  bufferInfos[1].buffer = app->uvr_vk_buffer[2].buffer; // CPU visible uniform buffer dynamic buffer
  bufferInfos[1].offset = sizeof(struct uvr_uniform_buffer) * PRECEIVED_SWAPCHAIN_IMAGE_SIZE;
  bufferInfos[1].range = modelTransferSpace->bufferAlignment;

  VkDescriptorImageInfo imageInfo = {};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = app->uvr_vk_image[2].imageViewHandles[0].view;   // created in create_vk_texture_image
  imageInfo.sampler = app->uvr_vk_sampler.sampler;                  // created in create_vk_image_sampler

  /* Binds multiple descriptors and their objects to the same set */
  VkWriteDescriptorSet descriptorWrites[descriptorBindingCount];
  for (uint32_t dw = 0; dw < ARRAY_LEN(descriptorWrites); dw++) {
    descriptorWrites[dw].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[dw].pNext = NULL;
    descriptorWrites[dw].dstSet = app->uvr_vk_descriptor_set.descriptorSetHandles[0].descriptorSet;
    descriptorWrites[dw].dstBinding = descSetLayoutBindings[dw].binding;
    descriptorWrites[dw].dstArrayElement = 0;
    descriptorWrites[dw].descriptorType = descSetLayoutBindings[dw].descriptorType;
    descriptorWrites[dw].descriptorCount = descSetLayoutBindings[dw].descriptorCount;
    descriptorWrites[dw].pBufferInfo = (dw < 2) ? &bufferInfos[dw] : NULL;
    descriptorWrites[dw].pImageInfo = (dw == 2) ? &imageInfo : NULL;
    descriptorWrites[dw].pTexelBufferView = NULL;
  }

  vkUpdateDescriptorSets(app->uvr_vk_lgdev.logicalDevice, descriptorBindingCount, descriptorWrites, 0, NULL);

  return 0;
}


int create_vk_graphics_pipeline(struct uvr_vk *app, VkSurfaceFormatKHR *surfaceFormat, VkExtent2D extent2D)
{
  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = app->uvr_vk_shader_module[0].shaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = app->uvr_vk_shader_module[1].shaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[2] = {vertShaderStageInfo, fragShaderStageInfo};

  VkVertexInputBindingDescription VkVertexInputBindingDescription = {};
  VkVertexInputBindingDescription.binding = 0;
  VkVertexInputBindingDescription.stride = sizeof(struct uvr_vertex_data); // Number of bytes from one entry to another
  VkVertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Move to next data entry after each vertex

  // Two incomming vertex atributes in the vertex shader.
  VkVertexInputAttributeDescription vertexAttributeDescriptions[3];

  // position attribute
  vertexAttributeDescriptions[0].location = 0;
  vertexAttributeDescriptions[0].binding = 0;
  // defines the byte size of attribute data
  vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 - RGB color channels match size of vec3
  // specifies the byte where struct uvr_vertex_data member vec3 pos is located
  vertexAttributeDescriptions[0].offset = offsetof(struct uvr_vertex_data, pos);

  // color attribute
  vertexAttributeDescriptions[1].location = 1;
  vertexAttributeDescriptions[1].binding = 0;
  vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec2 - RGB color channels match size of vec3
  // specifies the byte where struct uvr_vertex_data member vec3 color is located
  vertexAttributeDescriptions[1].offset = offsetof(struct uvr_vertex_data, color);

  // texture attribute
  vertexAttributeDescriptions[2].location = 2;
  vertexAttributeDescriptions[2].binding = 0;
  vertexAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT; // vec2 - RGB color channels match size of vec3
  // specifies the byte where struct uvr_vertex_data member vec3 color is located
  vertexAttributeDescriptions[2].offset = offsetof(struct uvr_vertex_data, texCoord);

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &VkVertexInputBindingDescription;
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
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
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
  depthStencilInfo.depthTestEnable = VK_TRUE;            // Enable checking depth to determine fragment write
  depthStencilInfo.depthWriteEnable = VK_TRUE;           // Enable depth buffer writes inorder to replace old values
  depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;  // Comparison operation that allows an overwrite (if is in front)
  depthStencilInfo.depthBoundsTestEnable = VK_FALSE;     // Enable if depth test should be enabled between @minDepthBounds & @maxDepthBounds
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

  struct uvr_vk_pipeline_layout_create_info graphicsPipelineLayoutCreateInfo;
  graphicsPipelineLayoutCreateInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
  graphicsPipelineLayoutCreateInfo.descriptorSetLayoutCount = 1;
  graphicsPipelineLayoutCreateInfo.descriptorSetLayouts = &app->uvr_vk_descriptor_set_layout.descriptorSetLayout;
  graphicsPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
  graphicsPipelineLayoutCreateInfo.pushConstantRanges = NULL;
  // graphicsPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  // graphicsPipelineLayoutCreateInfo.pushConstantRanges = &pushConstantRange;

  app->uvr_vk_pipeline_layout = uvr_vk_pipeline_layout_create(&graphicsPipelineLayoutCreateInfo);
  if (!app->uvr_vk_pipeline_layout.pipelineLayout)
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
  attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;               // Load data into attachment be sure to clear first
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

  VkSubpassDependency subpassDependency;
  subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  subpassDependency.dstSubpass = 0;
  subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  subpassDependency.srcAccessMask = 0;
  subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  subpassDependency.dependencyFlags = 0;

  struct uvr_vk_render_pass_create_info renderPassInfo;
  renderPassInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
  renderPassInfo.attachmentDescriptionCount = ARRAY_LEN(attachmentDescriptions);
  renderPassInfo.attachmentDescriptions = attachmentDescriptions;
  renderPassInfo.subpassDescriptionCount = 1;
  renderPassInfo.subpassDescriptions = &subpass;
  renderPassInfo.subpassDependencyCount = 1;
  renderPassInfo.subpassDependencies = &subpassDependency;

  app->uvr_vk_render_pass = uvr_vk_render_pass_create(&renderPassInfo);
  if (!app->uvr_vk_render_pass.renderPass)
    return -1;

  struct uvr_vk_graphics_pipeline_create_info graphicsPipelineInfo;
  graphicsPipelineInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
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
  graphicsPipelineInfo.pipelineLayout = app->uvr_vk_pipeline_layout.pipelineLayout;
  graphicsPipelineInfo.renderPass = app->uvr_vk_render_pass.renderPass;
  graphicsPipelineInfo.subpass = 0;

  app->uvr_vk_graphics_pipeline = uvr_vk_graphics_pipeline_create(&graphicsPipelineInfo);
  if (!app->uvr_vk_graphics_pipeline.graphicsPipeline)
    return -1;

  return 0;
}


int create_vk_framebuffers(struct uvr_vk *app, VkExtent2D extent2D)
{
  struct uvr_vk_framebuffer_create_info frameBufferInfo;
  frameBufferInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
  frameBufferInfo.frameBufferCount = app->uvr_vk_image[0].imageCount;           // Amount of images in swapchain
  frameBufferInfo.imageViewHandles = app->uvr_vk_image[0].imageViewHandles;     // swapchain image views
  frameBufferInfo.miscImageViewHandleCount = 1;                             // Should only be a depth image view
  frameBufferInfo.miscImageViewHandles = app->uvr_vk_image[1].imageViewHandles; // Depth Buffer image
  frameBufferInfo.renderPass = app->uvr_vk_render_pass.renderPass;
  frameBufferInfo.width = extent2D.width;
  frameBufferInfo.height = extent2D.height;
  frameBufferInfo.layers = 1;

  app->uvr_vk_framebuffer = uvr_vk_framebuffer_create(&frameBufferInfo);
  if (!app->uvr_vk_framebuffer.frameBufferHandles[0].frameBuffer)
    return -1;

  return 0;
}


int create_vk_sync_objs(struct uvr_vk *app)
{
  struct uvr_vk_sync_obj_create_info syncObjsCreateInfo;
  syncObjsCreateInfo.logicalDevice = app->uvr_vk_lgdev.logicalDevice;
  syncObjsCreateInfo.fenceCount = 1;
  syncObjsCreateInfo.semaphoreCount = 2;

  app->uvr_vk_sync_obj = uvr_vk_sync_obj_create(&syncObjsCreateInfo);
  if (!app->uvr_vk_sync_obj.fenceHandles[0].fence && !app->uvr_vk_sync_obj.semaphoreHandles[0].semaphore)
    return -1;

  return 0;
}


int record_vk_draw_commands(struct uvr_vk *app,
                            struct uvr_utils_aligned_buffer *modelTransferSpace,
                            uint32_t swapchainImageIndex,
                            VkExtent2D extent2D)
{
  struct uvr_vk_command_buffer_record_info commandBufferRecordInfo;
  commandBufferRecordInfo.commandBufferCount = app->uvr_vk_command_buffer.commandBufferCount;
  commandBufferRecordInfo.commandBufferHandles = app->uvr_vk_command_buffer.commandBufferHandles;
  commandBufferRecordInfo.commandBufferUsageflags = 0;

  if (uvr_vk_command_buffer_record_begin(&commandBufferRecordInfo) == -1)
    return -1;

  VkCommandBuffer cmdBuffer = app->uvr_vk_command_buffer.commandBufferHandles[0].commandBuffer;

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
   * Values used by VK_ATTACHMENT_LOAD_OP_CLEAR
   * Black with 100% opacity
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
  renderPassInfo.renderPass = app->uvr_vk_render_pass.renderPass;
  renderPassInfo.framebuffer = app->uvr_vk_framebuffer.frameBufferHandles[swapchainImageIndex].frameBuffer;
  renderPassInfo.renderArea = renderArea;
  renderPassInfo.clearValueCount = ARRAY_LEN(clearColors);
  renderPassInfo.pClearValues = clearColors;

  /*
   * 0. CPU visible vertex buffer
   * 1. GPU visible vertex buffer
   */
  VkBuffer vertexBuffer = app->uvr_vk_buffer[(VK_PHYSICAL_DEVICE_TYPE != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? 0 : 1].buffer;
  VkDeviceSize offsets[] = {sizeof(indices), sizeof(indices) + sizeof(meshData[0]) };
  uint32_t dynamicUniformBufferOffset = 0;

  vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->uvr_vk_graphics_pipeline.graphicsPipeline);
  vkCmdBindIndexBuffer(cmdBuffer, vertexBuffer, 0, VK_INDEX_TYPE_UINT16);

  vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
  vkCmdSetScissor(cmdBuffer, 0, 1, &renderArea);

  for (uint32_t i = 0; i < MAX_SCENE_OBJECTS; i++) {
    dynamicUniformBufferOffset = i * modelTransferSpace->bufferAlignment;
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->uvr_vk_pipeline_layout.pipelineLayout, 0, 1,
                                       &app->uvr_vk_descriptor_set.descriptorSetHandles[0].descriptorSet, 1, &dynamicUniformBufferOffset);
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, &offsets[i]);
    vkCmdDrawIndexed(cmdBuffer, ARRAY_LEN(indices), 1, 0, 0, 0);
  }

  vkCmdEndRenderPass(cmdBuffer);

  if (uvr_vk_command_buffer_record_end(&commandBufferRecordInfo) == -1)
    return -1;

  return 0;
}


void update_uniform_buffer(struct uvr_vk *app,
                           struct uvr_utils_aligned_buffer *modelTransferSpace,
                           uint32_t swapchainImageIndex,
                           VkExtent2D extent2D)
{
  VkDeviceMemory uniformBufferDeviceMemory = app->uvr_vk_buffer[2].deviceMemory;
  uint32_t uboSize = sizeof(struct uvr_uniform_buffer);
  struct uvr_uniform_buffer ubo = {};
  struct uvr_uniform_buffer_model uboModels[MAX_SCENE_OBJECTS];

  // Update view matrix
  vec3 eye = {2.0f, 2.0f, 2.0f};
  vec3 center = {0.0f, 0.0f, 0.0f};
  vec3 up = {0.0f, 0.0f, 1.0f};
  glm_lookat(eye, center, up, ubo.view);

  // Update projection matrix
  float fovy = glm_rad(45.0f); // Field of view
  float aspect = (float) extent2D.width / (float) extent2D.height;
  float nearPlane = 0.1f;
  float farPlane = 10.0f;
  glm_perspective(fovy, aspect, nearPlane, farPlane, ubo.proj);

  // invert y - coordinate on projection matrix
  ubo.proj[1][1] *= -1;

  // Copy VP data
  void *data = NULL;
  vkMapMemory(app->uvr_vk_lgdev.logicalDevice, uniformBufferDeviceMemory, swapchainImageIndex * uboSize, uboSize, 0, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(app->uvr_vk_lgdev.logicalDevice, uniformBufferDeviceMemory);
  data = NULL;

  static float lastTime = 0;
  float now = (float) (uvr_utils_nanosecond() / 10000000ULL);
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
  glm_translate(uboModels[0].model, (vec3){ 1.0f,  0.0f, 1.0f });
  glm_translate(uboModels[1].model, (vec3){ 1.0f, 1.0f, 1.0f });
  glm_rotate(uboModels[0].model, glm_rad(angle), axis);
  glm_rotate(uboModels[1].model, glm_rad(-angle * 4.0f), axis);

  // Copy Model data
  for (uint32_t meshItem = 0; meshItem < MAX_SCENE_OBJECTS; meshItem++) {
    struct uvr_uniform_buffer_model *model = (struct uvr_uniform_buffer_model *) ((uint64_t) modelTransferSpace->alignedBufferMemory + (meshItem * modelTransferSpace->bufferAlignment));
    memcpy(model, &uboModels[meshItem], modelTransferSpace->bufferAlignment);
  }

  // Map all Model data
  vkMapMemory(app->uvr_vk_lgdev.logicalDevice, uniformBufferDeviceMemory, uboSize * PRECEIVED_SWAPCHAIN_IMAGE_SIZE, modelTransferSpace->bufferAlignment * MAX_SCENE_OBJECTS, 0, &data);
  memcpy(data, modelTransferSpace->alignedBufferMemory, modelTransferSpace->bufferAlignment * MAX_SCENE_OBJECTS);
  vkUnmapMemory(app->uvr_vk_lgdev.logicalDevice, uniformBufferDeviceMemory);
}
