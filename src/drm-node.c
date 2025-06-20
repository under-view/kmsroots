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
#include <inttypes.h>
#include <linux/major.h>
#include <libudev.h>
#include <libseat.h>

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
enum kmr_drm_node_conn_prop_type
{
	KMR_DRM_NODE_CONN_PROP_EDID                = 0,
	KMR_DRM_NODE_CONN_PROP_DPMS                = 1,
	KMR_DRM_NODE_CONN_PROP_LINK_STATUS         = 2,
	KMR_DRM_NODE_CONN_PROP_NON_DESKTOP         = 3,
	KMR_DRM_NODE_CONN_PROP_TILE                = 4,
	KMR_DRM_NODE_CONN_PROP_CRTC_ID             = 5,
	KMR_DRM_NODE_CONN_PROP_SCALING_MODE        = 6,
	KMR_DRM_NODE_CONN_PROP_UNDERSCAN           = 7,
	KMR_DRM_NODE_CONN_PROP_UNDERSCAN_HBORDER   = 8,
	KMR_DRM_NODE_CONN_PROP_UNDERSCAN_VBORDER   = 9,
	KMR_DRM_NODE_CONN_PROP_MAX_BPC             = 10,
	KMR_DRM_NODE_CONN_PROP_HDR_OUTPUT_METADATA = 11,
	KMR_DRM_NODE_CONN_PROP_VRR_CAPABLE         = 12,
	KMR_DRM_NODE_CONN_PROP__COUNT              = 13
};


enum kmr_drm_node_crtc_prop_type
{
	KMR_DRM_NODE_CRTC_PROP_ACTIVE           = 0,
	KMR_DRM_NODE_CRTC_PROP_MODE_ID          = 1,
	KMR_DRM_NODE_CRTC_PROP_OUT_FENCE_PTR    = 2,
	KMR_DRM_NODE_CRTC_PROP_VRR_ENABLED      = 3,
	KMR_DRM_NODE_CRTC_PROP_DEGAMMA_LUT      = 4,
	KMR_DRM_NODE_CRTC_PROP_DEGAMMA_LUT_SIZE = 5,
	KMR_DRM_NODE_CRTC_PROP_CTM              = 6,
	KMR_DRM_NODE_CRTC_PROP_GAMMA_LUT        = 7,
	KMR_DRM_NODE_CRTC_PROP_GAMMA_LUT_SIZE   = 8,
	KMR_DRM_NODE_CRTC_PROP__COUNT           = 9
};


enum kmr_drm_node_plane_prop_type
{
	KMR_DRM_NODE_PLANE_PROP_TYPE           = 0,
	KMR_DRM_NODE_PLANE_PROP_FB_ID          = 1,
	KMR_DRM_NODE_PLANE_PROP_IN_FENCE_FD    = 2,
	KMR_DRM_NODE_PLANE_PROP_CRTC_ID        = 3,
	KMR_DRM_NODE_PLANE_PROP_CRTC_X         = 4,
	KMR_DRM_NODE_PLANE_PROP_CRTC_Y         = 5,
	KMR_DRM_NODE_PLANE_PROP_CRTC_W         = 6,
	KMR_DRM_NODE_PLANE_PROP_CRTC_H         = 7,
	KMR_DRM_NODE_PLANE_PROP_SRC_X          = 8,
	KMR_DRM_NODE_PLANE_PROP_SRC_Y          = 9,
	KMR_DRM_NODE_PLANE_PROP_SRC_W          = 10,
	KMR_DRM_NODE_PLANE_PROP_SRC_H          = 11,
	KMR_DRM_NODE_PLANE_PROP_IN_FORMATS     = 12,
	KMR_DRM_NODE_PLANE_PROP_COLOR_ENCODING = 13,
	KMR_DRM_NODE_PLANE_PROP_COLOR_RANGE    = 14,
	KMR_DRM_NODE_PLANE_PROP_ROTATION       = 15,
	KMR_DRM_NODE_PLANE_PROP__COUNT         = 16
};


/*
 * @brief kmsroots DRM Node Device Capabilites Structure
 *
 *        For more info see https://github.com/torvalds/linux/blob/master/include/uapi/drm/drm.h#L627
 *
 * @macro CAP_ADDFB2_MODIFIERS     - If or'd in struct kmr_drm_node { @device_cap}, the driver
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
 * @macro CAP_UNIVERSAL_PLANES     -
 * @macro CAP_ATOMIC               -
 */
enum kmr_drm_node_device_cap
{
	CAP_ADDFB2_MODIFIERS     = 0x0001,
	CAP_TIMESTAMP_MONOTONIC  = 0x0002,
	CAP_CRTC_IN_VBLANK_EVENT = 0x0004,
	CAP_DUMB_BUFFER          = 0x0008,
	CAP_UNIVERSAL_PLANES     = 0x0010,
	CAP_ATOMIC               = 0x0011
};


/*
 * @brief kmsroots DRM Node Display Object Properties Data Structure.
 *
 * @member id    - Driver assigned ID of a given property
 *                 belonging to a KMS object.
 * @member value - Enum value of given KMS object property.
 *                 Can be used for instance to check if plane
 *                 object is a primary plane (DRM_PLANE_TYPE_PRIMARY).
 */
struct kmr_drm_node_display_obj_props_data
{
	uint32_t id;
	uint64_t value;
};


