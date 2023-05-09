#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
#include <sys/vt.h>
#include <sys/kd.h>
#include <linux/major.h>

#include "kms.h"


/*
 * Verbatim TAKEN FROM Daniel Stone (gitlab/kms-quads)
 * Set up the VT/TTY so it runs in graphics mode and lets us handle our own
 * input. This uses the VT specified in $TTYNO if specified, or the current VT
 * if we're running directly from a VT, or failing that tries to find a free
 * one to switch to.
 */
static int UNUSED vt_setup(int *keyBoardMode)
{
	const char *ttyNumEnv = getenv("TTYNO");
	int ttyNum = 0, vtfd = -1;
	char ttyDevice[32];

	/* If $TTYNO is set in the environment, then use that first. */
	if (ttyNumEnv) {
		char *endptr = NULL;

		ttyNum = strtoul(ttyNumEnv, &endptr, 10);
		if (ttyNum == 0 || *endptr != '\0') {
			uvr_utils_log(UVR_DANGER, "[x] strtoul: invalid $TTYNO environment variable");
			return -1;
		}

		snprintf(ttyDevice, sizeof(ttyDevice), "/dev/tty%d", ttyNum);
	} else if (ttyname(STDIN_FILENO)) {
		/*
		 * Otherwise, if we're running from a VT ourselves, just reuse that.
		 */
		if (ttyname_r(STDIN_FILENO, ttyDevice, sizeof(ttyDevice))) {
			uvr_utils_log(UVR_DANGER, "[x] ttyname_r: %s", strerror(errno));
			return -1;
		}
	} else {
search_for_free_vt:
		int tty0;

		/*
		 * Other-other-wise, look for a free VT we can use by querying /dev/tty0.
		 */
		tty0 = open("/dev/tty0", O_WRONLY | O_CLOEXEC);
		if (tty0 < 0) {
			uvr_utils_log(UVR_DANGER, "[x] open('%s'): %s", "/dev/tty0", strerror(errno));
			return -1;
		}

		if (ioctl(tty0, VT_OPENQRY, &ttyNum) < 0 || ttyNum < 0) {
			uvr_utils_log(UVR_DANGER, "[x] ioctl(VT_OPENQRY): %s", strerror(errno));
			uvr_utils_log(UVR_DANGER, "[x] ioctl(VT_OPENQRY): couldn't get free TTY");
			close(tty0);
			return -1;
		}

		close(tty0);
		snprintf(ttyDevice, sizeof(ttyDevice), "/dev/tty%d", ttyNum);
	}

	vtfd = open(ttyDevice, O_RDWR | O_NOCTTY);
	if (vtfd < 0) {
		uvr_utils_log(UVR_DANGER, "[x] open('%s'): %s", ttyDevice, strerror(errno));
		return -1;
	}

	uvr_utils_log(UVR_INFO, "Using VT %s (fd: %d)", ttyDevice, vtfd);

	/* If we get our VT from stdin, work painfully backwards to find its VT number. */
	if (ttyNum == 0) {
		int deviceMajor = 0;
		struct stat vtStat;

		if (fstat(vtfd, &vtStat) == -1) {
			uvr_utils_log(UVR_DANGER, "[x] fstat(%s:%d): %s", ttyDevice, vtfd, strerror(errno));
			goto exit_error_vt_setup_exit;
		}

		deviceMajor = major(vtStat.st_rdev);
		if (deviceMajor == UNIX98_PTY_SLAVE_MAJOR || deviceMajor == PTY_SLAVE_MAJOR) {
			uvr_utils_log(UVR_DANGER, "VT file %s is a psuedo terminal.", ttyDevice);
			goto exit_error_vt_setup_exit;
		}

		if (deviceMajor != TTY_MAJOR) {
			uvr_utils_log(UVR_DANGER, "[x] major(%d): VT file %s is not a tty device file", vtStat.st_rdev, ttyDevice);
			goto exit_error_vt_setup_exit;
		}

		ttyNum = minor(vtStat.st_rdev);

		/*
		 * If current device file is a serial device.
		 * Search for an available VT
		 */
		if (ttyNum >= 64 && ttyNum <= 255) {
			close(vtfd); vtfd = -1; ttyNum = 0;
			goto search_for_free_vt;
		}
	}

	if (ttyNum == 0)
		goto exit_error_vt_setup_exit;

	/* Switch to the target VT. */
	if (ioctl(vtfd, VT_ACTIVATE, ttyNum) < 0 || ioctl(vtfd, VT_WAITACTIVE, ttyNum) < 0) {
		uvr_utils_log(UVR_DANGER, "[x] ioctl(VT_WAITACTIVE || VT_WAITACTIVE): couldn't switch to VT %d", ttyNum);
		goto exit_error_vt_setup_exit;
	}

	uvr_utils_log(UVR_INFO, "Switched to VT %s", ttyDevice);

	/*
	 * Completely disable kernel keyboard processing: this prevents us
	 * from being killed on Ctrl-C.
	 */
	if (ioctl(vtfd, KDGKBMODE, keyBoardMode) != 0 || ioctl(vtfd, KDSKBMODE, K_OFF) != 0) {
		uvr_utils_log(UVR_DANGER, "[x] ioctl(KDGKBMODE) || ioctl(KDSKBMODE): failed to disable TTY keyboard processing");
		goto exit_error_vt_setup_exit;
	}

	/*
	 * Change the VT into graphics mode, so the kernel no longer prints
	 * text out on top of us.
	 */
	if (ioctl(vtfd, KDSETMODE, KD_GRAPHICS) != 0) {
		uvr_utils_log(UVR_DANGER, "[x] ioctl(KDSETMODE): failed to switch TTY to graphics mode");
		goto exit_error_vt_setup_exit;
	}

	uvr_utils_log(UVR_SUCCESS, "VT setup complete");

	/*
	 * Normally we would also call VT_SETMODE to change the mode to
	 * VT_PROCESS here, which would allow us to intercept VT-switching
	 * requests and tear down KMS. But we don't, since that requires
	 * signal handling.
	 */
	return vtfd;

exit_error_vt_setup_exit:
	if (vtfd > 0)
		close(vtfd);
	return -1;
}


