#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
#include <sys/vt.h>
#include <sys/kd.h>
#include <linux/major.h>
#include <libudev.h>

#include "drm-node.h"


/***************************************
 * START OF GLOBAL TO FILE ENUM MACROS *
 ***************************************/

/*
 * No guaranteed that each GPU's driver KMS objects (plane->crtc->connector)
 * will have properties in this exact order or these exact listed properties.
 * But this are the properties that this library supports.
 */
enum kmr_drm_node_connector_prop_type {
	KMR_KMS_NODE_CONNECTOR_PROP_EDID                = 0,
	KMR_KMS_NODE_CONNECTOR_PROP_DPMS                = 1,
	KMR_KMS_NODE_CONNECTOR_PROP_LINK_STATUS         = 2,
	KMR_KMS_NODE_CONNECTOR_PROP_NON_DESKTOP         = 3,
	KMR_KMS_NODE_CONNECTOR_PROP_TILE                = 4,
	KMR_KMS_NODE_CONNECTOR_PROP_CRTC_ID             = 5,
	KMR_KMS_NODE_CONNECTOR_PROP_SCALING_MODE        = 6,
	KMR_KMS_NODE_CONNECTOR_PROP_UNDERSCAN           = 7,
	KMR_KMS_NODE_CONNECTOR_PROP_UNDERSCAN_HBORDER   = 8,
	KMR_KMS_NODE_CONNECTOR_PROP_UNDERSCAN_VBORDER   = 9,
	KMR_KMS_NODE_CONNECTOR_PROP_MAX_BPC             = 10,
	KMR_KMS_NODE_CONNECTOR_PROP_HDR_OUTPUT_METADATA = 11,
	KMR_KMS_NODE_CONNECTOR_PROP_VRR_CAPABLE         = 12,
	KMR_KMS_NODE_CONNECTOR_PROP__COUNT              = 13
};


enum kmr_drm_node_crtc_prop_type {
	KMR_KMS_NODE_CRTC_PROP_ACTIVE           = 0,
	KMR_KMS_NODE_CRTC_PROP_MODE_ID          = 1,
	KMR_KMS_NODE_CRTC_PROP_OUT_FENCE_PTR    = 2,
	KMR_KMS_NODE_CRTC_PROP_VRR_ENABLED      = 3,
	KMR_KMS_NODE_CRTC_PROP_DEGAMMA_LUT      = 4,
	KMR_KMS_NODE_CRTC_PROP_DEGAMMA_LUT_SIZE = 5,
	KMR_KMS_NODE_CRTC_PROP_CTM              = 6,
	KMR_KMS_NODE_CRTC_PROP_GAMMA_LUT        = 7,
	KMR_KMS_NODE_CRTC_PROP_GAMMA_LUT_SIZE   = 8,
	KMR_KMS_NODE_CRTC_PROP__COUNT           = 9
};


enum kmr_drm_node_plane_prop_type {
	KMR_KMS_NODE_PLANE_PROP_TYPE           = 0,
	KMR_KMS_NODE_PLANE_PROP_FB_ID          = 1,
	KMR_KMS_NODE_PLANE_PROP_IN_FENCE_FD    = 2,
	KMR_KMS_NODE_PLANE_PROP_CRTC_ID        = 3,
	KMR_KMS_NODE_PLANE_PROP_CRTC_X         = 4,
	KMR_KMS_NODE_PLANE_PROP_CRTC_Y         = 5,
	KMR_KMS_NODE_PLANE_PROP_CRTC_W         = 6,
	KMR_KMS_NODE_PLANE_PROP_CRTC_H         = 7,
	KMR_KMS_NODE_PLANE_PROP_SRC_X          = 8,
	KMR_KMS_NODE_PLANE_PROP_SRC_Y          = 9,
	KMR_KMS_NODE_PLANE_PROP_SRC_W          = 10,
	KMR_KMS_NODE_PLANE_PROP_SRC_H          = 11,
	KMR_KMS_NODE_PLANE_PROP_IN_FORMATS     = 12,
	KMR_KMS_NODE_PLANE_PROP_COLOR_ENCODING = 13,
	KMR_KMS_NODE_PLANE_PROP_COLOR_RANGE    = 14,
	KMR_KMS_NODE_PLANE_PROP_ROTATION       = 15,
	KMR_KMS_NODE_PLANE_PROP__COUNT         = 16
};

/***************************************
 * END OF GLOBAL TO FILE ENUM MACROS *
 ***************************************/


/****************************************************
 * START OF kmr_drm_node_{create,destroy} FUNCTIONS *
 ****************************************************/

