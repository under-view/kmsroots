#ifndef KMR_SESSION_H
#define KMR_SESSION_H

/*
 * Stores information about the kmr_buffer instance.
 */
struct kmr_session;


/*
 * @brief Create logind/seatd session to access devices without being root.
 *        Function populates all the members of the struct kmr_session.
 *
 * @return
 *	on success: pointer to a struct kmr_session
 *	on failure: NULL
 */
struct kmr_session *
kmr_session_create (void);


/*
 * @brief Calls libseat_switch_session() which requests that the seat switches session
 *        to the specified session number. For seats that are VT-bound, the session number
 *        matches the VT number, and switching session results in a VT switch.
 *
 * @param session - Pointer to a struct kmr_session stores information
 *                  about the current session.
 * @param vt      - Unsigned integer number of the TTY/VT (Virtual Terminal)
 *                  a given seat or seatd/systemd logind session should switch to.
 * 
 * @return
 *	on success: 0
 *	on failure: -1
 */
int
kmr_session_switch_vt (struct kmr_session *session,
                       const unsigned int vt);


/*
 * @brief The TakeDevice systemd-logind D-Bus interface function allows
 *        session controller to get a file descriptor for a specific
 *        device and allows one to acquire control over the given device
 *        (i.e GPU,keyboard,mouse,etc). This functions returns a file
 *        descriptor to the device that has been acquired.
 *
 * @param session - Pointer to a struct kmr_session stores
 *                  information about the current session.
 * @param devpath - Path to a given character device associated
 *                  with a connected device.
 *
 * @return
 *	on success: an open file descriptor
 *	on failure: -1
 */
int
kmr_session_take_control_of_device (struct kmr_session *session,
                                    const char *devpath);


/*
 * @brief The ReleaseDevice systemd-logind D-Bus interface function allows for one
 *        to release control over a given device (i.e GPU,keyboard,mouse,etc). The
 *        function also closes the passed file descriptor.
 *
 * @param session - Must pass a pointer to a struct kmr_session
 * @param fd      - Open file descriptor associated with a given character device file
 */
void
kmr_session_release_device (struct kmr_session *session,
                            const int fd);


/*
 * @brief Frees any allocated memory and closes FD’s (if open) created after
 *        kmr_session_create() call.
 *
 * @param session - Must pass a valid pointer to a struct kmr_session
 */
void
kmr_session_destroy (struct kmr_session *session);


#endif