/*
 * @brief kmsroots DRM Node Display Object Properties Structure.
 *
 *        It stores properties of certain KMS objects
 *        (connectors, CRTC and planes) that are used
 *        in atomic modeset setup and also in atomic page-flips.
 *
 * @member id               - Driver assigned ID of the KMS object.
 * @member props_data_count - Array size of @props_data.
 * @member props_data       - Stores array of data about the properties
 *                            of a KMS object used during KMS atomic
 *                            operations.
 */
struct kmr_drm_node_display_obj_props
{
	uint32_t                                   id;
	uint16_t                                   props_data_count;
	struct kmr_drm_node_display_obj_props_data props_data[KMR_DRM_NODE_PLANE_PROP__COUNT];
};


/*
 * @brief kmsroots DRM Node Display Mode Data Structure.
 *
 * @member id        - Stores the highest mode (resolution + refresh) property id.
 *                     When we perform an atomic commit, the driver expects a CRTC
 *                     property named "MODE_ID", which points to the id given to one
 *                     of the connected display resolution & refresh rate. At the
 *                     moment the highest mode is choosen.
 * @member mode_info - Stores the highest mode data (display resolution + refresh)
 *                     associated with display.
 */
struct kmr_drm_node_display_mode_data
{
	uint32_t        id;
	drmModeModeInfo mode_info;
};


/*
 * @brief struct kmr_drm_node_display (kmsroots DRM Node Display)
 *
 * @member width      - Highest mode (display resolution) width for
 *                      @connector attached to display.
 * @member height     - Highest mode (display resolution) height for
 *                      @connector attached to display.
 * @member pres_clock - Presentation clock stores the type of clock to
 *                      utilize for fps tracking. Clock will either be
 *                      set to CLOCK_MONOTONIC or CLOCK_REALTIME depending
 *                      upon the System/DRM device capabilities. CLOCK_MONOTONIC
 *                      will return the elapsed time from system boot, can only
 *                      increase, and can't be manually modified. While CLOCK_REALTIME
 *                      will return the real time system clock as set by the user.
 *                      This clock can however be modified.
 * @member mode_data  - Stores highest mode (display resolution & refresh)
 *                      along with the modeid property used during KMS atomic
 *                      operations.
 * @member conn       - Anything that can transfer pixels in some form (i.e HDMI).
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
	uint16_t                              width;
	uint16_t                              height;
	clockid_t                             pres_clock;
	struct kmr_drm_node_display_mode_data mode_data;
	struct kmr_drm_node_display_obj_props conn;
	struct kmr_drm_node_display_obj_props crtc;
	struct kmr_drm_node_display_obj_props plane;
};


/*
 * @brief kmsroots DRM Node Renderer Structure
 *
 * @member func       - Function pointer that allows custom external renderers
 *                      to be executed by the api upon @kmsfd polled events.
 * @member running    - Pointer to a boolean that determines if a given renderer
 *                      is running and in need of stopping.
 * @member cur_buff   - Pointer to an integer used by the api to update the
 *                      current displayable buffer.
 * @member fbid       - Pointer to an integer used as the value of the FB_ID
 *                      property for a plane related to he CRTC during the atomic
 *                      modeset operation.
 * @member data       - Pointer to an optional address. This address may be the
 *                      address of a struct. Reference/Address passed depends on
 *                      external renderer function.
 * @member atomic_req - Pointer to a KMS atomic request instance.
 */
struct kmr_drm_node_renderer
{
	kmr_drm_node_renderer_impl func;
	volatile bool              *running;
	unsigned int               *cur_buff;
	int                        *fbid;
	void                       *data;
	drmModeAtomicReq           *atomic_req;
};


/*
 * @brief kmsroots DRM Node Structure
 *
 * @member err        - Stores information about the error that occurred
 *                      for the given instance and may later be retrieved
 *                      by caller.
 * @member free       - If structure allocated with calloc(3) member will be
 *                      set to true so that, we know to call free(3) when
 *                      destroying the instance.
 * @member kmsfd      - Pollable file descriptor to an open KMS (GPU) device file.
 * @member session    - Stores address of libseat session. Used when
 *                      opening and releasing a device.
 * @member device_cap - Unsigned 16-bit integer that stores what capabilites
 *                      a given device (i.e GPU) has.
 * @member renderer   - struct containing information about the extenal function
 *                      and function parameters used for rendering operations.
 * @member display    - struct containing all plane->crtc->connector
 *                      data used during KMS atomic mode setting.
 */
struct kmr_drm_node
{
	struct cando_log_error_struct err;
	bool                          free;
	int                           kmsfd;
	const void                    *session;
	uint16_t                      device_cap;
	struct kmr_drm_node_renderer  renderer;
	struct kmr_drm_node_display   display;
};

/***********************************************
 * End of global to file enum macros & structs *
 ***********************************************/


/******************************************
 * Start of kmr_drm_node_create functions *
 ******************************************/

static void
p_destroy_udev (struct udev *udev,
                struct udev_enumerate *udev_enum)
{
	if (udev_enum)
		udev_enumerate_unref(udev_enum);
	if (udev)
		udev_unref(udev);
}


