#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libudev.h>
#include <libinput.h>

/*
 * Leave as generic interface for creating and
 * destroying libinput interfaces.
 */
#include "input.h"

#ifdef INCLUDE_LIBSEAT
#include "session.h"
#endif

static int
open_restricted (const char *path, int UNUSED flags, void UNUSED *data)
{
	int fd;
#ifdef INCLUDE_LIBSEAT
	fd = kmr_session_take_control_of_device((struct kmr_session *) data, path);
#else
	fd = open(path, flags);
#endif
	return fd < 0 ? -errno : fd;
}


static void
close_restricted (int fd, void UNUSED *data)
{
#ifdef INCLUDE_LIBSEAT
	kmr_session_release_device((struct kmr_session *) data, fd);
#else
	close(fd);
#endif
}


static const struct libinput_interface libinput_interface_impl = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted
};


struct kmr_input *
kmr_input_create (struct kmr_input_create_info UNUSED *inputInfo)
{
	int ret = -1;
	const char *seatName = NULL;

	struct udev *udev = NULL;
	struct kmr_session *session = NULL;

	struct kmr_input *input = NULL;

	input = calloc(1, sizeof(struct kmr_input));
	if (!input) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_kmr_input_create;
	}

#ifdef INCLUDE_LIBSEAT
	session = inputInfo->session;
#endif

	udev = udev_new();
	if (!udev) {
		kmr_utils_log(KMR_DANGER, "[x] udev_new: failed to create udev context.");
		goto exit_error_kmr_input_create;
	}

	input->inputInst = libinput_udev_create_context(&libinput_interface_impl, session, udev);
	if (!input->inputInst) {
		kmr_utils_log(KMR_DANGER, "[x] libinput_udev_create_context: failed to create libinput context\n");
		goto exit_error_kmr_input_create;
	}

#ifdef INCLUDE_LIBSEAT
	seatName = inputInfo->session->seatName;
#else
	seatName = "seat0";
#endif

	ret = libinput_udev_assign_seat(input->inputInst, seatName);
	if (ret < 0) {
		kmr_utils_log(KMR_DANGER, "[x] libinput_udev_assign_seat: failed to assign udev seat to libinput instance");
		goto exit_error_kmr_input_create;
	}

	udev_unref(udev);

	input->inputfd = libinput_get_fd(input->inputInst);

	return input;

exit_error_kmr_input_create:
	if (udev)
		udev_unref(udev);
	kmr_input_destroy(input);
	return NULL;
}


void
kmr_input_destroy (struct kmr_input *input)
{
	if (!input)
		return;

	libinput_unref(input->inputInst);
	free(input);
}
