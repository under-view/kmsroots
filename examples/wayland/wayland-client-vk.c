#include "vulkan-common.h"
#include "wclient-common.h"

/*
 * "VK_LAYER_KHRONOS_validation"
 * All of the useful standard validation is
 * bundled into a layer included in the SDK
 */
const char *validation_layers[] = { "VK_LAYER_KHRONOS_validation" };

const char *instance_extensions[] = {
  "VK_KHR_wayland_surface",
  "VK_KHR_surface",
  "VK_KHR_display",
  "VK_EXT_debug_utils"
};

const char *device_extensions[] = {
  "VK_KHR_swapchain"
};


/*
 * Example code demonstrating how use Vulkan with Wayland
 */
int main(void) {
  struct uvr_vk app;
  struct uvr_vk_destroy appd;
  memset(&app, 0, sizeof(app));
  memset(&appd, 0, sizeof(appd));

  struct uvr_wc wc;
  struct uvr_wc_destory wcd;
  memset(&wcd, 0, sizeof(wcd));
  memset(&wc, 0, sizeof(wc));

  struct uvr_shader_destroy shadercd;
  memset(&shadercd, 0, sizeof(shadercd));

  /*
   * Create Vulkan Instance
   */
  struct uvr_vk_instance_create_info vkinst = {
    .app_name = "Example App",
    .engine_name = "No Engine",
    .enabledLayerCount = ARRAY_LEN(validation_layers),
    .ppEnabledLayerNames = validation_layers,
    .enabledExtensionCount = ARRAY_LEN(instance_extensions),
    .ppEnabledExtensionNames = instance_extensions
  };

  app.instance = uvr_vk_instance_create(&vkinst);
  if (!app.instance)
    goto exit_error;

  struct uvr_wc_core_interface_create_info wcinterfaces_info = {
    .wl_display_name = NULL,
    .iType = UVR_WC_WL_COMPOSITOR_INTERFACE | UVR_WC_XDG_WM_BASE_INTERFACE | UVR_WC_WL_SEAT_INTERFACE
  };

  wc.wcinterfaces = uvr_wc_core_interface_create(&wcinterfaces_info);
  if (!wc.wcinterfaces.display || !wc.wcinterfaces.registry || !wc.wcinterfaces.compositor) goto exit_error;

  struct uvr_wc_surface_create_info uvrwcsurf_info = {
    .uvrwccore = &wc.wcinterfaces,
    .uvrwcbuff = NULL,
    .appname = "Example Window",
    .fullscreen = true
  };

  wc.wcsurf = uvr_wc_surface_create(&uvrwcsurf_info);
  if (!wc.wcsurf.surface) goto exit_error;

  /*
   * Create Vulkan Surface
   */
  struct uvr_vk_surface_create_info vksurf = {
    .vkinst = app.instance, .sType = WAYLAND_CLIENT_SURFACE,
    .display = wc.wcinterfaces.display, .surface = wc.wcsurf.surface
  };

  app.surface = uvr_vk_surface_create(&vksurf);
  if (!app.surface)
    goto exit_error;

  /*
   * Create Vulkan Physical Device Handle, After Window Surface
   * so that it doesn't effect VkPhysicalDevice selection
   */
  struct uvr_vk_phdev_create_info vkphdev = {
    .vkinst = app.instance,
    .vkpdtype = VK_PHYSICAL_DEVICE_TYPE
  };

  app.phdev = uvr_vk_phdev_create(&vkphdev);
  if (!app.phdev)
    goto exit_error;

  struct uvr_vk_queue_create_info vkqueueinfo = { .phdev = app.phdev, .queueFlag = VK_QUEUE_GRAPHICS_BIT };
  app.graphics_queue = uvr_vk_queue_create(&vkqueueinfo);
  if (app.graphics_queue.famindex == -1)
    goto exit_error;

  VkPhysicalDeviceFeatures phdevfeats = uvr_vk_get_phdev_features(app.phdev);
  struct uvr_vk_lgdev_create_info vklgdev_info = {
    .vkinst = app.instance, .phdev = app.phdev,
    .pEnabledFeatures = &phdevfeats,
    .enabledExtensionCount = ARRAY_LEN(device_extensions),
    .ppEnabledExtensionNames = device_extensions,
    .numqueues = 1,
    .queues = &app.graphics_queue
  };

  app.lgdev = uvr_vk_lgdev_create(&vklgdev_info);
  if (!app.lgdev.device)
    goto exit_error;

  /* choose swap chain surface format & present mode */
  VkSurfaceFormatKHR sformat;
  VkPresentModeKHR presmode;
  VkExtent2D extent2D = {3840, 2160};

  VkSurfaceCapabilitiesKHR surfcap = uvr_vk_get_surface_capabilities(app.phdev, app.surface);
  struct uvr_vk_surface_format sformats = uvr_vk_get_surface_formats(app.phdev, app.surface);
  struct uvr_vk_surface_present_mode spmodes = uvr_vk_get_surface_present_modes(app.phdev, app.surface);

  for (uint32_t s = 0; s < sformats.fcount; s++) {
    if (sformats.formats[s].format == VK_FORMAT_B8G8R8A8_SRGB && sformats.formats[s].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      sformat = sformats.formats[s];
    }
  }

  for (uint32_t p = 0; p < spmodes.mcount; p++) {
    if (spmodes.modes[p] == VK_PRESENT_MODE_MAILBOX_KHR) {
      presmode = spmodes.modes[p];
    }
  }

  free(sformats.formats); sformats.formats = NULL;
  free(spmodes.modes); spmodes.modes = NULL;

  struct uvr_vk_swapchain_create_info sc_create_info = {
    .lgdev = app.lgdev.device, .surfaceKHR = app.surface, .surfcap = surfcap, .surfaceFormat = sformat,
    .extent2D = extent2D, .imageArrayLayers = 1, .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE, .queueFamilyIndexCount = 0, .pQueueFamilyIndices = NULL,
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR, .presentMode = presmode, .clipped = VK_TRUE,
    .oldSwapchain = VK_NULL_HANDLE
  };

  app.schain = uvr_vk_swapchain_create(&sc_create_info);
  if (!app.schain.swapchain)
    goto exit_error;

  struct uvr_vk_image_create_info vkimage_create_info = {
    .lgdev = app.lgdev.device, .swapchain = app.schain.swapchain, .flags = 0,
    .viewType = VK_IMAGE_VIEW_TYPE_2D, .format = sformat.format,
    .components.r = VK_COMPONENT_SWIZZLE_IDENTITY, .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
    .components.b = VK_COMPONENT_SWIZZLE_IDENTITY, .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
    .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
    .subresourceRange.baseMipLevel = 0, .subresourceRange.levelCount = 1,
    .subresourceRange.baseArrayLayer = 0, .subresourceRange.layerCount = 1
  };

  app.vkimages = uvr_vk_image_create(&vkimage_create_info);
  if (!app.vkimages.views[0].view) goto exit_error;

#ifdef INCLUDE_SHADERC
  struct uvr_shader_spirv_create_info vert_shader_create_info = {
    .kind = VK_SHADER_STAGE_VERTEX_BIT, .source = triangle_vertex_shader,
    .filename = "vert.spv", .entry_point = "main"
  };

  struct uvr_shader_spirv_create_info frag_shader_create_info = {
    .kind = VK_SHADER_STAGE_FRAGMENT_BIT, .source = triangle_fragment_shader,
    .filename = "frag.spv", .entry_point = "main"
  };

  app.vertex_shader = uvr_shader_compile_buffer_to_spirv(&vert_shader_create_info);
  if (!app.vertex_shader.bytes) goto exit_error;

  app.fragment_shader = uvr_shader_compile_buffer_to_spirv(&frag_shader_create_info);
  if (!app.fragment_shader.bytes) goto exit_error;
#else
  app.vertex_shader = uvr_shader_file_load(TRIANGLE_VERTEX_SHADER_SPIRV);
  if (!app.vertex_shader.bytes) goto exit_error;

  app.fragment_shader = uvr_shader_file_load(TRIANGLE_FRAGMENT_SHADER_SPIRV);
  if (!app.fragment_shader.bytes) goto exit_error;
#endif

  app.shader_modules[0] = uvr_vk_shader_module_create(&(struct uvr_vk_shader_module_create_info) {
    .lgdev = app.lgdev.device, .codeSize = app.vertex_shader.bsize,
    .pCode = app.vertex_shader.bytes, .name = "vertex"
  });

  if (!app.shader_modules[0].shader) goto exit_error;

  app.shader_modules[1] = uvr_vk_shader_module_create(&(struct uvr_vk_shader_module_create_info) {
    .lgdev = app.lgdev.device, .codeSize = app.fragment_shader.bsize,
    .pCode = app.fragment_shader.bytes, .name = "fragment"
  });

  if (!app.shader_modules[1].shader) goto exit_error;

  /* Lines 206 - 324, taken directly from https://vulkan-tutorial.com */
  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = app.shader_modules[0].shader;
  vertShaderStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = app.shader_modules[1].shader;
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
  colorAttachment.format = sformat.format;
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

  struct uvr_vk_pipeline_layout_create_info gplayout_info = {
    .lgdev = app.lgdev.device,
    .setLayoutCount = 0, .pSetLayouts = NULL,
    .pushConstantRangeCount = 0, .pPushConstantRanges = NULL
  };

  app.gplayout = uvr_vk_pipeline_layout_create(&gplayout_info);
  if (!app.gplayout.playout) goto exit_error;

  struct uvr_vk_render_pass_create_info renderpass_info = {
    .lgdev = app.lgdev.device,
    .attachmentCount = 1, .pAttachments = &colorAttachment,
    .subpassCount = 1, .pSubpasses = &subpass,
    .dependencyCount = 0, .pDependencies = NULL
  };

  app.rpass = uvr_vk_render_pass_create(&renderpass_info);
  if (!app.rpass.renderpass) goto exit_error;

  struct uvr_vk_graphics_pipeline_create_info gpipeline_info = {
    .lgdev = app.lgdev.device,
    .stageCount = ARRAY_LEN(shaderStages), .pStages = shaderStages,
    .pVertexInputState = &vertexInputInfo, .pInputAssemblyState = &inputAssembly,
    .pTessellationState = NULL, .pViewportState = &viewportState,
    .pRasterizationState = &rasterizer, .pMultisampleState = &multisampling,
    .pDepthStencilState = NULL, .pColorBlendState = &colorBlending,
    .pDynamicState = &dynamicState, .layout = app.gplayout.playout,
    .renderPass = app.rpass.renderpass, .subpass = 0
  };

  app.gpipeline = uvr_vk_graphics_pipeline_create(&gpipeline_info);
  if (!app.gpipeline.graphics_pipeline) goto exit_error;

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
  appd.vklgdevs_cnt = 1;
  appd.vklgdevs = &app.lgdev;
  appd.vkswapchain_cnt = 1;
  appd.vkswapchains = &app.schain;
  appd.vkimage_cnt = 1;
  appd.vkimages = &app.vkimages;
  appd.vkshader_cnt = ARRAY_LEN(app.shader_modules);
  appd.vkshaders = app.shader_modules;
  appd.vkplayout_cnt = 1;
  appd.vkplayouts = &app.gplayout;
  appd.vkrenderpass_cnt = 1;
  appd.vkrenderpasses = &app.rpass;
  appd.vkgraphics_pipelines_cnt = 1;
  appd.vkgraphics_pipelines = &app.gpipeline;
  uvr_vk_destory(&appd);

  wcd.wccinterface = wc.wcinterfaces;
  wcd.wcsurface = wc.wcsurf;
  uvr_wc_destory(&wcd);
  return 0;
}
