#ifndef UVR_SD_DBUS_H
#define UVR_SD_DBUS_H

#include "common.h"
#include "utils.h"

/*
 * D-Bus (Desktop Bus) daemon through the use of an IPC mechanism allows
 * for the communication of multiple processes running concurrently.
 * As long as a process is D-Bus aware it can invoke functions of
 * another process.
 *
 * systemd-logind D-Bus interface:
 * https://www.freedesktop.org/software/systemd/man/org.freedesktop.login1.html
 */
#include <systemd/sd-bus.h>
#include <systemd/sd-login.h>


/*
 * struct uvrsd_session (Underview Renderer Systemd Session)
 *
 * members:
 * @bus     - An open D-Bus connection
 * @id      - Session id
 * @path    - Path to session object
 */
struct uvrsd_session {
  sd_bus *bus;
  char *id;
  char *path;
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
 * uvr_sd_session_take_control_of_device: The TakeDevice systemd-logind D-Bus interface function allows
 *                                        session controller to get a file descriptor for a specific
 *                                        device and allows one to acquire control over the given device
 *                                        (i.e GPU,keyboard,mouse,etc). This functions returns a file
 *                                        descriptor to the device that has been acquired.
 *
 * args:
 * @uvrsd   - pointer to a struct uvrsd_session stores information about the current session
 * @devpath - Path to a given character device associated with a connected device
 * return:
 *    open fd on success
 *   -1 on failure
 */
int uvr_sd_session_take_control_of_device(struct uvrsd_session *uvrsd, const char *devpath);


/*
 * uvr_sd_session_release_device: The ReleaseDevice systemd-logind D-Bus interface function allows for one
 *                                to release control over a given device (i.e GPU,keyboard,mouse,etc). The
 *                                function also close the passed file descriptor.
 *
 * args:
 * @uvrsd - pointer to a struct uvrsd_session stores information about the current session
 * @fd    - file descriptor associated with a given opened character device file
 */
void uvr_sd_session_release_device(struct uvrsd_session *uvrsd, int fd);


/*
 * uvr_sd_session_destroy: Destroy logind session and free up allocated space assigning to members
 *
 * args:
 * @uvrsd - pointer to a struct uvrsd_session stores information about the current session
 */
void uvr_sd_session_destroy(struct uvrsd_session *uvrsd);


#endif