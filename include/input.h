#ifndef KMR_INPUT_H
#define KMR_INPUT_H

#include "utils.h"

#include <libinput.h>


/*
 * struct kmr_input (kmsroots Input)
 *
 * members:
 * @input   - Pointer to a libinput instance. Instance is used   
 * @inputfd - Pollable file descriptor to a libinput instance. The libinput instance
 *            is used to manage all incomming data from device files associated
 *            with connected input devices.
 */
struct kmr_input {
	struct libinput *input;
	int             inputfd;
};


/*
 * struct kmr_input_create_info (kmsroots Input Create Info)
 *
 * members:
 * @session - Address of struct kmssd_session. Which members are used to communicate
 *            with systemd-logind via D-Bus systemd-logind interface. Needed by
 *            kmr_input_create to acquire and taken control of a device without the
 *            need of being root.
 */
struct kmr_input_create_info {
#ifdef INCLUDE_LIBSEAT
	struct kmr_session *session;
#endif
};


/*
 * kmr_input_create: Creates a libinput instance 
 *
 * args:
 * @kmsinp - pointer to a struct kmr_input_create_info used store information about
 *           the current seatd/sytemd-logind D-Bus session
 * return:
 *	on success struct kmr_input
 *	on failure struct kmr_input { with members nulled, int's set to -1 }
 */
struct kmr_input kmr_input_create(struct kmr_input_create_info *kmsinp);


/*
 * struct kmr_input_handle_input_event_info (kmsroots Input Handle Input Event Information)
 *
 * members:
 * @input - Must pass a valid struct kmr_input. Which contains address of libinput instance.
 */
struct kmr_input_handle_input_event_info {
	struct kmr_input input;
};


/*
 * kmr_input_handle_input_event: Function calls libinput_dispatch() which reads events of the file descriptors
 *                               associated with the libinput instance and processes them internally. This function
 *                               should be called after the libinput instance file-descriptor has polled readable.
 *
 * args:
 * @kmsinp - pointer to a struct kmr_input_handle_input_event_info
 * return
 *	on success 0
 *	on failure UINT64_MAX
 */
uint64_t kmr_input_handle_input_event(struct kmr_input_handle_input_event_info *kmsinp);


/*
 * struct kmr_input_destroy (kmsroots Input Destroy)
 *
 * members:
 * @kmr_input - Must pass a valid struct kmr_input. Which contains address of libinput instance,
 *              free'd members { @input }.
 */
struct kmr_input_destroy {
	struct kmr_input kmr_input;
};


/*
 * kmr_intput_destroy: Destroy any and all information
 *
 * args:
 * @kmsinp - pointer to a struct kmr_input_destroy
 */
void kmr_input_destroy(struct kmr_input_destroy *kmsinp);

#endif