static int
setup_atomic_modeset (int kmsfd)
{
	int err = 0;
	drm_magic_t magic;

	/*
	 * TAKEN FROM Daniel Stone (gitlab/kms-quads)
	 * In order to drive KMS, we need to be 'master'. This should already
	 * have happened for us thanks to being root and the first client.
	 * There can only be one master at a time, so this will fail if
	 * (e.g.) trying to run this test whilst a graphical session is
	 * already active on the current VT.
	 */
	err = drmGetMagic(kmsfd, &magic);
	if (err < 0) {
		kmr_utils_log(KMR_DANGER, "[x] drmGetMagic: KMS device '(fd: %d)' could not become master", kmsfd);
		return -1;
	}

	err = drmAuthMagic(kmsfd, magic);
	if (err < 0) {
		kmr_utils_log(KMR_DANGER, "[x] drmGetMagic: KMS device '(fd: %d)' could not become master", kmsfd);
		return -1;
	}

	/*
	 * Tell DRM core to expose atomic properties to userspace. This also enables
	 * DRM_CLIENT_CAP_UNIVERSAL_PLANES and DRM_CLIENT_CAP_ASPECT_RATIO.
	 * DRM_CLIENT_CAP_UNIVERSAL_PLANES - Tell DRM core to expose all planes (overlay, primary, and cursor) to userspace.
	 * DRM_CLIENT_CAP_ASPECT_RATIO     - Tells DRM core to provide aspect ratio information in modes
	 */
	err = drmSetClientCap(kmsfd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	if (err) {
		kmr_utils_log(KMR_DANGER, "[x] drmSetClientCap: Failed to set universal "
				          "planes capability for KMS device '(fd: %d)'", kmsfd);
		return -1;
	}

	/*
	 * Tell DRM core that we're going to use the KMS atomic API. It's supposed
	 * to set the DRM_CLIENT_CAP_UNIVERSAL_PLANES automatically.
	 */
	err = drmSetClientCap(kmsfd, DRM_CLIENT_CAP_ATOMIC, 1);
	if (err) {
		kmr_utils_log(KMR_DANGER, "[x] drmSetClientCap: Failed to set KMS atomic "
		                          "capability for KMS device '(fd: %d)'", kmsfd);
		return -1;
	}

	return 0;
}


static int
open_drm_node (struct kmr_drm_node_create_info UNUSED *nodeInfo,
               const char *deviceNode)
{
	int kmsfd = -1;

#ifdef INCLUDE_LIBSEAT
	kmsfd = kmr_session_take_control_of_device(nodeInfo->session, deviceNode);
#else
	kmsfd = open(deviceNode, O_RDWR | O_CLOEXEC, 0);
#endif
	if (kmsfd < 0) {
		kmr_utils_log(KMR_WARNING, "open('%s'): %s", deviceNode, strerror(errno));
		return -1;
	}

	return kmsfd;
}


static int
open_drm_node_udev (struct kmr_drm_node_create_info *nodeInfo,
                    struct kmr_drm_node *drmNode)
{
	const char *devNode = NULL;

	struct udev *udev = NULL;
	struct udev_enumerate *udevEnum = NULL;
	struct udev_list_entry *entry = NULL;
	struct udev_device *device = NULL;

	struct kmr_drm_node_device_capabilites deviceCap;

	udev = udev_new();
	if (!udev) {
		kmr_utils_log(KMR_DANGER, "[x] udev_new: failed to create udev context.");
		goto exit_error_open_node_udev;
	}

	udevEnum = udev_enumerate_new(udev);
	if (!udevEnum) {
		kmr_utils_log(KMR_DANGER, "[x] udev_enumerate_new: failed");
		goto exit_error_open_node_udev;
	}

	udev_enumerate_add_match_subsystem(udevEnum, "drm");
	udev_enumerate_add_match_sysname(udevEnum, DRM_PRIMARY_MINOR_NAME "[0-9]*");

	if (udev_enumerate_scan_devices(udevEnum) != 0) {
		kmr_utils_log(KMR_DANGER, "[x] udev_enumerate_scan_devices: failed");
		goto exit_error_open_node_udev;
	}

	udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(udevEnum)) {
		device = udev_device_new_from_syspath(udev, udev_list_entry_get_name(entry));
		if (!device)
			continue;

		devNode = udev_device_get_devnode(device);
		if (!devNode) {
			kmr_utils_log(KMR_WARNING, "udev_device_get_devnode: unable to acquire path to KMS node");
			udev_device_unref(device); device = NULL;
			continue;
		}

		drmNode->kmsfd = open_drm_node(nodeInfo, devNode);
		if (drmNode->kmsfd == -1) {
			udev_device_unref(device); device = NULL;
			continue;
		}

		deviceCap = kmr_drm_node_get_device_capabilities(drmNode->kmsfd);
		if (!deviceCap.CAP_CRTC_IN_VBLANK_EVENT) {
			udev_device_unref(device); device = NULL;
			continue;
		}

		if (!deviceCap.CAP_DUMB_BUFFER) {
			udev_device_unref(device); device = NULL;
			continue;
		}

		kmr_utils_log(KMR_SUCCESS, "Opened KMS node '%s' associated fd is %d", devNode, drmNode->kmsfd);

		udev_device_unref(device);
		udev_enumerate_unref(udevEnum);
		udev_unref(udev);

		return 0;
	}

exit_error_open_node_udev:
	if (udevEnum)
		udev_enumerate_unref(udevEnum);
	if (udev)
		udev_unref(udev);
	return -1;
}


