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
 * struct uvrkms_destroy (Underview Renderer KMS Destroy)
 *
 * members:
 * @kmsfd - The file descriptor associated with open KMS device node to be closed.
 * @info  - Stores information about logind session bus, id, and path needed to
 *          release control of a given device.
 */
struct uvrkms_destroy {
  int kmsfd;
  struct uvrkms_node_create_info info;
};


/*
 * uvr_kms_destroy: Destroy any and all KMS node information
 *
 * args:
 * @uvrkms - pointer to a struct uvrkms_destroy
 */
void uvr_kms_destroy(struct uvrkms_destroy *uvrkms);

#endif
