#include "vulkan.h"

VkInstance uvc_vk_create_instance(const char *app_name,
                                  const char *engine_name,
                                  uint32_t enabledLayerCount,
                                  const char *ppEnabledLayerNames[],
                                  uint32_t enabledExtensionCount,
                                  const char *ppEnabledExtensionNames[])
{

  VkInstance instance = VK_NULL_HANDLE;

  /* initialize the VkApplicationInfo structure */
  VkApplicationInfo app_info = {};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pNext = NULL;
  app_info.pApplicationName = app_name;
  app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
  app_info.pEngineName = engine_name;
  app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
  app_info.apiVersion = VK_MAKE_VERSION(1, 1, 0);

  /* enable validation layers best practice */
  VkValidationFeatureEnableEXT enables[] = {VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};
  VkValidationFeaturesEXT features = {};
  features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
  features.enabledValidationFeatureCount = ARRAY_LEN(enables);
  features.pEnabledValidationFeatures = enables;

  /*
   * Tells Vulkan which instance extensions
   * and validation layers we want to use
   */
  VkInstanceCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pNext = (ppEnabledLayerNames) ? &features : NULL;
  create_info.flags = 0;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledLayerCount = enabledLayerCount;
  create_info.ppEnabledLayerNames = ppEnabledLayerNames;
  create_info.enabledExtensionCount = enabledExtensionCount;
  create_info.ppEnabledExtensionNames = ppEnabledExtensionNames;

  /* Create the instance */
  VkResult res = vkCreateInstance(&create_info, NULL, &instance);
  if (res) {
    uvc_utils_log(UVC_DANGER, "[x] uvc_vk_create_instance: vkCreateInstance Failed");
    return VK_NULL_HANDLE;
  }

  uvc_utils_log(UVC_SUCCESS, "uvc_vk_create_instance: VkInstance created retval(%p)", instance);

  return instance;
}

void uvc_vk_destory(uvcvk *app) {
  if (app->instance)
    vkDestroyInstance(app->instance, NULL);
}