struct kmr_drm_node *
kmr_drm_node_create (struct kmr_drm_node_create_info *nodeInfo)
{
	int err = 0;
	struct kmr_drm_node *kmsNode = NULL;

	kmsNode = calloc(1, sizeof(struct kmr_drm_node));
	if (!kmsNode) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		return NULL;
	}

	if (nodeInfo->kmsNode) {
		err = kmsNode->kmsfd = open_drm_node(nodeInfo, nodeInfo->kmsNode);
	} else {
		err = open_drm_node_udev(nodeInfo, kmsNode);
	}

	if (err == -1)
		goto exit_error_kms_node_create;

	err = setup_atomic_modeset(kmsNode->kmsfd);
	if (err == -1)
		goto exit_error_kms_node_create;

#ifdef INCLUDE_LIBSEAT
	kmsNode->session = nodeInfo->session;
#endif
	return kmsNode;

exit_error_kms_node_create:
	kmr_drm_node_destroy(kmsNode);
	return NULL;
}


void
kmr_drm_node_destroy (struct kmr_drm_node *drmNode)
{
	if (!drmNode)
		return;

#ifdef INCLUDE_LIBSEAT
	kmr_session_release_device(drmNode->session, drmNode->kmsfd);
#else
	close(drmNode->kmsfd);
#endif
	free(drmNode);
}


/**************************************************
 * END OF kmr_drm_node_{create,destroy} FUNCTIONS *
 **************************************************/


/***********************************************************
 * START OF kmr_drm_node_get_device_capabilities FUNCTIONS *
 ***********************************************************/

struct kmr_drm_node_device_capabilites
kmr_drm_node_get_device_capabilities (int kmsfd)
{
	bool supported = false;
	uint64_t capabilites = 0, err = 0;

	struct kmr_drm_node_device_capabilites kmsNodeDeviceCapabilites;

	err = drmGetCap(kmsfd, DRM_CAP_ADDFB2_MODIFIERS, &capabilites);
	supported = (err == 0 && capabilites != 0);
	kmsNodeDeviceCapabilites.CAP_ADDFB2_MODIFIERS = supported ? true : false;
	kmr_utils_log(KMR_INFO, "device %s framebuffer modifiers", supported ? "supports" : "does not support");

	capabilites=0;
	err = drmGetCap(kmsfd, DRM_CAP_TIMESTAMP_MONOTONIC, &capabilites);
	supported = (err == 0 && capabilites != 0);
	kmsNodeDeviceCapabilites.CAP_TIMESTAMP_MONOTONIC = supported ? true : false;
	kmr_utils_log(KMR_INFO, "device %s clock monotonic timestamps", supported ? "supports" : "does not support");

	capabilites=0;
	err = drmGetCap(kmsfd, DRM_CAP_CRTC_IN_VBLANK_EVENT, &capabilites);
	supported = (err == 0 && capabilites != 0);
	kmsNodeDeviceCapabilites.CAP_CRTC_IN_VBLANK_EVENT = supported ? true : false;
	kmr_utils_log(KMR_INFO, "device %s atomic KMS", supported ? "supports" : "does not support");

	capabilites=0;
	err = drmGetCap(kmsfd, DRM_CAP_DUMB_BUFFER, &capabilites);
	supported = (err == 0 && capabilites != 0);
	kmsNodeDeviceCapabilites.CAP_DUMB_BUFFER = supported ? true : false;
	kmr_utils_log(KMR_INFO, "device %s dumb bufffers", supported ? "supports" : "does not support");

	return kmsNodeDeviceCapabilites;
}

/*********************************************************
 * END OF kmr_drm_node_get_device_capabilities FUNCTIONS *
 *********************************************************/


/************************************************************
 * START OF kmr_drm_node_display_{create,destroy} FUNCTIONS *
 ************************************************************/

/*
 * Helper function that retrieves the properties of a certain CRTC, plane or connector kms object.
 * There's no garunteed for the availability of kms object properties and the order which they
 * reside in @props->props. All we can do is account for as many as possible.
 * Function will only assign values and ids to properties we actually use for mode setting.
 */