static void vt_reset(int vtfd, int savedKeyBoardMode)
{
	ioctl(vtfd, KDSKBMODE, savedKeyBoardMode);
	ioctl(vtfd, KDSETMODE, KD_TEXT);
}


struct uvr_kms_node uvr_kms_node_create(struct uvr_kms_node_create_info *uvrkms)
{
	int deviceCount = 0, err = 0, kmsfd = -1;
	int keyBoardMode = -1, vtfd = -1;
	char *kmsNode = NULL;
	drmDevice **devices = NULL;
	drmDevice *device = NULL;

	/* If the const char *kmsnode member is defined attempt to open it */
	if (uvrkms->kmsNode) {
#ifdef INCLUDE_LIBSEAT
		kmsfd = uvr_session_take_control_of_device(uvrkms->session, uvrkms->kmsNode);
#else
		kmsfd = open(uvrkms->kmsNode, O_RDWR | O_CLOEXEC, 0);
#endif
		if (kmsfd < 0) {
			uvr_utils_log(UVR_DANGER, "[x] open('%s'): %s", uvrkms->kmsNode, strerror(errno));
			goto exit_error_kms_node_create_free_kms_dev;
		}
/*
		vtfd = vt_setup(&keyBoardMode);
		if (vtfd == -1)
			goto exit_error_kms_node_create_free_kms_dev;
*/
		uvr_utils_log(UVR_SUCCESS, "Opened KMS node '%s' associated fd is %d", uvrkms->kmsNode, kmsfd);
		return (struct uvr_kms_node) { .kmsfd = kmsfd, .vtfd = vtfd, .keyBoardMode = keyBoardMode
#ifdef INCLUDE_LIBSEAT
			                       , .session = uvrkms->session
#endif
		};
	}

	/* Query list of available kms nodes (GPU) to get the amount available */
	deviceCount = drmGetDevices2(0, NULL, deviceCount);
	if (deviceCount <= 0) {
		uvr_utils_log(UVR_DANGER, "[x] drmGetDevices2: no KMS devices available");
		goto exit_error_kms_node_create_free_kms_dev;
	}

	unsigned int deviceAllocSize = deviceCount * sizeof(*devices) + 1;
	devices = alloca(deviceAllocSize); devices[deviceCount] = 0;

	/* Query list of available kms nodes (GPU) to get information about it */
	deviceCount = drmGetDevices2(0, devices, deviceCount);
	if (deviceCount < 0) {
		uvr_utils_log(UVR_DANGER, "[x] no KMS devices available");
		goto exit_error_kms_node_create_free_kms_dev;
	}

	uvr_utils_log(UVR_INFO, "%d KMS devices available", deviceCount);

