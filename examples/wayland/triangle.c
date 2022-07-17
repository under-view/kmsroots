#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "wclient.h"
#include "vulkan.h"
#include "shader.h"


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
};


struct uvr_wc {
  struct uvr_wc_core_interface wcinterfaces;
  struct uvr_wc_surface wcsurf;
};


int create_wc_vk_surface(struct uvr_vk *app, struct uvr_wc *wc, int *cbuf, bool *running);
int create_vk_instance(struct uvr_vk *uvrvk);
int create_vk_device(struct uvr_vk *app);
int create_vk_swapchain(struct uvr_vk *app, VkSurfaceFormatKHR *sformat, VkExtent2D extent2D);
int create_vk_images(struct uvr_vk *app, VkSurfaceFormatKHR *sformat);
int create_vk_shader_modules(struct uvr_vk *app);
int create_vk_graphics_pipeline(struct uvr_vk *app, VkSurfaceFormatKHR *sformat, VkExtent2D extent2D);
int create_vk_framebuffers(struct uvr_vk *app, VkExtent2D extent2D);
int create_vk_command_buffers(struct uvr_vk *app);
int record_vk_command_buffers(struct uvr_vk *app);


void UNUSED render(bool UNUSED *running, int UNUSED *cbuf, void UNUSED *data) {
  // Initentionally left blank for now
}


/*
 * Example code demonstrating how use Vulkan with Wayland
 */
int main(void) {
  struct uvr_vk app;
  struct uvr_vk_destroy appd;
  memset(&app, 0, sizeof(app));
  memset(&appd, 0, sizeof(appd));

  struct uvr_wc wc;
  struct uvr_wc_destroy wcd;
  memset(&wcd, 0, sizeof(wcd));
  memset(&wc, 0, sizeof(wc));

  struct uvr_shader_destroy shadercd;
  memset(&shadercd, 0, sizeof(shadercd));

  if (create_vk_instance(&app) == -1)
    goto exit_error;

  static int cbuf = 0;
  static bool running = true;
  if (create_wc_vk_surface(&app, &wc, &cbuf, &running) == -1)
    goto exit_error;

  /*
   * Create Vulkan Physical Device Handle, After Window Surface
   * so that it doesn't effect VkPhysicalDevice selection
   */
  if (create_vk_device(&app) == -1)
    goto exit_error;

  VkSurfaceFormatKHR sformat;
  VkExtent2D extent2D = {3840, 2160};
  if (create_vk_swapchain(&app, &sformat, extent2D) == -1)
    goto exit_error;

  if (create_vk_images(&app, &sformat) == -1)
    goto exit_error;

  if (create_vk_shader_modules(&app) == -1)
    goto exit_error;

  if (create_vk_graphics_pipeline(&app, &sformat, extent2D) == -1)
    goto exit_error;

  if (record_vk_command_buffers(&app) == -1)
    goto exit_error;

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
  uvr_vk_destory(&appd);

  wcd.uvr_wc_core_interface = wc.wcinterfaces;
  wcd.uvr_wc_surface = wc.wcsurf;
  uvr_wc_destroy(&wcd);
  return 0;
}


int create_wc_vk_surface(struct uvr_vk *app, struct uvr_wc *wc, int *cbuf, bool *running) {

  struct uvr_wc_core_interface_create_info wc_core_interfaces_info;
  wc_core_interfaces_info.wl_display_name = NULL;
  wc_core_interfaces_info.iType = UVR_WC_WL_COMPOSITOR_INTERFACE | UVR_WC_XDG_WM_BASE_INTERFACE | UVR_WC_WL_SEAT_INTERFACE | UVR_WC_ZWP_FULLSCREEN_SHELL_V1;

  wc->wcinterfaces = uvr_wc_core_interface_create(&wc_core_interfaces_info);
  if (!wc->wcinterfaces.display || !wc->wcinterfaces.registry || !wc->wcinterfaces.compositor)
    return -1;

  struct uvr_wc_surface_create_info uvr_wc_surface_info;
  uvr_wc_surface_info.uvrwccore = &wc->wcinterfaces;
  uvr_wc_surface_info.uvrwcbuff = NULL;
  uvr_wc_surface_info.appname = "WL Vulkan Triangle Example";
  uvr_wc_surface_info.fullscreen = true;
  uvr_wc_surface_info.renderer = NULL;
  uvr_wc_surface_info.rendererdata = wc;
  uvr_wc_surface_info.renderercbuf = cbuf;
  uvr_wc_surface_info.rendererruning = running;

  wc->wcsurf = uvr_wc_surface_create(&uvr_wc_surface_info);
  if (!wc->wcsurf.surface)
    return -1;

  /*
   * Create Vulkan Surface
   */
  struct uvr_vk_surface_create_info vksurf;
  vksurf.vkinst = app->instance;
  vksurf.sType = WAYLAND_CLIENT_SURFACE;
  vksurf.display = wc->wcinterfaces.display;
  vksurf.surface = wc->wcsurf.surface;

  app->surface = uvr_vk_surface_create(&vksurf);
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
    "VK_KHR_wayland_surface",
    "VK_KHR_surface",
    "VK_KHR_display",
    "VK_EXT_debug_utils"
  };

  struct uvr_vk_instance_create_info vkinst;
  vkinst.app_name = "Example App";
  vkinst.engine_name = "No Engine";
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
  vkphdev.vkinst = app->instance;
  vkphdev.vkpdtype = VK_PHYSICAL_DEVICE_TYPE;
