#ifndef UVR_SESSION_H
#define UVR_SESSION_H

#include "utils.h"

#include <stdbool.h>
#include <libseat.h>


/*
 * struct uvr_session (Underview Renderer Session)
 *
 * members:
 * @active   - Determines if the session is active or not
 * @seatFd   - Pollable file descriptor to a libseat seatd/systemd-logind D-Bus session
 * @seatName - Pointer to name of given seat
 * @seat     - Pointer to libseat seatd/systemd-logind D-Bus session
 */
struct uvr_session {
	bool           active;
	int            seatFd;
	const char     *seatName;
	struct libseat *seat;
};


/*
 * uvr_session_create: Create logind/seatd session to access devices without being root.
 *                     Function populates all the members of the uvr_session struct
 *
 * args:
 * @session - pointer to a struct uvr_session stores information about the current session
 * return:
 *	on success pointer to a struct uvr_session { members populated }
 *	on failure NULL
 */
struct uvr_session *uvr_session_create();


/*
 * uvr_session_take_control_of_device: The TakeDevice systemd-logind D-Bus interface function allows
 *                                     session controller to get a file descriptor for a specific
 *                                     device and allows one to acquire control over the given device
 *                                     (i.e GPU,keyboard,mouse,etc). This functions returns a file
 *                                     descriptor to the device that has been acquired.
 *
 * args:
 * @session - pointer to a struct uvr_session stores information about the current session
 * @devpath - Path to a given character device associated with a connected device
 * return:
 *	on success an open file descriptor
 *	on failure -1
 */
int uvr_session_take_control_of_device(struct uvr_session *session, const char *devpath);


/*
 * uvr_session_release_device: The ReleaseDevice systemd-logind D-Bus interface function allows for one
 *                             to release control over a given device (i.e GPU,keyboard,mouse,etc). The
 *                             function also close the passed file descriptor.
 *
 * args:
 * @session - pointer to a struct uvr_session stores information about the current session
 * @fd      - file descriptor associated with a given opened character device file
 */
void uvr_session_release_device(struct uvr_session *session, int fd);


/*
 * uvr_session_destroy: Destroy logind session and free up allocated space assigning to members
 *
 * args:
 * @uvrsd - pointer to a struct uvr_session stores information about the current session
 */
void uvr_session_destroy(struct uvr_session *session);


#endif
