#ifndef UVR_SD_BUS_H
#define UVR_SD_BUS_H

#include "common.h"
#include "utils.h"

#include <systemd/sd-bus.h>
#include <systemd/sd-login.h>


/*
 * struct uvrsd_session (Underview Renderer Systemd Session)
 *
 * members:
 * @fd      - File descriptor associated with logind session
 * @bus     - An open D-Bus connection
 * @id      - Session id
 * @path    - Path to session object
 * @has_drm - When uvr_sd_session_take_control_of_device is invoked this member is updated.
 *            Storing a boolean indicating whether or not the current device one wants to
 *            take control over is a DRM device.
 */
struct uvrsd_session {
  int fd;
  sd_bus *bus;

  char *id;
  char *path;

  bool has_drm;
};


/*
 * uvr_sd_session_create: Create logind session to access devices without being root.
 *                        Function populates all the members of the uvrsd_session struct
 *
 * args:
 * @uvrsd - pointer to a struct uvrsd_session stores information about the current session
 * return:
 *    0 on success
 *   -1 on failure
 */
int uvr_sd_session_create(struct uvrsd_session *uvrsd);


/*
 * uvr_sd_session_destroy: Destroy logind session and free up allocated space assigning to members
 *
 * args:
 * @uvrsd - pointer to a struct uvrsd_session stores information about the current session
 */
void uvr_sd_session_destroy(struct uvrsd_session *uvrsd);

#endif