#ifdef INCLUDE_KMS
  vkphdev.kmsfd = -1;
#endif

  app->phdev = uvr_vk_phdev_create(&vkphdev);
  if (!app->phdev)
    return -1;

  struct uvr_vk_queue_create_info vkqueueinfo;
  vkqueueinfo.phdev = app->phdev;
  vkqueueinfo.queueFlag = VK_QUEUE_GRAPHICS_BIT;

  app->graphics_queue = uvr_vk_queue_create(&vkqueueinfo);
  if (app->graphics_queue.famindex == -1)
    return -1;

  VkPhysicalDeviceFeatures phdevfeats = uvr_vk_get_phdev_features(app->phdev);

  struct uvr_vk_lgdev_create_info vklgdevinfo;
  vklgdevinfo.vkinst = app->instance;
  vklgdevinfo.phdev = app->phdev;
  vklgdevinfo.pEnabledFeatures = &phdevfeats;
  vklgdevinfo.enabledExtensionCount = ARRAY_LEN(device_extensions);
  vklgdevinfo.ppEnabledExtensionNames = device_extensions;
  vklgdevinfo.numqueues = 1;
  vklgdevinfo.queues = &app->graphics_queue;

  app->lgdev = uvr_vk_lgdev_create(&vklgdevinfo);
  if (!app->lgdev.device)
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
  for (uint32_t s = 0; s < sformats.fcount; s++) {
    if (sformats.formats[s].format == VK_FORMAT_B8G8R8A8_SRGB && sformats.formats[s].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      *sformat = sformats.formats[s];
    }
  }

  for (uint32_t p = 0; p < spmodes.mcount; p++) {
    if (spmodes.modes[p] == VK_PRESENT_MODE_MAILBOX_KHR) {
      presmode = spmodes.modes[p];
    }
  }

  free(sformats.formats); sformats.formats = NULL;
  free(spmodes.modes); spmodes.modes = NULL;

  struct uvr_vk_swapchain_create_info scinfo;
  scinfo.lgdev = app->lgdev.device;
  scinfo.surfaceKHR = app->surface;
  scinfo.surfcap = surfcap;
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
  if (!app->schain.swapchain)
    return -1;

  return 0;
}


int create_vk_images(struct uvr_vk *app, VkSurfaceFormatKHR *sformat) {

  struct uvr_vk_image_create_info vkimage_create_info;
  vkimage_create_info.lgdev = app->lgdev.device;
  vkimage_create_info.swapchain = app->schain.swapchain;
  vkimage_create_info.flags = 0;
  vkimage_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
  vkimage_create_info.format = sformat->format;
  vkimage_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  vkimage_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  vkimage_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  vkimage_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  vkimage_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  vkimage_create_info.subresourceRange.baseMipLevel = 0;
  vkimage_create_info.subresourceRange.levelCount = 1;
  vkimage_create_info.subresourceRange.baseArrayLayer = 0;
  vkimage_create_info.subresourceRange.layerCount = 1;

  app->vkimages = uvr_vk_image_create(&vkimage_create_info);
  if (!app->vkimages.views[0].view)
    return -1;

  return 0;
}


