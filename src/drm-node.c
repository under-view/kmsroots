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

/*
 * TAKEN FROM Daniel Stone (gitlab/kms-quads)
 * Headers from the kernel's DRM uABI, allowing us to use ioctls directly.
 * These come from the kernel, via libdrm.
 */
#include <drm.h>
#include <drm_fourcc.h>
#include <drm_mode.h>

/*
 * TAKEN FROM Daniel Stone (gitlab/kms-quads)
 * Headers from the libdrm userspace library API, prefixed xf86*. These
 * mostly provide device and resource enumeration, as well as wrappers
 * around many ioctls, notably atomic modesetting.
 */
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <cando/cando.h>

#include "drm-node.h"


/*************************************************
 * Start of global to file enum macros & structs *
 *************************************************/

/*
 * No guaranteed that each GPU's driver KMS objects (plane->crtc->connector)
 * will have properties in this exact order or these exact listed properties.
 * But this are the properties that this library supports.
 */
enum kmr_drm_node_connector_prop_type
{
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


enum kmr_drm_node_crtc_prop_type
{
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


enum kmr_drm_node_plane_prop_type
{
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


/*
 * @brief struct kmr_drm_node_device_capabilites (kmsroots DRM Node Device Capabilites)
 *
 *        For more info see https://github.com/torvalds/linux/blob/master/include/uapi/drm/drm.h#L627
 *
 * @macro CAP_ADDFB2_MODIFIERS     - If or'd in struct kmr_drm_node { @deviceCap}, the driver
 *                                   supports supplying modifier in the &DRM_IOCTL_MODE_ADDFB2 ioctl.
 * @macro CAP_TIMESTAMP_MONOTONIC  - If not or'd, the kernel will report timestamps with
 *                                   ``CLOCK_REALTIME`` in struct drm_event_vblank. If or'd,
 *                                   the kernel will report timestamps with ``CLOCK_MONOTONIC``.
 *                                   See ``clock_gettime(2)`` for the definition of these clocks.
 * @macro CAP_CRTC_IN_VBLANK_EVENT - If or'd, the kernel supports reporting the CRTC ID in
 *                                   &drm_event_vblank.crtc_id for the &DRM_EVENT_VBLANK and
 *                                   &DRM_EVENT_FLIP_COMPLETE events.
 * @macro CAP_DUMB_BUFFER          - If set to true, the driver supports creating dumb buffers via
 *                                   the &DRM_IOCTL_MODE_CREATE_DUMB ioctl.
 *
 */
enum kmr_drm_node_device_capabilites
{
	CAP_ADDFB2_MODIFIERS     = 0x0001,
	CAP_TIMESTAMP_MONOTONIC  = 0x0002,
	CAP_CRTC_IN_VBLANK_EVENT = 0x0004,
	CAP_DUMB_BUFFER          = 0x0008,
	CAP_UNIVERSAL_PLANES     = 0x0010,
	CAP_ATOMIC               = 0x0011
};


/*
 * @brief struct kmr_drm_node_display_object_props_data
 *        (kmsroots DRM Node Display Object Properties Data)
 *
 * @member id    - Driver assigned ID of a given property belonging to a KMS object.
 * @member value - Enum value of given KMS object property. Can be used for instance
 *                 to check if plane object is a primary plane (DRM_PLANE_TYPE_PRIMARY).
 */
struct kmr_drm_node_display_object_props_data
{
	uint32_t id;
	uint64_t value;
};


/*
 * @brief struct kmr_drm_node_display_object_props
 *        (kmsroots DRM Node Display Object Properties)
 *
 *        It stores properties of certain KMS objects
 *        (connectors, CRTC and planes) that are used
 *        in atomic modeset setup and also in atomic page-flips.
 *
 * @member id             - Driver assigned ID of the KMS object.
 * @member propsDataCount - Array size of @propsData.
 * @member propsData      - Stores array of data about the properties
 *                          of a KMS object used during KMS atomic
 *                          operations.
 */
struct kmr_drm_node_display_object_props
{
	uint32_t                                      id;
	uint16_t                                      propsDataCount;
	struct kmr_drm_node_display_object_props_data propsData[KMR_KMS_NODE_PLANE_PROP__COUNT];
};


/*
 * @brief struct kmr_drm_node_display_mode_data (kmsroots DRM Node Display Mode Data)
 *
 * @member id       - Stores the highest mode (resolution + refresh) property id.
 *                    When we perform an atomic commit, the driver expects a CRTC
 *                    property named "MODE_ID", which points to the id given to one
 *                    of the connected display resolution & refresh rate. At the
 *                    moment the highest mode is choosen.
 * @member modeInfo - Stores the highest mode data (display resolution + refresh)
 *                    associated with display.
 */
struct kmr_drm_node_display_mode_data
{
	uint32_t        id;
	drmModeModeInfo modeInfo;
};


/*
 * @brief struct kmr_drm_node_display (kmsroots DRM Node Display)
 *
 * @member width      - Highest mode (display resolution) width for
 *                      @connector attached to display.
 * @member height     - Highest mode (display resolution) height for
 *                      @connector attached to display.
 * @member presClock  - Presentation clock stores the type of clock to
 *                      utilize for fps tracking. Clock will either be
 *                      set to CLOCK_MONOTONIC or CLOCK_REALTIME depending
 *                      upon the System/DRM device capabilities. CLOCK_MONOTONIC
 *                      will return the elapsed time from system boot, can only
 *                      increase, and can't be manually modified. While CLOCK_REALTIME
 *                      will return the real time system clock as set by the user.
 *                      This clock can however be modified.
 * @member modeData   - Stores highest mode (display resolution & refresh)
 *                      along with the modeid property used during KMS atomic
 *                      operations.
 * @member connector  - Anything that can transfer pixels in some form (i.e HDMI).
 *                      Connectors can be hotplugged and unplugged at runtime.
 *                      Stores connector properties used during KMS atomic
 *                      modesetting and page-flips.
 * @member crtc       - Represents a part of the chip that contains a pointer
 *                      to a scanout buffer. Stores crtc properties used during
 *                      KMS atomic modesetting and page-flips.
 * @member plane      - A plane represents an image source that can be blended
 *                      with or overlayed on top of a CRTC during the scanout
 *                      process. Planes are associated with a frame buffer to
 *                      crop a portion of the image memory (source) and optionally
 *                      scale it to a destination size. The result is then blended
 *                      with or overlayed on top of a CRTC. Stores primary plane
 *                      properties used during KMS atomic modesetting and page-flips.
 *
 * For more info see https://manpages.org/drm-kms/7
 */
struct kmr_drm_node_display
{
	uint16_t                                 width;
	uint16_t                                 height;
	clockid_t                                presClock;
	struct kmr_drm_node_display_mode_data    modeData;
	struct kmr_drm_node_display_object_props connector;
	struct kmr_drm_node_display_object_props crtc;
	struct kmr_drm_node_display_object_props plane;
};


/*
 * @brief struct kmr_drm_node (kmsroots DRM Node)
 *
 * @member err                   - Stores information about the error that occurred
 *                                 for the given instance and may later be retrieved
 *                                 by caller.
 * @member kmsfd                 - Pollable file descriptor to an open KMS (GPU) device file.
 * @member session               - Stores address of struct kmr_session. Used when
 *                                 opening and releasing a device.
 * @member display               - Pointer to a struct containing all plane->crtc->connector
 *                                 data used during KMS atomic mode setting.
 * @member renderer              - Function pointer that allows custom external renderers
 *                                 to be executed by the api upon @kmsfd polled events.
 * @member rendererRunning       - Pointer to a boolean that determines if a given renderer
 *                                 is running and in need of stopping.
 * @member rendererCurrentBuffer - Pointer to an integer used by the api to update the
 *                                 current displayable buffer.
 * @member rendererFbId          - Pointer to an integer used as the value of the FB_ID
 *                                 property for a plane related to he CRTC during the atomic
 *                                 modeset operation.
 * @member rendererData          - Pointer to an optional address. This address may be the
 *                                 address of a struct. Reference/Address passed depends on
 *                                 external renderer function.
 * @member atomicRequest         - Pointer to a KMS atomic request instance.
 * @member display               - Stores critical information about the display.
 */
struct kmr_drm_node
{
	struct cando_log_error_struct err;
#ifdef INCLUDE_LIBSEAT
	struct kmr_session            *session;
#endif /* INCLUDE_LIBSEAT */
	int                           kmsfd;
	uint16_t                      deviceCap;
	kmr_drm_node_renderer_impl    renderer;
	volatile bool                 *rendererRunning;
	unsigned int                  *rendererCurrentBuffer;
	int                           *rendererFbId;
	void                          *rendererData;
	drmModeAtomicReq              *atomicRequest;
	struct kmr_drm_node_display   display;
};

/***********************************************
 * End of global to file enum macros & structs *
 ***********************************************/


/******************************************
 * Start of kmr_drm_node_create functions *
 *****************************************/

static void
destroy_udev (struct udev *udev,
              struct udev_enumerate *udevEnum)
{
	if (udevEnum)
		udev_enumerate_unref(udevEnum);
	if (udev)
		udev_unref(udev);
}


static int
setup_atomic_modeset (struct kmr_drm_node *drmNode)
{
	drm_magic_t magic;

	bool supported = false;

	uint64_t capabilites = 0;

	int err = 0, kmsfd = drmNode->kmsfd;

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
		cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
		                  "drmGetMagic: KMS device '(fd: %d)' " \
		                  "could not become master", kmsfd);
		return -1;
	}

