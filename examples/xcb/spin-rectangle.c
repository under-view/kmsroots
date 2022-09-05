#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>   // For offsetof(3)
#include <stdalign.h> // _Alignas( )

#include <cglm/cglm.h>

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
  struct uvr_vk_phdev phdev;
  struct uvr_vk_lgdev lgdev;
  struct uvr_vk_queue graphics_queue;

  VkSurfaceKHR surface;
  struct uvr_vk_swapchain schain;

  struct uvr_vk_image vkimages;
#ifdef INCLUDE_SHADERC
  struct uvr_shader_spirv vertex_shader;
  struct uvr_shader_spirv fragment_shader;
#else
  struct uvr_shader_file vertex_shader;
  struct uvr_shader_file fragment_shader;
#endif
  struct uvr_vk_shader_module shader_modules[2];

  struct uvr_vk_pipeline_layout gplayout;
  struct uvr_vk_render_pass rpass;
  struct uvr_vk_graphics_pipeline gpipeline;
  struct uvr_vk_framebuffer vkframebuffs;
  struct uvr_vk_command_buffer vkcbuffs;
  struct uvr_vk_sync_obj vksyncs;

  /*
   * 0. CPU visible transfer buffer that stores: (index + vertices)
   * 1. GPU visible vertex buffer that stores: (index + vertices)
   * 2. CPU visible uniform buffer
   */
  struct uvr_vk_buffer vkbuffers[3];

  struct uvr_vk_descriptor_set_layout vkdesclayout;
  struct uvr_vk_descriptor_set vkdescset;
};


struct uvr_vk_xcb {
  struct uvr_utils_aligned_buffer *model_transfer_space;
  struct uvr_xcb_window *uvr_xcb_window;
  struct uvr_vk *uvr_vk;
};


struct uvr_vertex_data {
  vec2 pos;
  vec3 color;
};


struct uvr_uniform_buffer_model {
  mat4 model;
};


struct uvr_uniform_buffer {
  _Alignas(16) mat4 view;
  _Alignas(16) mat4 proj;
};


/*
 * Comments define how to draw rectangle without index buffer
 * Actual array itself draws triangles utilizing index buffers.
 */
