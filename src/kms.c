#include "kms.h"


int uvr_kms_node_create(struct uvrkms_node_create_info *uvrkms) {
  int num_devices = 0, err = 0, kmsfd = -1;
  drm_magic_t magic;
  char *knode = NULL;
  drmDevicePtr candidate = NULL;
  drmDevicePtr *devices = NULL;

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

  for (int i = 0; i < num_devices; i++) {
    candidate = devices[i];
    knode = (uvrkms->kmsnode) ? (char*) uvrkms->kmsnode : candidate->nodes[DRM_NODE_PRIMARY];

    /*
     * TAKEN FROM Daniel Stone (gitlab/kms-quads)
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
      uvr_utils_log(UVR_WARNING, "uvr_kms_node_create(open): %s", strerror(errno));
      continue;
    }

    drmFreeDevices(devices, num_devices); devices = NULL;
    uvr_utils_log(UVR_SUCCESS, "Opened KMS node '%s' associated fd is %d", knode, kmsfd);

    /*
     * TAKEN FROM Daniel Stone (gitlab/kms-quads)
     * In order to drive KMS, we need to be 'master'. This should already
     * have happened for us thanks to being root and the first client.
     * There can only be one master at a time, so this will fail if
     * (e.g.) trying to run this test whilst a graphical session is
     * already active on the current VT.
     */
    if (drmGetMagic(kmsfd, &magic) || drmAuthMagic(kmsfd, magic)) {
      uvr_utils_log(UVR_DANGER, "[x] KMS device '%s' is not master", knode);
      goto error_free_kms_dev;
    }

    /*
     * Tell DRM core to expose atomic properties to userspace. This also enables
     * DRM_CLIENT_CAP_UNIVERSAL_PLANES and DRM_CLIENT_CAP_ASPECT_RATIO.
     * DRM_CLIENT_CAP_UNIVERSAL_PLANES - Tell DRM core to expose all planes (overlay, primary, and cursor) to userspace.
     * DRM_CLIENT_CAP_ASPECT_RATIO     - Tells DRM core to provide aspect ratio information in modes
     */
    err = drmSetClientCap(kmsfd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    err |= drmSetClientCap(kmsfd, DRM_CLIENT_CAP_ATOMIC, 1);
    if (err != 0) {
      uvr_utils_log(UVR_DANGER, "[x] KMS device '%s' has no support for universal planes or kms atomic", knode);
      goto error_free_kms_dev;
    }

    return kmsfd;
  }

error_free_kms_dev:
  if (kmsfd != -1) {
#ifdef INCLUDE_SDBUS
    if (uvrkms->use_logind)
      uvr_sd_session_release_device(uvrkms->uvrsd_session, kmsfd);
    else
#endif
      close(kmsfd);
  }
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