static int
p_setup_atomic_modeset (struct kmr_drm_node *drm_node)
{
	drm_magic_t magic;

	bool supported = false;

	uint64_t capabilites = 0;

	int err = 0, kmsfd = drm_node->kmsfd;

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
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
		                    "drmGetMagic: KMS device '(fd: %d)' " \
		                    "could not become master", kmsfd);
		return -1;
	}

	err = drmAuthMagic(kmsfd, magic);
	if (err < 0) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
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
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
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
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
		                    "drmSetClientCap: Failed to set KMS atomic " \
		                    "capability for KMS device '(fd: %d)'", kmsfd);
		return -1;
	}

	drm_node->device_cap |= (CAP_UNIVERSAL_PLANES | CAP_ATOMIC);

	err = drmGetCap(kmsfd, DRM_CAP_ADDFB2_MODIFIERS, &capabilites);
	supported = (err == 0 && capabilites != 0);
	drm_node->device_cap |= supported ? CAP_ADDFB2_MODIFIERS : 0;
	cando_log_info("device %s frame_buffer modifiers\n", \
	               supported ? "supports" : "does not support");

	capabilites=0;
	err = drmGetCap(kmsfd, DRM_CAP_TIMESTAMP_MONOTONIC, &capabilites);
	supported = (err == 0 && capabilites != 0);
	drm_node->device_cap |= supported ? CAP_TIMESTAMP_MONOTONIC : 0;
	cando_log_info("device %s clock monotonic timestamps\n", \
	               supported ? "supports" : "does not support");

	capabilites=0;
	err = drmGetCap(kmsfd, DRM_CAP_CRTC_IN_VBLANK_EVENT, &capabilites);
	supported = (err == 0 && capabilites != 0);
	drm_node->device_cap |= supported ? CAP_CRTC_IN_VBLANK_EVENT : 0;
	cando_log_info("device %s atomic KMS\n", \
	               supported ? "supports" : "does not support");

	capabilites=0;
	err = drmGetCap(kmsfd, DRM_CAP_DUMB_BUFFER, &capabilites);
	supported = (err == 0 && capabilites != 0);
	drm_node->device_cap |= supported ? CAP_DUMB_BUFFER : 0;
	cando_log_info("device %s dumb bufffers\n", \
	               supported ? "supports" : "does not support");

	return 0;
}


static int
p_open_drm_node (struct kmr_drm_node *drm_node,
                 const struct kmr_drm_node_create_info *node_info,
                 const char *dev_node)
{
	int ret = -1, fd = -1;

	ret = (node_info->session) ? \
	libseat_open_device((struct libseat*)node_info->session, dev_node, &fd) : \
	open(dev_node, O_RDWR|O_CLOEXEC, 0);
	if (ret < 0) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
		                    "open('%s'): %s", dev_node,
		                    strerror(errno));
		return -1;
	}

	drm_node->kmsfd = (node_info->session) ? fd : ret;

	return 0;
}


static int
p_open_drm_node_udev (struct kmr_drm_node *drm_node,
                      const struct kmr_drm_node_create_info *node_info)
{
	int err = -1;

	const char *dev_node = NULL;

	struct udev *udev = NULL;
	struct udev_enumerate *udev_enum = NULL;
	struct udev_list_entry *entry = NULL;
	struct udev_device *device = NULL;

	udev = udev_new();
	if (!udev) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
		                    "udev_new: failed to create udev context.");
		return -1;
	}

	udev_enum = udev_enumerate_new(udev);
	if (!udev_enum) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
		                    "udev_enumerate_new: failed");
		p_destroy_udev(udev, udev_enum);
		return -1;
	}

	udev_enumerate_add_match_subsystem(udev_enum, "drm");
	udev_enumerate_add_match_sysname(udev_enum, DRM_PRIMARY_MINOR_NAME "[0-9]*");

	err = udev_enumerate_scan_devices(udev_enum);
	if (err != 0) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
		                    "udev_enumerate_scan_devices: failed");
		p_destroy_udev(udev, udev_enum);
		return -1;
	}

	udev_list_entry_foreach(entry, udev_enumerate_get_list_entry(udev_enum)) {
		device = udev_device_new_from_syspath(udev, udev_list_entry_get_name(entry));
		if (!device)
			continue;

		dev_node = udev_device_get_devnode(device);
		if (!dev_node) {
			cando_log_warning("udev_device_get_devnode: unable " \
			                  "to acquire KMS node\n");
			udev_device_unref(device); device = NULL;
			continue;
		}

		err = p_open_drm_node(drm_node, node_info, dev_node);
		if (err == -1) {
			udev_device_unref(device); device = NULL;
			continue;
		}

		if (!(drm_node->device_cap & CAP_CRTC_IN_VBLANK_EVENT)) {
			udev_device_unref(device); device = NULL;
			continue;
		}

		if (!(drm_node->device_cap & CAP_DUMB_BUFFER)) {
			udev_device_unref(device); device = NULL;
			continue;
		}

		cando_log_success("Opened KMS node '%s' associated fd is %d",
		                  dev_node, drm_node->kmsfd);

		udev_device_unref(device);
		udev_enumerate_unref(udev_enum);
		udev_unref(udev);
	}

	return 0;
}


struct kmr_drm_node *
kmr_drm_node_create (struct kmr_drm_node *p_drm_node,
                     const void *p_node_info)
{
	int err = 0;

	struct kmr_drm_node *drm_node = p_drm_node;

	const struct kmr_drm_node_create_info *node_info = p_node_info;

	if (!node_info) {
		cando_log_error("Incorrect data passed\n");
		return NULL;
	}

	if (!drm_node) {
		drm_node = calloc(1, sizeof(struct kmr_drm_node));
		if (!drm_node) {
			cando_log_error("calloc: %s\n", strerror(errno));
			return NULL;
		}
	}

	err = (node_info->kms_node) ? \
	p_open_drm_node(drm_node, node_info, node_info->kms_node) : \
	p_open_drm_node_udev(drm_node, node_info);
	if (err == -1) {
		cando_log_error("%s\n", cando_log_get_error(drm_node));
		kmr_drm_node_destroy(drm_node);
		return NULL;
	}

	err = p_setup_atomic_modeset(drm_node);
	if (err == -1) {
		cando_log_error("%s\n", cando_log_get_error(drm_node));
		kmr_drm_node_destroy(drm_node);
		return NULL;
	}

	drm_node->session = node_info->session;

	return drm_node;
}

