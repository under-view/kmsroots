#ifndef UVR_INPUT_H
#define UVR_INPUT_H

#include "utils.h"

#include <libinput.h>


/*
 * struct uvr_input (Underview Renderer Input)
 *
 * members:
 * @input   - Pointer to a libinput instance. Instance is used   
 * @inputfd - Pollable file descriptor to a libinput instance. The libinput instance
 *            is used to manage all incomming data from device files associated
 *            with connected input devices.
 */
struct uvr_input {
	struct libinput *input;
	int             inputfd;
};


/*
 * struct uvr_input_create_info (Underview Renderer Input Create Info)
 *
 * members:
 * @session - Address of struct uvrsd_session. Which members are used to communicate
 *            with systemd-logind via D-Bus systemd-logind interface. Needed by
 *            uvr_input_create to acquire and taken control of a device without the
 *            need of being root.
 */
struct uvr_input_create_info {
#ifdef INCLUDE_LIBSEAT
	struct uvr_session *session;
#endif
};


/*
 * uvr_input_create: Creates a libinput instance 
 *
 * args:
 * @uvrinp - pointer to a struct uvr_input_create_info used store information about
 *           the current seatd/sytemd-logind D-Bus session
 * return:
 *	on success struct uvr_input
 *	on failure struct uvr_input { with members nulled, int's set to -1 }
 */
struct uvr_input uvr_input_create(struct uvr_input_create_info *uvrinp);


/*
 * struct uvr_input_handle_input_event_info (Underview Renderer Input Handle Input Event Information)
 *
 * members:
 * @input - Must pass a valid struct uvr_input. Which contains address of libinput instance.
 */
struct uvr_input_handle_input_event_info {
	struct uvr_input input;
};


/*
 * uvr_input_handle_input_event: Function calls libinput_dispatch() which reads events of the file descriptors
 *                               associated with the libinput instance and processes them internally. This function
 *                               should be called after the libinput instance file-descriptor has polled readable.
 *
 * args:
 * @uvrinp - pointer to a struct uvr_input_handle_input_event_info
 * return
 *	on success 0
 *	on failure UINT64_MAX
 */
uint64_t uvr_input_handle_input_event(struct uvr_input_handle_input_event_info *uvrinp);


/*
 * struct uvr_input_destroy (Underview Renderer Input Destroy)
 *
 * members:
 * @uvr_input - Must pass a valid struct uvr_input. Which contains address of libinput instance,
 *              free'd members { @input }.
 */
struct uvr_input_destroy {
	struct uvr_input uvr_input;
};


/*
 * uvr_intput_destroy: Destroy any and all information
 *
 * args:
 * @uvrinp - pointer to a struct uvr_input_destroy
 */
void uvr_input_destroy(struct uvr_input_destroy *uvrinp);

#endif