static int
acquire_kms_object_properties (int fd,
                               struct kmr_drm_node_display_object_props *obj,
			       uint32_t type)
{
	unsigned int i;
	uint16_t propsDataCount = 0;

	char *typeStr = NULL;
	drmModePropertyRes *propData = NULL;
	drmModeObjectProperties *props = NULL;

	switch(type) {
		case DRM_MODE_OBJECT_CONNECTOR:
			typeStr = "connector";
			propsDataCount = KMR_KMS_NODE_CONNECTOR_PROP__COUNT;
			break;
		case DRM_MODE_OBJECT_PLANE:
			typeStr = "plane";
			propsDataCount = KMR_KMS_NODE_PLANE_PROP__COUNT;
			break;
		case DRM_MODE_OBJECT_CRTC:
			typeStr = "CRTC";
			propsDataCount = KMR_KMS_NODE_CRTC_PROP__COUNT;
			break;
		default:
			typeStr = "unknown type";
			break;
	}

	props = drmModeObjectGetProperties(fd, obj->id, type);
	if (!props) {
		kmr_utils_log(KMR_DANGER, "[x] cannot get %s %d properties: %s",
		                          typeStr, obj->id, strerror(errno));
		return -1;
	}

	obj->propsDataCount = propsDataCount;
	obj->propsData = calloc(obj->propsDataCount, sizeof(struct kmr_drm_node_display_object_props_data));
	if (!obj->propsData) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_acquire_kms_object_properties;
	}

	for (i = 0; i < obj->propsDataCount; i++) {
		propData = drmModeGetProperty(fd, props->props[i]);
		if (!propData) {
			kmr_utils_log(KMR_DANGER, "[x] drmModeGetProperty: failed to get property data.");
			goto exit_error_acquire_kms_object_properties;
		}

		switch (type) {
			case DRM_MODE_OBJECT_CONNECTOR:
				if (!strncmp(propData->name, "CRTC_ID", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_CONNECTOR_PROP_CRTC_ID].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_CONNECTOR_PROP_CRTC_ID].value = props->prop_values[i];
					break;
				}

				break;
			case DRM_MODE_OBJECT_PLANE:
				if (!strncmp(propData->name, "FB_ID", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_FB_ID].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_FB_ID].value = props->prop_values[i];
					break;
				}

				if (!strncmp(propData->name, "CRTC_ID", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_ID].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_ID].value = props->prop_values[i];
					break;
				}

				if (!strncmp(propData->name, "SRC_X", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_X].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_X].value = props->prop_values[i];
					break;
				}

				if (!strncmp(propData->name, "SRC_Y", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_Y].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_Y].value = props->prop_values[i];
					break;
				}

				if (!strncmp(propData->name, "SRC_W", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_W].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_W].value = props->prop_values[i];
					break;
				}

				if (!strncmp(propData->name, "SRC_H", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_H].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_H].value = props->prop_values[i];
					break;
				}

				if (!strncmp(propData->name, "CRTC_X", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_X].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_X].value = props->prop_values[i];
					break;
				}

				if (!strncmp(propData->name, "CRTC_Y", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_Y].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_Y].value = props->prop_values[i];
					break;
				}

				if (!strncmp(propData->name, "CRTC_W", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_W].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_W].value = props->prop_values[i];
					break;
				}

				if (!strncmp(propData->name, "CRTC_H", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_H].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_H].value = props->prop_values[i];
					break;
				}

				break;
			case DRM_MODE_OBJECT_CRTC:
				if (!strncmp(propData->name, "MODE_ID", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_CRTC_PROP_MODE_ID].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_CRTC_PROP_MODE_ID].value = props->prop_values[i];
					break;
				}

				if (!strncmp(propData->name, "ACTIVE", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_CRTC_PROP_ACTIVE].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_CRTC_PROP_ACTIVE].value = props->prop_values[i];
					break;
				}

				break;
			default:
				break;
		}

		drmModeFreeProperty(propData); propData = NULL;
	}

	drmModeFreeObjectProperties(props);

	return 0;

exit_error_acquire_kms_object_properties:
	if (propData)
		drmModeFreeProperty(propData);
	if (props)
		drmModeFreeObjectProperties(props);
	free(obj->propsData);
	obj->propsData = NULL;
	return -1;
}


static drmModeConnector *
drm_node_get_connector (int kmsfd, uint32_t connectorID)
{
	drmModeConnector *connector = NULL;

	connector = drmModeGetConnector(kmsfd, connectorID);
	if (!connector) {
		kmr_utils_log(KMR_DANGER, "[x] drmModeGetConnector: Failed to get connector");
		return NULL;
	}

	/* check if a monitor is connected */
	if (connector->encoder_id == 0 || connector->connection != DRM_MODE_CONNECTED) {
		kmr_utils_log(KMR_INFO, "[CONNECTOR:%" PRIu32 "]: no encoder or not connected to display", connector->connector_id);
		drmModeFreeConnector(connector);
		return NULL;
	}

	return connector;
}


static drmModeEncoder *
drm_node_get_encoder (int kmsfd, uint32_t encoderID)
{
	drmModeEncoder *encoder = NULL;

	encoder = drmModeGetEncoder(kmsfd, encoderID);
	if (!encoder) {
		kmr_utils_log(KMR_DANGER, "[x] drmModeGetEncoder: Failed to get encoder");
		return NULL;
	}

	if (encoder->crtc_id == 0) {
		kmr_utils_log(KMR_INFO, "[ENCODER:%" PRIu32 "]: no CRTC", encoder->encoder_id);
		drmModeFreeEncoder(encoder);
		return NULL;
	}

	return encoder;
}


static drmModeCrtc *
drm_node_get_crtc (int kmsfd, uint32_t crtcID)
{
	drmModeCrtc *crtc = NULL;

	crtc = drmModeGetCrtc(kmsfd, crtcID);
	if (!crtc) {
		kmr_utils_log(KMR_DANGER, "[x] drmModeGetCrtc: Failed to get crtc KMS object");
		return NULL;
	}

	/* Ensure the CRTC is active. */
	if (crtc->buffer_id == 0) {
		kmr_utils_log(KMR_INFO, "[CRTC:%" PRIu32 "]: not active", crtc->crtc_id);
		drmModeFreeCrtc(crtc);
		return NULL;
	}

	return crtc;
}


