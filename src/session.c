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


static void handle_enable_seat(struct libseat UNUSED *seat, void *data)
{
	struct uvr_session *session = (struct uvr_session *) data;
	session->active = true;
}


static void handle_disable_seat(struct libseat *seat, void *data)
{
	struct uvr_session *session = (struct uvr_session *) data;
	session->active = false;

	libseat_disable_seat(seat);
}


static struct libseat_seat_listener seat_listener = {
	.enable_seat = handle_enable_seat,
	.disable_seat = handle_disable_seat,
};


/* Create logind session to access devices without being root */
struct uvr_session *uvr_session_create()
{
	struct uvr_session *session = NULL;

	// libseat will take care of updating the logind state if necessary
	setenv("XDG_SESSION_TYPE", "wayland", 1);

	session = calloc(1, sizeof(struct uvr_session));
	if (!session) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_uvr_session_create;
	}

	session->seat = libseat_open_seat(&seat_listener, session);
	if (!session->seat) {
		uvr_utils_log(UVR_DANGER, "[x] libseat_open_seat: Unable to create seat");
		goto exit_uvr_session_create_free_session;
	}

	while (session->active == 0) {
		if (libseat_dispatch(session->seat, -1) == -1) {
			uvr_utils_log(UVR_DANGER, "[x] libseat_dispatch: %s\n", strerror(errno));
			goto exit_uvr_session_create_close_seat;
		}
	}

	uvr_utils_log(UVR_SUCCESS, "Session created with libseat established!");

	session->seatName = (char *) libseat_seat_name(session->seat);
	if (!session->seatName) {
		uvr_utils_log(UVR_DANGER, "[x] libseat_seat_name: Unable to acquire seat name");
		goto exit_uvr_session_create_close_seat;
	}

	uvr_utils_log(UVR_INFO, "seatName: %s", session->seatName);

	return session;

exit_uvr_session_create_close_seat:
	if (session->seat)
		libseat_close_seat(session->seat);
exit_uvr_session_create_free_session:
	free(session);
exit_uvr_session_create:
	return NULL;
}


int uvr_session_take_control_of_device(struct uvr_session *session, const char *devpath)
{
	int fd, device;
	device = libseat_open_device(session->seat, devpath, &fd);
	uvr_utils_log(UVR_INFO, "libseat_open_device(seat: %p, path: %s, fd: %p) = %d",
	                        (void*) session->seat, devpath, (void*) &fd, device);
	if (device == -1) {
		uvr_utils_log(UVR_DANGER, "[x] libseat_open_device: %s\n", strerror(errno));
		return device;
	}

	return device;
}


void uvr_session_release_device(struct uvr_session *session, int fd)
{
	libseat_close_device(session->seat, fd);
	close(fd);
}


void uvr_session_destroy(struct uvr_session *session)
{
	if (session->seat)
		libseat_close_seat(session->seat);
	free(session);
}
