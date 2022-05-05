#include "kms.h"


int uvr_kms_node_create(struct uvrkms_node_create_info *uvrkms) {
  int num_devices = 0;
  drmDevicePtr *devices = NULL;
  int kmsfd = -1;

  /* If the const char *kmsnode member is defined attempt to open it */
  if (uvrkms->kmsnode) {
#ifdef INCLUDE_SDBUS
    if (uvrkms->use_logind)
      kmsfd = uvr_sd_session_take_control_of_device(uvrkms->uvrsd_session, uvrkms->kmsnode);
    else
#endif
      kmsfd = open(uvrkms->kmsnode, O_RDONLY | O_CLOEXEC, 0);
    if (kmsfd < 0) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_kms_node_create(open): %s", strerror(errno));
      goto error_free_kms_dev;
    }

    uvr_utils_log(UVR_SUCCESS, "Opened KMS node '%s' associated fd is %d", uvrkms->kmsnode, kmsfd);
    return kmsfd;
  }

  /* Query list of available kms nodes (GPU) to get the amount available */
  num_devices = drmGetDevices2(0, NULL, num_devices);
  if (num_devices <= 0) {
    uvr_utils_log(UVR_DANGER, "[x] uvr_kms_node_create(drmGetDevices2): no KMS devices available");
    goto error_free_kms_dev;
  }

  devices = alloca(num_devices * sizeof(drmDevicePtr));

  /* Query list of available kms nodes (GPU) to get information about it */
  num_devices = drmGetDevices2(0, devices, num_devices);
  if (num_devices < 0) {
    uvr_utils_log(UVR_DANGER, "[x] no KMS devices available");
    goto error_free_kms_dev;
  }

  uvr_utils_log(UVR_INFO, "%d KMS devices available", num_devices);

  char *knode = NULL;
  for (int i = 0; i < num_devices; i++) {
    drmDevicePtr candidate = devices[i];
    knode = (uvrkms->kmsnode) ? (char*) uvrkms->kmsnode : candidate->nodes[DRM_NODE_PRIMARY];

    /*
     * We need /dev/dri/cardN nodes for modesetting, not render
     * nodes; render nodes are only used for GPU rendering, and
     * control nodes are totally useless. Primary nodes are the
     * only ones which let us control KMS.
     */
    if (!(candidate->available_nodes & (1 << DRM_NODE_PRIMARY)))
      continue;

#ifdef INCLUDE_SDBUS
    if (uvrkms->use_logind)
      kmsfd = uvr_sd_session_take_control_of_device(uvrkms->uvrsd_session, knode);
    else
#endif
      kmsfd = open(knode, O_RDONLY | O_CLOEXEC, 0);
    if (kmsfd < 0) {
      uvr_utils_log(UVR_WARNING, "[x] uvr_kms_node_create(open): %s", strerror(errno));
      continue;
    }

    drmFreeDevices(devices, num_devices);
    uvr_utils_log(UVR_SUCCESS, "Opened KMS node '%s' associated fd is %d", knode, kmsfd);
    return kmsfd;
  }

error_free_kms_dev:
  if (devices)
    drmFreeDevices(devices, num_devices);
  return -1;
}


void uvr_kms_destroy(struct uvrkms_destroy *uvrkms) {
  if (uvrkms->kmsfd != -1) {
#ifdef INCLUDE_SDBUS
    if (uvrkms->info.use_logind)
      uvr_sd_session_release_device(uvrkms->info.uvrsd_session, uvrkms->kmsfd);
    else
#endif
      close(uvrkms->kmsfd);
  }
}