struct kmr_drm_node_display *
kmr_drm_node_display_create (struct kmr_drm_node_display_create_info *displayInfo)
{
	int p, e, c, conn, planesCount = 0, err = -1;

	drmModeRes *drmResources = NULL;
	drmModePlane **planes = NULL;
	drmModePlaneRes *drmPlaneResources = NULL;
	drmModeConnector *connector = NULL;
	drmModeEncoder *encoder = NULL;
	drmModeCrtc *crtc = NULL;
	drmModePlane *plane = NULL;

	struct kmr_drm_node_device_capabilites deviceCap;
	struct kmr_drm_node_display *display = NULL;

	display = calloc(1, sizeof(struct kmr_drm_node_display));
	if (!display) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		return NULL;
	}

	/* Query for connector->encoder->crtc KMS objecs */
	drmResources = drmModeGetResources(displayInfo->kmsfd);
	if (!drmResources) {
		kmr_utils_log(KMR_DANGER, "[x] Couldn't get card resources from KMS fd '%d'", displayInfo->kmsfd);
		goto exit_error_kms_node_display_create;
	}

	/* Query for plane KMS objecs */
	drmPlaneResources = drmModeGetPlaneResources(displayInfo->kmsfd);
	if (!drmPlaneResources) {
		kmr_utils_log(KMR_DANGER, "[x] KMS fd '%d' has no planes", displayInfo->kmsfd);
		goto exit_error_kms_node_display_create;
	}

	/* Check if some form of a display output chain exist */
	if (drmResources->count_crtcs       <= 0 ||
	    drmResources->count_connectors  <= 0 ||
	    drmResources->count_encoders    <= 0 ||
	    drmPlaneResources->count_planes <= 0)
	{
		kmr_utils_log(KMR_DANGER, "[x] KMS fd '%d' has no way of creating a "
		                          "display output chain", displayInfo->kmsfd);
		goto exit_error_kms_node_display_create;
	}

	/* Query KMS device plane info */
	planesCount = drmPlaneResources->count_planes;
	planes = alloca(planesCount * sizeof(drmModePlane));
	memset(planes, 0, planesCount * sizeof(drmModePlane));

	for (p = 0; p < planesCount; p++) {
		planes[p] = drmModeGetPlane(displayInfo->kmsfd, drmPlaneResources->planes[p]);
		if (!planes[p]) {
			kmr_utils_log(KMR_DANGER, "[x] drmModeGetPlane: Failed to get plane");
			goto exit_error_kms_node_display_create;
		}
	}

	/*
	 * Go through connectors one by one and try to find a usable output chain.
	 * OUTPUT CHAIN: connector->encoder->crtc->plane
	 * @encoder - (Deprecated) Takes pixel data from a crtc and converts it to an output that
	 *            the connector can understand. This is a Deprecated kms object
	 */
	for (conn = 0; conn < drmResources->count_connectors; conn++) {
		connector = drm_node_get_connector(displayInfo->kmsfd, drmResources->connectors[conn]);
		if (!connector)
			continue;

		/* Find the encoder (a deprecated KMS object) for this connector. */
		for (e = 0; e < drmResources->count_encoders; e++) {
			if (drmResources->encoders[e] == connector->encoder_id) {
				encoder = drm_node_get_encoder(displayInfo->kmsfd, drmResources->encoders[e]);
				if (!encoder)
					goto exit_error_kms_node_display_create;
			}
		}

		for (c = 0; c < drmResources->count_crtcs; c++) {
			if (drmResources->crtcs[c] == encoder->crtc_id) {
				crtc = drm_node_get_crtc(displayInfo->kmsfd, drmResources->crtcs[c]);
				if (!crtc)
					goto exit_error_kms_node_display_create;
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
		for (p = 0; p < (int) drmPlaneResources->count_planes; p++) {
			if (planes[p]->crtc_id == crtc->crtc_id && planes[p]->fb_id == crtc->buffer_id) {
				plane = planes[p];
				break;
			}
		}

		kmr_utils_log(KMR_SUCCESS, "Successfully found a display output chain");

		// Stores mode id given to one of the displays resolution + refresh
		memcpy(&display->modeData.modeInfo, &connector->modes[0], sizeof(drmModeModeInfo));
		err = drmModeCreatePropertyBlob(displayInfo->kmsfd,
		                                &connector->modes[0],
					        sizeof(connector->modes[0]),
					        &display->modeData.id);
		if (err != 0) {
			kmr_utils_log(KMR_DANGER, "[x] drmModeCreatePropertyBlob: couldn't create a blob property");
			goto exit_error_kms_node_display_create;
		}

		display->connector.id = connector->connector_id;
		err = acquire_kms_object_properties(displayInfo->kmsfd, &display->connector, DRM_MODE_OBJECT_CONNECTOR);
		if (err == -1)
			goto exit_error_kms_node_display_create;

		display->crtc.id = crtc->crtc_id;
		err = acquire_kms_object_properties(displayInfo->kmsfd, &display->crtc, DRM_MODE_OBJECT_CRTC);
		if (err == -1)
			goto exit_error_kms_node_display_create;

		display->plane.id = plane->plane_id;
		err = acquire_kms_object_properties(displayInfo->kmsfd, &display->plane, DRM_MODE_OBJECT_PLANE);
		if (err == -1)
			goto exit_error_kms_node_display_create;

		kmr_utils_log(KMR_INFO, "Plane ID: %u", plane->plane_id);
		kmr_utils_log(KMR_INFO, "CRTC ID: %u", crtc->crtc_id);
		kmr_utils_log(KMR_INFO, "ENCODER ID: %u", encoder->encoder_id);
		kmr_utils_log(KMR_INFO, "CONNECTOR ID: %u", connector->connector_id);
		kmr_utils_log(KMR_INFO, "MODE (resolution + refresh) ID: %u", display->modeData.id);

		deviceCap = kmr_drm_node_get_device_capabilities(displayInfo->kmsfd);

		display->width = connector->modes[0].hdisplay;
		display->height = connector->modes[0].vdisplay;
		display->kmsfd = displayInfo->kmsfd;	
		display->presClock = (deviceCap.CAP_TIMESTAMP_MONOTONIC) ? CLOCK_MONOTONIC : CLOCK_REALTIME;

		/* Free all unused resources */
		for (p = 0; p < planesCount; p++)
			drmModeFreePlane(planes[p]);
		drmModeFreeEncoder(encoder);
		drmModeFreeCrtc(crtc);
		drmModeFreeConnector(connector);
		drmModeFreePlaneResources(drmPlaneResources);
		drmModeFreeResources(drmResources);

		return display;
	}

exit_error_kms_node_display_create:
	free(display->plane.propsData);
	free(display->crtc.propsData);
	free(display->connector.propsData);
	if (display->modeData.id)
		drmModeDestroyPropertyBlob(displayInfo->kmsfd, display->modeData.id);
	if (crtc)
		drmModeFreeCrtc(crtc);
	if (encoder)
		drmModeFreeEncoder(encoder);
	if (connector)
		drmModeFreeConnector(connector);
	for (p = 0; p < planesCount; p++)
		if (planes[p])
			drmModeFreePlane(planes[p]);
	if (drmPlaneResources)
		drmModeFreePlaneResources(drmPlaneResources);
	if (drmResources)
		drmModeFreeResources(drmResources);
	kmr_drm_node_display_destroy(display);
	return NULL;
}


void
kmr_drm_node_display_destroy (struct kmr_drm_node_display *display)
{
	if (!display)
		return;

	free(display->plane.propsData);
	free(display->crtc.propsData);
	free(display->connector.propsData);
	if (display->modeData.id)
		drmModeDestroyPropertyBlob(display->kmsfd, display->modeData.id);

	free(display);
}

/**********************************************************
 * END OF kmr_drm_node_display_{create,destroy} FUNCTIONS *
 **********************************************************/


/************************************************************
 * START OF kmr_drm_node_display_mode_{set,reset} FUNCTIONS *
 ************************************************************/

int
kmr_drm_node_display_mode_set (struct kmr_drm_node_display_mode_info *displayModeInfo)
{
	int ret = -1;

	ret = drmModeSetCrtc(displayModeInfo->display->kmsfd,
	                     displayModeInfo->display->crtc.id,
	                     displayModeInfo->fbid,
			     0,
			     0,
			     &displayModeInfo->display->connector.id,
			     1,
			     &displayModeInfo->display->modeData.modeInfo);

	if (ret) {
		kmr_utils_log(KMR_DANGER, "[x] drmModeSetCrtc: %s", strerror(errno));
		kmr_utils_log(KMR_DANGER, "[x] drmModeSetCrtc: failed to set preferred display mode");
		return -1;
	}

	return 0;
}


int
kmr_drm_node_display_mode_reset (struct kmr_drm_node_display_mode_info *displayModeInfo)
{
	int ret = -1;

	ret = drmModeSetCrtc(displayModeInfo->display->kmsfd,
	                     displayModeInfo->display->crtc.id,
			     0, 0, 0, NULL, 0, NULL);

	if (ret) {
		kmr_utils_log(KMR_DANGER, "[x] drmModeSetCrtc: %s", strerror(errno));
		kmr_utils_log(KMR_DANGER, "[x] drmModeSetCrtc: failed to reset current display mode");
		return -1;
	}

	return 0;
}

/**********************************************************
 * END OF kmr_drm_node_display_mode_{set,reset} FUNCTIONS *
 **********************************************************/


/*******************************************************************
 * START OF kmr_drm_node_atomic_request_{create,destroy} FUNCTIONS *
 *******************************************************************/

/*
 * Here we set the values of properties (of our connector, CRTC and plane objects)
 * that we want to change in the atomic commit. These changes are temporarily stored
 * in drmModeAtomicReq *atomicRequest until DRM core receives commit.
 */
static void
modeset_atomic_prepare_commit(drmModeAtomicReq *atomicRequest,
                              struct kmr_drm_node_display *display,
                              int fbid)
{
	/* set id of the CRTC id that the connector is using */
	drmModeAtomicAddProperty(atomicRequest,
	                         display->connector.id,
	                         display->connector.propsData[KMR_KMS_NODE_CONNECTOR_PROP_CRTC_ID].id,
	                         display->crtc.id);

	/*
	 * set the mode id of the CRTC; this property receives the id of a blob
	 * property that holds the struct that actually contains the mode info
	 */
	drmModeAtomicAddProperty(atomicRequest,
	                         display->crtc.id,
	                         display->crtc.propsData[KMR_KMS_NODE_CRTC_PROP_MODE_ID].id,
	                         display->modeData.id);

	/* set the CRTC object as active */
	drmModeAtomicAddProperty(atomicRequest,
	                         display->crtc.id,
	                         display->crtc.propsData[KMR_KMS_NODE_CRTC_PROP_ACTIVE].id,
	                         1);

	/* set properties of the plane related to the CRTC and the framebuffer */
	drmModeAtomicAddProperty(atomicRequest,
	                         display->plane.id,
	                         display->plane.propsData[KMR_KMS_NODE_PLANE_PROP_FB_ID].id,
	                         fbid);

	drmModeAtomicAddProperty(atomicRequest,
	                         display->plane.id,
	                         display->plane.propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_ID].id,
	                         display->crtc.id);

	drmModeAtomicAddProperty(atomicRequest,
	                         display->plane.id,
	                         display->plane.propsData[KMR_KMS_NODE_PLANE_PROP_SRC_X].id,
	                         0);

	drmModeAtomicAddProperty(atomicRequest,
	                         display->plane.id,
	                         display->plane.propsData[KMR_KMS_NODE_PLANE_PROP_SRC_Y].id,
	                         0);

	drmModeAtomicAddProperty(atomicRequest,
	                         display->plane.id,
	                         display->plane.propsData[KMR_KMS_NODE_PLANE_PROP_SRC_W].id,
	                         (display->width << 16));

	drmModeAtomicAddProperty(atomicRequest,
	                         display->plane.id,
	                         display->plane.propsData[KMR_KMS_NODE_PLANE_PROP_SRC_H].id,
	                         (display->height << 16));

	drmModeAtomicAddProperty(atomicRequest,
	                         display->plane.id,
	                         display->plane.propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_X].id,
	                         0);

	drmModeAtomicAddProperty(atomicRequest,
	                         display->plane.id,
	                         display->plane.propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_Y].id,
	                         0);

	drmModeAtomicAddProperty(atomicRequest,
	                         display->plane.id,
	                         display->plane.propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_W].id,
	                         display->width);

	drmModeAtomicAddProperty(atomicRequest,
	                         display->plane.id,
	                         display->plane.propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_H].id,
	                         display->height);
}


