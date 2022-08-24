#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h> // For offsetof(3)

#include <cglm/cglm.h>

#include "xclient.h"
#include "vulkan.h"
#include "shader.h"

#define WIDTH 1920
#define HEIGHT 1080
//#define WIDTH 3840
//#define HEIGHT 2160

struct uvr_vk {
  VkInstance instance;
  VkPhysicalDevice phdev;
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
   * 1. CPU visible transfer buffer
   * 2. GPU visible vertex buffer
   */
  struct uvr_vk_buffer vkbuffers[2];
};


struct uvr_vk_xcb {
  struct uvr_xcb_window *uvr_xcb_window;
  struct uvr_vk *uvr_vk;
};


struct uvr_vertex_data {
  vec2 pos;
  vec3 color;
};


const struct uvr_vertex_data vertices_pos_color[3] = {
  {{ 0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
  {{ 0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}},
  {{-0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}
};


int create_xcb_vk_surface(struct uvr_vk *app, struct uvr_xcb_window *xc);
int create_vk_instance(struct uvr_vk *uvrvk);
int create_vk_device(struct uvr_vk *app);
int create_vk_swapchain(struct uvr_vk *app, VkSurfaceFormatKHR *sformat, VkExtent2D extent2D);
int create_vk_images(struct uvr_vk *app, VkSurfaceFormatKHR *sformat);
int create_vk_shader_modules(struct uvr_vk *app);
int create_vk_buffers(struct uvr_vk *app);
int create_vk_graphics_pipeline(struct uvr_vk *app, VkSurfaceFormatKHR *sformat, VkExtent2D extent2D);
int create_vk_framebuffers(struct uvr_vk *app, VkExtent2D extent2D);
int create_vk_command_buffers(struct uvr_vk *app);
int create_vk_sync_objs(struct uvr_vk *app);
int record_vk_draw_commands(struct uvr_vk *app, uint32_t vkSwapchainImageIndex, VkExtent2D extent2D);


void render(bool UNUSED *running, uint32_t *imageIndex, void *data) {
  VkExtent2D extent2D = { .width = WIDTH, .height = HEIGHT };
  struct uvr_vk_xcb *vkxcb = (struct uvr_vk_xcb *) data;
  struct uvr_xcb_window UNUSED *xc = vkxcb->uvr_xcb_window;
  struct uvr_vk *app = vkxcb->uvr_vk;

  if (!app->vksyncs.vkFences || !app->vksyncs.vkSemaphores)
    return;

  VkFence imageFence = app->vksyncs.vkFences[0].fence;
  VkSemaphore imageSemaphore = app->vksyncs.vkSemaphores[0].semaphore;
  VkSemaphore renderSemaphore = app->vksyncs.vkSemaphores[1].semaphore;

  vkWaitForFences(app->lgdev.vkDevice, 1, &imageFence, VK_TRUE, UINT64_MAX);

  vkAcquireNextImageKHR(app->lgdev.vkDevice, app->schain.vkSwapchain, UINT64_MAX, imageSemaphore, VK_NULL_HANDLE, imageIndex);

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
  submitInfo.pCommandBuffers = &app->vkcbuffs.commandBuffers[0].commandBuffer;
  submitInfo.signalSemaphoreCount = ARRAY_LEN(signalSemaphores);
  submitInfo.pSignalSemaphores = signalSemaphores;

  vkResetFences(app->lgdev.vkDevice, 1, &imageFence);

  /* Submit draw command */
  vkQueueSubmit(app->graphics_queue.vkQueue, 1, &submitInfo, imageFence);

  VkPresentInfoKHR presentInfo;
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.pNext = NULL;
  presentInfo.waitSemaphoreCount = ARRAY_LEN(signalSemaphores);
  presentInfo.pWaitSemaphores = signalSemaphores;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &app->schain.vkSwapchain;
  presentInfo.pImageIndices = imageIndex;
  presentInfo.pResults = NULL;

  vkQueuePresentKHR(app->graphics_queue.vkQueue, &presentInfo);
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

  if (create_vk_graphics_pipeline(&app, &sformat, extent2D) == -1)
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
  uvr_xcb_window_make_visible(&xc);

  static uint32_t cbuf = 0;
  static bool running = true;

  static struct uvr_vk_xcb vkxc;
  vkxc.uvr_xcb_window = &xc;
  vkxc.uvr_vk = &app;

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
  appd.vkinst = app.instance;
  appd.vksurf = app.surface;
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
  xcb_win_info.appName = "Triangle Example App";
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
  vk_surface_info.vkInst = app->instance;
  vk_surface_info.sType = UVR_XCB_CLIENT_SURFACE;
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
  vkinst.appName = "Triangle Example App";
  vkinst.engineName = "No Engine";
  vkinst.enabledLayerCount = ARRAY_LEN(validation_layers);
  vkinst.ppEnabledLayerNames = validation_layers;
  vkinst.enabledExtensionCount = ARRAY_LEN(instance_extensions);
  vkinst.ppEnabledExtensionNames = instance_extensions;

  app->instance = uvr_vk_instance_create(&vkinst);
  if (!app->instance) return -1;

  return 0;
}


int create_vk_device(struct uvr_vk *app) {

  const char *device_extensions[] = {
    "VK_KHR_swapchain"
  };

  struct uvr_vk_phdev_create_info vkphdev;
  vkphdev.vkInst = app->instance;
  vkphdev.vkPhdevType = VK_PHYSICAL_DEVICE_TYPE;
#ifdef INCLUDE_KMS
  vkphdev.kmsFd = -1;
#endif

  app->phdev = uvr_vk_phdev_create(&vkphdev);
  if (!app->phdev)
    return -1;

  struct uvr_vk_queue_create_info vkqueueinfo;
  vkqueueinfo.vkPhdev = app->phdev;
  vkqueueinfo.queueFlag = VK_QUEUE_GRAPHICS_BIT;

  app->graphics_queue = uvr_vk_queue_create(&vkqueueinfo);
  if (app->graphics_queue.familyIndex == -1)
    return -1;

  VkPhysicalDeviceFeatures phdevfeats = uvr_vk_get_phdev_features(app->phdev);

  struct uvr_vk_lgdev_create_info vklgdevinfo;
  vklgdevinfo.vkInst = app->instance;
  vklgdevinfo.vkPhdev = app->phdev;
  vklgdevinfo.pEnabledFeatures = &phdevfeats;
  vklgdevinfo.enabledExtensionCount = ARRAY_LEN(device_extensions);
  vklgdevinfo.ppEnabledExtensionNames = device_extensions;
  vklgdevinfo.queueCount = 1;
  vklgdevinfo.queues = &app->graphics_queue;

  app->lgdev = uvr_vk_lgdev_create(&vklgdevinfo);
  if (!app->lgdev.vkDevice)
    return -1;

  return 0;
}


/* choose swap chain surface format & present mode */
int create_vk_swapchain(struct uvr_vk *app, VkSurfaceFormatKHR *sformat, VkExtent2D extent2D) {
  VkPresentModeKHR presmode;

  VkSurfaceCapabilitiesKHR surfcap = uvr_vk_get_surface_capabilities(app->phdev, app->surface);
  struct uvr_vk_surface_format sformats = uvr_vk_get_surface_formats(app->phdev, app->surface);
  struct uvr_vk_surface_present_mode spmodes = uvr_vk_get_surface_present_modes(app->phdev, app->surface);

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
  scinfo.vkDevice = app->lgdev.vkDevice;
  scinfo.vkSurface = app->surface;
  scinfo.surfaceCapabilities = surfcap;
  scinfo.surfaceFormat = *sformat;
  scinfo.extent2D = extent2D;
  scinfo.imageArrayLayers = 1;
  scinfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  scinfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  scinfo.queueFamilyIndexCount = 0;
  scinfo.pQueueFamilyIndices = NULL;
  scinfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  scinfo.presentMode = presmode;
  scinfo.clipped = VK_TRUE;
  scinfo.oldSwapchain = VK_NULL_HANDLE;

  app->schain = uvr_vk_swapchain_create(&scinfo);
  if (!app->schain.vkSwapchain)
    return -1;

  return 0;
}


int create_vk_images(struct uvr_vk *app, VkSurfaceFormatKHR *sformat) {

  struct uvr_vk_image_create_info vkimage_create_info;
  vkimage_create_info.vkDevice = app->lgdev.vkDevice;
  vkimage_create_info.vkSwapchain = app->schain.vkSwapchain;
  vkimage_create_info.imageViewCount = 0;                                                // set to zero as VkSwapchainKHR != VK_NULL_HANDLE
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
  vkimage_create_info.vkPhdev = VK_NULL_HANDLE;
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
  if (!app->vkimages.vkImageViews[0].view)
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
    "layout(location = 1) in vec3 inColor;\n"
    "void main() {\n"
    "   gl_Position = vec4(inPosition, 0.0, 1.0);\n"
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
  app->vertex_shader = uvr_shader_file_load(TRIANGLE_VERTEX_SHADER_SPIRV);
  if (!app->vertex_shader.bytes)
    return -1;

  app->fragment_shader = uvr_shader_file_load(TRIANGLE_FRAGMENT_SHADER_SPIRV);
  if (!app->fragment_shader.bytes)
    return -1;
#endif

  struct uvr_vk_shader_module_create_info vertex_shader_module_create_info;
  vertex_shader_module_create_info.vkDevice = app->lgdev.vkDevice;
  vertex_shader_module_create_info.codeSize = app->vertex_shader.byteSize;
  vertex_shader_module_create_info.pCode = app->vertex_shader.bytes;
  vertex_shader_module_create_info.name = "vertex";

  app->shader_modules[0] = uvr_vk_shader_module_create(&vertex_shader_module_create_info);
  if (!app->shader_modules[0].shader)
    return -1;

  struct uvr_vk_shader_module_create_info frag_shader_module_create_info;
  frag_shader_module_create_info.vkDevice = app->lgdev.vkDevice;
  frag_shader_module_create_info.codeSize = app->fragment_shader.byteSize;
  frag_shader_module_create_info.pCode = app->fragment_shader.bytes;
  frag_shader_module_create_info.name = "fragment";

  app->shader_modules[1] = uvr_vk_shader_module_create(&frag_shader_module_create_info);
  if (!app->shader_modules[1].shader)
    return -1;

  return 0;
}


int create_vk_buffers(struct uvr_vk *app) {
  uint32_t cpuVisibleBuffer = 0, gpuVisibleBuffer = 1;

  // Create CPU visible vertex buffer
  struct uvr_vk_buffer_create_info vkVertexBufferCreateInfo;
  vkVertexBufferCreateInfo.vkDevice = app->lgdev.vkDevice;
  vkVertexBufferCreateInfo.vkPhdev = app->phdev;
  vkVertexBufferCreateInfo.bufferFlags = 0;
  vkVertexBufferCreateInfo.bufferSize = sizeof(vertices_pos_color);
  vkVertexBufferCreateInfo.bufferUsage = (VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU || \
                                          VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_CPU) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  vkVertexBufferCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkVertexBufferCreateInfo.queueFamilyIndexCount = 0;
  vkVertexBufferCreateInfo.queueFamilyIndices = NULL;
  vkVertexBufferCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  app->vkbuffers[cpuVisibleBuffer] = uvr_vk_buffer_create(&vkVertexBufferCreateInfo);
  if (!app->vkbuffers[cpuVisibleBuffer].vkBuffer || !app->vkbuffers[0].vkDeviceMemory)
    return -1;

  // Copy vertex data into CPU visible vertex
  void *data = NULL;
  vkMapMemory(app->lgdev.vkDevice, app->vkbuffers[cpuVisibleBuffer].vkDeviceMemory, 0, vkVertexBufferCreateInfo.bufferSize, 0, &data);
  memcpy(data, vertices_pos_color, vkVertexBufferCreateInfo.bufferSize);
  vkUnmapMemory(app->lgdev.vkDevice, app->vkbuffers[cpuVisibleBuffer].vkDeviceMemory);

  if (VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    // Create GPU visible vertex buffer
    struct uvr_vk_buffer_create_info vkVertexBufferGPUCreateInfo;
    vkVertexBufferGPUCreateInfo.vkDevice = app->lgdev.vkDevice;
    vkVertexBufferGPUCreateInfo.vkPhdev = app->phdev;
    vkVertexBufferGPUCreateInfo.bufferFlags = 0;
    vkVertexBufferGPUCreateInfo.bufferSize = vkVertexBufferCreateInfo.bufferSize;
    vkVertexBufferGPUCreateInfo.bufferUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vkVertexBufferGPUCreateInfo.bufferSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkVertexBufferGPUCreateInfo.queueFamilyIndexCount = 0;
    vkVertexBufferGPUCreateInfo.queueFamilyIndices = NULL;
    vkVertexBufferGPUCreateInfo.memPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    app->vkbuffers[gpuVisibleBuffer] = uvr_vk_buffer_create(&vkVertexBufferGPUCreateInfo);
    if (!app->vkbuffers[gpuVisibleBuffer].vkBuffer || !app->vkbuffers[1].vkDeviceMemory)
      return -1;

    VkBufferCopy copyRegion;
    copyRegion.srcOffset = 0;
    copyRegion.dstOffset = 0;
    copyRegion.size = vkVertexBufferCreateInfo.bufferSize;

    // Copy contents from CPU visible buffer over to GPU visible vertex buffer
    struct uvr_vk_copy_info bufferCopyInfo;
    bufferCopyInfo.resourceCopyType = UVR_VK_COPY_VK_BUFFER_TO_VK_BUFFER;
    bufferCopyInfo.commandBuffer = app->vkcbuffs.commandBuffers[0].commandBuffer;
    bufferCopyInfo.vkQueue = app->graphics_queue.vkQueue;
    bufferCopyInfo.srcResource = app->vkbuffers[cpuVisibleBuffer].vkBuffer;
    bufferCopyInfo.dstResource = app->vkbuffers[gpuVisibleBuffer].vkBuffer;
    bufferCopyInfo.bufferCopyInfo = &copyRegion;
    bufferCopyInfo.bufferImageCopyInfo = NULL;
    bufferCopyInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (uvr_vk_copy(&bufferCopyInfo) == -1)
      return -1;
  }

  return 0;
}


int create_vk_graphics_pipeline(struct uvr_vk *app, VkSurfaceFormatKHR *sformat, VkExtent2D extent2D) {

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = app->shader_modules[0].shader;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = app->shader_modules[1].shader;
  fragShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[2] = {vertShaderStageInfo, fragShaderStageInfo};

  VkVertexInputBindingDescription VkVertexInputBindingDescription = {};
  VkVertexInputBindingDescription.binding = 0;
  VkVertexInputBindingDescription.stride = sizeof(struct uvr_vertex_data); // Number of bytes from one entry to another
  VkVertexInputBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Move to next data entry after each vertex

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

  struct uvr_vk_pipeline_layout_create_info gplayout_info;
  gplayout_info.vkDevice = app->lgdev.vkDevice;
  gplayout_info.setLayoutCount = 0;
  gplayout_info.pSetLayouts = NULL;
  gplayout_info.pushConstantRangeCount = 0;
  gplayout_info.pPushConstantRanges = NULL;

  app->gplayout = uvr_vk_pipeline_layout_create(&gplayout_info);
  if (!app->gplayout.vkPipelineLayout)
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
  renderpass_info.vkDevice = app->lgdev.vkDevice;
  renderpass_info.attachmentCount = 1;
  renderpass_info.pAttachments = &colorAttachment;
  renderpass_info.subpassCount = 1;
  renderpass_info.pSubpasses = &subpass;
  renderpass_info.dependencyCount = 1;
  renderpass_info.pDependencies = &subPassDependency;

  app->rpass = uvr_vk_render_pass_create(&renderpass_info);
  if (!app->rpass.renderPass)
    return -1;

  struct uvr_vk_graphics_pipeline_create_info gpipeline_info;
  gpipeline_info.vkDevice = app->lgdev.vkDevice;
  gpipeline_info.stageCount = ARRAY_LEN(shaderStages);
  gpipeline_info.pStages = shaderStages;
  gpipeline_info.pVertexInputState = &vertexInputInfo;
  gpipeline_info.pInputAssemblyState = &inputAssembly;
  gpipeline_info.pTessellationState = NULL;
  gpipeline_info.pViewportState = &viewportState;
  gpipeline_info.pRasterizationState = &rasterizer;
  gpipeline_info.pMultisampleState = &multisampling;
  gpipeline_info.pDepthStencilState = NULL;
  gpipeline_info.pColorBlendState = &colorBlending;
  gpipeline_info.pDynamicState = &dynamicState;
  gpipeline_info.vkPipelineLayout = app->gplayout.vkPipelineLayout;
  gpipeline_info.renderPass = app->rpass.renderPass;
  gpipeline_info.subpass = 0;

  app->gpipeline = uvr_vk_graphics_pipeline_create(&gpipeline_info);
  if (!app->gpipeline.graphicsPipeline)
    return -1;

  return 0;
}


int create_vk_framebuffers(struct uvr_vk *app, VkExtent2D extent2D) {

  struct uvr_vk_framebuffer_create_info vkframebuffer_create_info;
  vkframebuffer_create_info.vkDevice = app->lgdev.vkDevice;
  vkframebuffer_create_info.frameBufferCount = app->vkimages.imageCount;
  vkframebuffer_create_info.vkImageViews = app->vkimages.vkImageViews;
  vkframebuffer_create_info.renderPass = app->rpass.renderPass;
  vkframebuffer_create_info.width = extent2D.width;
  vkframebuffer_create_info.height = extent2D.height;
  vkframebuffer_create_info.layers = 1;

  app->vkframebuffs = uvr_vk_framebuffer_create(&vkframebuffer_create_info);
  if (!app->vkframebuffs.vkFrameBuffers[0].fb)
    return -1;

  return 0;
}


int create_vk_command_buffers(struct uvr_vk *app) {
  struct uvr_vk_command_buffer_create_info commandBufferCreateInfo;
  commandBufferCreateInfo.vkDevice = app->lgdev.vkDevice;
  commandBufferCreateInfo.queueFamilyIndex = app->graphics_queue.familyIndex;
  commandBufferCreateInfo.commandBufferCount = 1;

  app->vkcbuffs = uvr_vk_command_buffer_create(&commandBufferCreateInfo);
  if (!app->vkcbuffs.commandPool)
    return -1;

  return 0;
}


int create_vk_sync_objs(struct uvr_vk *app) {
  struct uvr_vk_sync_obj_create_info syncObjsCreateInfo;
  syncObjsCreateInfo.vkDevice = app->lgdev.vkDevice;
  syncObjsCreateInfo.fenceCount = 1;
  syncObjsCreateInfo.semaphoreCount = 2;

  app->vksyncs = uvr_vk_sync_obj_create(&syncObjsCreateInfo);
  if (!app->vksyncs.vkFences[0].fence && !app->vksyncs.vkSemaphores[0].semaphore)
    return -1;

  return 0;
}


int record_vk_draw_commands(struct uvr_vk *app, uint32_t vkSwapchainImageIndex, VkExtent2D extent2D) {
  struct uvr_vk_command_buffer_record_info commandBufferRecordInfo;
  commandBufferRecordInfo.commandBufferCount = app->vkcbuffs.commandBufferCount;
  commandBufferRecordInfo.commandBuffers = app->vkcbuffs.commandBuffers;
  commandBufferRecordInfo.flags = 0;

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
  renderPassInfo.framebuffer = app->vkframebuffs.vkFrameBuffers[vkSwapchainImageIndex].fb;
  renderPassInfo.renderArea = renderArea;
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = clearColor;

  VkBuffer vertexBuffer;
  if (VK_PHYSICAL_DEVICE_TYPE == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
    vertexBuffer = app->vkbuffers[1].vkBuffer;
  } else {
    vertexBuffer = app->vkbuffers[0].vkBuffer;
  }

  VkDeviceSize offsets[] = {0};

  vkCmdBeginRenderPass(cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, app->gpipeline.graphicsPipeline);
  vkCmdBindVertexBuffers(cmdBuffer, 0, 1, &vertexBuffer, offsets);
  vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
  vkCmdSetScissor(cmdBuffer, 0, 1, &renderArea);
  vkCmdDraw(cmdBuffer, ARRAY_LEN(vertices_pos_color), 1, 0, 0);
  vkCmdEndRenderPass(cmdBuffer);

  if (uvr_vk_command_buffer_record_end(&commandBufferRecordInfo) == -1)
    return -1;

  return 0;
}
