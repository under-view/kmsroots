#include "vulkan-common.h"

#ifdef INCLUDE_SDBUS
#include "sd-dbus.h"
#endif

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


/*
 * Example code demonstrating how to use Vulkan with KMS
 */
int main(void) {
  struct uvrvk app;
  struct uvrvk_destroy appd;
  memset(&app, 0, sizeof(app));
  memset(&appd, 0, sizeof(appd));

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
   * Let the api know how many vulkan instances where created
   * in order to properly destroy them all.
   */
  appd.vkinsts = &app.instance;
  appd.vkinst_cnt = 1;

#ifdef INCLUDE_SDBUS
  struct uvrsd_session uvrsd;
  if (uvr_sd_session_create(&uvrsd) == -1)
    goto exit_error;
#endif


#ifdef INCLUDE_SDBUS
  uvr_sd_session_destroy(&uvrsd);
#endif
  uvr_vk_destory(&appd);
  return 0;

exit_error:
#ifdef INCLUDE_SDBUS
  uvr_sd_session_destroy(&uvrsd);
#endif
  uvr_vk_destory(&appd);
  return 1;
}