int create_vk_shader_modules(struct uvr_vk *app) {

#ifdef INCLUDE_SHADERC
  const char vertex_shader[] =
    "#version 450\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "out gl_PerVertex {\n"
    "   vec4 gl_Position;\n"
    "};\n"
    "layout(location = 0) in vec2 i_Position;\n"
    "layout(location = 1) in vec3 i_Color;\n"
    "layout(location = 0) out vec3 v_Color;\n"
    "void main() {\n"
    "   gl_Position = vec4(i_Position, 0.0, 1.0);\n"
    "   v_Color = i_Color;\n"
    "}";


  const char fragment_shader[] =
    "#version 450\n"
    "#extension GL_ARB_separate_shader_objects : enable\n"
    "layout(location = 0) in vec3 v_Color;\n"
    "layout(location = 0) out vec4 o_Color;\n"
    "void main() { o_Color = vec4(v_Color, 1.0); }";

  struct uvr_shader_spirv_create_info vert_shader_create_info;
  vert_shader_create_info.kind = VK_SHADER_STAGE_VERTEX_BIT;
  vert_shader_create_info.source = vertex_shader;
  vert_shader_create_info.filename = "vert.spv";
  vert_shader_create_info.entry_point = "main";

  struct uvr_shader_spirv_create_info frag_shader_create_info;
  frag_shader_create_info.kind = VK_SHADER_STAGE_FRAGMENT_BIT;
  frag_shader_create_info.source = fragment_shader;
  frag_shader_create_info.filename = "frag.spv";
  frag_shader_create_info.entry_point = "main";

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
  if (!app->fragment_shader.bytes) goto exit_error;
#endif

  struct uvr_vk_shader_module_create_info vertex_shader_module_create_info;
  vertex_shader_module_create_info.lgdev = app->lgdev.device;
  vertex_shader_module_create_info.codeSize = app->vertex_shader.bsize;
  vertex_shader_module_create_info.pCode = app->vertex_shader.bytes;
  vertex_shader_module_create_info.name = "vertex";

  app->shader_modules[0] = uvr_vk_shader_module_create(&vertex_shader_module_create_info);
  if (!app->shader_modules[0].shader)
    return -1;

  struct uvr_vk_shader_module_create_info frag_shader_module_create_info;
  frag_shader_module_create_info.lgdev = app->lgdev.device;
  frag_shader_module_create_info.codeSize = app->fragment_shader.bsize;
  frag_shader_module_create_info.pCode = app->fragment_shader.bytes;
  frag_shader_module_create_info.name = "fragment";

  app->shader_modules[1] = uvr_vk_shader_module_create(&frag_shader_module_create_info);
  if (!app->shader_modules[1].shader)
    return -1;

  return 0;
}


int create_vk_graphics_pipeline(struct uvr_vk *app, VkSurfaceFormatKHR *sformat, VkExtent2D extent2D) {

  /* Taken directly from https://vulkan-tutorial.com */
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

  /*
   * Provides details for loading vertex data
   */
  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = NULL;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = NULL;

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
  dynamicStates[1] = VK_DYNAMIC_STATE_LINE_WIDTH;

  VkPipelineDynamicStateCreateInfo dynamicState = {};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = ARRAY_LEN(dynamicStates);
  dynamicState.pDynamicStates = dynamicStates;

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

  struct uvr_vk_pipeline_layout_create_info gplayout_info;
  gplayout_info.lgdev = app->lgdev.device;
  gplayout_info.setLayoutCount = 0;
  gplayout_info.pSetLayouts = NULL;
  gplayout_info.pushConstantRangeCount = 0;
  gplayout_info.pPushConstantRanges = NULL;

  app->gplayout = uvr_vk_pipeline_layout_create(&gplayout_info);
  if (!app->gplayout.playout)
    return -1;

  struct uvr_vk_render_pass_create_info renderpass_info;
  renderpass_info.lgdev = app->lgdev.device;
  renderpass_info.attachmentCount = 1;
  renderpass_info.pAttachments = &colorAttachment;
  renderpass_info.subpassCount = 1;
  renderpass_info.pSubpasses = &subpass;
  renderpass_info.dependencyCount = 0;
  renderpass_info.pDependencies = NULL;

  app->rpass = uvr_vk_render_pass_create(&renderpass_info);
  if (!app->rpass.renderpass)
    return -1;

  struct uvr_vk_graphics_pipeline_create_info gpipeline_info;
  gpipeline_info.lgdev = app->lgdev.device;
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
  gpipeline_info.layout = app->gplayout.playout;
  gpipeline_info.renderPass = app->rpass.renderpass;
  gpipeline_info.subpass = 0;

  app->gpipeline = uvr_vk_graphics_pipeline_create(&gpipeline_info);
  if (!app->gpipeline.graphics_pipeline)
    return -1;

  return 0;
}


int create_vk_framebuffers(struct uvr_vk *app, VkExtent2D extent2D) {

  struct uvr_vk_framebuffer_create_info vkframebuffer_create_info;
  vkframebuffer_create_info.lgdev = app->lgdev.device;
  vkframebuffer_create_info.fbcount = app->vkimages.vcount;
  vkframebuffer_create_info.vkimageviews = app->vkimages.views;
  vkframebuffer_create_info.renderPass = app->rpass.renderpass;
  vkframebuffer_create_info.width = extent2D.width;
  vkframebuffer_create_info.height = extent2D.height;
  vkframebuffer_create_info.layers = 1;

  app->vkframebuffs = uvr_vk_framebuffer_create(&vkframebuffer_create_info);
  if (!app->vkframebuffs.vkfbs[0].vkfb)
    return -1;

  return 0;
}


int record_vk_command_buffers(struct uvr_vk *app) {
  struct uvr_vk_command_buffer_create_info cmdbuff_create_info;
  cmdbuff_create_info.lgdev = app->lgdev.device;
  cmdbuff_create_info.queueFamilyIndex = app->graphics_queue.famindex;
  cmdbuff_create_info.commandBufferCount = 1;

  app->vkcbuffs = uvr_vk_command_buffer_create(&cmdbuff_create_info);
  if (!app->vkcbuffs.cmdpool)
    return -1;

  struct uvr_vk_command_buffer_record_info cbuffrec;
  cbuffrec.cmdbuff_cnt = app->vkcbuffs.cmdbuff_cnt;
  cbuffrec.cmdbuffs = app->vkcbuffs.cmdbuffs;
  cbuffrec.flags = 0;

  if (uvr_vk_command_buffer_record_begin(&cbuffrec) == -1)
    return -1;

  if (uvr_vk_command_buffer_record_end(&cbuffrec) == -1)
    return -1;

  return 0;
}
