#ifndef VULKAN_COMMON_H
#define VULKAN_COMMON_H

#include "vulkan.h"
#include "shader.h"

/*
 * struct uvr_vk (Underview Renderer Vulkan)
 *
 * @instance - Keeps track of all application state. The connection to the vulkan loader.
 * @surface  - Containing the platform-specific information about the surface
 * @phdev    - Physical device handle representing actual connection to the physical device (GPU, CPU, etc...)
 */
struct uvr_vk {
  VkInstance instance;
  VkPhysicalDevice phdev;
  struct uvr_vk_lgdev lgdev;
  struct uvr_vk_queue graphics_queue;

#if defined(INCLUDE_WAYLAND) || defined(INCLUDE_XCB)
  VkSurfaceKHR surface;
  struct uvr_vk_swapchain schain;
#endif

  struct uvr_vk_image vkimages;
#ifdef INCLUDE_SHADERC
  struct uvr_shader_spirv vertex_shader;
  struct uvr_shader_spirv fragment_shader;
#else
  struct uvr_shader_file vertex_shader;
  struct uvr_shader_file fragment_shader;
#endif
  struct uvr_vk_shader_module shader_modules[2];
};

#endif


const char triangle_vertex_shader[] =
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

const char triangle_fragment_shader[] =
  "#version 450\n"
  "#extension GL_ARB_separate_shader_objects : enable\n"
  "layout(location = 0) in vec3 v_Color;\n"
  "layout(location = 0) out vec4 o_Color;\n"
  "void main() { o_Color = vec4(v_Color, 1.0); }";
