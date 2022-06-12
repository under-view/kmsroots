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


struct uvr_kms {
  struct uvr_kms_node kmsdev;
  struct uvr_kms_node_display_output_chain dochain;
  struct uvr_buffer kmsbuffs;
#ifdef INCLUDE_SDBUS
  struct uvr_sd_session uvrsd;
#endif
};


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

  struct uvr_kms_node_create_info kmsnodeinfo;

#ifdef INCLUDE_SDBUS
  if (uvr_sd_session_create(&kms.uvrsd) == -1)
    goto exit_error;
  kmsnodeinfo.uvr_sd_session = &kms.uvrsd;
  kmsnodeinfo.use_logind = true;
#endif

  kmsnodeinfo.kmsnode = NULL;
  kms.kmsdev = uvr_kms_node_create(&kmsnodeinfo);
  if (kms.kmsdev.kmsfd == -1)
    goto exit_error;

  struct uvr_kms_node_display_output_chain_create_info dochain_info = { .kmsfd = kms.kmsdev.kmsfd };
  kms.dochain = uvr_kms_node_display_output_chain_create(&dochain_info);
  if (!kms.dochain.connector || !kms.dochain.encoder || !kms.dochain.crtc || !kms.dochain.plane)
    goto exit_error;

  struct uvr_kms_node_device_capabilites UNUSED kmsnode_devcap;
  kmsnode_devcap = uvr_kms_node_get_device_capabilities(kms.kmsdev.kmsfd);

  struct uvr_buffer_create_info UNUSED kmsbuffs_info = {
    .bType = UINT32_MAX, .kmsfd = kms.kmsdev.kmsfd, .buff_cnt = 2,
    .width = 3840, .height = 2160, .bitdepth = 24, .bpp = 32,
    .gbm_bo_flags = GBM_BO_USE_RENDERING | GBM_BO_USE_SCANOUT,
    .pixformat = GBM_BO_FORMAT_XRGB8888, .modifiers = NULL,
    .modifiers_cnt = 0
  };

  kmsbuffs_info.bType = GBM_BUFFER;
  kms.kmsbuffs = uvr_buffer_create(&kmsbuffs_info);
  if (!kms.kmsbuffs.gbmdev) goto exit_error;

  /*
   * Create Vulkan Physical Device Handle, After buffer creation
   * as it can actually effect VkPhysicalDevice creation
   */
  struct uvr_vk_phdev_create_info vkphdev = {
    .vkinst = app.instance,
    .vkpdtype = VK_PHYSICAL_DEVICE_TYPE,
    .kmsfd = kms.kmsdev.kmsfd
  };

  app.phdev = uvr_vk_phdev_create(&vkphdev);
  if (!app.phdev)
    goto exit_error;

  struct uvr_vk_queue_create_info vkqueueinfo = { .phdev = app.phdev, .queueFlag = VK_QUEUE_GRAPHICS_BIT };
  app.graphics_queue = uvr_vk_queue_create(&vkqueueinfo);
  if (app.graphics_queue.famindex == -1)
    goto exit_error;

  VkPhysicalDeviceFeatures phdevfeats = uvr_vk_get_phdev_features(app.phdev);
  struct uvr_vk_lgdev_create_info vklgdev_info = {
    .vkinst = app.instance, .phdev = app.phdev,
    .pEnabledFeatures = &phdevfeats,
    .enabledExtensionCount = ARRAY_LEN(device_extensions),
    .ppEnabledExtensionNames = device_extensions,
    .numqueues = 1,
    .queues = &app.graphics_queue
  };

  app.lgdev = uvr_vk_lgdev_create(&vklgdev_info);
  if (!app.lgdev.device)
    goto exit_error;

exit_error:
  /*
   * Let the api know of what addresses to free and fd's to close
   */
  kmsbuffsd.gbmdev = kms.kmsbuffs.gbmdev;
  kmsbuffsd.buff_cnt = kmsbuffs_info.buff_cnt;
  kmsbuffsd.info_buffers = kms.kmsbuffs.info_buffers;
  uvr_buffer_destory(&kmsbuffsd);

  kmsdevd.kmsnode = kms.kmsdev;
  kmsdevd.dochain = kms.dochain;
  uvr_kms_node_destroy(&kmsdevd);

  appd.vkinst = app.instance;
  appd.vklgdevs_cnt = 1;
  appd.vklgdevs = &app.lgdev;
  uvr_vk_destory(&appd);
#ifdef INCLUDE_SDBUS
  uvr_sd_session_destroy(&kms.uvrsd);
#endif
  return 0;
}
