#include "vulkan-common.h"
#include "xclient.h"

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


/*
 * Example code demonstrating how to use Vulkan with XCB
 */
int main(void) {
  struct uvr_vk app;
  struct uvr_vk_destroy appd;
  memset(&app, 0, sizeof(app));
  memset(&appd, 0, sizeof(appd));

  struct uvr_xcb xclient;
  struct uvr_xcb_destroy xclientd;
  memset(&xclient, 0, sizeof(xclient));
  memset(&xclientd, 0, sizeof(xclientd));

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

  /*
   * Create Vulkan Physical Device Handle
   */
  struct uvr_vk_phdev_create_info vkphdev = {
    .vkinst = app.instance,
    .vkpdtype = VK_PHYSICAL_DEVICE_TYPE
  };

  app.phdev = uvr_vk_phdev_create(&vkphdev);
  if (!app.phdev)
    goto exit_error;

  /*
   * Create xcb client
   */
  struct uvr_xcb_window_create_info xcbwin = {
    .display = NULL, .screen = NULL,
    .appname = "Example App", .fullscreen = true
  };

  xclient.conn = uvr_xcb_client_create(&xcbwin);
  if (!xclient.conn)
    goto exit_error;

  xclient.window = xcbwin.window;

  /*
   * Create Vulkan Surface
   */
  struct uvr_vk_surface_create_info vksurf = {
    .vkinst = app.instance, .sType = XCB_CLIENT_SURFACE,
    .display = xclient.conn, .window = xclient.window
  };

  app.surface = uvr_vk_surface_create(&vksurf);
  if (!app.surface)
    goto exit_error;

  uvr_xcb_display_window(&xclient);

  /* Wait for 5 seconds to display */
  sleep(5);

exit_error:
  /*
   * Let the api know of what addresses to free and fd's to close
   */
  appd.vkinst = app.instance;
  appd.vksurf = app.surface;
  xclientd.conn = xclient.conn;
  xclientd.window = xclient.window;

  uvr_vk_destory(&appd);
  uvr_xcb_destory(&xclientd);
  return 0;
}
