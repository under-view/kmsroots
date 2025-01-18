#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include <libseat.h>

#include <cando/cando.h>

#include "session.h"

/*
 * @brief struct kmr_session (kmsroots Session)
 *
 * @member err      - Stores information about the error that occured
 *                    for the given instance and may later be retrieved
 *                    by caller.
 * @member active   - Determines if the session is active or not
 * @member seatfd   - Pollable file descriptor to a libseat seatd/systemd-logind D-Bus session
 * @member seatName - Pointer to name of given seat
 * @member seat     - Pointer to libseat seatd/systemd-logind D-Bus session
 */
struct kmr_session
{
	struct cando_log_error_struct err;
	bool                          active;
	int                           seatfd;
	const char                    *seatName;
	struct libseat                *seat;
};


/*****************************************
 * Start of kmr_session_create functions *
 *****************************************/

static void
handle_enable_seat (struct libseat CANDO_UNUSED *seat, void *data)
{
	struct kmr_session *session = (struct kmr_session *) data;
	session->active = true;
}


static void
handle_disable_seat (struct libseat *seat, void *data)
{
	struct kmr_session *session = (struct kmr_session *) data;
	session->active = false;
	libseat_disable_seat(seat);
}


static struct libseat_seat_listener seat_listener = {
	.enable_seat = handle_enable_seat,
	.disable_seat = handle_disable_seat,
};


/*
 * Create logind session to access
 * devices without being root.
 */
struct kmr_session *
kmr_session_create (void)
{
	struct kmr_session *session = NULL;

	// libseat will take care of updating the logind state if necessary
	setenv("XDG_SESSION_TYPE", "wayland", 1);

	session = mmap(NULL,
	               sizeof(struct kmr_session),
	               PROT_READ|PROT_WRITE,
	               MAP_PRIVATE|MAP_ANONYMOUS,
	               -1,0);
	if (session == (void*)-1) {
		cando_log_error("mmap: %s\n", strerror(errno));
		kmr_session_destroy(session);
		return NULL;
	}

	session->seat = libseat_open_seat(&seat_listener, session);
	if (!(session->seat)) {
		cando_log_error("libseat_open_seat: Unable to create seat\n");
		kmr_session_destroy(session);
		return NULL;
	}

	while (session->active == 0) {
		if (libseat_dispatch(session->seat, -1) == -1) {
			cando_log_error("libseat_dispatch: %s\n", strerror(errno));
			kmr_session_destroy(session);
			return NULL;
		}
	}

	cando_log(CANDO_LOG_SUCCESS, "Session created with libseat established!\n");

	session->seatName = libseat_seat_name(session->seat);
	if (!(session->seatName)) {
		cando_log_error("libseat_seat_name: Unable to acquire seat name\n");
		kmr_session_destroy(session);
		return NULL;
	}

	cando_log(CANDO_LOG_INFO, "seatName: %s", session->seatName);

	session->seatfd = libseat_get_fd(session->seat);
	if (session->seatfd == -1) {
		cando_log_error("libseat_get_fd: %s\n", strerror(errno));
		kmr_session_destroy(session);
		return NULL;
	}

	cando_log(CANDO_LOG_INFO, "libseat instance pollable fd: %d\n", session->seatfd);

	return session;
}

/***************************************
 * End of kmr_session_create functions *
 ***************************************/


/********************************************
 * Start of kmr_session_switch_vt functions *
 ********************************************/

int
kmr_session_switch_vt (struct kmr_session *session,
                       const unsigned int vt)
{
	if (!session)
		return -1;

	return libseat_switch_session(session->seat, vt);
}

/******************************************
 * End of kmr_session_switch_vt functions *
 ******************************************/


/*******************************************************************
 * Start of kmr_session_{take_control_of,release}_device functions *
 *******************************************************************/

int
kmr_session_take_control_of_device (struct kmr_session *session,
                                    const char *devpath)
{
	int fd;

	if (libseat_open_device(session->seat, devpath, &fd) == -1) {
		cando_log_set_error(session, errno,
		                    "libseat_open_device: %s\n",
		                    strerror(errno));
		return -1;
	}

	cando_log(CANDO_LOG_INFO,
	          "libseat_open_device(seat: %p, path: %s, fd: %p) = %d",
	          (void*) session->seat, devpath, (void*) &fd, fd);

	return fd;
}


void
kmr_session_release_device (struct kmr_session *session,
                            const int fd)
{
	libseat_close_device(session->seat, fd);
	close(fd);
}

/*****************************************************************
 * End of kmr_session_{take_control_of,release}_device functions *
 *****************************************************************/


/******************************************
 * Start of kmr_session_destroy functions *
 ******************************************/

void
kmr_session_destroy (struct kmr_session *session)
{
	if (!session)
		return;

	if (session->seat)
		libseat_close_seat(session->seat);

	munmap(session, sizeof(struct kmr_session));
}

/****************************************
 * End of kmr_session_destroy functions *
 ****************************************/
