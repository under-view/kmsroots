#ifndef KMR_SESSION_H
#define KMR_SESSION_H

#include "utils.h"

#include <stdbool.h>
#include <libseat.h>


/*
 * struct kmr_session (kmsroots Session)
 *
 * members:
 * @active   - Determines if the session is active or not
 * @seatFd   - Pollable file descriptor to a libseat seatd/systemd-logind D-Bus session
 * @seatName - Pointer to name of given seat
 * @seat     - Pointer to libseat seatd/systemd-logind D-Bus session
 */
struct kmr_session {
	bool           active;
	int            seatFd;
	const char     *seatName;
	struct libseat *seat;
};


/*
 * kmr_session_create: Create logind/seatd session to access devices without being root.
 *                     Function populates all the members of the struct kmr_session.
 *
 * parameters:
 * @session - pointer to a struct kmr_session stores information about the current session
 * returns:
 *	on success: pointer to a struct kmr_session
 *	on failure: NULL
 */
struct kmr_session *
kmr_session_create (void);


/*
 * kmr_session_destroy: Frees any allocated memory and closes FD’s (if open) created after
 *                      kmr_session_create() call.
 *
 * parameters:
 * @session - Must pass a valid pointer to a struct kmr_session
 */
void
kmr_session_destroy (struct kmr_session *session);


/*
 * kmr_session_swtch_vt: Calls libseat_switch_session() which requests that the seat switches session
 *                       to the specified session number. For seats that are VT-bound, the session number
 *                       matches the VT number, and switching session results in a VT switch.
 *
 * parameters:
 * @session - pointer to a struct kmr_session stores information about the current session
 * @vt      - Unsigned integer number of the TTY/VT (Virtual Terminal) a given seat or
 *            seatd/systemd logind session should switch to.
 * returns:
 *	on success: 0
 *	on failure: -1
 */
int
kmr_session_switch_vt (struct kmr_session *session, unsigned int vt);


/*
 * kmr_session_take_control_of_device: The TakeDevice systemd-logind D-Bus interface function allows
 *                                     session controller to get a file descriptor for a specific
 *                                     device and allows one to acquire control over the given device
 *                                     (i.e GPU,keyboard,mouse,etc). This functions returns a file
 *                                     descriptor to the device that has been acquired.
 *
 * parameters:
 * @session - pointer to a struct kmr_session stores information about the current session
 * @devpath - Path to a given character device associated with a connected device
 * returns:
 *	on success: an open file descriptor
 *	on failure: -1
 */
int
kmr_session_take_control_of_device (struct kmr_session *session, const char *devpath);


/*
 * kmr_session_release_device: The ReleaseDevice systemd-logind D-Bus interface function allows for one
 *                             to release control over a given device (i.e GPU,keyboard,mouse,etc). The
 *                             function also closes the passed file descriptor.
 *
 * parameters:
 * @session - Must pass a pointer to a struct kmr_session
 * @fd      - Open file descriptor associated with a given character device file
 */
void
kmr_session_release_device (struct kmr_session *session, int fd);


#endif