/****************************************
 * End of kmr_drm_node_create functions *
 ****************************************/


/***************************************
 * Start of kmr_drm_node_set functions *
 ***************************************/

struct p_display
{
        int              planes_count;
	drmModePlane     **planes;
	drmModeRes       *drm_res;
	drmModePlaneRes  *drm_plane_res;
	drmModeConnector *conn;
	drmModeEncoder   *encoder;
	drmModeCrtc      *crtc;
	drmModePlane     *plane;
};


static void
p_display_destroy (struct kmr_drm_node *drm_node,
                   struct p_display *display)
{
	int p;

	if (drm_node->display.mode_data.id)
		drmModeDestroyPropertyBlob(drm_node->kmsfd, drm_node->display.mode_data.id);
	if (display->crtc)
		drmModeFreeCrtc(display->crtc);
	if (display->encoder)
		drmModeFreeEncoder(display->encoder);
	if (display->conn)
		drmModeFreeConnector(display->conn);
	for (p = 0; p < display->planes_count; p++) {
		if (display->planes[p])
			drmModeFreePlane(display->planes[p]);
	}
	if (display->drm_plane_res)
		drmModeFreePlaneResources(display->drm_plane_res);
	if (display->drm_res)
		drmModeFreeResources(display->drm_res);
}


static void
p_mode_prop_destroy (drmModePropertyRes *prop_data,
                     drmModeObjectProperties *props)
{
	if (prop_data)
		drmModeFreeProperty(prop_data);
	if (props)
		drmModeFreeObjectProperties(props);
}


/*
 * Helper function that retrieves the properties of
 * certain CRTC, plane or connector kms object. There's
 * no garunteed for the availability of kms object properties
 * and the order which they reside in @props->props. All we
 * can do is account for as many as possible. Function will
 * only assign values and ids to properties we actually use
 * for mode setting.
 */
static int
p_acquire_kms_obj_properties (const int fd,
                              struct kmr_drm_node_display_obj_props *obj,
                              const uint32_t type)
{
	unsigned int p;

	char type_str[32];

	uint16_t props_data_count = 0;

	drmModePropertyRes *prop_data = NULL;
	drmModeObjectProperties *props = NULL;

	switch (type) {
		case DRM_MODE_OBJECT_CONNECTOR:
			strncpy(type_str, "connector", sizeof(type_str));
			props_data_count = KMR_DRM_NODE_CONN_PROP__COUNT;
			break;

		case DRM_MODE_OBJECT_PLANE:
			strncpy(type_str, "plane", sizeof(type_str));
			props_data_count = KMR_DRM_NODE_PLANE_PROP__COUNT;
			break;

		case DRM_MODE_OBJECT_CRTC:
			strncpy(type_str, "CRTC", sizeof(type_str));
			props_data_count = KMR_DRM_NODE_CRTC_PROP__COUNT;
			break;

		default:
			strncpy(type_str, "unknown type", sizeof(type_str));
			break;
	}

	props = drmModeObjectGetProperties(fd, obj->id, type);
	if (!props) {
		cando_log_error("Cannot get %s %d properties: %s",
		                type_str, obj->id, strerror(errno));
		return -1;
	}

	obj->props_data_count = props_data_count;

	for (p = 0; p < obj->props_data_count; p++) {
		prop_data = drmModeGetProperty(fd, props->props[p]);
		if (!prop_data) {
			cando_log_error("drmModeGetProperty: failed to get property data.");
			p_mode_prop_destroy(prop_data, props);
			return -1;
		}

		switch (type) {
			case DRM_MODE_OBJECT_CONNECTOR:
				if (!strncmp(prop_data->name, "CRTC_ID", DRM_PROP_NAME_LEN)) {
					obj->props_data[KMR_DRM_NODE_CONN_PROP_CRTC_ID].id = prop_data->prop_id;
					obj->props_data[KMR_DRM_NODE_CONN_PROP_CRTC_ID].value = props->prop_values[p];
					break;
				}

				break;

			case DRM_MODE_OBJECT_PLANE:
				if (!strncmp(prop_data->name, "FB_ID", DRM_PROP_NAME_LEN)) {
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_FB_ID].id = prop_data->prop_id;
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_FB_ID].value = props->prop_values[p];
					break;
				}

				if (!strncmp(prop_data->name, "CRTC_ID", DRM_PROP_NAME_LEN)) {
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_CRTC_ID].id = prop_data->prop_id;
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_CRTC_ID].value = props->prop_values[p];
					break;
				}

				if (!strncmp(prop_data->name, "SRC_X", DRM_PROP_NAME_LEN)) {
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_SRC_X].id = prop_data->prop_id;
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_SRC_X].value = props->prop_values[p];
					break;
				}

				if (!strncmp(prop_data->name, "SRC_Y", DRM_PROP_NAME_LEN)) {
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_SRC_Y].id = prop_data->prop_id;
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_SRC_Y].value = props->prop_values[p];
					break;
				}

				if (!strncmp(prop_data->name, "SRC_W", DRM_PROP_NAME_LEN)) {
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_SRC_W].id = prop_data->prop_id;
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_SRC_W].value = props->prop_values[p];
					break;
				}

				if (!strncmp(prop_data->name, "SRC_H", DRM_PROP_NAME_LEN)) {
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_SRC_H].id = prop_data->prop_id;
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_SRC_H].value = props->prop_values[p];
					break;
				}

				if (!strncmp(prop_data->name, "CRTC_X", DRM_PROP_NAME_LEN)) {
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_CRTC_X].id = prop_data->prop_id;
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_CRTC_X].value = props->prop_values[p];
					break;
				}

				if (!strncmp(prop_data->name, "CRTC_Y", DRM_PROP_NAME_LEN)) {
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_CRTC_Y].id = prop_data->prop_id;
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_CRTC_Y].value = props->prop_values[p];
					break;
				}

				if (!strncmp(prop_data->name, "CRTC_W", DRM_PROP_NAME_LEN)) {
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_CRTC_W].id = prop_data->prop_id;
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_CRTC_W].value = props->prop_values[p];
					break;
				}

				if (!strncmp(prop_data->name, "CRTC_H", DRM_PROP_NAME_LEN)) {
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_CRTC_H].id = prop_data->prop_id;
					obj->props_data[KMR_DRM_NODE_PLANE_PROP_CRTC_H].value = props->prop_values[p];
					break;
				}

				break;

			case DRM_MODE_OBJECT_CRTC:
				if (!strncmp(prop_data->name, "MODE_ID", DRM_PROP_NAME_LEN)) {
					obj->props_data[KMR_DRM_NODE_CRTC_PROP_MODE_ID].id = prop_data->prop_id;
					obj->props_data[KMR_DRM_NODE_CRTC_PROP_MODE_ID].value = props->prop_values[p];
					break;
				}

				if (!strncmp(prop_data->name, "ACTIVE", DRM_PROP_NAME_LEN)) {
					obj->props_data[KMR_DRM_NODE_CRTC_PROP_ACTIVE].id = prop_data->prop_id;
					obj->props_data[KMR_DRM_NODE_CRTC_PROP_ACTIVE].value = props->prop_values[p];
					break;
				}

				break;

			default:
				break;
		}

		drmModeFreeProperty(prop_data); prop_data = NULL;
	}

	drmModeFreeObjectProperties(props);

	return 0;
}