	err = drmAuthMagic(kmsfd, magic);
	if (err < 0) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
		                  "drmAuthMagic: KMS device '(fd: %d)' " \
		                  "could not become master", kmsfd);
		return -1;
	}

	/*
	 * Tell DRM core to expose atomic properties to userspace. This also enables
	 * DRM_CLIENT_CAP_UNIVERSAL_PLANES and DRM_CLIENT_CAP_ASPECT_RATIO.
	 * DRM_CLIENT_CAP_UNIVERSAL_PLANES - Tell DRM core to expose all planes (overlay, primary, and cursor) to userspace.
	 * DRM_CLIENT_CAP_ASPECT_RATIO     - Tells DRM core to provide aspect ratio information in modes
	 */
	err = drmSetClientCap(kmsfd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
	if (err < 0) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
		                  "drmSetClientCap: Failed to set universal " \
		                  "planes capability for KMS device '(fd: %d)'", kmsfd);
		return -1;
	}

	/*
	 * Tell DRM core that we're going to use the KMS atomic API. It's supposed
	 * to set the DRM_CLIENT_CAP_UNIVERSAL_PLANES automatically.
	 */
	err = drmSetClientCap(kmsfd, DRM_CLIENT_CAP_ATOMIC, 1);
	if (err < 0) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
		                  "drmSetClientCap: Failed to set KMS atomic " \
		                  "capability for KMS device '(fd: %d)'", kmsfd);
		return -1;
	}

	drmNode->deviceCap |= (CAP_UNIVERSAL_PLANES | CAP_ATOMIC);

	err = drmGetCap(kmsfd, DRM_CAP_ADDFB2_MODIFIERS, &capabilites);
	supported = (err == 0 && capabilites != 0);
	drmNode->deviceCap |= supported ? CAP_ADDFB2_MODIFIERS : 0;
	cando_log(CANDO_LOG_INFO, "device %s framebuffer modifiers", \
	          supported ? "supports" : "does not support");

	capabilites=0;
	err = drmGetCap(kmsfd, DRM_CAP_TIMESTAMP_MONOTONIC, &capabilites);
	supported = (err == 0 && capabilites != 0);
	drmNode->deviceCap |= supported ? CAP_TIMESTAMP_MONOTONIC : 0;
	cando_log(CANDO_LOG_INFO, "device %s clock monotonic timestamps", \
	          supported ? "supports" : "does not support");

	capabilites=0;
	err = drmGetCap(kmsfd, DRM_CAP_CRTC_IN_VBLANK_EVENT, &capabilites);
	supported = (err == 0 && capabilites != 0);
	drmNode->deviceCap |= supported ? CAP_CRTC_IN_VBLANK_EVENT : 0;
	cando_log(CANDO_LOG_INFO, "device %s atomic KMS", \
	          supported ? "supports" : "does not support");

	capabilites=0;
	err = drmGetCap(kmsfd, DRM_CAP_DUMB_BUFFER, &capabilites);
	supported = (err == 0 && capabilites != 0);
	drmNode->deviceCap |= supported ? CAP_DUMB_BUFFER : 0;
	cando_log(CANDO_LOG_INFO, "device %s dumb bufffers", \
	          supported ? "supports" : "does not support");

	return 0;
}