	for (int i = 0; i < deviceCount; i++) {
		device = devices[i];
		kmsNode = (uvrkms->kmsNode) ? (char*) uvrkms->kmsNode : device->nodes[DRM_NODE_PRIMARY];

		/*
		 * TAKEN FROM Daniel Stone (gitlab/kms-quads)
		 * We need /dev/dri/cardN nodes for modesetting, not render
		 * nodes; render nodes are only used for GPU rendering, and
		 * control nodes are totally useless. Primary nodes are the
		 * only ones which let us control KMS.
		 */
		if (!(device->available_nodes & (1 << DRM_NODE_PRIMARY)))
			continue;

#ifdef INCLUDE_LIBSEAT
		kmsfd = uvr_session_take_control_of_device(uvrkms->session, kmsNode);
#else
		kmsfd = open(kmsNode, O_RDWR | O_CLOEXEC, 0);
#endif
		if (kmsfd < 0) {
			uvr_utils_log(UVR_WARNING, "open('%s'): %s", kmsNode, strerror(errno));
			continue;
		}

		uvr_utils_log(UVR_SUCCESS, "Opened KMS node '%s' associated fd is %d", kmsNode, kmsfd);

		drmFreeDevices(devices, deviceCount); devices = NULL;

		/*
		 * TAKEN FROM Daniel Stone (gitlab/kms-quads)
		 * In order to drive KMS, we need to be 'master'. This should already
		 * have happened for us thanks to being root and the first client.
		 * There can only be one master at a time, so this will fail if
		 * (e.g.) trying to run this test whilst a graphical session is
		 * already active on the current VT.
		 */
		drm_magic_t magic;
		if (drmGetMagic(kmsfd, &magic) < 0) {
			uvr_utils_log(UVR_DANGER, "[x] drmGetMagic: KMS device '%s(fd: %d)' could not become master", kmsNode, kmsfd);
			goto exit_error_kms_node_create_free_kms_dev;
		}

		if (drmAuthMagic(kmsfd, magic) < 0) {
			uvr_utils_log(UVR_DANGER, "[x] drmGetMagic: KMS device '%s(fd: %d)' could not become master", kmsNode, kmsfd);
			goto exit_error_kms_node_create_free_kms_dev;
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
			uvr_utils_log(UVR_DANGER, "[x] KMS device '%s' has no support for universal planes or kms atomic", kmsNode);
			goto exit_error_kms_node_create_free_kms_dev;
		}
/*
		vtfd = vt_setup(&keyBoardMode);
		if (vtfd == -1)
			goto exit_error_kms_node_create_free_kms_dev;
*/
		return (struct uvr_kms_node) { .kmsfd = kmsfd, .vtfd = vtfd, .keyBoardMode = keyBoardMode
#ifdef INCLUDE_LIBSEAT
		                               , .session = uvrkms->session
#endif
		};
	}

exit_error_kms_node_create_free_kms_dev:
	if (kmsfd != -1) {
#ifdef INCLUDE_LIBSEAT
		uvr_session_release_device(uvrkms->session, kmsfd);
#else
		close(kmsfd);
#endif
	}
	if (devices)
		drmFreeDevices(devices, deviceCount);

	return (struct uvr_kms_node) { .kmsfd = -1, .vtfd = -1, .keyBoardMode = -1
#ifdef INCLUDE_LIBSEAT
		                       , .session = NULL
#endif
	};
}


struct uvr_kms_node_device_capabilites uvr_kms_node_get_device_capabilities(int kmsfd)
{
	struct uvr_kms_node_device_capabilites kmsNodeDeviceCapabilites;
	memset(&kmsNodeDeviceCapabilites, 0, sizeof(kmsNodeDeviceCapabilites));

	uint64_t capabilites = 0, err = 0;
	err = drmGetCap(kmsfd, DRM_CAP_ADDFB2_MODIFIERS, &capabilites);
	if (err == 0 && capabilites != 0) {
		kmsNodeDeviceCapabilites.ADDFB2_MODIFIERS = true;
		uvr_utils_log(UVR_INFO, "device %s framebuffer modifiers", (err == 0 && capabilites != 0) ? "supports" : "does not support");
	}

	capabilites=0;
	err = drmGetCap(kmsfd, DRM_CAP_TIMESTAMP_MONOTONIC, &capabilites);
	if (err == 0 && capabilites != 0) {
		kmsNodeDeviceCapabilites.TIMESTAMP_MONOTONIC = true;
		uvr_utils_log(UVR_INFO, "device %s clock monotonic timestamps", (err == 0 && capabilites != 0) ? "supports" : "does not support");
	}

	return kmsNodeDeviceCapabilites;
}


static void free_kms_object_properties(drmModeObjectProperties *props, drmModePropertyRes **propsData)
{
	uint32_t i = 0;

	if (!props) return;
	if (!propsData) return;

	for (i = 0; i < props->count_props; i++) {
		if (propsData[i])
			drmModeFreeProperty(propsData[i]);
	}

	drmModeFreeObjectProperties(props);

	free(propsData);
}


/*
 * acquire_kms_object_properties: helpfer function that retrieves the properties of a certain CRTC, plane or connector object.
 * https://github.com/dvdhrm/docs/blob/master/drm-howto/modeset-atomic.c#L191
 */
static int acquire_kms_object_properties(int fd, struct uvr_kms_node_object_props *obj, uint32_t type)
{
	char *typeStr;
	unsigned int i;

	obj->props = drmModeObjectGetProperties(fd, obj->id, type);
	if (!obj->props) {
		switch(type) {
			case DRM_MODE_OBJECT_CONNECTOR:
				typeStr = "connector";
				break;
			case DRM_MODE_OBJECT_PLANE:
				typeStr = "plane";
				break;
			case DRM_MODE_OBJECT_CRTC:
				typeStr = "CRTC";
				break;
			default:
				typeStr = "unknown type";
				break;
		}
		uvr_utils_log(UVR_DANGER, "[x] cannot get %s %d properties: %s", typeStr, obj->id, strerror(errno));
		return -1;
	}

	obj->propsData = calloc(obj->props->count_props, sizeof(obj->propsData));
	if (!obj->propsData) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_acquire_kms_object_properties;
	}

	for (i = 0; i < obj->props->count_props; i++) {
		obj->propsData[i] = drmModeGetProperty(fd, obj->props->props[i]);
		if (!obj->propsData[i]) {
			uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
			goto exit_error_acquire_kms_object_properties;
		}
	}

	return 0;

