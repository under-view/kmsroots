#include "vulkan-common.h"
#include "kms.h"

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

  struct uvrkms_node_destroy kmsdevd;
  struct uvrkms_node_create_info kmsnodeinfo;
  memset(&kmsdevd, 0, sizeof(kmsdevd));
  memset(&kmsnodeinfo, 0, sizeof(kmsnodeinfo));

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
  memset(&uvrsd, 0, sizeof(uvrsd));

  if (uvr_sd_session_create(&uvrsd) == -1)
    goto exit_error;
  kmsnodeinfo.uvrsd_session = &uvrsd;
  kmsnodeinfo.use_logind = true;
  kmsdevd.info.uvrsd_session = &uvrsd;
#endif

  kmsnodeinfo.kmsnode = NULL;
  int kmsfd = uvr_kms_node_create(&kmsnodeinfo);
  if (kmsfd == -1)
    goto exit_error;

  /* Let the api know of what filedescriptor to close */
  kmsdevd.kmsfd = kmsfd;

  struct uvrkms_node_display_output_chain dochain;
  struct uvrkms_node_get_display_output_chain_info dochain_info = { .kmsfd = kmsfd };
  dochain = uvr_kms_node_get_display_output_chain(&dochain_info);
  if (!dochain.connector || !dochain.encoder || !dochain.crtc || !dochain.plane)
    goto exit_error;

  /* Let the api know of what display output chain to remove */
  kmsdevd.dochain = &dochain;

  uvr_kms_node_destroy(&kmsdevd);
  uvr_vk_destory(&appd);
#ifdef INCLUDE_SDBUS
  uvr_sd_session_destroy(&uvrsd);
#endif
  return 0;

exit_error:
  uvr_kms_node_destroy(&kmsdevd);
  uvr_vk_destory(&appd);
#ifdef INCLUDE_SDBUS
  uvr_sd_session_destroy(&uvrsd);
#endif
  return 1;
}
