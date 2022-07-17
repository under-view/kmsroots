#include "vulkan.h"
#include "kms.h"
#include "buffer.h"


struct uvr_vk {
  VkInstance instance;
  VkPhysicalDevice phdev;
  struct uvr_vk_lgdev lgdev;
  struct uvr_vk_queue graphics_queue;
};


struct uvr_kms {
  struct uvr_kms_node kmsdev;
  struct uvr_kms_node_display_output_chain dochain;
  struct uvr_buffer kmsbuffs;
#ifdef INCLUDE_SDBUS
  struct uvr_sd_session uvrsd;
#endif
};


int create_vk_instance(struct uvr_vk *app);
int create_kms_node(struct uvr_kms *kms);
int create_gbm_buffers(struct uvr_kms *kms);
int create_vk_device(struct uvr_vk *app, struct uvr_kms *kms);


/*
 * Example code demonstrating how to use Vulkan with KMS
 */
int main(void) {
  struct uvr_vk app;
  struct uvr_vk_destroy appd;
  memset(&app, 0, sizeof(app));
  memset(&appd, 0, sizeof(appd));

  struct uvr_kms kms;
  struct uvr_kms_node_destroy kmsdevd;
  struct uvr_buffer_destroy kmsbuffsd;
  memset(&kms, 0, sizeof(kms));
  memset(&kmsdevd, 0, sizeof(kmsdevd));
  memset(&kmsbuffsd, 0, sizeof(kmsbuffsd));

  if (create_vk_instance(&app) == -1)
    goto exit_error;

  if (create_kms_node(&kms) == -1)
    goto exit_error;

  if (create_gbm_buffers(&kms) == -1)
    goto exit_error;

  if (create_vk_device(&app, &kms) == -1)
    goto exit_error;

exit_error:
  /*
   * Let the api know of what addresses to free and fd's to close
   */
  kmsbuffsd.uvr_buffer_cnt = 1;
  kmsbuffsd.uvr_buffer = &kms.kmsbuffs;
  uvr_buffer_destory(&kmsbuffsd);

  kmsdevd.uvr_kms_node = kms.kmsdev;
  kmsdevd.uvr_kms_node_display_output_chain = kms.dochain;
  uvr_kms_node_destroy(&kmsdevd);

  appd.vkinst = app.instance;
  appd.uvr_vk_lgdev_cnt = 1;
  appd.uvr_vk_lgdev = &app.lgdev;
  uvr_vk_destory(&appd);
#ifdef INCLUDE_SDBUS
  uvr_sd_session_destroy(&kms.uvrsd);
#endif
  return 0;
}


int create_vk_instance(struct uvr_vk *app) {
  /*
   * "VK_LAYER_KHRONOS_validation"
   * All of the useful standard validation is
   * bundled into a layer included in the SDK
   */
  const char *validation_layers[] = {
    "VK_LAYER_KHRONOS_validation"
  };


  const char *instance_extensions[] = {
    "VK_KHR_get_physical_device_properties2",
  };

  /*
   * Create Vulkan Instance
   */
  struct uvr_vk_instance_create_info vk_instance_info;
  vk_instance_info.app_name = "Example App";
  vk_instance_info.engine_name = "No Engine";
  vk_instance_info.enabledLayerCount = ARRAY_LEN(validation_layers);
  vk_instance_info.ppEnabledLayerNames = validation_layers;
  vk_instance_info.enabledExtensionCount = ARRAY_LEN(instance_extensions);
  vk_instance_info.ppEnabledExtensionNames = instance_extensions;

  app->instance = uvr_vk_instance_create(&vk_instance_info);
  if (!app->instance)
    return -1;

  return 0;
}


int create_kms_node(struct uvr_kms *kms) {
  struct uvr_kms_node_create_info kmsnodeinfo;

#ifdef INCLUDE_SDBUS
  if (uvr_sd_session_create(&(kms->uvrsd)) == -1)
    return -1;

  kmsnodeinfo.uvr_sd_session = &(kms->uvrsd);
  kmsnodeinfo.use_logind = true;
#endif

  kmsnodeinfo.kmsnode = NULL;
  kms->kmsdev = uvr_kms_node_create(&kmsnodeinfo);
  if (kms->kmsdev.kmsfd == -1)
    return -1;

  struct uvr_kms_node_display_output_chain_create_info dochain_info;
  dochain_info.kmsfd = kms->kmsdev.kmsfd;

  kms->dochain = uvr_kms_node_display_output_chain_create(&dochain_info);
  if (!kms->dochain.connector || !kms->dochain.encoder || !kms->dochain.crtc || !kms->dochain.plane)
    return -1;

  struct uvr_kms_node_device_capabilites UNUSED kmsnode_devcap;
  kmsnode_devcap = uvr_kms_node_get_device_capabilities(kms->kmsdev.kmsfd);

  return 0;
}


int create_gbm_buffers(struct uvr_kms *kms) {
  struct uvr_buffer_create_info kms_buffs_info;
  kms_buffs_info.bType = UINT32_MAX;
  kms_buffs_info.kmsfd = kms->kmsdev.kmsfd;
  kms_buffs_info.buff_cnt = 2;
  kms_buffs_info.width = 3840;
  kms_buffs_info.height = 2160;
  kms_buffs_info.bitdepth = 24;
  kms_buffs_info.bpp = 32;
  kms_buffs_info.gbm_bo_flags = GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT;
  kms_buffs_info.pixformat = GBM_BO_FORMAT_XRGB8888;
  kms_buffs_info.modifiers = NULL;
  kms_buffs_info.modifiers_cnt = 0;
  kms_buffs_info.bType = GBM_BUFFER;

  kms->kmsbuffs = uvr_buffer_create(&kms_buffs_info);
  if (!kms->kmsbuffs.gbmdev)
    return -1;

  return 0;
}


int create_vk_device(struct uvr_vk *app, struct uvr_kms *kms) {

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
   * Create Vulkan Physical Device Handle, After buffer creation
   * as it can actually effect VkPhysicalDevice creation
   */
  struct uvr_vk_phdev_create_info vk_phdev_info;
  vk_phdev_info.vkinst = app->instance;
  vk_phdev_info.vkpdtype = VK_PHYSICAL_DEVICE_TYPE;
  vk_phdev_info.kmsfd = kms->kmsdev.kmsfd;

  app->phdev = uvr_vk_phdev_create(&vk_phdev_info);
  if (!app->phdev)
    return -1;

  struct uvr_vk_queue_create_info vk_queue_info;
  vk_queue_info.phdev = app->phdev;
  vk_queue_info.queueFlag = VK_QUEUE_GRAPHICS_BIT;

  app->graphics_queue = uvr_vk_queue_create(&vk_queue_info);
  if (app->graphics_queue.famindex == -1)
    return -1;

  VkPhysicalDeviceFeatures phdevfeats = uvr_vk_get_phdev_features(app->phdev);

  struct uvr_vk_lgdev_create_info vk_lgdev_info;
  vk_lgdev_info.vkinst = app->instance;
  vk_lgdev_info.phdev = app->phdev;
  vk_lgdev_info.pEnabledFeatures = &phdevfeats;
  vk_lgdev_info.enabledExtensionCount = ARRAY_LEN(device_extensions);
  vk_lgdev_info.ppEnabledExtensionNames = device_extensions;
  vk_lgdev_info.numqueues = 1;
  vk_lgdev_info.queues = &app->graphics_queue;

  app->lgdev = uvr_vk_lgdev_create(&vk_lgdev_info);
  if (!app->lgdev.device)
    return -1;

  return 0;
}