static int
open_drm_node (struct kmr_drm_node *drmNode,
               const struct kmr_drm_node_create_info CANDO_UNUSED *nodeInfo,
               const char *deviceNode)
{
#ifdef INCLUDE_LIBSEAT
	drmNode->kmsfd = kmr_session_take_control_of_device(nodeInfo->session, deviceNode);
#else
	drmNode->kmsfd = open(deviceNode, O_RDWR|O_CLOEXEC, 0);
#endif /* INCLUDE_LIBSEAT */
	if (drmNode->kmsfd < 0) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
		                  "open('%s'): %s", deviceNode,
		                  strerror(errno));
		return -1;
	}

	return 0;
}


static int
open_drm_node_udev (struct kmr_drm_node *drmNode,
                    const struct kmr_drm_node_create_info *nodeInfo)
{
	int err = -1;

	const char *devNode = NULL;

	struct udev *udev = NULL;
	struct udev_enumerate *udevEnum = NULL;
	struct udev_list_entry *entry = NULL;
	struct udev_device *device = NULL;

	udev = udev_new();
	if (!udev) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
		                  "udev_new: failed to create udev context.");
		return -1;
	}

	udevEnum = udev_enumerate_new(udev);
	if (!udevEnum) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
		                  "udev_enumerate_new: failed");
		destroy_udev(udev, udevEnum);
		return -1;
	}

	udev_enumerate_add_match_subsystem(udevEnum, "drm");
	udev_enumerate_add_match_sysname(udevEnum, DRM_PRIMARY_MINOR_NAME "[0-9]*");

	err = udev_enumerate_scan_devices(udevEnum);
	if (err != 0) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
		                  "udev_enumerate_scan_devices: failed");
		destroy_udev(udev, udevEnum);
		return -1;
	}

	udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(udevEnum)) {
		device = udev_device_new_from_syspath(udev, udev_list_entry_get_name(entry));
		if (!device)
			continue;

		devNode = udev_device_get_devnode(device);
		if (!devNode) {
			cando_log(CANDO_LOG_WARNING, \
			          "udev_device_get_devnode: unable " \
			          "to acquire KMS node\n");
			udev_device_unref(device); device = NULL;
			continue;
		}

		err = open_drm_node(drmNode, nodeInfo, devNode);
		if (err == -1) {
			udev_device_unref(device); device = NULL;
			continue;
		}

		if (!(drmNode->deviceCap & CAP_CRTC_IN_VBLANK_EVENT)) {
			udev_device_unref(device); device = NULL;
			continue;
		}

		if (!(drmNode->deviceCap & CAP_DUMB_BUFFER)) {
			udev_device_unref(device); device = NULL;
			continue;
		}

		cando_log(CANDO_LOG_SUCCESS,
		          "Opened KMS node '%s' associated fd is %d",
		          devNode, drmNode->kmsfd);

		udev_device_unref(device);
		udev_enumerate_unref(udevEnum);
		udev_unref(udev);
	}

	return 0;
}