exit_error_acquire_kms_object_properties:
	free_kms_object_properties(obj->props, obj->propsData);
	return -1;
}


struct uvr_kms_node_display_output_chain uvr_kms_node_display_output_chain_create(struct uvr_kms_node_display_output_chain_create_info *uvrkms)
{
	drmModeRes *drmResources = NULL;
	drmModePlaneRes *drmPlaneResources = NULL;
	unsigned int p;

	/* Query connector->encoder->crtc->plane properties */
	drmResources = drmModeGetResources(uvrkms->kmsfd);
	if (!drmResources) {
		uvr_utils_log(UVR_DANGER, "[x] Couldn't get card resources from KMS fd '%d'", uvrkms->kmsfd);
		goto exit_error_kms_node_doc_create;
	}

	drmPlaneResources = drmModeGetPlaneResources(uvrkms->kmsfd);
	if (!drmPlaneResources) {
		uvr_utils_log(UVR_DANGER, "[x] KMS fd '%d' has no planes", uvrkms->kmsfd);
		goto exit_error_kms_node_doc_create_drm_mode_free_resources;
	}

	/* Check if some form of a display output chain exist */
	if (drmResources->count_crtcs       <= 0 ||
	    drmResources->count_connectors  <= 0 ||
	    drmResources->count_encoders    <= 0 ||
	    drmPlaneResources->count_planes <= 0) {
		uvr_utils_log(UVR_DANGER, "[x] KMS fd '%d' has no display output chain", uvrkms->kmsfd);
		goto exit_error_kms_node_doc_create_drm_mode_free_plane_resources;
	}

	/* Query KMS device plane info */
	unsigned int planesCount = drmPlaneResources->count_planes;
	drmModePlane **planes = alloca(planesCount * sizeof(drmModePlane));
	memset(planes, 0, planesCount * sizeof(drmModePlane));

