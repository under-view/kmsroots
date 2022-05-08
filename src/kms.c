#include "kms.h"


int uvr_kms_node_create(struct uvrkms_node_create_info *uvrkms) {
  int num_devices = 0, err = 0, kmsfd = -1;
  drm_magic_t magic;
  char *knode = NULL;
  drmDevice *candidate = NULL;
  drmDevice **devices = NULL;

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

  unsigned int dev_alloc_sz = num_devices * sizeof(*devices) + 1;
  devices = alloca(dev_alloc_sz); devices[num_devices] = 0;

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

    uvr_utils_log(UVR_SUCCESS, "Opened KMS node '%s' associated fd is %d", knode, kmsfd);

    drmFreeDevices(devices, num_devices); devices = NULL;

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


struct uvrkms_node_display_output_chain uvr_kms_node_get_display_output_chain(struct uvrkms_node_get_display_output_chain_info *uvrkms) {
  drmModeRes *drmres = NULL;
  drmModePlaneRes *drmplaneres = NULL;

  /* Query connector->encoder->crtc->plane properties */
	drmres = drmModeGetResources(uvrkms->kmsfd);
	if (!drmres) {
    uvr_utils_log(UVR_DANGER, "[x] Couldn't get card resources from KMS fd '%d'", uvrkms->kmsfd);
		goto err_ret_null_chain_output;
	}

	drmplaneres = drmModeGetPlaneResources(uvrkms->kmsfd);
	if (!drmplaneres) {
    uvr_utils_log(UVR_DANGER, "[x] KMS fd '%d' has no planes", uvrkms->kmsfd);
		goto err_res;
	}

  /* Check if some form of a display output chain exist */
  if (drmres->count_crtcs       <= 0 ||
      drmres->count_connectors  <= 0 ||
      drmres->count_encoders    <= 0 ||
      drmplaneres->count_planes <= 0) {
    uvr_utils_log(UVR_DANGER, "[x] KMS fd '%d' has no display output chain", uvrkms->kmsfd);
    goto err_plane_res;
  }

  /* Query KMS device plane info */
  drmModePlane **planes = alloca(drmplaneres->count_planes * sizeof(drmModePlane));
  memset(planes, 0, drmplaneres->count_planes * sizeof(drmModePlane));

  for (unsigned int i = 0; i < drmplaneres->count_planes; i++) {
		planes[i] = drmModeGetPlane(uvrkms->kmsfd, drmplaneres->planes[i]);
    if (!planes[i]) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_kms_node_get_display_output_chain(drmModeGetPlane): Failed to get plane");
      goto err_free_disp_planes;
    }
	}

  /*
	 * Go through connectors one by one and try to find a usable output chain.
   * OUTPUT CHAIN: connector->encoder->crtc->plane
	 */
  drmModeConnector *connector = NULL;
  drmModeEncoder *encoder = NULL;
  drmModeCrtc *crtc = NULL;
  drmModePlane *plane = NULL;

  for (int i = 0; i < drmres->count_connectors; i++) {
    connector = drmModeGetConnector(uvrkms->kmsfd, drmres->connectors[i]);
    if (!connector) {
      uvr_utils_log(UVR_DANGER, "[x] uvr_kms_node_get_display_output_chain(drmModeGetConnector): Failed to get connector");
      goto err_free_disp_planes;
    }

    /* Find the encoder (a deprecated KMS object) for this connector. */
    if (connector->encoder_id == 0) {
      uvr_utils_log(UVR_INFO, "[CONN:%" PRIu32 "]: no encoder", connector->connector_id);
      goto free_disp_connector;
    }

  	for (int e = 0; e < drmres->count_encoders; e++) {
  		if (drmres->encoders[e] == connector->encoder_id) {
  			encoder = drmModeGetEncoder(uvrkms->kmsfd, drmres->encoders[e]);
        if (!encoder) {
          uvr_utils_log(UVR_DANGER, "[x] uvr_kms_node_get_display_output_chain(drmModeGetEncoder): Failed to get encoder KMS object associated with connector id '%d'",connector->connector_id);
          goto err_free_disp_connector;
        }

        break;
  		}
  	}

    if (encoder->crtc_id == 0) {
  		uvr_utils_log(UVR_INFO, "[CONN:%" PRIu32 "]: no CRTC", connector->connector_id);
  		goto free_disp_encoder;
  	}

    for (int c = 0; c < drmres->count_crtcs; c++) {
      if (drmres->crtcs[c] == encoder->crtc_id) {
        crtc = drmModeGetCrtc(uvrkms->kmsfd, drmres->crtcs[c]);
        if (!crtc) {
          uvr_utils_log(UVR_DANGER, "[x] uvr_kms_node_get_display_output_chain(drmModeGetCrtc): Failed to get crtc KMS object associated with encoder id '%d'", encoder->encoder_id);
          goto err_free_disp_encoder;
        }

        break;
      }
    }

    /* Ensure the CRTC is active. */
  	if (crtc->buffer_id == 0) {
  		uvr_utils_log(UVR_INFO, "[CONN:%" PRIu32 "]: not active", connector->connector_id);
  		goto free_disp_crtc;
  	}

    /*
     * TAKEN FROM Daniel Stone (gitlab/kms-quads)
     * The kernel doesn't directly tell us what it considers to be the
     * single primary plane for this CRTC (i.e. what would be updated
     * by drmModeSetCrtc), but if it's already active then we can cheat
     * by looking for something displaying the same framebuffer ID,
     * since that information is duplicated.
     */
    for (unsigned int p = 0; p < drmplaneres->count_planes; p++) {
      //uvr_utils_log(UVR_INFO, "[PLANE: %" PRIu32 "] CRTC ID %" PRIu32 ", FB %" PRIu32 "", planes[p]->plane_id, planes[p]->crtc_id, planes[p]->fb_id);
      if (planes[p]->crtc_id == crtc->crtc_id && planes[p]->fb_id == crtc->buffer_id) {
        plane = planes[p];
        break;
      }
  	}

    uvr_utils_log(UVR_SUCCESS, "Successfully created a display output chain");

    /* Free all planes not in use */
    for (unsigned int p = 0; p < drmplaneres->count_planes; p++)
      if (planes[p]->fb_id != plane->fb_id)
        drmModeFreePlane(planes[p]);

    drmModeFreePlaneResources(drmplaneres);
    drmModeFreeResources(drmres);

    return (struct uvrkms_node_display_output_chain) { .connector = connector, .encoder = encoder, .crtc = crtc , .plane = plane };

free_disp_crtc:
    if (crtc) {
      drmModeFreeCrtc(crtc);
      crtc = NULL;
    }
free_disp_encoder:
    if (encoder) {
      drmModeFreeEncoder(encoder);
      encoder = NULL;
    }
free_disp_connector:
    if (connector) {
      drmModeFreeConnector(connector);
      connector = NULL;
    }
  }


err_free_disp_encoder:
  if (encoder)
    drmModeFreeEncoder(encoder);
err_free_disp_connector:
  if (connector)
    drmModeFreeConnector(connector);
err_free_disp_planes:
  for (unsigned int p = 0; p < drmplaneres->count_planes; p++)
    if (planes[p])
      drmModeFreePlane(planes[p]);
err_plane_res:
  if (drmplaneres)
    drmModeFreePlaneResources(drmplaneres);
err_res:
  if (drmres)
    drmModeFreeResources(drmres);
err_ret_null_chain_output:
  return (struct uvrkms_node_display_output_chain) { .connector = 0, .encoder = 0, .crtc = 0 , .plane = 0 };
}


void uvr_kms_node_destroy(struct uvrkms_node_destroy *uvrkms) {
  if (uvrkms->dochain) {
    if (uvrkms->dochain->plane)
      drmModeFreePlane(uvrkms->dochain->plane);
    if (uvrkms->dochain->crtc)
      drmModeFreeCrtc(uvrkms->dochain->crtc);
    if (uvrkms->dochain->encoder)
      drmModeFreeEncoder(uvrkms->dochain->encoder);
    if (uvrkms->dochain->connector)
      drmModeFreeConnector(uvrkms->dochain->connector);
  }
  if (uvrkms->kmsfd != -1) {
#ifdef INCLUDE_SDBUS
    if (uvrkms->info.use_logind)
      uvr_sd_session_release_device(uvrkms->info.uvrsd_session, uvrkms->kmsfd);
    else
#endif
      close(uvrkms->kmsfd);
  }
}