struct kmr_drm_node *
kmr_drm_node_create (const void *_nodeInfo)
{
	int err = 0;

	struct kmr_drm_node *drmNode = NULL;

	const struct kmr_drm_node_create_info *nodeInfo = _nodeInfo;

	if (!nodeInfo) {
		cando_log_err("Incorrect data passed\n");
		return NULL;
	}

	drmNode = mmap(NULL,
	               sizeof(struct kmr_drm_node),
	               PROT_READ|PROT_WRITE,
	               MAP_PRIVATE|MAP_ANONYMOUS,
	               -1, 0);
	if (!drmNode) {
		cando_log_err("[x] mmap: %s", strerror(errno));
		return NULL;
	}

	err = (nodeInfo->kmsNode) ? \
	open_drm_node(drmNode, nodeInfo, nodeInfo->kmsNode) : \
	open_drm_node_udev(drmNode, nodeInfo);

	if (err == -1) {
		cando_log_err("%s\n", cando_log_get_error(drmNode));
		kmr_drm_node_destroy(drmNode);
		return NULL;
	}

	err = setup_atomic_modeset(drmNode);
	if (err == -1) {
		cando_log_err("%s\n", cando_log_get_error(drmNode));
		kmr_drm_node_destroy(drmNode);
		return NULL;
	}

#ifdef INCLUDE_LIBSEAT
	drmNode->session = nodeInfo->session;
#endif /* INCLUDE_LIBSEAT */

	err = CANDO_PAGE_SET_READ(drmNode, sizeof(struct kmr_drm_node));
	if (err == -1) {
		cando_log_err("mprotect: %s\n", strerror(errno));
		kmr_drm_node_destroy(drmNode);
		return NULL;
	}

	return drmNode;
}

/****************************************
 * End of kmr_drm_node_create functions *
 ****************************************/


/***************************************
 * Start of kmr_drm_node_set functions *
 ***************************************/

struct _display
{
        int              planesCount;
	drmModePlane     **planes;
	drmModeRes       *drmResources;
	drmModePlaneRes  *drmPlaneResources;
	drmModeConnector *connector;
	drmModeEncoder   *encoder;
	drmModeCrtc      *crtc;
	drmModePlane     *plane;
};


void
display_destroy (struct kmr_drm_node *drmNode,
                 struct _display *display)
{
	int p;

	if (drmNode->display.modeData.id)
		drmModeDestroyPropertyBlob(drmNode->kmsfd, drmNode->display.modeData.id);
	if (display->crtc)
		drmModeFreeCrtc(display->crtc);
	if (display->encoder)
		drmModeFreeEncoder(display->encoder);
	if (display->connector)
		drmModeFreeConnector(display->connector);
	for (p = 0; p < display->planesCount; p++) {
		if (display->planes[p])
			drmModeFreePlane(display->planes[p]);
	}
	if (display->drmPlaneResources)
		drmModeFreePlaneResources(display->drmPlaneResources);
	if (display->drmResources)
		drmModeFreeResources(display->drmResources);
}


