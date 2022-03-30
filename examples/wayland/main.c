
#include "common.h"
#include "vulkan.h"
#include "wclient.h"

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

struct appwhole {
  uvrvk app;
  uvrwc wclient;
};

void appwhole_destory(struct appwhole *whole) {
  uvr_vk_destory(&whole->app);
  uvr_wclient_destory(&whole->wclient);
}

/*
 * Example code demonstrating how to connect Vulkan to X11
 */
int main(void) {
  struct appwhole whole;
  memset(&whole, 0, sizeof(struct appwhole));

  whole.wclient.display = uvr_wclient_display_connect(NULL);
  if (!whole.wclient.display) goto exit_error;

  if (uvr_wclient_alloc_interfaces(&whole.wclient) == -1)
    goto exit_error;

  whole.app.instance = uvr_vk_create_instance("Example App", "No Engine",
                                              ARRAY_LEN(validation_layers), validation_layers,
                                              ARRAY_LEN(instance_extensions), instance_extensions);
  if (!whole.app.instance) goto exit_error;

//  uvr_wclient_process_events(&whole.wclient);

  appwhole_destory(&whole);

  return 0;

exit_error:
  appwhole_destory(&whole);
  exit(1);
}
