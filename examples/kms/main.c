#include "vulkan.h"

/*
 * "VK_LAYER_KHRONOS_validation"
 * All of the useful standard validation is
 * bundled into a layer included in the SDK
 */
const char *validation_layers[] = { };


const char *instance_extensions[] = {
  "VK_KHR_get_physical_device_properties2",
};


const char *device_extensions[] = {
  "VK_EXT_image_drm_format_modifier",
  "VK_KHR_image_format_list",
  "VK_KHR_bind_memory2",
  "VK_KHR_sampler_ycbcr_conversion",
  "VK_KHR_maintenance1",
  "VK_KHR_get_memory_requirements2",
  "VK_KHR_driver_properties"
};


struct appwhole {
  struct uvrvk app;
};


static void appwhole_destory(struct appwhole *whole) {
  uvr_vk_destory(&whole->app);
}


/*
 * Example code demonstrating how to connect Vulkan to X11
 */
int main(void) {
  struct appwhole whole;
  memset(&whole, 0, sizeof(struct appwhole));

  if (uvr_vk_create_instance(&whole.app, "Example App", "No Engine",
                             ARRAY_LEN(validation_layers), validation_layers,
                             ARRAY_LEN(instance_extensions), instance_extensions) == -1)
    goto exit_error;

  appwhole_destory(&whole);
  return 0;

exit_error:
  appwhole_destory(&whole);
  return 1;
}