/*
 * struct kmr_drm_node_renderer_info (kmsroots KMS Node Renderer Information)
 *
 * members:
 * @display               - Pointer to a struct containing all plane->crtc->connector data used during
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
struct kmr_drm_node_renderer_info {
	struct kmr_drm_node_display *display;
	kmr_drm_node_renderer_impl  renderer;
	volatile bool               *rendererRunning;
	uint8_t                     *rendererCurrentBuffer;
	drmModeAtomicReq            *rendererAtomicRequest;
	int                         *rendererFbId;
	void                        *rendererData;
};


struct kmr_drm_node_atomic_request *
kmr_drm_node_atomic_request_create (struct kmr_drm_node_atomic_request_create_info *atomicInfo)
{
	int err = -1;
	struct kmr_drm_node_renderer_info *rendererInfo = NULL;
	struct kmr_drm_node_atomic_request *atomic = NULL;

	atomic = calloc(1, sizeof(struct kmr_drm_node_atomic_request));
	if (!atomic) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		return NULL;
	}

	atomic->atomicRequest = drmModeAtomicAlloc();
	if (!atomic->atomicRequest)
		goto exit_error_kmr_drm_node_atomic_request_create;

	rendererInfo = calloc(1, sizeof(struct kmr_drm_node_renderer_info));
	if (!rendererInfo) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_drm_node_atomic_request_create;
	}

	rendererInfo->display = atomicInfo->display;
	rendererInfo->renderer = atomicInfo->renderer;
	rendererInfo->rendererRunning = atomicInfo->rendererRunning;
	rendererInfo->rendererCurrentBuffer = atomicInfo->rendererCurrentBuffer;
	rendererInfo->rendererAtomicRequest = atomic->atomicRequest;
	rendererInfo->rendererFbId = atomicInfo->rendererFbId;
	rendererInfo->rendererData = atomicInfo->rendererData;

	modeset_atomic_prepare_commit(rendererInfo->rendererAtomicRequest,
	                              rendererInfo->display,
	                              *rendererInfo->rendererFbId);

	/* perform test-only atomic commit */
	err = drmModeAtomicCommit(atomicInfo->kmsfd,
	                          rendererInfo->rendererAtomicRequest,
	                          DRM_MODE_ATOMIC_TEST_ONLY | DRM_MODE_ATOMIC_ALLOW_MODESET,
	                          rendererInfo);
	if (err < 0) {
		kmr_utils_log(KMR_DANGER, "drmModeAtomicCommit: %s", strerror(errno));
		goto exit_error_kmr_drm_node_atomic_request_create;
	}

	/* initial modeset on all outputs */
	err = drmModeAtomicCommit(atomicInfo->kmsfd,
	                          rendererInfo->rendererAtomicRequest,
	                          DRM_MODE_ATOMIC_ALLOW_MODESET | DRM_MODE_PAGE_FLIP_EVENT,
	                          rendererInfo);
	if (err < 0) {
		kmr_utils_log(KMR_DANGER, "[x] drmModeAtomicCommit: modeset atomic commit failed.");
		kmr_utils_log(KMR_DANGER, "[x] drmModeAtomicCommit: %s", strerror(errno));
		goto exit_error_kmr_drm_node_atomic_request_create;
	}

	atomic->rendererInfo = rendererInfo;
	return atomic;

