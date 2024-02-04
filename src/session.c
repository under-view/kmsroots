/*
 * This file is pretty much an exact copy of what's in wlroots logind session backend:
 * https://github.com/swaywm/wlroots/blob/master/backend/session/logind.c
 * and in Daniel Stone kms-quads:
 * https://gitlab.freedesktop.org/daniels/kms-quads/-/blob/master/logind.c
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <libseat.h>

#include "session.h"


/***************************************************
 * START OF kmr_session_{create,destroy} FUNCTIONS *
 ***************************************************/

static void
handle_enable_seat (struct libseat UNUSED *seat, void *data)
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


/* Create logind session to access devices without being root */
struct kmr_session *
kmr_session_create (void)
{
	struct kmr_session *session = NULL;

	// libseat will take care of updating the logind state if necessary
	setenv("XDG_SESSION_TYPE", "wayland", 1);

	session = calloc(1, sizeof(struct kmr_session));
	if (!session) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_session_create;
	}

	session->seat = libseat_open_seat(&seat_listener, session);
	if (!session->seat) {
		kmr_utils_log(KMR_DANGER, "[x] libseat_open_seat: Unable to create seat");
		goto exit_error_kmr_session_create;
	}

	while (session->active == 0) {
		if (libseat_dispatch(session->seat, -1) == -1) {
			kmr_utils_log(KMR_DANGER, "[x] libseat_dispatch: %s\n", strerror(errno));
			goto exit_error_kmr_session_create;
		}
	}

	kmr_utils_log(KMR_SUCCESS, "Session created with libseat established!");

	session->seatName = libseat_seat_name(session->seat);
	if (!session->seatName) {
		kmr_utils_log(KMR_DANGER, "[x] libseat_seat_name: Unable to acquire seat name");
		goto exit_error_kmr_session_create;
	}

	kmr_utils_log(KMR_INFO, "seatName: %s", session->seatName);

	session->seatFd = libseat_get_fd(session->seat);
	if (session->seatFd == -1) {
		kmr_utils_log(KMR_DANGER, "[x] libseat_get_fd: %s", strerror(errno));
		goto exit_error_kmr_session_create;
	}

	kmr_utils_log(KMR_INFO, "libseat instance pollable fd: %d", session->seatFd);

	return session;

exit_error_kmr_session_create:
	kmr_session_destroy(session);
	return NULL;
}


void
kmr_session_destroy (struct kmr_session *session)
{
	if (!session)
		return;

	if (session->seat)
		libseat_close_seat(session->seat);
	free(session);
}

/*************************************************
 * END OF kmr_session_{create,destroy} FUNCTIONS *
 *************************************************/


/********************************************
 * START OF kmr_session_switch_vt FUNCTIONS *
 ********************************************/

int
kmr_session_switch_vt (struct kmr_session *session, unsigned int vt)
{
	if (!session)
		return -1;

	return libseat_switch_session(session->seat, vt);
}

/******************************************
 * END OF kmr_session_switch_vt FUNCTIONS *
 ******************************************/


/*******************************************************************
 * START OF kmr_session_{take_control_of,release}_device FUNCTIONS *
 *******************************************************************/

int
kmr_session_take_control_of_device (struct kmr_session *session, const char *devpath)
{
	int fd;

	if (libseat_open_device(session->seat, devpath, &fd) == -1) {
		kmr_utils_log(KMR_DANGER, "[x] libseat_open_device: %s\n", strerror(errno));
		return -1;
	}

	kmr_utils_log(KMR_INFO, "libseat_open_device(seat: %p, path: %s, fd: %p) = %d",
	                        (void*) session->seat, devpath, (void*) &fd, fd);

	return fd;
}


void
kmr_session_release_device (struct kmr_session *session, int fd)
{
	libseat_close_device(session->seat, fd);
	close(fd);
}

/*****************************************************************
 * END OF kmr_session_{take_control_of,release}_device FUNCTIONS *
 *****************************************************************/
