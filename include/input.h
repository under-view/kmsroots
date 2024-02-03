#ifndef KMR_INPUT_H
#define KMR_INPUT_H

#include "utils.h"
#ifdef INCLUDE_LIBSEAT
#include "session.h"
#endif

#include <libinput.h>


/*
 * struct kmr_input (kmsroots Input)
 *
 * members:
 * @inputInst - Pointer to a libinput instance.
 * @inputfd   - Pollable file descriptor to a libinput instance. The libinput instance
 *              is used to manage all incomming data from device files associated
 *              with connected input devices.
 */
struct kmr_input {
	struct libinput *inputInst;
	int             inputfd;
};


/*
 * struct kmr_input_create_info (kmsroots Input Create Info)
 *
 * members:
 * @session - Address of struct kmr_session. Which members are used to communicate
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
 * kmr_input_create: Creates a libinput instance and pollable fd.
 *
 * parameters:
 * @inputInfo - Pointer to a struct kmr_input_create_info used store information about
 *              the current seatd/sytemd-logind D-Bus session
 * returns:
 *	on success: Pointer to a struct kmr_input
 *	on failure: NULL
 */
struct kmr_input *
kmr_input_create (struct kmr_input_create_info *inputInfo);


/*
 * kmr_intput_destroy: Frees any allocated memory and closes FD's (if open) created after
 *                     kmr_input_create() call.
 *
 * parameters:
 * @input - Must pass a valid pointer to a struct kmr_input
 */
void
kmr_input_destroy (struct kmr_input *input);


#endif