exit_error_kmr_drm_node_atomic_request_create:
	free(rendererInfo);
	if (atomic) {
		drmModeAtomicFree(atomic->atomicRequest);
		free(atomic);
	}
	return NULL;
}


void
kmr_drm_node_atomic_request_destroy (struct kmr_drm_node_atomic_request *atomic)
{
	if (!atomic)
		return;

	drmModeAtomicFree(atomic->atomicRequest);
	free(atomic->rendererInfo);
}

/*****************************************************************
 * END OF kmr_drm_node_atomic_request_{create,destroy} FUNCTIONS *
 *****************************************************************/


/****************************************************
 * START OF kmr_drm_node_handle_drm_event FUNCTIONS *
 ****************************************************/

static void
handle_page_flip_event (int fd,
                        unsigned int UNUSED seq,
                        unsigned int UNUSED tv_sec,
                        unsigned int UNUSED tv_usec,
                        unsigned int UNUSED crtc_id,
                        void *data)
{
	static double finalTime = 0;
	static uint16_t fpsCounter = 0;

	struct timespec startTime, stopTime;
	struct kmr_drm_node_renderer_info *rendererInfo = NULL;
	struct kmr_drm_node_display *displayOutputChain = NULL;

	rendererInfo = data;
	displayOutputChain = rendererInfo->display;
	clock_gettime(displayOutputChain->presClock, &startTime);

	/*
	 * Application updates @rendererFbId to the next displayable
	 * GBM[GEM]/DUMP buffer and renders into that buffer.
	 * This buffer is displayed when atomic commit is performed
	 */
	rendererInfo->renderer(rendererInfo->rendererRunning,
	                       rendererInfo->rendererCurrentBuffer,
	                       rendererInfo->rendererFbId,
	                       rendererInfo->rendererData);

	/*
	 * Pepare properties for DRM core and temporarily store
	 * them in @rendererAtomicRequest. @rendererFbId should be
	 * an already populated buffer.
	 */
	modeset_atomic_prepare_commit(rendererInfo->rendererAtomicRequest,
	                              rendererInfo->display,
	                              *rendererInfo->rendererFbId);

	/*
	 * Send properties to DRM core and asks the driver to perform an atomic commit.
	 * This will lead to a page-flip and the content of the @rendererFbId will be displayed.
	 */
	drmModeAtomicCommit(fd,
	                    rendererInfo->rendererAtomicRequest,
	                    DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK,
	                    rendererInfo);

	clock_gettime(displayOutputChain->presClock, &stopTime);

	fpsCounter++;
	finalTime += (stopTime.tv_sec - startTime.tv_sec) + (double) (stopTime.tv_nsec - startTime.tv_nsec) / 1000000000ULL;

	if (finalTime >= 1.0f) {
		kmr_utils_log(KMR_INFO, "%u fps in %lf seconds for crtc %u", fpsCounter, finalTime, crtc_id);
		finalTime = 0; fpsCounter = 0;
	}
}


/*
 * Version 3 is the first version that allows us to use page_flip_handler2, which
 * is just like page_flip_handler but with the addition of passing the
 * crtc_id as argument to the function which allows us to find which output
 * the page-flip happened.
 *
 * The usage of page_flip_handler2 is the reason why we needed to verify
 * the support for DRM_CAP_CRTC_IN_VBLANK_EVENT.
 */
int
kmr_drm_node_handle_drm_event (struct kmr_drm_node_handle_drm_event_info *eventInfo)
{
	drmEventContext event;
	event.version = 3;
	event.page_flip_handler2 = handle_page_flip_event;
	return drmHandleEvent(eventInfo->kmsfd, &event);
}

/**************************************************
 * END OF kmr_drm_node_handle_drm_event FUNCTIONS *
 **************************************************/
