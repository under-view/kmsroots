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
  struct uvrvk app;
  struct uvrvk_destroy appd;
  memset(&app, 0, sizeof(app));
  memset(&appd, 0, sizeof(appd));

  struct uvrxcb xclient;
  struct uvrxcb_destroy xclientd;
  memset(&xclient, 0, sizeof(xclient));
  memset(&xclientd, 0, sizeof(xclientd));

  /*
   * Create Vulkan Instance
   */
  struct uvrvk_instance vkinst = {
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
   * Let the api know of the vulkan instance hande we created
   * in order to properly destroy it
   */
  appd.vkinst = app.instance;

  /*
   * Create Vulkan Physical Device Handle
   */
  struct uvrvk_phdev vkphdev = {
    .vkinst = app.instance,
    .vkpdtype = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
  };

  app.phdev = uvr_vk_phdev_create(&vkphdev);
  if (!app.phdev)
    goto exit_error;


  /*
   * Create xcb client
   */
  struct uvrxcb_window xcbwin = {
    .display = NULL, .screen = NULL,
    .appname = "Example App", .fullscreen = true
  };

  xclient.conn = uvr_xcb_client_create(&xcbwin);
  if (!xclient.conn)
    goto exit_error;

  xclient.window = xcbwin.window;

  /*
   * Let the api know how many xcb client windows where created
   * in order to properly destroy them all.
   */
  xclientd.conn = xclient.conn;
  xclientd.window = xclient.window;

  /*
   * Create Vulkan Surface
   */
  struct uvrvk_surface vksurf = {
    .vkinst = app.instance, .sType = XCB_CLIENT_SURFACE,
    .display = xclient.conn, .window = xclient.window
  };

  app.surface = uvr_vk_surface_create(&vksurf);
  if (!app.surface)
    goto exit_error;

  /*
   * Let the api know of the vulkan surface hande we created
   * in order to properly destroy it
   */
  appd.vksurf = app.surface;

  uvr_xcb_display_window(&xclient);

  /* Wait for 5 seconds to display */
  sleep(5);

  uvr_vk_destory(&appd);
  uvr_xcb_destory(&xclientd);
  return 0;

exit_error:
  uvr_vk_destory(&appd);
  uvr_xcb_destory(&xclientd);
  return 1;
}