static drmModeConnector *
drm_node_get_connector (int kmsfd, uint32_t connector_id)
{
	drmModeConnector *connector = NULL;

	connector = drmModeGetConnector(kmsfd, connector_id);
	if (!connector) {
		cando_log_warning("drmModeGetConnector: Failed to get connector");
		return NULL;
	}

	/* check if a monitor is connected */
	if (connector->encoder_id == 0 || \
	    connector->connection != DRM_MODE_CONNECTED)
	{
		cando_log_info("[CONNECTOR:%" PRIu32 "]: no encoder "
		               "or not connected to display\n",
		               connector->connector_id);
		drmModeFreeConnector(connector);
		return NULL;
	}

	return connector;
}


static drmModeEncoder *
drm_node_get_encoder (struct kmr_drm_node *drm_node,
                      uint32_t encoderID)
{
	drmModeEncoder *encoder = NULL;

	encoder = drmModeGetEncoder(drm_node->kmsfd, encoderID);
	if (!encoder) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
		                  "drmModeGetEncoder: Failed to get encoder");
		return NULL;
	}

	if (encoder->crtc_id == 0) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
		                  "[ENCODER:%" PRIu32 "]: no CRTC",
		                  encoder->encoder_id);
		drmModeFreeEncoder(encoder);
		return NULL;
	}

	return encoder;
}


static drmModeCrtc *
drm_node_get_crtc (struct kmr_drm_node *drm_node,
                   uint32_t crtc_id)
{
	drmModeCrtc *crtc = NULL;

	crtc = drmModeGetCrtc(drm_node->kmsfd, crtc_id);
	if (!crtc) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
		                  "drmModeGetCrtc: Failed to get crtc KMS object");
		return NULL;
	}

	/* Ensure the CRTC is active. */
	if (crtc->buffer_id == 0) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
		                  "[CRTC:%" PRIu32 "]: not active",
		                  crtc->crtc_id);
		drmModeFreeCrtc(crtc);
		return NULL;
	}

	return crtc;
}


