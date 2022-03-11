
#include "common.h"
#include "vulkan.h"
#include "xcb-client.h"

/*
 * "VK_LAYER_KHRONOS_validation"
 * All of the useful standard validation is
 * bundled into a layer included in the SDK
 */
const char *validation_layers[] = { };

const char *instance_extensions[] = {
  "VK_KHR_xcb_surface",
  "VK_KHR_surface"
};

struct appwhole {
  uvcvk app;
  uvcxcb xclient;
};

void appwhole_destory(struct appwhole *whole) {
  uvc_xcb_destory(&whole->xclient);
  uvc_vk_destory(&whole->app);
}

/*
 * Example code demonstrating how to connect Vulkan to X11
 */
int main(void) {
  struct appwhole whole;

  whole.app.instance = uvc_vk_create_instance("Example App", "No Engine",
                                              ARRAY_LEN(validation_layers), validation_layers,
                                              ARRAY_LEN(instance_extensions), instance_extensions);

  whole.xclient = uvc_xcb_create_client(NULL, NULL, "Example App", true);
  if (!whole.xclient.conn || whole.xclient.window == UINT32_MAX) {
    appwhole_destory(&whole);
    exit(1);
  }

  uvc_xcb_display_window(&whole.xclient);

  /* Wait for 5 seconds to display */
  sleep(5);

  appwhole_destory(&whole);

  return 0;
}
