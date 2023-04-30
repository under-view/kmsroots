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
static int vt_setup(int *keyBoardMode)
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
		ttyname_r(STDIN_FILENO, ttyDevice, sizeof(ttyDevice));
	} else {
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

	/* If we get our VT from stdin, work painfully backwards to find its VT number. */
	if (ttyNum == 0) {
		struct stat vtStat;

		if (fstat(vtfd, &vtStat) == -1 || major(vtStat.st_rdev) != TTY_MAJOR) {
			uvr_utils_log(UVR_DANGER, "[x] fstat() || major(): VT file %s is bad", ttyDevice);
			goto exit_error_vt_setup_exit;
		}

		ttyNum = minor(vtStat.st_rdev);
	}

	if (!ttyNum)
		goto exit_error_vt_setup_exit;

	uvr_utils_log(UVR_INFO, "Using VT %d", ttyNum);

	/* Switch to the target VT. */
	if (ioctl(vtfd, VT_ACTIVATE, ttyNum) != 0 || ioctl(vtfd, VT_WAITACTIVE, ttyNum) != 0) {
		uvr_utils_log(UVR_DANGER, "[x] ioctl(VT_ACTIVATE) || ioctl(VT_WAITACTIVE): couldn't switch to VT %d", ttyNum);
		goto exit_error_vt_setup_exit;
	}

	uvr_utils_log(UVR_INFO, "Switched to VT %d", ttyNum);

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
	 * */
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
#ifdef INCLUDE_SDBUS
		if (uvrkms->useLogind)
			kmsfd = uvr_sd_session_take_control_of_device(uvrkms->systemdSession, uvrkms->kmsNode);
		else
#endif
			kmsfd = open(uvrkms->kmsNode, O_RDONLY | O_CLOEXEC, 0);
		if (kmsfd < 0) {
			uvr_utils_log(UVR_DANGER, "[x] open('%s'): %s", uvrkms->kmsNode, strerror(errno));
			goto exit_error_kms_node_create_free_kms_dev;
		}

		vtfd = vt_setup(&keyBoardMode);
		if (vtfd == -1)
			goto exit_error_kms_node_create_free_kms_dev;

		uvr_utils_log(UVR_SUCCESS, "Opened KMS node '%s' associated fd is %d", uvrkms->kmsNode, kmsfd);
		return (struct uvr_kms_node) { .kmsfd = kmsfd, .vtfd = vtfd, .keyBoardMode = keyBoardMode
#ifdef INCLUDE_SDBUS
			                       , .systemdSession = uvrkms->systemdSession, .useLogind = uvrkms->useLogind
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

#ifdef INCLUDE_SDBUS
		if (uvrkms->useLogind)
			kmsfd = uvr_sd_session_take_control_of_device(uvrkms->systemdSession, kmsNode);
		else
#endif
			kmsfd = open(kmsNode, O_RDONLY | O_CLOEXEC, 0);
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
		if (drmGetMagic(kmsfd, &magic) || drmAuthMagic(kmsfd, magic)) {
			uvr_utils_log(UVR_DANGER, "[x] KMS device '%s' is not master", kmsNode);
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

		vtfd = vt_setup(&keyBoardMode);
		if (vtfd == -1)
			goto exit_error_kms_node_create_free_kms_dev;

		return (struct uvr_kms_node) { .kmsfd = kmsfd, .vtfd = vtfd, .keyBoardMode = keyBoardMode
#ifdef INCLUDE_SDBUS
		                               , .systemdSession = uvrkms->systemdSession, .useLogind = uvrkms->useLogind
#endif
		};
	}

exit_error_kms_node_create_free_kms_dev:
	if (kmsfd != -1) {
#ifdef INCLUDE_SDBUS
		if (uvrkms->useLogind)
			uvr_sd_session_release_device(uvrkms->systemdSession, kmsfd);
		else
#endif
			close(kmsfd);
	}
	if (devices)
		drmFreeDevices(devices, deviceCount);

	return (struct uvr_kms_node) { .kmsfd = -1, .vtfd = -1, .keyBoardMode = -1
#ifdef INCLUDE_SDBUS
		                       , .systemdSession = NULL, .useLogind = false
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


struct uvr_kms_node_display_output_chain uvr_kms_node_display_output_chain_create(struct uvr_kms_node_display_output_chain_create_info *uvrkms)
{
	drmModeRes *drmResources = NULL;
	drmModePlaneRes *drmPlaneResources = NULL;
	unsigned int p;
	int i;

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
	drmModePlane **planes = alloca(drmPlaneResources->count_planes * sizeof(drmModePlane));
	memset(planes, 0, drmPlaneResources->count_planes * sizeof(drmModePlane));

	for (p = 0; p < drmPlaneResources->count_planes; i++) {
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

	for (i = 0; i < drmResources->count_connectors; i++) {
		connector = drmModeGetConnector(uvrkms->kmsfd, drmResources->connectors[i]);
		if (!connector) {
			uvr_utils_log(UVR_DANGER, "[x] drmModeGetConnector: Failed to get connector");
			goto exit_error_kms_node_doc_create_drm_mode_free_planes;
		}

		if (connector->encoder_id == 0) {
			uvr_utils_log(UVR_INFO, "[CONN:%" PRIu32 "]: no encoder", connector->connector_id);
			goto exit_error_kms_node_doc_create_drm_mode_free_connector;
		}

		/* Find the encoder (a deprecated KMS object) for this connector. */
		for (int e = 0; e < drmResources->count_encoders; e++) {
			if (drmResources->encoders[e] == connector->encoder_id) {
				encoder = drmModeGetEncoder(uvrkms->kmsfd, drmResources->encoders[e]);
				if (!encoder) {
					uvr_utils_log(UVR_DANGER, "[x] drmModeGetEncoder: Failed to get encoder KMS object associated with connector id '%d'",connector->connector_id);
					goto exit_error_kms_node_doc_create_drm_mode_free_connector;
				}
				break;
			}
		}

		if (encoder->crtc_id == 0) {
			uvr_utils_log(UVR_INFO, "[CONN:%" PRIu32 "]: no CRTC", connector->connector_id);
			goto exit_error_kms_node_doc_create_drm_mode_free_encoder;
		}

		for (int c = 0; c < drmResources->count_crtcs; c++) {
			if (drmResources->crtcs[c] == encoder->crtc_id) {
				crtc = drmModeGetCrtc(uvrkms->kmsfd, drmResources->crtcs[c]);
				if (!crtc) {
					uvr_utils_log(UVR_DANGER, "[x] drmModeGetCrtc: Failed to get crtc KMS object associated with encoder id '%d'", encoder->encoder_id);
					goto exit_error_kms_node_doc_create_drm_mode_free_encoder;
				}
				break;
			}
		}

		/* Ensure the CRTC is active. */
		if (crtc->buffer_id == 0) {
			uvr_utils_log(UVR_INFO, "[CONN:%" PRIu32 "]: not active", connector->connector_id);
			goto exit_error_kms_node_doc_create_drm_mode_free_crtc;
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
		for (p = 0; p < drmPlaneResources->count_planes; p++)
			if (planes[p]->fb_id != plane->fb_id)
				drmModeFreePlane(planes[p]);

		drmModeFreePlaneResources(drmPlaneResources);
		drmModeFreeResources(drmResources);

		return (struct uvr_kms_node_display_output_chain) { .connector = connector, .encoder = encoder, .crtc = crtc , .plane = plane };
	}

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
	for (p = 0; p < drmPlaneResources->count_planes; p++)
		if (planes[p])
			drmModeFreePlane(planes[p]);
exit_error_kms_node_doc_create_drm_mode_free_plane_resources:
	if (drmPlaneResources)
		drmModeFreePlaneResources(drmPlaneResources);
exit_error_kms_node_doc_create_drm_mode_free_resources:
	if (drmResources)
		drmModeFreeResources(drmResources);
exit_error_kms_node_doc_create:
	return (struct uvr_kms_node_display_output_chain) { .connector = 0, .encoder = 0, .crtc = 0 , .plane = 0 };
}


void uvr_kms_node_destroy(struct uvr_kms_node_destroy *uvrkms)
{
	if (uvrkms->uvr_kms_node_display_output_chain.plane)
		drmModeFreePlane(uvrkms->uvr_kms_node_display_output_chain.plane);
	if (uvrkms->uvr_kms_node_display_output_chain.crtc)
		drmModeFreeCrtc(uvrkms->uvr_kms_node_display_output_chain.crtc);
	if (uvrkms->uvr_kms_node_display_output_chain.encoder)
		drmModeFreeEncoder(uvrkms->uvr_kms_node_display_output_chain.encoder);
	if (uvrkms->uvr_kms_node_display_output_chain.connector)
		drmModeFreeConnector(uvrkms->uvr_kms_node_display_output_chain.connector);
	if (uvrkms->uvr_kms_node.kmsfd != -1) {
		if (uvrkms->uvr_kms_node.vtfd != -1 && uvrkms->uvr_kms_node.keyBoardMode != -1)
			vt_reset(uvrkms->uvr_kms_node.vtfd, uvrkms->uvr_kms_node.keyBoardMode);
#ifdef INCLUDE_SDBUS
	if (uvrkms->uvr_kms_node.useLogind)
		uvr_sd_session_release_device(uvrkms->uvr_kms_node.systemdSession, uvrkms->uvr_kms_node.kmsfd);
	else
#endif
		close(uvrkms->uvr_kms_node.kmsfd);
	}
}