void
mode_prop_destroy (drmModePropertyRes *propData,
                   drmModeObjectProperties *props)
{
	if (propData)
		drmModeFreeProperty(propData);
	if (props)
		drmModeFreeObjectProperties(props);
}


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
	unsigned int p;

	char typeStr[32];

	uint16_t propsDataCount = 0;

	drmModePropertyRes *propData = NULL;
	drmModeObjectProperties *props = NULL;

	switch(type) {
		case DRM_MODE_OBJECT_CONNECTOR:
			strncpy(typeStr, "connector", sizeof(typeStr));
			propsDataCount = KMR_KMS_NODE_CONNECTOR_PROP__COUNT;
			break;
		case DRM_MODE_OBJECT_PLANE:
			strncpy(typeStr, "plane", sizeof(typeStr));
			propsDataCount = KMR_KMS_NODE_PLANE_PROP__COUNT;
			break;
		case DRM_MODE_OBJECT_CRTC:
			strncpy(typeStr, "CRTC", sizeof(typeStr));
			propsDataCount = KMR_KMS_NODE_CRTC_PROP__COUNT;
			break;
		default:
			strncpy(typeStr, "unknown type", sizeof(typeStr));
			break;
	}

	props = drmModeObjectGetProperties(fd, obj->id, type);
	if (!props) {
		cando_log(CANDO_LOG_DANGER,
		          "cannot get %s %d properties: %s",
		          typeStr, obj->id, strerror(errno));
		return -1;
	}

	obj->propsDataCount = propsDataCount;

	for (p = 0; p < obj->propsDataCount; p++) {
		propData = drmModeGetProperty(fd, props->props[p]);
		if (!propData) {
			cando_log(CANDO_LOG_DANGER, "drmModeGetProperty: failed to get property data.");
			mode_prop_destroy(propData, props);
			return -1;
		}

		switch (type) {
			case DRM_MODE_OBJECT_CONNECTOR:
				if (!strncmp(propData->name, "CRTC_ID", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_CONNECTOR_PROP_CRTC_ID].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_CONNECTOR_PROP_CRTC_ID].value = props->prop_values[p];
					break;
				}

				break;
			case DRM_MODE_OBJECT_PLANE:
				if (!strncmp(propData->name, "FB_ID", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_FB_ID].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_FB_ID].value = props->prop_values[p];
					break;
				}

				if (!strncmp(propData->name, "CRTC_ID", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_ID].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_ID].value = props->prop_values[p];
					break;
				}

				if (!strncmp(propData->name, "SRC_X", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_X].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_X].value = props->prop_values[p];
					break;
				}

				if (!strncmp(propData->name, "SRC_Y", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_Y].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_Y].value = props->prop_values[p];
					break;
				}

				if (!strncmp(propData->name, "SRC_W", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_W].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_W].value = props->prop_values[p];
					break;
				}

				if (!strncmp(propData->name, "SRC_H", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_H].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_SRC_H].value = props->prop_values[p];
					break;
				}

				if (!strncmp(propData->name, "CRTC_X", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_X].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_X].value = props->prop_values[p];
					break;
				}

				if (!strncmp(propData->name, "CRTC_Y", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_Y].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_Y].value = props->prop_values[p];
					break;
				}

				if (!strncmp(propData->name, "CRTC_W", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_W].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_W].value = props->prop_values[p];
					break;
				}

				if (!strncmp(propData->name, "CRTC_H", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_H].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_PLANE_PROP_CRTC_H].value = props->prop_values[p];
					break;
				}

				break;

			case DRM_MODE_OBJECT_CRTC:
				if (!strncmp(propData->name, "MODE_ID", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_CRTC_PROP_MODE_ID].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_CRTC_PROP_MODE_ID].value = props->prop_values[p];
					break;
				}

				if (!strncmp(propData->name, "ACTIVE", DRM_PROP_NAME_LEN)) {
					obj->propsData[KMR_KMS_NODE_CRTC_PROP_ACTIVE].id = propData->prop_id;
					obj->propsData[KMR_KMS_NODE_CRTC_PROP_ACTIVE].value = props->prop_values[p];
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
}


static drmModeConnector *
drm_node_get_connector (int kmsfd, uint32_t connectorID)
{
	drmModeConnector *connector = NULL;

	connector = drmModeGetConnector(kmsfd, connectorID);
	if (!connector) {
		cando_log(CANDO_LOG_WARNING, "drmModeGetConnector: Failed to get connector");
		return NULL;
	}

	/* check if a monitor is connected */
	if (connector->encoder_id == 0 || \
	    connector->connection != DRM_MODE_CONNECTED)
	{
		cando_log(CANDO_LOG_INFO,
		          "[CONNECTOR:%" PRIu32 "]: no encoder "
		          "or not connected to display",
		          connector->connector_id);
		drmModeFreeConnector(connector);
		return NULL;
	}

	return connector;
}