const struct uvr_vertex_data vertices_pos_color[2][4] = {
  {
    {{-0.4f,  0.4f}, {1.0f, 0.0f, 0.0f}},   // Vertex 0. Top-right    - red
    {{-0.4f, -0.4f}, {0.0f, 1.0f, 0.0f}},   // Vertex 1. Bottom-right - green
    {{0.4f,  -0.4f}, {0.0f, 0.0f, 1.0f}},   // Vertex 2. Bottom-left  - blue
    //{{0.4f, -0.4f}}, {0.0f, 0.0f, 1.0f}}, // Vertex 2. Bottom-left  - blue
    {{0.4f,   0.4f}, {1.0f, 1.0f, 0.0f}}    // Vertex 3. Top-left     - yellow
    //{{-0.4f,  0.4f}, {1.0f, 0.0f, 0.0f}}, // Vertex 0. Top-right    - red
  },
  {
    {{-0.25f,  0.6f}, {1.0f, 0.0f, 0.0f}},   // Vertex 0. Top-right    - red
    {{-0.25f, -0.6f}, {0.0f, 1.0f, 0.0f}},   // Vertex 1. Bottom-right - green
    {{ 0.25f, -0.6f}, {0.0f, 0.0f, 1.0f}},   // Vertex 2. Bottom-left  - blue
    //{{ 0.25f, -0.6f}, {0.0f, 0.0f, 1.0f}}, // Vertex 2. Bottom-left  - blue
    {{ 0.25f,  0.6f}, {1.0f, 1.0f, 0.0f}}    // Vertex 3. Top-left     - yellow
    //{{-0.25f,  0.6f, {1.0f, 0.0f, 0.0f}},  // Vertex 0. Top-right    - red
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
int create_vk_swapchain(struct uvr_vk *app, VkSurfaceFormatKHR *sformat, VkExtent2D extent2D);
int create_vk_images(struct uvr_vk *app, VkSurfaceFormatKHR *sformat);
int create_vk_shader_modules(struct uvr_vk *app);
int create_vk_command_buffers(struct uvr_vk *app);
int create_vk_buffers(struct uvr_vk *app, struct uvr_utils_aligned_buffer *modelTransferSpace);
int create_vk_resource_descriptor_sets(struct uvr_vk *app, struct uvr_utils_aligned_buffer *modelTransferSpace);
int create_vk_graphics_pipeline(struct uvr_vk *app, VkSurfaceFormatKHR *sformat, VkExtent2D extent2D);
int create_vk_framebuffers(struct uvr_vk *app, VkExtent2D extent2D);
int create_vk_sync_objs(struct uvr_vk *app);
int record_vk_draw_commands(struct uvr_vk *app, struct uvr_utils_aligned_buffer *modelTransferSpace, uint32_t vkSwapchainImageIndex, VkExtent2D extent2D);
void update_uniform_buffer(struct uvr_vk *app, struct uvr_utils_aligned_buffer *modelTransferSpace, uint32_t vkSwapchainImageIndex, VkExtent2D extent2D);


void render(bool UNUSED *running, uint32_t *imageIndex, void *data) {
  VkExtent2D extent2D = { .width = WIDTH, .height = HEIGHT };
  struct uvr_vk_xcb *vkxcb = (struct uvr_vk_xcb *) data;
  struct uvr_utils_aligned_buffer *modelTransferSpace = vkxcb->model_transfer_space;
  struct uvr_xcb_window UNUSED *xc = vkxcb->uvr_xcb_window;
  struct uvr_vk *app = vkxcb->uvr_vk;

  if (!app->vksyncs.fenceHandles || !app->vksyncs.semaphoreHandles)
    return;

  VkFence imageFence = app->vksyncs.fenceHandles[0].fence;
  VkSemaphore imageSemaphore = app->vksyncs.semaphoreHandles[0].semaphore;
  VkSemaphore renderSemaphore = app->vksyncs.semaphoreHandles[1].semaphore;

  vkWaitForFences(app->lgdev.logicalDevice, 1, &imageFence, VK_TRUE, UINT64_MAX);

  vkAcquireNextImageKHR(app->lgdev.logicalDevice, app->schain.swapChain, UINT64_MAX, imageSemaphore, VK_NULL_HANDLE, imageIndex);

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
  submitInfo.pCommandBuffers = &app->vkcbuffs.commandBuffers[0].commandBuffer;
  submitInfo.signalSemaphoreCount = ARRAY_LEN(signalSemaphores);
  submitInfo.pSignalSemaphores = signalSemaphores;

  vkResetFences(app->lgdev.logicalDevice, 1, &imageFence);

  /* Submit draw command */
  vkQueueSubmit(app->graphics_queue.queue, 1, &submitInfo, imageFence);

  VkPresentInfoKHR presentInfo;
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = NULL;
  presentInfo.waitSemaphoreCount = ARRAY_LEN(signalSemaphores);
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &app->schain.swapChain;
  presentInfo.pImageIndices = imageIndex;
  presentInfo.pResults = NULL;

  vkQueuePresentKHR(app->graphics_queue.queue, &presentInfo);
}


/*
 * Example code demonstrating how use Vulkan with Wayland
 */
int main(void) {
  struct uvr_vk app;
  struct uvr_vk_destroy appd;
  memset(&app, 0, sizeof(app));
  memset(&appd, 0, sizeof(appd));

  struct uvr_xcb_window xc;
  struct uvr_xcb_destroy xcd;
  memset(&xc, 0, sizeof(xc));
  memset(&xcd, 0, sizeof(xcd));

  struct uvr_shader_destroy shadercd;
  memset(&shadercd, 0, sizeof(shadercd));

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

  VkSurfaceFormatKHR sformat;
  VkExtent2D extent2D = { .width = WIDTH, .height = HEIGHT };
  if (create_vk_swapchain(&app, &sformat, extent2D) == -1)
    goto exit_error;

  if (create_vk_images(&app, &sformat) == -1)
    goto exit_error;

  if (create_vk_shader_modules(&app) == -1)
    goto exit_error;

  if (create_vk_command_buffers(&app) == -1)
    goto exit_error;

  struct uvr_utils_aligned_buffer modelTransferSpace;
  if (create_vk_buffers(&app, &modelTransferSpace) == -1)
    goto exit_error;

  if (create_vk_resource_descriptor_sets(&app, &modelTransferSpace) == -1)
    goto exit_error;

  if (create_vk_graphics_pipeline(&app, &sformat, extent2D) == -1)
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
  eventInfo.uvrXcbWindow = &xc;
  eventInfo.renderer = render;
  eventInfo.rendererData = &vkxc;
  eventInfo.rendererCbuf = &cbuf;
  eventInfo.rendererRuning = &running;

  while (uvr_xcb_window_handle_event(&eventInfo) && running) {
    // Initentionally left blank
  }


exit_error:
  free(modelTransferSpace.alignedBufferMemory);
#ifdef INCLUDE_SHADERC
  shadercd.uvr_shader_spirv = app.vertex_shader;
  shadercd.uvr_shader_spirv = app.fragment_shader;
#else
  shadercd.uvr_shader_file = app.vertex_shader;
  shadercd.uvr_shader_file = app.fragment_shader;
#endif
  uvr_shader_destroy(&shadercd);

  /*
   * Let the api know of what addresses to free and fd's to close
   */
  appd.instance = app.instance;
  appd.surface = app.surface;
  appd.uvr_vk_lgdev_cnt = 1;
  appd.uvr_vk_lgdev = &app.lgdev;
  appd.uvr_vk_swapchain_cnt = 1;
  appd.uvr_vk_swapchain = &app.schain;
  appd.uvr_vk_image_cnt = 1;
  appd.uvr_vk_image = &app.vkimages;
  appd.uvr_vk_shader_module_cnt = ARRAY_LEN(app.shader_modules);
  appd.uvr_vk_shader_module = app.shader_modules;
  appd.uvr_vk_pipeline_layout_cnt = 1;
  appd.uvr_vk_pipeline_layout = &app.gplayout;
  appd.uvr_vk_render_pass_cnt = 1;
  appd.uvr_vk_render_pass = &app.rpass;
  appd.uvr_vk_graphics_pipeline_cnt = 1;
  appd.uvr_vk_graphics_pipeline = &app.gpipeline;
  appd.uvr_vk_framebuffer_cnt = 1;
  appd.uvr_vk_framebuffer = &app.vkframebuffs;
  appd.uvr_vk_command_buffer_cnt = 1;
  appd.uvr_vk_command_buffer = &app.vkcbuffs;
  appd.uvr_vk_sync_obj_cnt = 1;
  appd.uvr_vk_sync_obj = &app.vksyncs;
  appd.uvr_vk_buffer_cnt = ARRAY_LEN(app.vkbuffers);
  appd.uvr_vk_buffer = app.vkbuffers;
  appd.uvr_vk_descriptor_set_layout_cnt = 1;
  appd.uvr_vk_descriptor_set_layout = &app.vkdesclayout;
  appd.uvr_vk_descriptor_set_cnt = 1;
  appd.uvr_vk_descriptor_set = &app.vkdescset;
  uvr_vk_destory(&appd);

  xcd.uvr_xcb_window = xc;
  uvr_xcb_destory(&xcd);
  return 0;
}


int create_xcb_vk_surface(struct uvr_vk *app, struct uvr_xcb_window *xc) {

  /*
   * Create xcb client
   */
  struct uvr_xcb_window_create_info xcb_win_info;
  xcb_win_info.display = NULL;
  xcb_win_info.screen = NULL;
  xcb_win_info.appName = "Spin Rectangle Example App";
  xcb_win_info.width = WIDTH;
  xcb_win_info.height = HEIGHT;
  xcb_win_info.fullscreen = false;
  xcb_win_info.transparent = false;

  *xc = uvr_xcb_window_create(&xcb_win_info);
  if (!xc->conn)
    return -1;

  /*
   * Create Vulkan Surface
   */
  struct uvr_vk_surface_create_info vk_surface_info;
  vk_surface_info.surfaceType = UVR_XCB_CLIENT_SURFACE;
  vk_surface_info.instance = app->instance;
  vk_surface_info.display = xc->conn;
  vk_surface_info.window = xc->window;

  app->surface = uvr_vk_surface_create(&vk_surface_info);
  if (!app->surface)
    return -1;

  return 0;
}


int create_vk_instance(struct uvr_vk *app) {

  /*
   * "VK_LAYER_KHRONOS_validation"
   * All of the useful standard validation is
   * bundled into a layer included in the SDK
   */
  const char *validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
  };

  const char *instance_extensions[] = {
    "VK_KHR_xcb_surface",
    "VK_KHR_surface",
    "VK_KHR_display",
    "VK_EXT_debug_utils"
  };

  struct uvr_vk_instance_create_info vkinst;
  vkinst.appName = "Spin Rectangle Example App";
  vkinst.engineName = "No Engine";
  vkinst.enabledLayerCount = ARRAY_LEN(validation_layers);
  vkinst.enabledLayerNames = validation_layers;
  vkinst.enabledExtensionCount = ARRAY_LEN(instance_extensions);
  vkinst.enabledExtensionNames = instance_extensions;

  app->instance = uvr_vk_instance_create(&vkinst);
  if (!app->instance) return -1;

  return 0;
}


int create_vk_device(struct uvr_vk *app) {

  const char *device_extensions[] = {
    "VK_KHR_swapchain"
  };

  struct uvr_vk_phdev_create_info vkphdev;
  vkphdev.instance = app->instance;
  vkphdev.deviceType = VK_PHYSICAL_DEVICE_TYPE;
#ifdef INCLUDE_KMS
  vkphdev.kmsfd = -1;
#endif

  app->phdev = uvr_vk_phdev_create(&vkphdev);
  if (!app->phdev.physDevice)
    return -1;

  struct uvr_vk_queue_create_info vkqueueinfo;
  vkqueueinfo.physDevice = app->phdev.physDevice;
  vkqueueinfo.queueFlag = VK_QUEUE_GRAPHICS_BIT;

  app->graphics_queue = uvr_vk_queue_create(&vkqueueinfo);
  if (app->graphics_queue.familyIndex == -1)
    return -1;

  /*
   * Can Hardset features prior
   * https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceFeatures.html
   * app->phdev.physDeviceFeatures.depthBiasClamp = VK_TRUE;
   */
  struct uvr_vk_lgdev_create_info vklgdevinfo;
  vklgdevinfo.instance = app->instance;
  vklgdevinfo.physDevice = app->phdev.physDevice;
  vklgdevinfo.enabledFeatures = &app->phdev.physDeviceFeatures;
  vklgdevinfo.enabledExtensionCount = ARRAY_LEN(device_extensions);
  vklgdevinfo.enabledExtensionNames = device_extensions;
  vklgdevinfo.queueCount = 1;
  vklgdevinfo.queues = &app->graphics_queue;

  app->lgdev = uvr_vk_lgdev_create(&vklgdevinfo);
  if (!app->lgdev.logicalDevice)
    return -1;

  return 0;
}


/* choose swap chain surface format & present mode */
int create_vk_swapchain(struct uvr_vk *app, VkSurfaceFormatKHR *sformat, VkExtent2D extent2D) {
  VkPresentModeKHR presmode;

  VkSurfaceCapabilitiesKHR surfcap = uvr_vk_get_surface_capabilities(app->phdev.physDevice, app->surface);
  struct uvr_vk_surface_format sformats = uvr_vk_get_surface_formats(app->phdev.physDevice, app->surface);
  struct uvr_vk_surface_present_mode spmodes = uvr_vk_get_surface_present_modes(app->phdev.physDevice, app->surface);

  /* Choose surface format based */
  for (uint32_t s = 0; s < sformats.surfaceFormatCount; s++) {
    if (sformats.surfaceFormats[s].format == VK_FORMAT_B8G8R8A8_SRGB && sformats.surfaceFormats[s].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      *sformat = sformats.surfaceFormats[s];
    }
  }

  for (uint32_t p = 0; p < spmodes.presentModeCount; p++) {
    if (spmodes.presentModes[p] == VK_PRESENT_MODE_MAILBOX_KHR) {
      presmode = spmodes.presentModes[p];
    }
  }

  free(sformats.surfaceFormats); sformats.surfaceFormats = NULL;
  free(spmodes.presentModes); spmodes.presentModes = NULL;

  struct uvr_vk_swapchain_create_info scinfo;
  scinfo.logicalDevice = app->lgdev.logicalDevice;
  scinfo.surface = app->surface;
  scinfo.surfaceCapabilities = surfcap;
  scinfo.surfaceFormat = *sformat;
  scinfo.extent2D = extent2D;
  scinfo.imageArrayLayers = 1;
  scinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  scinfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  scinfo.queueFamilyIndexCount = 0;
  scinfo.queueFamilyIndices = NULL;
  scinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  scinfo.presentMode = presmode;
  scinfo.clipped = VK_TRUE;
  scinfo.oldSwapChain = VK_NULL_HANDLE;

  app->schain = uvr_vk_swapchain_create(&scinfo);
  if (!app->schain.swapChain)
    return -1;

  return 0;
}


int create_vk_images(struct uvr_vk *app, VkSurfaceFormatKHR *sformat) {

  struct uvr_vk_image_create_info vkimage_create_info;
  vkimage_create_info.logicalDevice = app->lgdev.logicalDevice;
  vkimage_create_info.swapChain = app->schain.swapChain;
  vkimage_create_info.imageCount = 0;                                                // set to zero as VkSwapchainKHR != VK_NULL_HANDLE
  vkimage_create_info.imageViewflags = 0;
  vkimage_create_info.imageViewType = VK_IMAGE_VIEW_TYPE_2D;
  vkimage_create_info.imageViewFormat = sformat->format;
  vkimage_create_info.imageViewComponents.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  vkimage_create_info.imageViewComponents.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  vkimage_create_info.imageViewComponents.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  vkimage_create_info.imageViewComponents.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  vkimage_create_info.imageViewSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;  // Which aspect of image to view (i.e VK_IMAGE_ASPECT_COLOR_BIT view color)
  vkimage_create_info.imageViewSubresourceRange.baseMipLevel = 0;                        // Start mipmap level to view from (https://en.wikipedia.org/wiki/Mipmap)
  vkimage_create_info.imageViewSubresourceRange.levelCount = 1;                          // Number of mipmap levels to view
  vkimage_create_info.imageViewSubresourceRange.baseArrayLayer = 0;                      // Start array level to view from
  vkimage_create_info.imageViewSubresourceRange.layerCount = 1;                          // Number of array levels to view
  /* Not create images manually so rest of struct members can be safely ignored */
  vkimage_create_info.physDevice = VK_NULL_HANDLE;
  vkimage_create_info.memPropertyFlags = 0;
  vkimage_create_info.imageflags = 0;
  vkimage_create_info.imageType = 0;
  vkimage_create_info.imageExtent3D = (VkExtent3D) { 0, 0, 0 };
  vkimage_create_info.imageMipLevels = 0;
  vkimage_create_info.imageArrayLayers = 0;
  vkimage_create_info.imageSamples = 0;
  vkimage_create_info.imageTiling = 0;
  vkimage_create_info.imageUsage = 0;
  vkimage_create_info.imageSharingMode = 0;
  vkimage_create_info.imageQueueFamilyIndexCount = 0;
  vkimage_create_info.imageQueueFamilyIndices = NULL;
  vkimage_create_info.imageInitialLayout = 0;

  app->vkimages = uvr_vk_image_create(&vkimage_create_info);
  if (!app->vkimages.imageViewHandles[0].view)
    return -1;

  return 0;
}


int create_vk_shader_modules(struct uvr_vk *app) {

#ifdef INCLUDE_SHADERC
  const char vertex_shader[] =
    "#version 450\n"  // GLSL 4.5
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "layout(location = 0) out vec3 outColor;\n\n"
    "layout(location = 0) in vec2 inPosition;\n"
    "layout(location = 1) in vec3 inColor;\n\n"
    "layout(set = 0, binding = 0) uniform uniform_buffer {\n"
    "  mat4 view;\n"
    "  mat4 proj;\n"
    "} uboViewProjection;\n"
    "layout(set = 0, binding = 1) uniform uniform_buffer_model {\n"
    "  mat4 model;\n"
    "} uboModel;\n"
    "void main() {\n"
    "   gl_Position = uboViewProjection.proj * uboViewProjection.view * uboModel.model * vec4(inPosition, 0.0, 1.0);\n"
    "   outColor = inColor;\n"
    "}";

  const char fragment_shader[] =
    "#version 450\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "layout(location = 0) in vec3 inColor;\n"
    "layout(location = 0) out vec4 outColor;\n"
    "void main() { outColor = vec4(inColor, 1.0); }";

  struct uvr_shader_spirv_create_info vert_shader_create_info;
  vert_shader_create_info.kind = VK_SHADER_STAGE_VERTEX_BIT;
  vert_shader_create_info.source = vertex_shader;
  vert_shader_create_info.filename = "vert.spv";
  vert_shader_create_info.entryPoint = "main";

  struct uvr_shader_spirv_create_info frag_shader_create_info;
  frag_shader_create_info.kind = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_shader_create_info.source = fragment_shader;
  frag_shader_create_info.filename = "frag.spv";
  frag_shader_create_info.entryPoint = "main";

  app->vertex_shader = uvr_shader_compile_buffer_to_spirv(&vert_shader_create_info);
  if (!app->vertex_shader.bytes)
    return -1;

  app->fragment_shader = uvr_shader_compile_buffer_to_spirv(&frag_shader_create_info);
  if (!app->fragment_shader.bytes)
    return -1;

#else
  app->vertex_shader = uvr_shader_file_load(TRIANGLE_UNIFORM_VERTEX_SHADER_SPIRV);
  if (!app->vertex_shader.bytes)
    return -1;

  app->fragment_shader = uvr_shader_file_load(TRIANGLE_UNIFORM_FRAGMENT_SHADER_SPIRV);
  if (!app->fragment_shader.bytes)
    return -1;
#endif

  struct uvr_vk_shader_module_create_info vertex_shader_module_create_info;
  vertex_shader_module_create_info.logicalDevice = app->lgdev.logicalDevice;
  vertex_shader_module_create_info.sprivByteSize = app->vertex_shader.byteSize;
  vertex_shader_module_create_info.sprivBytes = app->vertex_shader.bytes;
  vertex_shader_module_create_info.shaderName = "vertex";

  app->shader_modules[0] = uvr_vk_shader_module_create(&vertex_shader_module_create_info);
  if (!app->shader_modules[0].shaderModule)
    return -1;

  struct uvr_vk_shader_module_create_info frag_shader_module_create_info;
  frag_shader_module_create_info.logicalDevice = app->lgdev.logicalDevice;
  frag_shader_module_create_info.sprivByteSize = app->fragment_shader.byteSize;
  frag_shader_module_create_info.sprivBytes = app->fragment_shader.bytes;
  frag_shader_module_create_info.shaderName = "fragment";

  app->shader_modules[1] = uvr_vk_shader_module_create(&frag_shader_module_create_info);
  if (!app->shader_modules[1].shaderModule)
    return -1;

  return 0;
}


int create_vk_command_buffers(struct uvr_vk *app) {
  struct uvr_vk_command_buffer_create_info commandBufferCreateInfo;
  commandBufferCreateInfo.logicalDevice = app->lgdev.logicalDevice;
  commandBufferCreateInfo.queueFamilyIndex = app->graphics_queue.familyIndex;
  commandBufferCreateInfo.commandBufferCount = 1;

  app->vkcbuffs = uvr_vk_command_buffer_create(&commandBufferCreateInfo);
  if (!app->vkcbuffs.commandPool)
    return -1;

  return 0;
}


int create_vk_buffers(struct uvr_vk *app, struct uvr_utils_aligned_buffer *modelTransferSpace) {
  uint32_t cpuVisibleBuffer = 0, gpuVisibleBuffer = 1;
  void *data = NULL;

  size_t singleMeshSize = sizeof(vertices_pos_color[0]);
  size_t singleIndexBufferSize = sizeof(indices);

  // Create CPU visible vertex + index buffer
  struct uvr_vk_buffer_create_info vkVertexBufferCreateInfo;
  vkVertexBufferCreateInfo.logicalDevice = app->lgdev.logicalDevice;
  vkVertexBufferCreateInfo.physDevice = app->phdev.physDevice;
  vkVertexBufferCreateInfo.bufferFlags = 0;
  vkVertexBufferCreateInfo.bufferSize = singleIndexBufferSize + sizeof(vertices_pos_color);
  vkVertexBufferCreateInfo.bufferUsage = (VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU || \
                                          VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_CPU) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT : VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  vkVertexBufferCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkVertexBufferCreateInfo.queueFamilyIndexCount = 0;
  vkVertexBufferCreateInfo.queueFamilyIndices = NULL;
  vkVertexBufferCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  app->vkbuffers[cpuVisibleBuffer] = uvr_vk_buffer_create(&vkVertexBufferCreateInfo);
  if (!app->vkbuffers[cpuVisibleBuffer].buffer || !app->vkbuffers[cpuVisibleBuffer].deviceMemory)
    return -1;

  // Copy index data into CPU visible index buffer
  vkMapMemory(app->lgdev.logicalDevice, app->vkbuffers[cpuVisibleBuffer].deviceMemory, 0, singleIndexBufferSize, 0, &data);
  memcpy(data, indices, singleIndexBufferSize);
  vkUnmapMemory(app->lgdev.logicalDevice, app->vkbuffers[cpuVisibleBuffer].deviceMemory);
  data = NULL;

  // Copy vertex data into CPU visible vertex buffer
  for (uint32_t vbuff = 0; vbuff < 2; vbuff++) {
    vkMapMemory(app->lgdev.logicalDevice, app->vkbuffers[cpuVisibleBuffer].deviceMemory, singleIndexBufferSize + (singleMeshSize * vbuff), singleMeshSize, 0, &data);
    memcpy(data, vertices_pos_color[vbuff], singleMeshSize);
    vkUnmapMemory(app->lgdev.logicalDevice, app->vkbuffers[cpuVisibleBuffer].deviceMemory);
    data = NULL;
  }

  if (VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    // Create GPU visible vertex buffer
    struct uvr_vk_buffer_create_info vkVertexBufferGPUCreateInfo;
    vkVertexBufferGPUCreateInfo.logicalDevice = app->lgdev.logicalDevice;
    vkVertexBufferGPUCreateInfo.physDevice = app->phdev.physDevice;
    vkVertexBufferGPUCreateInfo.bufferFlags = 0;
    vkVertexBufferGPUCreateInfo.bufferSize = vkVertexBufferCreateInfo.bufferSize;
    vkVertexBufferGPUCreateInfo.bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    vkVertexBufferGPUCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkVertexBufferGPUCreateInfo.queueFamilyIndexCount = 0;
    vkVertexBufferGPUCreateInfo.queueFamilyIndices = NULL;
    vkVertexBufferGPUCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    app->vkbuffers[gpuVisibleBuffer] = uvr_vk_buffer_create(&vkVertexBufferGPUCreateInfo);
    if (!app->vkbuffers[gpuVisibleBuffer].buffer || !app->vkbuffers[gpuVisibleBuffer].deviceMemory)
      return -1;

    VkBufferCopy copyRegion;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = vkVertexBufferCreateInfo.bufferSize;

    // Copy contents from CPU visible buffer over to GPU visible buffer
    struct uvr_vk_resource_copy_info bufferCopyInfo;
    bufferCopyInfo.resourceCopyType = UVR_VK_COPY_VK_BUFFER_TO_VK_BUFFER;
    bufferCopyInfo.commandBuffer = app->vkcbuffs.commandBuffers[0].commandBuffer;
    bufferCopyInfo.queue = app->graphics_queue.queue;
    bufferCopyInfo.srcResource = app->vkbuffers[cpuVisibleBuffer].buffer;
    bufferCopyInfo.dstResource = app->vkbuffers[gpuVisibleBuffer].buffer;
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
  modelUniformBufferAlignment.bufferAlignment = app->phdev.physDeviceProperties.limits.minUniformBufferOffsetAlignment;

  *modelTransferSpace = uvr_utils_aligned_buffer_create(&modelUniformBufferAlignment);
  if (!modelTransferSpace->alignedBufferMemory)
    return -1;

  // Create CPU visible uniform buffer to store (view projection matrices in first have) (Dynamic uniform buffer (model matrix) in second half)
  struct uvr_vk_buffer_create_info vkUniformBufferCreateInfo;
  vkUniformBufferCreateInfo.logicalDevice = app->lgdev.logicalDevice;
  vkUniformBufferCreateInfo.physDevice = app->phdev.physDevice;
  vkUniformBufferCreateInfo.bufferFlags = 0;
  vkUniformBufferCreateInfo.bufferSize = (sizeof(struct uvr_uniform_buffer) * PRECEIVED_SWAPCHAIN_IMAGE_SIZE) + \
                                         (modelTransferSpace->bufferAlignment * MAX_SCENE_OBJECTS);
  vkUniformBufferCreateInfo.bufferUsage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  vkUniformBufferCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkUniformBufferCreateInfo.queueFamilyIndexCount = 0;
  vkUniformBufferCreateInfo.queueFamilyIndices = NULL;
  vkUniformBufferCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  app->vkbuffers[cpuVisibleBuffer] = uvr_vk_buffer_create(&vkUniformBufferCreateInfo);
  if (!app->vkbuffers[cpuVisibleBuffer].buffer || !app->vkbuffers[cpuVisibleBuffer].deviceMemory)
    return -1;

  return 0;
}


int create_vk_resource_descriptor_sets(struct uvr_vk *app, struct uvr_utils_aligned_buffer *modelTransferSpace) {
  VkDescriptorSetLayoutBinding descSetLayoutBindings[2];
  uint32_t descriptorBindingCount = ARRAY_LEN(descSetLayoutBindings);

  descSetLayoutBindings[0].binding = 0;
  descSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  // See struct uvr_vk_descriptor_set_handle for more information in this
  descSetLayoutBindings[0].descriptorCount = 1;
  descSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  descSetLayoutBindings[0].pImmutableSamplers = NULL;

  descSetLayoutBindings[1].binding = 1;
  descSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
  // See struct uvr_vk_descriptor_set_handle for more information in this
  descSetLayoutBindings[1].descriptorCount = 1;
  descSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  descSetLayoutBindings[1].pImmutableSamplers = NULL;

  struct uvr_vk_descriptor_set_layout_create_info descriptorCreateInfo;
  descriptorCreateInfo.logicalDevice = app->lgdev.logicalDevice;
  descriptorCreateInfo.descriptorSetLayoutCreateflags = 0;
  descriptorCreateInfo.descriptorSetLayoutBindingCount = descriptorBindingCount;
  descriptorCreateInfo.descriptorSetLayoutBindings = descSetLayoutBindings;

  app->vkdesclayout = uvr_vk_descriptor_set_layout_create(&descriptorCreateInfo);
  if (!app->vkdesclayout.descriptorSetLayout)
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
  descriptorSetsCreateInfo.logicalDevice = app->lgdev.logicalDevice;
  descriptorSetsCreateInfo.descriptorPoolInfos = descriptorPoolInfos;
  descriptorSetsCreateInfo.descriptorPoolInfoCount = descriptorBindingCount;
  descriptorSetsCreateInfo.descriptorSetLayouts = &app->vkdesclayout.descriptorSetLayout;
  descriptorSetsCreateInfo.descriptorSetLayoutCount = 1;
  descriptorSetsCreateInfo.descriptorPoolCreateflags = 0;

  app->vkdescset = uvr_vk_descriptor_set_create(&descriptorSetsCreateInfo);
  if (!app->vkdescset.descriptorPool || !app->vkdescset.descriptorSetHandles[0].descriptorSet)
    return -1;

  VkDescriptorBufferInfo bufferInfos[descriptorBindingCount];
  bufferInfos[0].buffer = app->vkbuffers[2].buffer; // CPU visible uniform buffer
  bufferInfos[0].offset = 0;
  bufferInfos[0].range = sizeof(struct uvr_uniform_buffer);

  bufferInfos[1].buffer = app->vkbuffers[2].buffer; // CPU visible uniform buffer dynamic buffer
  bufferInfos[1].offset = sizeof(struct uvr_uniform_buffer) * PRECEIVED_SWAPCHAIN_IMAGE_SIZE;
  bufferInfos[1].range = modelTransferSpace->bufferAlignment;

  /* Binds multiple descriptors and their objects to the same set */
  VkWriteDescriptorSet descriptorWrites[descriptorBindingCount];
  for (uint32_t dw = 0; dw < descriptorBindingCount; dw++) {
    descriptorWrites[dw].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[dw].pNext = NULL;
    descriptorWrites[dw].dstSet = app->vkdescset.descriptorSetHandles[0].descriptorSet;
    descriptorWrites[dw].dstBinding = descSetLayoutBindings[dw].binding;
    descriptorWrites[dw].dstArrayElement = 0;
    descriptorWrites[dw].descriptorType = descSetLayoutBindings[dw].descriptorType;
    descriptorWrites[dw].descriptorCount = descSetLayoutBindings[dw].descriptorCount;
    descriptorWrites[dw].pBufferInfo = &bufferInfos[dw];
    descriptorWrites[dw].pImageInfo = NULL;
    descriptorWrites[dw].pTexelBufferView = NULL;
  }

  vkUpdateDescriptorSets(app->lgdev.logicalDevice, descriptorBindingCount, descriptorWrites, 0, NULL);

  return 0;
}


int create_vk_graphics_pipeline(struct uvr_vk *app, VkSurfaceFormatKHR *sformat, VkExtent2D extent2D) {

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = app->shader_modules[0].shaderModule;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = app->shader_modules[1].shaderModule;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[2] = {vertShaderStageInfo, fragShaderStageInfo};

  VkVertexInputBindingDescription vertexInputBindingDescription = {};
  vertexInputBindingDescription.binding = 0;
  vertexInputBindingDescription.stride = sizeof(struct uvr_vertex_data); // Number of bytes from one entry to another
  vertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Move to next data entry after each vertex

  // Two incomming vertex atributes in the vertex shader.
  VkVertexInputAttributeDescription vertexAttributeDescriptions[2];

  // position attribute
  vertexAttributeDescriptions[0].location = 0;
  vertexAttributeDescriptions[0].binding = 0;
  // defines the byte size of attribute data
  vertexAttributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // vec2 - RG color channels match size of vec2
  // specifies the byte where struct uvr_vertex_data member vec2 pos is located
  vertexAttributeDescriptions[0].offset = offsetof(struct uvr_vertex_data, pos);

  // color attribute
  vertexAttributeDescriptions[1].location = 1;
  vertexAttributeDescriptions[1].binding = 0;
  vertexAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec2 - RGB color channels match size of vec3
  // specifies the byte where struct uvr_vertex_data member vec3 color is located
  vertexAttributeDescriptions[1].offset = offsetof(struct uvr_vertex_data, color);

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

  /*
  VkPushConstantRange pushConstantRange;
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(uniform_buffer_model); // sizeof data to pass
  */

  struct uvr_vk_pipeline_layout_create_info gplayout_info;
  gplayout_info.logicalDevice = app->lgdev.logicalDevice;
  gplayout_info.descriptorSetLayoutCount = 1;
  gplayout_info.descriptorSetLayouts = &app->vkdesclayout.descriptorSetLayout;
  gplayout_info.pushConstantRangeCount = 0;
  gplayout_info.pushConstantRanges = NULL;
  // gplayout_info.pushConstantRangeCount = 1;
  // gplayout_info.pushConstantRanges = &pushConstantRange;

  app->gplayout = uvr_vk_pipeline_layout_create(&gplayout_info);
  if (!app->gplayout.pipelineLayout)
    return -1;

  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format = sformat->format;
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

  VkSubpassDependency subPassDependency;
  subPassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  subPassDependency.dstSubpass = 0;
  subPassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subPassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subPassDependency.srcAccessMask = 0;
  subPassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subPassDependency.dependencyFlags = 0;

  struct uvr_vk_render_pass_create_info renderpass_info;
  renderpass_info.logicalDevice = app->lgdev.logicalDevice;
  renderpass_info.attachmentDescriptionCount = 1;
  renderpass_info.attachmentDescriptions = &colorAttachment;
  renderpass_info.subpassDescriptionCount = 1;
  renderpass_info.subpassDescriptions = &subpass;
  renderpass_info.subpassDependencyCount = 1;
  renderpass_info.subpassDependencies = &subPassDependency;

  app->rpass = uvr_vk_render_pass_create(&renderpass_info);
  if (!app->rpass.renderPass)
    return -1;

  struct uvr_vk_graphics_pipeline_create_info gpipeline_info;
  gpipeline_info.logicalDevice = app->lgdev.logicalDevice;
  gpipeline_info.shaderStageCount = ARRAY_LEN(shaderStages);
  gpipeline_info.shaderStages = shaderStages;
  gpipeline_info.vertexInputState = &vertexInputInfo;
  gpipeline_info.inputAssemblyState = &inputAssembly;
  gpipeline_info.tessellationState = NULL;
  gpipeline_info.viewportState = &viewportState;
  gpipeline_info.rasterizationState = &rasterizer;
  gpipeline_info.multisampleState = &multisampling;
  gpipeline_info.depthStencilState = NULL;
  gpipeline_info.colorBlendState = &colorBlending;
  gpipeline_info.dynamicState = &dynamicState;
  gpipeline_info.pipelineLayout = app->gplayout.pipelineLayout;
  gpipeline_info.renderPass = app->rpass.renderPass;
  gpipeline_info.subpass = 0;

  app->gpipeline = uvr_vk_graphics_pipeline_create(&gpipeline_info);
  if (!app->gpipeline.graphicsPipeline)
    return -1;

  return 0;
}


int create_vk_framebuffers(struct uvr_vk *app, VkExtent2D extent2D) {

  struct uvr_vk_framebuffer_create_info vkframebuffer_create_info;
  vkframebuffer_create_info.logicalDevice = app->lgdev.logicalDevice;
  vkframebuffer_create_info.frameBufferCount = app->vkimages.imageCount;
  vkframebuffer_create_info.imageViewHandles = app->vkimages.imageViewHandles;
  vkframebuffer_create_info.renderPass = app->rpass.renderPass;
  vkframebuffer_create_info.width = extent2D.width;
  vkframebuffer_create_info.height = extent2D.height;
  vkframebuffer_create_info.layers = 1;

  app->vkframebuffs = uvr_vk_framebuffer_create(&vkframebuffer_create_info);
  if (!app->vkframebuffs.frameBufferHandles[0].frameBuffer)
    return -1;

  return 0;
}


int create_vk_sync_objs(struct uvr_vk *app) {
  struct uvr_vk_sync_obj_create_info syncObjsCreateInfo;
  syncObjsCreateInfo.logicalDevice = app->lgdev.logicalDevice;
  syncObjsCreateInfo.fenceCount = 1;
  syncObjsCreateInfo.semaphoreCount = 2;

  app->vksyncs = uvr_vk_sync_obj_create(&syncObjsCreateInfo);
  if (!app->vksyncs.fenceHandles[0].fence && !app->vksyncs.semaphoreHandles[0].semaphore)
    return -1;

  return 0;
}


int record_vk_draw_commands(struct uvr_vk *app, struct uvr_utils_aligned_buffer *modelTransferSpace, uint32_t vkSwapchainImageIndex, VkExtent2D extent2D) {
  struct uvr_vk_command_buffer_record_info commandBufferRecordInfo;
  commandBufferRecordInfo.commandBufferCount = app->vkcbuffs.commandBufferCount;
  commandBufferRecordInfo.commandBuffers = app->vkcbuffs.commandBuffers;
  commandBufferRecordInfo.commandBufferUsageflags = 0;

  if (uvr_vk_command_buffer_record_begin(&commandBufferRecordInfo) == -1)
    return -1;

  VkCommandBuffer cmdBuffer = app->vkcbuffs.commandBuffers[0].commandBuffer;

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

  VkClearValue clearColor[1];
  memcpy(clearColor[0].color.float32, float32, ARRAY_LEN(float32));
  memcpy(clearColor[0].color.int32, int32, ARRAY_LEN(int32));
  memcpy(clearColor[0].color.int32, uint32, ARRAY_LEN(uint32));
  clearColor[0].depthStencil.depth = 0.0f;
  clearColor[0].depthStencil.stencil = 0;

  VkRenderPassBeginInfo renderPassInfo;
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.pNext = NULL;
  renderPassInfo.renderPass = app->rpass.renderPass;
  renderPassInfo.framebuffer = app->vkframebuffs.frameBufferHandles[vkSwapchainImageIndex].frameBuffer;
  renderPassInfo.renderArea = renderArea;
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = clearColor;

  /*
   * 0. CPU visible vertex buffer
   * 1. GPU visible vertex buffer
   */
  VkBuffer vertexBuffer = app->vkbuffers[(VK_PHYSICAL_DEVICE_TYPE != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) ? 0 : 1].buffer;
  VkDeviceSize offsets[] = {sizeof(indices), sizeof(indices) + sizeof(vertices_pos_color[0]) };
  uint32_t dynamicUniformBufferOffset = 0;

  vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gpipeline.graphicsPipeline);
  vkCmdBindIndexBuffer(cmdBuffer, vertexBuffer, 0, VK_INDEX_TYPE_UINT16);

  vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
  vkCmdSetScissor(cmdBuffer, 0, 1, &renderArea);

  for (uint32_t i = 0; i < MAX_SCENE_OBJECTS; i++) {
    dynamicUniformBufferOffset = i * modelTransferSpace->bufferAlignment;
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gplayout.pipelineLayout, 0, 1,
                                       &app->vkdescset.descriptorSetHandles[0].descriptorSet, 1, &dynamicUniformBufferOffset);
    vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, &offsets[i]);
    vkCmdDrawIndexed(cmdBuffer, ARRAY_LEN(indices), 1, 0, 0, 0);
  }

  vkCmdEndRenderPass(cmdBuffer);

  if (uvr_vk_command_buffer_record_end(&commandBufferRecordInfo) == -1)
    return -1;

  return 0;
}


void update_uniform_buffer(struct uvr_vk *app, struct uvr_utils_aligned_buffer *modelTransferSpace, uint32_t vkSwapchainImageIndex, VkExtent2D extent2D) {
  VkDeviceMemory uniformBufferDeviceMemory = app->vkbuffers[2].deviceMemory;
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
  vkMapMemory(app->lgdev.logicalDevice, uniformBufferDeviceMemory, vkSwapchainImageIndex * uboSize, uboSize, 0, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(app->lgdev.logicalDevice, uniformBufferDeviceMemory);
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
  vkMapMemory(app->lgdev.logicalDevice, uniformBufferDeviceMemory, uboSize * PRECEIVED_SWAPCHAIN_IMAGE_SIZE, modelTransferSpace->bufferAlignment * MAX_SCENE_OBJECTS, 0, &data);
  memcpy(data, modelTransferSpace->alignedBufferMemory, modelTransferSpace->bufferAlignment * MAX_SCENE_OBJECTS);
  vkUnmapMemory(app->lgdev.logicalDevice, uniformBufferDeviceMemory);
}