	for (p = 0; p < planesCount; p++) {
		planes[p] = drmModeGetPlane(uvrkms->kmsfd, drmPlaneResources->planes[p]);
		if (!planes[p]) {
			uvr_utils_log(UVR_DANGER, "[x] drmModeGetPlane: Failed to get plane");
			goto exit_error_kms_node_doc_create_drm_mode_free_planes;
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
	uint32_t modeid = 0;

	struct uvr_kms_node_object_props connectorProps;
	struct uvr_kms_node_object_props crtcProps;
	struct uvr_kms_node_object_props planeProps;

	memset(&connectorProps, 0, sizeof(connectorProps));
	memset(&crtcProps, 0, sizeof(crtcProps));
	memset(&planeProps, 0, sizeof(planeProps));

	for (int conn = 0; conn < drmResources->count_connectors; conn++) {
		connector = drmModeGetConnector(uvrkms->kmsfd, drmResources->connectors[conn]);
		if (!connector) {
			uvr_utils_log(UVR_DANGER, "[x] drmModeGetConnector: Failed to get connector");
			goto exit_error_kms_node_doc_create_drm_mode_free_planes;
		}

		/* check if a monitor is connected */
		if (connector->encoder_id == 0 || connector->connection != DRM_MODE_CONNECTED) {
			uvr_utils_log(UVR_INFO, "[CONNECTOR:%" PRIu32 "]: no encoder or not connected to display", connector->connector_id);
			drmModeFreeConnector(connector); connector = NULL;
			continue;
		}

		/* Find the encoder (a deprecated KMS object) for this connector. */
		for (int e = 0; e < drmResources->count_encoders; e++) {
			if (drmResources->encoders[e] == connector->encoder_id) {
				encoder = drmModeGetEncoder(uvrkms->kmsfd, drmResources->encoders[e]);
				if (!encoder) {
					uvr_utils_log(UVR_DANGER, "[x] drmModeGetEncoder: Failed to get encoder KMS object associated with connector id '%d'",connector->connector_id);
					goto exit_error_kms_node_doc_create_drm_mode_free_connector;
				}

				if (encoder->crtc_id == 0) {
					uvr_utils_log(UVR_INFO, "[ENCODER:%" PRIu32 "]: no CRTC", encoder->encoder_id);
					drmModeFreeEncoder(encoder); encoder = NULL;
					continue;
				}
			}
		}


		for (int c = 0; c < drmResources->count_crtcs; c++) {
			if (drmResources->crtcs[c] == encoder->crtc_id) {
				crtc = drmModeGetCrtc(uvrkms->kmsfd, drmResources->crtcs[c]);
				if (!crtc) {
					uvr_utils_log(UVR_DANGER, "[x] drmModeGetCrtc: Failed to get crtc KMS object associated with encoder id '%d'", encoder->encoder_id);
					goto exit_error_kms_node_doc_create_drm_mode_free_encoder;
				}

				/* Ensure the CRTC is active. */
				if (crtc->buffer_id == 0) {
					uvr_utils_log(UVR_INFO, "[CRTC:%" PRIu32 "]: not active", crtc->crtc_id);
					drmModeFreeCrtc(crtc); crtc = NULL;
					continue;
				}
			}
		}

		/*
		 * TAKEN FROM Daniel Stone (gitlab/kms-quads)
		 * The kernel doesn't directly tell us what it considers to be the
		 * single primary plane for this CRTC (i.e. what would be updated
		 * by drmModeSetCrtc), but if it's already active then we can cheat
		 * by looking for something displaying the same framebuffer ID,
		 * since that information is duplicated.
		 */
		for (p = 0; p < drmPlaneResources->count_planes; p++) {
			//uvr_utils_log(UVR_INFO, "[PLANE: %" PRIu32 "] CRTC ID %" PRIu32 ", FB %" PRIu32 "", planes[p]->plane_id, planes[p]->crtc_id, planes[p]->fb_id);
			if (planes[p]->crtc_id == crtc->crtc_id && planes[p]->fb_id == crtc->buffer_id) {
				plane = planes[p];
				break;
			}
		}

		uvr_utils_log(UVR_SUCCESS, "Successfully created a display output chain");

		/* Free all plane resources not in use */
		for (p = 0; p < planesCount; p++) {
			if (planes[p]->fb_id != plane->fb_id) {
				drmModeFreePlane(planes[p]);
				planes[p] = NULL;
			}
		}

		drmModeFreePlaneResources(drmPlaneResources); drmPlaneResources = NULL;
		drmModeFreeResources(drmResources); drmResources = NULL;

		// Stores mode id given to one of the displays resolution + refresh
		if (drmModeCreatePropertyBlob(uvrkms->kmsfd, &connector->modes[0], sizeof(connector->modes[0]), &modeid) != 0) {
			uvr_utils_log(UVR_DANGER, "[x] drmModeCreatePropertyBlob: couldn't create a blob property");
			goto exit_error_kms_node_doc_create_drm_mode_free_crtc;
		}

		connectorProps.id = connector->connector_id;
		if (acquire_kms_object_properties(uvrkms->kmsfd, &connectorProps, DRM_MODE_OBJECT_CONNECTOR) == -1)
			goto exit_error_kms_node_doc_create_drm_mode_destroy_mode_id;

		crtcProps.id = crtc->crtc_id;
		if (acquire_kms_object_properties(uvrkms->kmsfd, &crtcProps, DRM_MODE_OBJECT_CRTC) == -1)
			goto exit_error_kms_node_doc_create_free_kms_obj_props;

		planeProps.id = plane->plane_id;
		if (acquire_kms_object_properties(uvrkms->kmsfd, &planeProps, DRM_MODE_OBJECT_PLANE) == -1)
			goto exit_error_kms_node_doc_create_free_kms_obj_props;

		uvr_utils_log(UVR_INFO, "Plane ID: %u", plane->plane_id);
		uvr_utils_log(UVR_INFO, "CRTC ID: %u", crtc->crtc_id);
		uvr_utils_log(UVR_INFO, "ENCODER ID: %u", encoder->encoder_id);
		uvr_utils_log(UVR_INFO, "CONNECTOR ID: %u", connector->connector_id);
		uvr_utils_log(UVR_INFO, "MODE (resolution + refresh) ID: %u", modeid);

		return (struct uvr_kms_node_display_output_chain) { .connector = connector, .connectorProps = connectorProps, .encoder = encoder,
		                                                    .crtc = crtc, .crtcProps = crtcProps, .plane = plane, .planeProps = planeProps,
			                                            .width = connector->modes[0].hdisplay, .height = connector->modes[0].vdisplay,
								    .kmsfd = uvrkms->kmsfd, .modeid = modeid };
	}

exit_error_kms_node_doc_create_free_kms_obj_props:
	free_kms_object_properties(planeProps.props, planeProps.propsData);
	free_kms_object_properties(crtcProps.props, crtcProps.propsData);
	free_kms_object_properties(connectorProps.props, connectorProps.propsData);
	memset(&connectorProps, 0, sizeof(connectorProps));
	memset(&crtcProps, 0, sizeof(crtcProps));
	memset(&planeProps, 0, sizeof(planeProps));
exit_error_kms_node_doc_create_drm_mode_destroy_mode_id:
	if (modeid)
		drmModeDestroyPropertyBlob(uvrkms->kmsfd, modeid);
exit_error_kms_node_doc_create_drm_mode_free_crtc:
	if (crtc)
		drmModeFreeCrtc(crtc);
exit_error_kms_node_doc_create_drm_mode_free_encoder:
	if (encoder)
		drmModeFreeEncoder(encoder);
exit_error_kms_node_doc_create_drm_mode_free_connector:
	if (connector)
		drmModeFreeConnector(connector);
exit_error_kms_node_doc_create_drm_mode_free_planes:
	for (p = 0; p < planesCount; p++)
		if (planes[p])
			drmModeFreePlane(planes[p]);
exit_error_kms_node_doc_create_drm_mode_free_plane_resources:
	if (drmPlaneResources)
		drmModeFreePlaneResources(drmPlaneResources);
exit_error_kms_node_doc_create_drm_mode_free_resources:
	if (drmResources)
		drmModeFreeResources(drmResources);
exit_error_kms_node_doc_create:
	return (struct uvr_kms_node_display_output_chain) { .connector = 0, .connectorProps = connectorProps, .encoder = 0,
	                                                    .crtc = 0, .crtcProps = crtcProps, .plane = 0, .planeProps = planeProps,
	                                                    .width = 0, .height = 0, .kmsfd = -1, .modeid = 0 };
}


int uvr_kms_node_display_mode_set(struct uvr_kms_node_display_mode_info *uvrkms)
{
	drmModeConnector *conn = uvrkms->displayChain->connector;
	uint32_t connector = uvrkms->displayChain->connector->connector_id;
	uint32_t crtc = uvrkms->displayChain->crtc->crtc_id;
	int kmsfd = uvrkms->displayChain->kmsfd;

	drmModeModeInfo mode;
	memcpy(&mode, &conn->modes[0], sizeof(mode));

	if (drmModeSetCrtc(kmsfd, crtc, uvrkms->fbid, 0, 0, &connector, 1, &mode)) {
		uvr_utils_log(UVR_DANGER, "[x] drmModeSetCrtc: %s", strerror(errno));
		uvr_utils_log(UVR_DANGER, "[x] drmModeSetCrtc: failed to set preferred display mode");
		return -1;
	}

	return 0;
}


int uvr_kms_node_display_mode_reset(struct uvr_kms_node_display_mode_info *uvrkms)
{
	uint32_t crtc = uvrkms->displayChain->crtc->crtc_id;
	int kmsfd = uvrkms->displayChain->kmsfd;

	if (drmModeSetCrtc(kmsfd, crtc, 0, 0, 0, NULL, 0, NULL)) {
		uvr_utils_log(UVR_DANGER, "[x] drmModeSetCrtc: %s", strerror(errno));
		uvr_utils_log(UVR_DANGER, "[x] drmModeSetCrtc: failed to reset current display mode");
		return -1;
	}

	return 0;
}


/* https://github.com/dvdhrm/docs/blob/master/drm-howto/modeset-atomic.c#L233 */
static int set_kms_object_property(drmModeAtomicReq *atomicRequest,
                                   struct uvr_kms_node_object_props obj,
                                   const char *name,
                                   uint64_t value)
{
	uint32_t i, prop_id = 0;

	for (i = 0; i < obj.props->count_props; i++) {
		if (!strcmp(obj.propsData[i]->name, name)) { // TODO: remove inefficient strcmp operation
			prop_id = obj.propsData[i]->prop_id;
			break;
		}
	}

	if (prop_id == 0) {
		uvr_utils_log(UVR_DANGER, "[x] set_kms_object_property: %s has no object property", name);
		return -EINVAL;
	}

	return drmModeAtomicAddProperty(atomicRequest, obj.id, prop_id, value);
}


/*
 * Here we set the values of properties (of our connector, CRTC and plane objects)
 * that we want to change in the atomic commit. These changes are temporarily stored
 * in drmModeAtomicReq *atomicRequest until the commit actually happens.
 */
static int modeset_atomic_prepare_commit(drmModeAtomicReq *atomicRequest, int fbid,
                                         struct uvr_kms_node_display_output_chain *displayOutputChain)
{
	/* set id of the CRTC id that the connector is using */
	if (set_kms_object_property(atomicRequest, displayOutputChain->connectorProps, "CRTC_ID", displayOutputChain->crtcProps.id) < 0)
		return -1;

	/*
	 * set the mode id of the CRTC; this property receives the id of a blob
	 * property that holds the struct that actually contains the mode info
	 */
	if (set_kms_object_property(atomicRequest, displayOutputChain->crtcProps, "MODE_ID", displayOutputChain->modeid) < 0)
		return -1;

	/* set the CRTC object as active */
	if (set_kms_object_property(atomicRequest, displayOutputChain->crtcProps, "ACTIVE", 1) < 0)
		return -1;

	struct uvr_kms_node_object_props plane = displayOutputChain->planeProps;

	/* set properties of the plane related to the CRTC and the framebuffer */
	if (set_kms_object_property(atomicRequest, plane, "FB_ID", fbid) < 0)
		return -1;

	if (set_kms_object_property(atomicRequest, plane, "CRTC_ID", displayOutputChain->crtcProps.id) < 0)
		return -1;

	if (set_kms_object_property(atomicRequest, plane, "SRC_X", 0) < 0)
		return -1;

	if (set_kms_object_property(atomicRequest, plane, "SRC_Y", 0) < 0)
		return -1;

	if (set_kms_object_property(atomicRequest, plane, "SRC_W", displayOutputChain->width << 16) < 0)
		return -1;

	if (set_kms_object_property(atomicRequest, plane, "SRC_H", displayOutputChain->height << 16) < 0)
		return -1;

	if (set_kms_object_property(atomicRequest, plane, "CRTC_X", 0) < 0)
		return -1;

	if (set_kms_object_property(atomicRequest, plane, "CRTC_Y", 0) < 0)
		return -1;

	if (set_kms_object_property(atomicRequest, plane, "CRTC_W", displayOutputChain->width) < 0)
		return -1;

	if (set_kms_object_property(atomicRequest, plane, "CRTC_H", displayOutputChain->height) < 0)
		return -1;

	return 0;
}


/*
 * struct uvr_kms_node_renderer_info (Underview Renderer KMS Node Renderer Information)
 *
 * members:
 * @displayOutputChain    - Pointer to a struct containing all plane->crtc->connector data used during
 *                          KMS atomic mode setting.
 * @renderer              - Function pointer that allows custom external renderers to be executed by the api
 *                          upon @kmsfd polled events.
 * @rendererRunning       - Pointer to a boolean that determines if a given renderer is running and in need
 *                          of stopping.
 * @rendererCurrentBuffer - Pointer to an integer used by the api to update the current displayable buffer.
 * @rendererFbId          - Pointer to an integer used as the value of the FB_ID property for a plane related to
 *                          the CRTC during the atomic modeset operation
 * @rendererData          - Pointer to an optional address. This address may be the address of a struct.
 *                          Reference/Address passed depends on external renderer function.
 */
struct uvr_kms_node_renderer_info {
	struct uvr_kms_node_display_output_chain *displayOutputChain;
	uvr_kms_node_renderer_impl               renderer;
	bool                                     *rendererRunning;
	uint8_t                                  *rendererCurrentBuffer;
	drmModeAtomicReq                         *rendererAtomicRequest;
	int                                      *rendererFbId;
	void                                     *rendererData;
};


struct uvr_kms_node_atomic_request uvr_kms_node_atomic_request_create(struct uvr_kms_node_atomic_request_create_info *uvrkms)
{
	int atomicRequestFlags = 0;
	drmModeAtomicReq *atomicRequest = NULL;
	struct uvr_kms_node_renderer_info *rendererInfo = NULL;

	atomicRequest = drmModeAtomicAlloc();
	if (!atomicRequest)
		goto exit_error_uvr_kms_node_atomic_request_create;

	rendererInfo = calloc(1, sizeof(struct uvr_kms_node_renderer_info));
	if (!rendererInfo) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_uvr_kms_node_atomic_request_create_free_request;
	}

	rendererInfo->displayOutputChain = uvrkms->displayOutputChain;
	rendererInfo->renderer = uvrkms->renderer;
	rendererInfo->rendererRunning = uvrkms->rendererRunning;
	rendererInfo->rendererCurrentBuffer = uvrkms->rendererCurrentBuffer;
	rendererInfo->rendererAtomicRequest = atomicRequest;
	rendererInfo->rendererFbId = uvrkms->rendererFbId;
	rendererInfo->rendererData = uvrkms->rendererData;

	if (modeset_atomic_prepare_commit(rendererInfo->rendererAtomicRequest, *uvrkms->rendererFbId, uvrkms->displayOutputChain) == -1)
		goto exit_error_uvr_kms_node_atomic_request_create_free_request;

	/* perform test-only atomic commit */
	atomicRequestFlags = DRM_MODE_ATOMIC_TEST_ONLY | DRM_MODE_ATOMIC_ALLOW_MODESET;
	if (drmModeAtomicCommit(uvrkms->kmsfd, rendererInfo->rendererAtomicRequest, atomicRequestFlags, rendererInfo) < 0) {
		uvr_utils_log(UVR_DANGER, "drmModeAtomicCommit: %s", strerror(errno));
		goto exit_error_uvr_kms_node_atomic_request_create_free_rendererInfo;
	}

	/* initial modeset on all outputs */
	atomicRequestFlags = DRM_MODE_ATOMIC_ALLOW_MODESET | DRM_MODE_PAGE_FLIP_EVENT;
	if (drmModeAtomicCommit(uvrkms->kmsfd, rendererInfo->rendererAtomicRequest, atomicRequestFlags, rendererInfo) < 0) {
		uvr_utils_log(UVR_DANGER, "[x] drmModeAtomicCommit: modeset atomic commit failed.");
		uvr_utils_log(UVR_DANGER, "[x] drmModeAtomicCommit: %s", strerror(errno));
		goto exit_error_uvr_kms_node_atomic_request_create_free_rendererInfo;
	}

	return (struct uvr_kms_node_atomic_request) { .atomicRequest = rendererInfo->rendererAtomicRequest, .rendererInfo = rendererInfo };

exit_error_uvr_kms_node_atomic_request_create_free_rendererInfo:
	free(rendererInfo);
exit_error_uvr_kms_node_atomic_request_create_free_request:
	drmModeAtomicFree(atomicRequest);
exit_error_uvr_kms_node_atomic_request_create:
	return (struct uvr_kms_node_atomic_request) { .atomicRequest = NULL, .rendererInfo = NULL };
}


static void handle_page_flip_event(int fd,
                                   unsigned int UNUSED seq,
                                   unsigned int UNUSED tv_sec,
                                   unsigned int UNUSED tv_usec,
                                   unsigned int UNUSED crtc_id,
                                   void *data)
{
	struct uvr_kms_node_renderer_info *rendererInfo = (struct uvr_kms_node_renderer_info *) data;

	rendererInfo->renderer(rendererInfo->rendererRunning, rendererInfo->rendererCurrentBuffer, rendererInfo->rendererFbId, rendererInfo->rendererData);

	if (modeset_atomic_prepare_commit(rendererInfo->rendererAtomicRequest, *rendererInfo->rendererFbId, rendererInfo->displayOutputChain) == -1)
		return;

	if (drmModeAtomicCommit(fd, rendererInfo->rendererAtomicRequest, DRM_MODE_ATOMIC_ALLOW_MODESET | DRM_MODE_PAGE_FLIP_EVENT, rendererInfo) < 0) {
		uvr_utils_log(UVR_WARNING, "[x] drmModeAtomicCommit: modeset atomic commit failed.");
		uvr_utils_log(UVR_WARNING, "[x] drmModeAtomicCommit: %s", strerror(errno));
		return;
	}
}


int uvr_kms_node_handle_drm_event(struct uvr_kms_node_handle_drm_event_info *uvrkms)
{
	/*
	 * Version 3 is the first version that allow us to use page_flip_handler2, which
	 * is just like page_flip_handler but with the addition of passing the
	 * crtc_id as argument to the function which allows us to find which output
	 * the page-flip happened.
	 *
	 * The usage of page_flip_handler2 is the reason why we needed to verify
	 * the support for DRM_CAP_CRTC_IN_VBLANK_EVENT.
	 */
	drmEventContext event;
	event.version = 3;
	event.page_flip_handler2 = handle_page_flip_event;

	if (drmHandleEvent(uvrkms->kmsfd, &event) != 0) {
		uvr_utils_log(UVR_DANGER, "[x] drmHandleEvent: failed");
		return -1;
	}

	return 0;
}


void uvr_kms_node_destroy(struct uvr_kms_node_destroy *uvrkms)
{
	if (uvrkms->uvr_kms_node.kmsfd != -1) {
		if (uvrkms->uvr_kms_node_atomic_request.atomicRequest)
			drmModeAtomicFree(uvrkms->uvr_kms_node_atomic_request.atomicRequest);
		free(uvrkms->uvr_kms_node_atomic_request.rendererInfo);
		if (uvrkms->uvr_kms_node_display_output_chain.plane) {
			drmModeFreePlane(uvrkms->uvr_kms_node_display_output_chain.plane);
			free_kms_object_properties(uvrkms->uvr_kms_node_display_output_chain.planeProps.props,
						   uvrkms->uvr_kms_node_display_output_chain.planeProps.propsData);
		}
		if (uvrkms->uvr_kms_node_display_output_chain.crtc) {
			drmModeFreeCrtc(uvrkms->uvr_kms_node_display_output_chain.crtc);
			free_kms_object_properties(uvrkms->uvr_kms_node_display_output_chain.crtcProps.props,
						   uvrkms->uvr_kms_node_display_output_chain.crtcProps.propsData);

		}
		if (uvrkms->uvr_kms_node_display_output_chain.encoder)
			drmModeFreeEncoder(uvrkms->uvr_kms_node_display_output_chain.encoder);
		if (uvrkms->uvr_kms_node_display_output_chain.connector) {
			drmModeFreeConnector(uvrkms->uvr_kms_node_display_output_chain.connector);
			free_kms_object_properties(uvrkms->uvr_kms_node_display_output_chain.connectorProps.props,
						   uvrkms->uvr_kms_node_display_output_chain.connectorProps.propsData);
		}
		if (uvrkms->uvr_kms_node_display_output_chain.modeid)
			drmModeDestroyPropertyBlob(uvrkms->uvr_kms_node.kmsfd, uvrkms->uvr_kms_node_display_output_chain.modeid);
		if (uvrkms->uvr_kms_node.vtfd != -1 && uvrkms->uvr_kms_node.keyBoardMode != -1)
			vt_reset(uvrkms->uvr_kms_node.vtfd, uvrkms->uvr_kms_node.keyBoardMode);
#ifdef INCLUDE_LIBSEAT
		uvr_session_release_device(uvrkms->uvr_kms_node.session, uvrkms->uvr_kms_node.kmsfd);
#else
		close(uvrkms->uvr_kms_node.kmsfd);
#endif
	}
}
