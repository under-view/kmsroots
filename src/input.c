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
	fd = kmr_session_take_control_of_device((struct kmr_session *) data, path);
#else
	fd = open(path, flags);
#endif
	return fd < 0 ? -errno : fd;
}


static void close_restricted(int fd, void UNUSED *data)
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


struct kmr_input kmr_input_create(struct kmr_input_create_info UNUSED *kmsinp)
{
	struct udev *udev = NULL;
	struct libinput *input = NULL;
	struct kmr_session *session = NULL;
	int inputfd = -1;

#ifdef INCLUDE_LIBSEAT
	session = kmsinp->session;
#endif

	udev = udev_new();
	if (!udev) {
		kmr_utils_log(KMR_DANGER, "[x] udev_new: failed to create udev context.");
		goto exit_error_kmr_input_create;
	}

	input = libinput_udev_create_context(&libinput_interface_impl, session, udev);
	if (!input) {
	    kmr_utils_log(KMR_DANGER, "[x] libinput_udev_create_context: failed to create libinput context\n");
	    goto exit_error_kmr_udev_unref;
	}

	const char *seatName = NULL;
#ifdef INCLUDE_LIBSEAT
	seatName = kmsinp->session->seatName;
#else
	seatName = "seat0";
#endif

	if (libinput_udev_assign_seat(input, seatName) < 0) {
		kmr_utils_log(KMR_DANGER, "[x] libinput_udev_assign_seat: failed to assign udev seat to libinput instance");
		goto exit_error_kmr_libinput_unref;
	}
	
	udev_unref(udev);

	inputfd = libinput_get_fd(input);

	return (struct kmr_input) { .input = input, .inputfd = inputfd };

exit_error_kmr_libinput_unref:
	libinput_unref(input);
exit_error_kmr_udev_unref:
	udev_unref(udev);
exit_error_kmr_input_create:
	return (struct kmr_input) { .input = NULL, .inputfd = -1 };
}


uint64_t kmr_input_handle_input_event(struct kmr_input_handle_input_event_info *kmsinp)
{
	struct libinput *input = NULL;
	struct libinput_event *event = NULL;
	enum libinput_event_type type;

	uint64_t code = 0;

	input = kmsinp->input.input;
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


void kmr_input_destroy(struct kmr_input_destroy *kmsinp)
{
	if (kmsinp->kmr_input.input)
		libinput_unref(kmsinp->kmr_input.input);
}