int
kmr_drm_node_set_display (struct kmr_drm_node *drm_node,
                          const void CANDO_UNUSED *p_display_info)
{
	int p, e, c, conn, err = -1;

	struct p_display display;

	if (!drm_node) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	memset(&display, 0, sizeof(struct p_display));

	/* Query for connector->encoder->crtc KMS objecs */
	display.drm_res = drmModeGetResources(drm_node->kmsfd);
	if (!(display.drm_res)) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
		                  "Couldn't get card resources from KMS fd '%d'",
		                  drm_node->kmsfd);
		p_display_destroy(drm_node, &display);
		return -1;
	}

	/* Query for plane KMS objecs */
	display.drm_plane_res = drmModeGetPlaneResources(drm_node->kmsfd);
	if (!(display.drm_plane_res)) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
		                  "KMS fd '%d' has no planes",
		                  drm_node->kmsfd);
		p_display_destroy(drm_node, &display);
		return -1;
	}

	/* Check if some form of a display output chain exist */
	if (display.drm_res->count_crtcs       <= 0 ||
	    display.drm_res->count_connectors  <= 0 ||
	    display.drm_res->count_encoders    <= 0 ||
	    display.drm_plane_res->count_planes <= 0)
	{
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
		                  "KMS fd '%d' has no way of creating a "
		                  "display output chain", drm_node->kmsfd);
		p_display_destroy(drm_node, &display);
		return -1;
	}

	/* Query KMS device plane info */
	display.planes_count = display.drm_plane_res->count_planes;
	display.planes = alloca(display.planes_count * sizeof(drmModePlane));
	memset(display.planes, 0, display.planes_count * sizeof(drmModePlane));

	for (p = 0; p < display.planes_count; p++) {

		display.planes[p] = drmModeGetPlane(drm_node->kmsfd,
			display.drm_plane_res->planes[p]);

		if (!(display.planes[p])) {
			cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
			                  "drmModeGetPlane: Failed to get plane");
			p_display_destroy(drm_node, &display);
			return -1;
		}
	}

	/*
	 * Go through connectors one by one and try to find a usable output chain.
	 * OUTPUT CHAIN: connector->encoder->crtc->plane
	 * @encoder - (Deprecated) Takes pixel data from a crtc and converts it to an output that
	 *            the connector can understand. This is a Deprecated kms object
	 */
	for (conn = 0; conn < display.drm_res->count_connectors; conn++) {

		display.conn = drm_node_get_connector(drm_node->kmsfd,
			display.drm_res->connectors[conn]);

		if (!(display.conn))
			continue;

		/* Find the encoder (a deprecated KMS object) for this connector. */
		for (e = 0; e < display.drm_res->count_encoders; e++)
		{
			if (display.drm_res->encoders[e] == display.conn->encoder_id)
			{
				display.encoder = drm_node_get_encoder(drm_node,
					display.drm_res->encoders[e]);

				if (!(display.encoder))
				{
					p_display_destroy(drm_node, &display);
					return -1;
				}
			}
		}

		/* Find CRTC associated with encoder */
		for (c = 0; c < display.drm_res->count_crtcs; c++)
		{
			if (display.drm_res->crtcs[c] == display.encoder->crtc_id)
			{
				display.crtc = drm_node_get_crtc(drm_node,
					display.drm_res->crtcs[c]);

				if (!(display.crtc))
				{
					p_display_destroy(drm_node, &display);
					return -1;
				}
			}
		}

		/*
		 * TAKEN FROM Daniel Stone (gitlab/kms-quads)
		 * The kernel doesn't directly tell us what it considers to be the
		 * single primary plane for this CRTC (i.e. what would be updated
		 * by drmModeSetCrtc), but if it's already active then we can cheat
		 * by looking for something displaying the same frame_buffer ID,
		 * since that information is duplicated.
		 */
		for (p = 0; p < (int) display.drm_plane_res->count_planes; p++)
		{
			if (display.planes[p]->crtc_id == display.crtc->crtc_id && \
			    display.planes[p]->fb_id == display.crtc->buffer_id)
			{
				display.plane = display.planes[p];
				break;
			}
		}

		/* Stores mode id given to one of the displays resolution + refresh */
		memcpy(&(drm_node->display.mode_data.mode_info),
		       &(display.conn->modes[0]),
		       sizeof(drmModeModeInfo));

		err = drmModeCreatePropertyBlob(drm_node->kmsfd,
		                                &(display.conn->modes[0]),
					        sizeof(display.conn->modes[0]),
					        &(drm_node->display.mode_data.id));
		if (err != 0) {
			cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
			"drmModeCreatePropertyBlob: couldn't create a blob property");
			p_display_destroy(drm_node, &display);
			return -1;
		}

		drm_node->display.conn.id = display.conn->connector_id;
		drm_node->display.crtc.id = display.crtc->crtc_id;
		drm_node->display.plane.id = display.plane->plane_id;
		drm_node->display.width = display.conn->modes[0].hdisplay;
		drm_node->display.height = display.conn->modes[0].vdisplay;
		drm_node->display.pres_clock = \
			(drm_node->device_cap & CAP_TIMESTAMP_MONOTONIC) ? \
			CLOCK_MONOTONIC : CLOCK_REALTIME;

		/* Release memory we no longer require */
		p_display_destroy(drm_node, &display);

		err = p_acquire_kms_obj_properties(drm_node->kmsfd,
		                                 &(drm_node->display.conn),
		                                 DRM_MODE_OBJECT_CONNECTOR);
		if (err == -1) {
			p_display_destroy(drm_node, &display);
			return -1;
		}

		err = p_acquire_kms_obj_properties(drm_node->kmsfd,
		                                 &(drm_node->display.crtc),
		                                 DRM_MODE_OBJECT_CRTC);
		if (err == -1) {
			p_display_destroy(drm_node, &display);
			return -1;
		}

		err = p_acquire_kms_obj_properties(drm_node->kmsfd,
		                                 &(drm_node->display.plane),
		                                 DRM_MODE_OBJECT_PLANE);
		if (err == -1) {
			p_display_destroy(drm_node, &display);
			return -1;
		}

		cando_log(CANDO_LOG_SUCCESS, "Successfully found a display output chain\n");

		return 0;
	}

	return 0;
}


