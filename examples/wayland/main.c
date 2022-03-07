
#include "common.h"
#include "vulkan.h"

/*
 * "VK_LAYER_KHRONOS_validation"
 * All of the useful standard validation is
 * bundled into a layer included in the SDK
 */
const char *validation_layers[] = { };

const char *instance_extensions[] = {
  "VK_KHR_wayland_surface",
  "VK_KHR_surface"
};

/*
 * Example code demonstrating how to connect Vulkan to X11
 */
int main(void) {
  uvcvk app;

  app.instance = uvc_vk_create_instance("Example App", "No Engine",
                                        ARRAY_LEN(validation_layers), validation_layers,
                                        ARRAY_LEN(instance_extensions), instance_extensions);

  uvc_vk_destory(&app);

  return 0;
}