static drmModeEncoder *
drm_node_get_encoder (struct kmr_drm_node *drmNode,
                      uint32_t encoderID)
{
	drmModeEncoder *encoder = NULL;

	encoder = drmModeGetEncoder(drmNode->kmsfd, encoderID);
	if (!encoder) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
		                  "drmModeGetEncoder: Failed to get encoder");
		return NULL;
	}

	if (encoder->crtc_id == 0) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
		                  "[ENCODER:%" PRIu32 "]: no CRTC",
		                  encoder->encoder_id);
		drmModeFreeEncoder(encoder);
		return NULL;
	}

	return encoder;
}


static drmModeCrtc *
drm_node_get_crtc (struct kmr_drm_node *drmNode,
                   uint32_t crtcID)
{
	drmModeCrtc *crtc = NULL;

	crtc = drmModeGetCrtc(drmNode->kmsfd, crtcID);
	if (!crtc) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
		                  "drmModeGetCrtc: Failed to get crtc KMS object");
		return NULL;
	}

	/* Ensure the CRTC is active. */
	if (crtc->buffer_id == 0) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
		                  "[CRTC:%" PRIu32 "]: not active",
		                  crtc->crtc_id);
		drmModeFreeCrtc(crtc);
		return NULL;
	}

	return crtc;
}