int
kmr_drm_node_set_display_mode (struct kmr_drm_node *drm_node,
                               const void *p_mode_info)
{
	int err = -1;

	const struct kmr_drm_node_display_mode_info *mode_info = p_mode_info;

	if (!drm_node || \
            !mode_info)
	{
		cando_log_set_error(drm_node, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	switch (mode_info->display_action) {
		case KMR_DRM_NODE_DISPLAY_MODE_SET:
			err = drmModeSetCrtc(drm_node->kmsfd,
					     drm_node->display.crtc.id,
					     mode_info->fbid, 0, 0,
					     &(drm_node->display.conn.id), 1,
					     &(drm_node->display.mode_data.mode_info));

			break;

		case KMR_DRM_NODE_DISPLAY_MODE_RESET:
			err = drmModeSetCrtc(drm_node->kmsfd,
					     drm_node->display.crtc.id,
					     0, 0, 0, NULL, 0, NULL);
			break;

		default:
			cando_log_set_error(drm_node, CANDO_LOG_ERR_INCORRECT_DATA,
				"Passed incorrect enum kmr_drm_node_display_mode");
			return -1;
	}

	if (err != 0) {
		cando_log_set_error(drm_node, errno, "drmModeSetCrtc: %s", strerror(errno));
		return -1;
	}

	return 0;
}

/*************************************
 * End of kmr_drm_node_set functions *
 *************************************/


/***************************************
 * Start of kmr_drm_node_get functions *
 ***************************************/

int
kmr_drm_node_get_kms_fd (struct kmr_drm_node *drm_node)
{
	if (!drm_node) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return drm_node->kmsfd;
}


int
kmr_drm_node_get_display_width (struct kmr_drm_node *drm_node)
{
	if (!drm_node) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return drm_node->display.width;
}


int
kmr_drm_node_get_display_height (struct kmr_drm_node *drm_node)
{
	if (!drm_node) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	return drm_node->display.height;
}

/*************************************
 * End of kmr_drm_node_get functions *
 *************************************/


/**************************************************
 * Start of kmr_drm_node_atomic_request functions *
 **************************************************/

/*
 * Here we set the values of properties
 * (of our connector, CRTC and plane objects)
 * that we want to change in the atomic commit.
 * These changes are temporarily stored in
 * drmModeAtomicReq *atomic_req until DRM
 * core receives commit.
 */
static void
modeset_atomic_prepare_commit (drmModeAtomicReq *atomic_req,
                               struct kmr_drm_node_display *display,
                               int fbid)
{
	/* set id of the CRTC id that the connector is using */
	drmModeAtomicAddProperty(atomic_req,
	                         display->conn.id,
	                         display->conn.props_data[KMR_DRM_NODE_CONN_PROP_CRTC_ID].id,
	                         display->crtc.id);

	/*
	 * set the mode id of the CRTC; this property receives the id of a blob
	 * property that holds the struct that actually contains the mode info
	 */
	drmModeAtomicAddProperty(atomic_req,
	                         display->crtc.id,
	                         display->crtc.props_data[KMR_DRM_NODE_CRTC_PROP_MODE_ID].id,
	                         display->mode_data.id);

	/* set the CRTC object as active */
	drmModeAtomicAddProperty(atomic_req,
	                         display->crtc.id,
	                         display->crtc.props_data[KMR_DRM_NODE_CRTC_PROP_ACTIVE].id,
	                         1);

	/* set properties of the plane related to the CRTC and the frame_buffer */
	drmModeAtomicAddProperty(atomic_req,
	                         display->plane.id,
	                         display->plane.props_data[KMR_DRM_NODE_PLANE_PROP_FB_ID].id,
	                         fbid);

	drmModeAtomicAddProperty(atomic_req,
	                         display->plane.id,
	                         display->plane.props_data[KMR_DRM_NODE_PLANE_PROP_CRTC_ID].id,
	                         display->crtc.id);

	drmModeAtomicAddProperty(atomic_req,
	                         display->plane.id,
	                         display->plane.props_data[KMR_DRM_NODE_PLANE_PROP_SRC_X].id,
	                         0);

	drmModeAtomicAddProperty(atomic_req,
	                         display->plane.id,
	                         display->plane.props_data[KMR_DRM_NODE_PLANE_PROP_SRC_Y].id,
	                         0);

	drmModeAtomicAddProperty(atomic_req,
	                         display->plane.id,
	                         display->plane.props_data[KMR_DRM_NODE_PLANE_PROP_SRC_W].id,
	                         (display->width << 16));

	drmModeAtomicAddProperty(atomic_req,
	                         display->plane.id,
	                         display->plane.props_data[KMR_DRM_NODE_PLANE_PROP_SRC_H].id,
	                         (display->height << 16));

	drmModeAtomicAddProperty(atomic_req,
	                         display->plane.id,
	                         display->plane.props_data[KMR_DRM_NODE_PLANE_PROP_CRTC_X].id,
	                         0);

	drmModeAtomicAddProperty(atomic_req,
	                         display->plane.id,
	                         display->plane.props_data[KMR_DRM_NODE_PLANE_PROP_CRTC_Y].id,
	                         0);

	drmModeAtomicAddProperty(atomic_req,
	                         display->plane.id,
	                         display->plane.props_data[KMR_DRM_NODE_PLANE_PROP_CRTC_W].id,
	                         display->width);

	drmModeAtomicAddProperty(atomic_req,
	                         display->plane.id,
	                         display->plane.props_data[KMR_DRM_NODE_PLANE_PROP_CRTC_H].id,
	                         display->height);
}


int
kmr_drm_node_atomic_request (struct kmr_drm_node *drm_node,
                             const void *p_atomic_info)
{
	int err = -1;

	const struct kmr_drm_node_atomic_req_info *atomic_info = p_atomic_info;

	if (!drm_node || \
	    !atomic_info)
	{
		cando_log_set_error(drm_node, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	drm_node->renderer.atomic_req = drmModeAtomicAlloc();
	if (!(drm_node->renderer.atomic_req)) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_UNCOMMON,
                "drmModeAtomicAlloc: failed to allocate space.");
		return -1;
	}

	drm_node->renderer.func = atomic_info->renderer;
	drm_node->renderer.running = atomic_info->renderer_running;
	drm_node->renderer.cur_buff = atomic_info->renderer_cur_buff;
	drm_node->renderer.fbid = atomic_info->renderer_fbid;
	drm_node->renderer.data = atomic_info->renderer_data;

	modeset_atomic_prepare_commit(drm_node->renderer.atomic_req,
	                              &(drm_node->display),
	                              *drm_node->renderer.fbid);

	/* perform test-only atomic commit */
	err = drmModeAtomicCommit(drm_node->kmsfd,
	                          drm_node->renderer.atomic_req,
	                          DRM_MODE_ATOMIC_TEST_ONLY | DRM_MODE_ATOMIC_ALLOW_MODESET,
	                          drm_node);
	if (err < 0) {
		cando_log_set_error(drm_node, errno, "drmModeAtomicCommit: %s", strerror(errno));
		return -1;
	}

	/* initial modeset on all outputs */
	err = drmModeAtomicCommit(drm_node->kmsfd,
	                          drm_node->renderer.atomic_req,
	                          DRM_MODE_ATOMIC_ALLOW_MODESET | DRM_MODE_PAGE_FLIP_EVENT,
	                          drm_node);
	if (err < 0) {
		cando_log_set_error(drm_node, errno, "drmModeAtomicCommit: %s", strerror(errno));
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
                        unsigned int CANDO_UNUSED seq,
                        unsigned int CANDO_UNUSED tv_sec,
                        unsigned int CANDO_UNUSED tv_usec,
                        unsigned int CANDO_UNUSED crtc_id,
                        void *data)
{
	static double final_time = 0;
	static uint16_t fps_counter = 0;

	struct timespec start_time, stop_time;

	struct kmr_drm_node *drm_node = (struct kmr_drm_node *) data;
	struct kmr_drm_node_display *display = &(drm_node->display);
	struct kmr_drm_node_renderer *renderer = &(drm_node->renderer);

	clock_gettime(display->pres_clock, &start_time);

	/*
	 * Application updates @renderer_fbid to the next displayable
	 * GBM[GEM]/DUMP buffer and renders into that buffer.
	 * This buffer is displayed when atomic commit is performed
	 */
	renderer->func(renderer->running,
	               renderer->cur_buff,
	               renderer->fbid,
	               renderer->data);

	/*
	 * Pepare properties for DRM core and temporarily store
	 * them in @rendererAtomicRequest. @renderer_fbid should be
	 * an already populated buffer.
	 */
	modeset_atomic_prepare_commit(renderer->atomic_req,
	                              display, *(renderer->fbid));

	/*
	 * Send properties to DRM core and asks the
	 * driver to perform an atomic commit. This
	 * will lead to a page-flip and the content
	 * of the @renderer_fbid will be displayed.
	 */
	drmModeAtomicCommit(fd,
	                    renderer->atomic_req,
	                    DRM_MODE_PAGE_FLIP_EVENT | DRM_MODE_ATOMIC_NONBLOCK,
	                    drm_node);

	clock_gettime(display->pres_clock, &stop_time);

	fps_counter++;
	final_time += (stop_time.tv_sec - start_time.tv_sec) + \
		(double) (stop_time.tv_nsec - start_time.tv_nsec) / 1000000000ULL;

	if (final_time >= 1.0f) {
		cando_log_info("%u fps in %lf seconds for crtc %u",
		               fps_counter, final_time, crtc_id);
		final_time = 0; fps_counter = 0;
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
kmr_drm_node_handle_drm_event (struct kmr_drm_node *drm_node,
                               const void CANDO_UNUSED *event_info)
{
	drmEventContext event;

	if (!drm_node) {
		cando_log_set_error(drm_node, CANDO_LOG_ERR_INCORRECT_DATA, "");
		return -1;
	}

	event.version = 3;
	event.page_flip_handler2 = handle_page_flip_event;
	return drmHandleEvent(drm_node->kmsfd, &event);
}

/**************************************************
 * End of kmr_drm_node_handle_drm_event functions *
 **************************************************/


/*******************************************
 * Start of kmr_drm_node_destroy functions *
 *******************************************/

void
kmr_drm_node_destroy (struct kmr_drm_node *drm_node)
{
	if (!drm_node)
		return;

	if (drm_node->display.mode_data.id) {
		drmModeDestroyPropertyBlob(drm_node->kmsfd,
			drm_node->display.mode_data.id);
	}

	if (drm_node->renderer.atomic_req)
		drmModeAtomicFree(drm_node->renderer.atomic_req);

	if (drm_node->session)
		libseat_close_device((struct libseat*)drm_node->session, drm_node->kmsfd);
	close(drm_node->kmsfd);

	if (drm_node->free) {
		free(drm_node);
	} else {
		memset(drm_node, 0, sizeof(struct kmr_drm_node));
	}
}

/*****************************************
 * End of kmr_drm_node_destroy functions *
 *****************************************/
