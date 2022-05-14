#include "vulkan-common.h"
#include "kms.h"
#include "buffer.h"

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

  struct uvrbuff UNUSED kmsbuffs;
  struct uvrbuff_destroy kmsbuffsd;
  memset(&kmsbuffs, 0, sizeof(kmsbuffs));
  memset(&kmsbuffsd, 0, sizeof(kmsbuffsd));

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

#ifdef INCLUDE_SDBUS
  struct uvrsd_session uvrsd;
  memset(&uvrsd, 0, sizeof(uvrsd));

  if (uvr_sd_session_create(&uvrsd) == -1)
    goto exit_error;
  kmsnodeinfo.uvrsd_session = &uvrsd;
  kmsnodeinfo.use_logind = true;
#endif

  kmsnodeinfo.kmsnode = NULL;
  int kmsfd = uvr_kms_node_create(&kmsnodeinfo);
  if (kmsfd == -1)
    goto exit_error;

  /*
   * Create Vulkan Physical Device Handle
   */
  struct uvrvk_phdev vkphdev = {
    .vkinst = app.instance,
    .vkpdtype = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    .kmsfd = kmsfd
  };

  app.phdev = uvr_vk_phdev_create(&vkphdev);
  if (!app.phdev)
    goto exit_error;

  struct uvrkms_node_display_output_chain dochain;
  struct uvrkms_node_get_display_output_chain_info dochain_info = { .kmsfd = kmsfd };
  dochain = uvr_kms_node_get_display_output_chain(&dochain_info);
  if (!dochain.connector || !dochain.encoder || !dochain.crtc || !dochain.plane)
    goto exit_error;

  struct uvrkms_node_device_capabilites UNUSED kmsnode_devcap;
  kmsnode_devcap = uvr_kms_node_get_device_capabilities(kmsfd);

  struct uvrbuff_create_info UNUSED kmsbuffs_info = {
    .bType = UINT32_MAX, .kmsfd = kmsfd, .buff_cnt = 2,
    .width = 3840, .height = 2160, .bitdepth = 24, .bpp = 32,
    .gbm_bo_flags = GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT,
    .pixformat = GBM_BO_FORMAT_XRGB8888, .modifiers = NULL,
    .modifiers_cnt = 0
  };

  kmsbuffs_info.bType = GBM_BUFFER;
  kmsbuffs = uvr_buffer_create(&kmsbuffs_info);
  if (!kmsbuffs.gbmdev) goto exit_error;

exit_error:
  /*
   * Let the api know of what addresses to free and fd's to close
   */
  appd.vkinst = app.instance;
  kmsdevd.dochain = &dochain;
  kmsdevd.kmsfd = kmsfd;
  kmsbuffsd.gbmdev = kmsbuffs.gbmdev;
  kmsbuffsd.buff_cnt = kmsbuffs_info.buff_cnt;
  kmsbuffsd.info_buffers = kmsbuffs.info_buffers;

  uvr_buffer_destory(&kmsbuffsd);
  uvr_kms_node_destroy(&kmsdevd);
  uvr_vk_destory(&appd);
#ifdef INCLUDE_SDBUS
  kmsdevd.info.uvrsd_session = &uvrsd;
  uvr_sd_session_destroy(&uvrsd);
#endif
  return 0;
}