int
kmr_drm_node_set_display (struct kmr_drm_node *drmNode,
                          const void CANDO_UNUSED *_displayInfo)
{
	int p, e, c, conn, err = -1;

	struct _display display;

	if (!drmNode) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	memset(&display, 0, sizeof(struct _display));

	/* Query for connector->encoder->crtc KMS objecs */
	display.drmResources = drmModeGetResources(drmNode->kmsfd);
	if (!(display.drmResources)) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
		                  "Couldn't get card resources from KMS fd '%d'",
		                  drmNode->kmsfd);
		display_destroy(drmNode, &display);
		return -1;
	}

	/* Query for plane KMS objecs */
	display.drmPlaneResources = drmModeGetPlaneResources(drmNode->kmsfd);
	if (!(display.drmPlaneResources)) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
		                  "KMS fd '%d' has no planes",
		                  drmNode->kmsfd);
		display_destroy(drmNode, &display);
		return -1;
	}

	/* Check if some form of a display output chain exist */
	if (display.drmResources->count_crtcs       <= 0 ||
	    display.drmResources->count_connectors  <= 0 ||
	    display.drmResources->count_encoders    <= 0 ||
	    display.drmPlaneResources->count_planes <= 0)
	{
		cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
		                  "KMS fd '%d' has no way of creating a "
		                  "display output chain", drmNode->kmsfd);
		display_destroy(drmNode, &display);
		return -1;
	}

	/* Query KMS device plane info */
	display.planesCount = display.drmPlaneResources->count_planes;
	display.planes = alloca(display.planesCount * sizeof(drmModePlane));
	memset(display.planes, 0, display.planesCount * sizeof(drmModePlane));

	for (p = 0; p < display.planesCount; p++) {

		display.planes[p] = drmModeGetPlane(drmNode->kmsfd,
			display.drmPlaneResources->planes[p]);

		if (!(display.planes[p])) {
			cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
			                  "drmModeGetPlane: Failed to get plane");
			display_destroy(drmNode, &display);
			return -1;
		}
	}

	/*
	 * Go through connectors one by one and try to find a usable output chain.
	 * OUTPUT CHAIN: connector->encoder->crtc->plane
	 * @encoder - (Deprecated) Takes pixel data from a crtc and converts it to an output that
	 *            the connector can understand. This is a Deprecated kms object
	 */
	for (conn = 0; conn < display.drmResources->count_connectors; conn++) {

		display.connector = drm_node_get_connector(drmNode->kmsfd,
			display.drmResources->connectors[conn]);

		if (!(display.connector))
			continue;

		/* Find the encoder (a deprecated KMS object) for this connector. */
		for (e = 0; e < display.drmResources->count_encoders; e++)
		{
			if (display.drmResources->encoders[e] == display.connector->encoder_id)
			{
				display.encoder = drm_node_get_encoder(drmNode,
					display.drmResources->encoders[e]);

				if (!(display.encoder))
				{
					display_destroy(drmNode, &display);
					return -1;
				}
			}
		}

		/* Find CRTC associated with encoder */
		for (c = 0; c < display.drmResources->count_crtcs; c++)
		{
			if (display.drmResources->crtcs[c] == display.encoder->crtc_id)
			{
				display.crtc = drm_node_get_crtc(drmNode,
					display.drmResources->crtcs[c]);

				if (!(display.crtc))
				{
					display_destroy(drmNode, &display);
					return -1;
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
		for (p = 0; p < (int) display.drmPlaneResources->count_planes; p++)
		{
			if (display.planes[p]->crtc_id == display.crtc->crtc_id && \
			    display.planes[p]->fb_id == display.crtc->buffer_id)
			{
				display.plane = display.planes[p];
				break;
			}
		}

		cando_log(CANDO_LOG_SUCCESS, "Successfully found a display output chain");

		/* Stores mode id given to one of the displays resolution + refresh */
		memcpy(&(drmNode->display.modeData.modeInfo),
		       &(display.connector->modes[0]),
		       sizeof(drmModeModeInfo));

		err = drmModeCreatePropertyBlob(drmNode->kmsfd,
		                                &(display.connector->modes[0]),
					        sizeof(display.connector->modes[0]),
					        &(drmNode->display.modeData.id));
		if (err != 0) {
			cando_log_set_err(drmNode, CANDO_LOG_ERR_UNCOMMON,
			                  "drmModeCreatePropertyBlob: couldn't create a blob property");
			display_destroy(drmNode, &display);
			return -1;
		}

		err = CANDO_PAGE_SET_WRITE(&(drmNode->display), sizeof(drmNode->display));
		if (err == -1) {
			cando_log_set_err(drmNode, errno, "mprotect: %s", strerror(errno));
			display_destroy(drmNode, &display);
		}

		drmNode->display.connector.id = display.connector->connector_id;
		drmNode->display.crtc.id = display.crtc->crtc_id;
		drmNode->display.plane.id = display.plane->plane_id;
		drmNode->display.width = display.connector->modes[0].hdisplay;
		drmNode->display.height = display.connector->modes[0].vdisplay;
		drmNode->display.presClock = \
			(drmNode->deviceCap & CAP_TIMESTAMP_MONOTONIC) ? \
			CLOCK_MONOTONIC : CLOCK_REALTIME;

		/* Release memory we no longer require */
		display_destroy(drmNode, &display);

		err = acquire_kms_object_properties(drmNode->kmsfd,
		                                    &(drmNode->display.connector),
		                                    DRM_MODE_OBJECT_CONNECTOR);
		if (err == -1) {
			display_destroy(drmNode, &display);
			return -1;
		}

		err = acquire_kms_object_properties(drmNode->kmsfd,
		                                    &(drmNode->display.crtc),
		                                    DRM_MODE_OBJECT_CRTC);
		if (err == -1) {
			display_destroy(drmNode, &display);
			return -1;
		}

		err = acquire_kms_object_properties(drmNode->kmsfd,
		                                    &(drmNode->display.plane),
		                                    DRM_MODE_OBJECT_PLANE);
		if (err == -1) {
			display_destroy(drmNode, &display);
			return -1;
		}

		err = CANDO_PAGE_SET_READ(&(drmNode->display), sizeof(drmNode->display));
		if (err == -1) {
			cando_log_set_err(drmNode, errno, "mprotect: %s", strerror(errno));
			display_destroy(drmNode, &display);
		}

		return 0;
	}

	return 0;
}


int
kmr_drm_node_set_display_mode (struct kmr_drm_node *drmNode,
                               const void *_displayModeInfo)
{
	int err = -1;

	const struct kmr_drm_node_display_mode_info *displayModeInfo = _displayModeInfo;

	if (!drmNode || \
            !displayModeInfo)
	{
		cando_log_set_err(drmNode, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	switch (displayModeInfo->displayAction) {
		case KMR_DRM_NODE_DISPLAY_MODE_SET:
			err = drmModeSetCrtc(drmNode->kmsfd,
					     drmNode->display.crtc.id,
					     displayModeInfo->fbid, 0, 0,
					     &(drmNode->display.connector.id), 1,
					     &(drmNode->display.modeData.modeInfo));

			break;

		case KMR_DRM_NODE_DISPLAY_MODE_RESET:
			err = drmModeSetCrtc(drmNode->kmsfd,
					     drmNode->display.crtc.id,
					     0, 0, 0, NULL, 0, NULL);
	}

	if (err != 0) {
		cando_log_set_err(drmNode, errno, "drmModeSetCrtc: %s", strerror(errno));
		return -1;
	}

	return 0;
}

/*************************************
 * End of kmr_drm_node_set functions *
 *************************************/


/**************************************************
 * Start of kmr_drm_node_atomic_request functions *
 **************************************************/

/*
 * Here we set the values of properties (of our connector, CRTC and plane objects)
 * that we want to change in the atomic commit. These changes are temporarily stored
 * in drmModeAtomicReq *atomicRequest until DRM core receives commit.
 */
static void
modeset_atomic_prepare_commit (drmModeAtomicReq *atomicRequest,
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


int
kmr_drm_node_atomic_request (struct kmr_drm_node *drmNode,
                             const void *_atomicInfo)
{
	int err = -1;

	const struct kmr_drm_node_atomic_request_create_info *atomicInfo = _atomicInfo;

	if (!drmNode || \
	    !atomicInfo)
	{
		cando_log_set_err(drmNode, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	drmNode->atomicRequest = drmModeAtomicAlloc();
	if (!(drmNode->atomicRequest)) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_INCORRECT_DATA,
		                  "drmModeAtomicAlloc: failed to allocate space.");
		return -1;
	}

	drmNode->renderer = atomicInfo->renderer;
	drmNode->rendererRunning = atomicInfo->rendererRunning;
	drmNode->rendererCurrentBuffer = atomicInfo->rendererCurrentBuffer;
	drmNode->rendererFbId = atomicInfo->rendererFbId;
	drmNode->rendererData = atomicInfo->rendererData;

	modeset_atomic_prepare_commit(drmNode->atomicRequest,
	                              &(drmNode->display),
	                              *drmNode->rendererFbId);

	/* perform test-only atomic commit */
	err = drmModeAtomicCommit(drmNode->kmsfd,
	                          drmNode->atomicRequest,
	                          DRM_MODE_ATOMIC_TEST_ONLY | DRM_MODE_ATOMIC_ALLOW_MODESET,
	                          drmNode);
	if (err < 0) {
		cando_log_set_err(drmNode, errno, "drmModeAtomicCommit: %s", strerror(errno));
		return -1;
	}

	/* initial modeset on all outputs */
	err = drmModeAtomicCommit(drmNode->kmsfd,
	                          drmNode->atomicRequest,
	                          DRM_MODE_ATOMIC_ALLOW_MODESET | DRM_MODE_PAGE_FLIP_EVENT,
	                          drmNode);
	if (err < 0) {
		cando_log_set_err(drmNode, errno, "drmModeAtomicCommit: %s", strerror(errno));
		return -1;
	}

	return 0;
}

/************************************************
 * End of kmr_drm_node_atomic_request functions *
 ************************************************/


/****************************************************
 * Start of kmr_drm_node_handle_drm_event functions *
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

	struct kmr_drm_node *drmNode = (struct kmr_drm_node *) data;
	struct kmr_drm_node_display *display = &(drmNode->display);

	clock_gettime(display->presClock, &startTime);

	/*
	 * Application updates @rendererFbId to the next displayable
	 * GBM[GEM]/DUMP buffer and renders into that buffer.
	 * This buffer is displayed when atomic commit is performed
	 */
	drmNode->renderer(drmNode->rendererRunning,
	                  drmNode->rendererCurrentBuffer,
	                  drmNode->rendererFbId,
	                  drmNode->rendererData);

	/*
	 * Pepare properties for DRM core and temporarily store
	 * them in @rendererAtomicRequest. @rendererFbId should be
	 * an already populated buffer.
	 */
	modeset_atomic_prepare_commit(drmNode->atomicRequest,
	                              display, *drmNode->rendererFbId);

	/*
	 * Send properties to DRM core and asks the
	 * driver to perform an atomic commit. This
	 * will lead to a page-flip and the content
	 * of the @rendererFbId will be displayed.
	 */
	drmModeAtomicCommit(fd,
	                    drmNode->atomicRequest,
	                    DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK,
	                    drmNode);

	clock_gettime(display->presClock, &stopTime);

	fpsCounter++;
	finalTime += (stopTime.tv_sec - startTime.tv_sec) + \
	             (double) (stopTime.tv_nsec - startTime.tv_nsec) / 1000000000ULL;

	if (finalTime >= 1.0f) {
		cando_log(CANDO_LOG_INFO,
		          "%u fps in %lf seconds for crtc %u",
		          fpsCounter, finalTime, crtc_id);
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
kmr_drm_node_handle_drm_event (struct kmr_drm_node *drmNode,
                               const void CANDO_UNUSED *eventInfo)
{
	drmEventContext event;

	if (!drmNode) {
		cando_log_set_err(drmNode, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	event.version = 3;
	event.page_flip_handler2 = handle_page_flip_event;
	return drmHandleEvent(drmNode->kmsfd, &event);
}

/**************************************************
 * End of kmr_drm_node_handle_drm_event functions *
 **************************************************/


/*******************************************
 * Start of kmr_drm_node_destroy functions *
 *******************************************/

void
kmr_drm_node_destroy (struct kmr_drm_node *drmNode)
{
	if (!drmNode)
		return;

	if (drmNode->display.modeData.id) {
		drmModeDestroyPropertyBlob(drmNode->kmsfd,
			drmNode->display.modeData.id);
	}

	if (drmNode->atomicRequest)
		drmModeAtomicFree(drmNode->atomicRequest);

#ifdef INCLUDE_LIBSEAT
	kmr_session_release_device(drmNode->session, drmNode->kmsfd);
#else
	close(drmNode->kmsfd);
#endif /* INCLUDE_LIBSEAT */
	munmap(drmNode, sizeof(struct kmr_drm_node));
}

/*****************************************
 * End of kmr_drm_node_destroy functions *
 *****************************************/
