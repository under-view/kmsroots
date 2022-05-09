#ifndef UVR_KMS_H
#define UVR_KMS_H

#include "common.h"
#include "utils.h"

#ifdef INCLUDE_SDBUS
#include "sd-dbus.h"
#endif

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


/*
 * struct uvrkms_node_create_info (Underview Renderer KMS Node Create Information)
 *
 * members:
 * @uvrsd_session - Address of struct uvrsd_session. Which members are used to communicate
 *                  with systemd-logind via D-Bus systemd-logind interface. Needed by
 *                  kms_node_create to acquire and taken control of a device without the
 *                  need of being root.
 * @use_logind    - Not redundant. If one include -Dsd-bus=yes meson option, but doesn't
 *                  want to utilize systemd-logind D-bus interface to open a GPU device set
 *                  memeber to false. Other set to true which will use systemd-logind D-bus
 *                  interface.
 * @kmsnode       - Path to character device associated with GPU. If set to NULL. List of
 *                  available kmsnode's will be queried and one will be automatically
 *                  choosen for you.
 */
struct uvrkms_node_create_info {
#ifdef INCLUDE_SDBUS
  struct uvrsd_session *uvrsd_session;
  bool use_logind;
#endif
  const char *kmsnode;
};


/*
 * uvr_kms_node_create: Function opens and if have a systemd-logind session takes control of a KMS
 *                      device node. Returned fd is exposed to all planes (overlay, primary, and cursor)
 *                      and has access to the aspect ratio information in modes in userspace. In order
 *                      to drive KMS, we need to be 'master'. Function fails if we aren't DRM-Master more
 *                      info here: https://en.wikipedia.org/wiki/Direct_Rendering_Manager#DRM-Master_and_DRM-Auth
 *                      So, if a graphical session is already active on the current VT function falls.
 *
 * args:
 * @uvrkms - pointer to a struct uvrkms_node_create_info used to determine what operation
 *           will happen in the function
 * return:
 *    open fd on success
 *   -1 on failure
 */
int uvr_kms_node_create(struct uvrkms_node_create_info *uvrkms);


/*
 * struct uvrkms_node_device_capabilites (Underview Renderer KMS Node Device Capabilites)
 *
 * members:
 * @ADDFB2_MODIFIERS
 * @TIMESTAMP_MONOTONIC
 *
 * For more info see https://github.com/torvalds/linux/blob/master/include/uapi/drm/drm.h#L627
 */
struct uvrkms_node_device_capabilites {
  bool ADDFB2_MODIFIERS;
  bool TIMESTAMP_MONOTONIC;
};


/*
 * uvr_kms_node_get_device_capabilities: Function takes in a DRM device fd and populates the struct uvrkms_node_device_capabilites
 *                                       to give details on what capabilites the particular kms device supports.
 *
 * args:
 * @kmsfd - Number associated with open KMS device node
 * return:
 *    struct uvrkms_node_driver_capabilites
 */
struct uvrkms_node_device_capabilites uvr_kms_node_get_device_capabilities(int kmsfd);


/*
 * struct uvrkms_display_output_chain (Underview Renderer KMS Node Display Output Chain)
 *
 * members:
 * @connector - Anything that can display pixels in some form. (i.e HDMI). Connectors can
 *              be hotplugged and unplugged at runtime
 * @encoder   - Takes pixel data from a crtc and converts it to an output that
 *              the connector can understand. This is a Deprecated kms object
 * @crtc      - Represents a part of the chip that contains a pointer to a scanout buffer.
 * @plane     - A plane represents an image source that can be blended with or overlayed on top of
 *              a CRTC during the scanout process. Planes are associated with a frame buffer to crop
 *              a portion of the image memory (source) and optionally scale it to a destination size.
 *              The result is then blended with or overlayed on top of a CRTC.
 * For more info see https://manpages.org/drm-kms/7
 */
struct uvrkms_node_display_output_chain {
  drmModeConnector *connector;
  drmModeEncoder *encoder;
  drmModeCrtc *crtc;
  drmModePlane *plane;
};


/*
 * struct uvrkms_destroy (Underview Renderer KMS Node Get display Output Chain Information)
 *
 * members:
 * @kmsfd - The file descriptor associated with open KMS device node.
 */
struct uvrkms_node_get_display_output_chain_info {
  int kmsfd;
};


/*
 * uvr_kms_node_create: Function takes in a pointer to a struct uvrkms_node_get_display_output_chain_info
 *                      and produces one connector->encoder->CRTC->plane display output chain. Populating
 *                      the members of struct uvrkms_node_display_output_chain whose information will be
 *                      later used in modesetting.
 *
 * args:
 * @uvrkms - pointer to a struct uvrkms_node_get_display_output_chain_info used to determine what operation
 *           will happen in the function
 * return:
 *    struct uvrkms_display_output_chain on success
 *    0 for all  struct uvrkms_display_output_chain members on failure
 */
struct uvrkms_node_display_output_chain uvr_kms_node_get_display_output_chain(struct uvrkms_node_get_display_output_chain_info *uvrkms);


/*
 * struct uvrkms_node_destroy (Underview Renderer KMS Node Destroy)
 *
 * members:
 * @kmsfd   - The file descriptor associated with open KMS device node to be closed.
 * @info    - Stores information about logind session bus, id, and path needed to
 *            release control of a given device.
 * @dochain - Pointer to struct uvrkms_node_display_output_chain. Stores information
 *            about KMS device node connector->encoder->crtc->plane pair
 */
struct uvrkms_node_destroy {
  int kmsfd;
  struct uvrkms_node_create_info info;
  struct uvrkms_node_display_output_chain *dochain;
};


/*
 * uvr_kms_node_destroy: Destroy any and all KMS node information
 *
 * args:
 * @uvrkms - pointer to a struct uvrkms_destroy
 */
void uvr_kms_node_destroy(struct uvrkms_node_destroy *uvrkms);

#endif
