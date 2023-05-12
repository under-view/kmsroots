#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libudev.h>
#include <libinput.h>

#include "input.h"
#ifdef INCLUDE_LIBSEAT
#include "session.h"
#endif


static int open_restricted(const char *path, int UNUSED flags, void UNUSED *data)
{
	int fd;
#ifdef INCLUDE_LIBSEAT
	fd = uvr_session_take_control_of_device((struct uvr_session *) data, path);
#else
	fd = open(path, flags);
#endif
	return fd < 0 ? -errno : fd;
}


static void close_restricted(int fd, void UNUSED *data)
{
#ifdef INCLUDE_LIBSEAT
	uvr_session_release_device((struct uvr_session *) data, fd);
#else
	close(fd);
#endif
}


static const struct libinput_interface libinput_interface_impl = {
    .open_restricted = open_restricted,
    .close_restricted = close_restricted
};


struct uvr_input uvr_input_create(struct uvr_input_create_info UNUSED *uvrinp)
{
	struct udev *udev = NULL;
	struct libinput *input = NULL;
	struct uvr_session *session = NULL;
	int inputfd = -1;

#ifdef INCLUDE_LIBSEAT
	session = uvrinp->session;
#endif

	udev = udev_new();
	if (!udev) {
		uvr_utils_log(UVR_DANGER, "[x] udev_new: failed to create udev context.");
		goto exit_error_uvr_input_create;
	}

	input = libinput_udev_create_context(&libinput_interface_impl, session, udev);
	if (!input) {
	    uvr_utils_log(UVR_DANGER, "[x] libinput_udev_create_context: failed to create libinput context\n");
	    goto exit_error_uvr_udev_unref;
	}

	const char *seatName = NULL;
#ifdef INCLUDE_LIBSEAT
	seatName = uvrinp->session->seatName;
#else
	seatName = "seat0";
#endif

	if (libinput_udev_assign_seat(input, seatName) < 0) {
		uvr_utils_log(UVR_DANGER, "[x] libinput_udev_assign_seat: failed to assign udev seat to libinput instance");
		goto exit_error_uvr_libinput_unref;
	}
	
	udev_unref(udev);

	inputfd = libinput_get_fd(input);

	return (struct uvr_input) { .input = input, .inputfd = inputfd };

exit_error_uvr_libinput_unref:
	libinput_unref(input);
exit_error_uvr_udev_unref:
	udev_unref(udev);
exit_error_uvr_input_create:
	return (struct uvr_input) { .input = NULL, .inputfd = -1 };
}


uint64_t uvr_input_handle_input_event(struct uvr_input_handle_input_event_info *uvrinp)
{
	struct libinput *input = NULL;
	struct libinput_event *event = NULL;
	enum libinput_event_type type;

	uint64_t code = 0;

	input = uvrinp->input.input;
	libinput_dispatch(input);

	event = libinput_get_event(input);
	if (!event)
		return UINT64_MAX;

	type = libinput_event_get_type(event);

	switch (type) {
		case LIBINPUT_EVENT_KEYBOARD_KEY:
			struct libinput_event_keyboard *keyEvent = NULL;
			keyEvent = libinput_event_get_keyboard_event(event);
			code = libinput_event_keyboard_get_key(keyEvent);
			break;
		default:
			break;
	}

	libinput_event_destroy(event);
	libinput_dispatch(input);

	return code;
}


void uvr_input_destroy(struct uvr_input_destroy *uvrinp)
{
	if (uvrinp->uvr_input.input)
		libinput_unref(uvrinp->uvr_input.input);
}
