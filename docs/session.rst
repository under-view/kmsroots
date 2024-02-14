.. default-domain:: C

session
=======

Header: kmsroots/session.h

Table of contents (click to go)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

======
Macros
======

=====
Enums
=====

======
Unions
======

=======
Structs
=======

1. :c:struct:`kmr_session`

=========
Functions
=========

1. :c:func:`kmr_session_create`
#. :c:func:`kmr_session_destroy`
#. :c:func:`kmr_session_switch_vt`
#. :c:func:`kmr_session_take_control_of_device`
#. :c:func:`kmr_session_release_device`

=================
Function Pointers
=================

API Documentation
~~~~~~~~~~~~~~~~~

===========
kmr_session
===========

.. c:struct:: kmr_session

	.. c:member::
		bool           active;
		int            seatFd;
		const char     *seatName;
		struct libseat *seat;

	:c:member:`active`
		| Determines if the session is active or not

	:c:member:`seatFd`
		| Pollable file descriptor to a libseat seatd/systemd-logind D-Bus session

	:c:member:`seatName`
		| Pointer to name of given seat

	:c:member:`seat`
		| Pointer to libseat seatd/systemd-logind D-Bus session

==================
kmr_session_create
==================

.. c:function:: struct kmr_session *kmr_session_create(void);

	Create logind/seatd session to access devices without being root.
	Function populates all the members of the ``struct`` :c:struct:`kmr_session`.

	Parameters:
		| **session**
		| Pointer to a ``struct`` :c:struct:`kmr_session` stores information about the current session

	Returns:
		| **on success:** pointer to ``struct`` :c:struct:`kmr_session`
		| **on failure:** NULL

===================
kmr_session_destroy
===================

.. c:function:: void kmr_session_destroy(struct kmr_session *session);

	Frees any allocated memory and closes FD’s (if open) created after
	:c:func:`kmr_session_create` call

	Parameters:
		| **session**
		| Must pass a valid pointer to a ``struct`` :c:struct:`kmr_session`

	.. code-block::

		/* Free'd members with fd's closed */
		struct kmr_session {
			int seatFd;
			struct libseat *seat;
		};

=========================================================================================================================================

=====================
kmr_session_switch_vt
=====================

.. c:function:: int kmr_session_switch_vt(struct kmr_session *session, unsigned int vt);

	Calls `libseat_switch_session()`_ which requests that the seat switches session
	to the specified session number. For seats that are VT-bound, the session number
	matches the VT number, and switching session results in a VT switch.

=========================================================================================================================================

==================================
kmr_session_take_control_of_device
==================================

.. c:function:: int kmr_session_take_control_of_device(struct kmr_session *session, const char *devpath);

	The TakeDevice systemd-logind D-Bus interface function allows
	session controller to get a file descriptor for a specific
	device and allows one to acquire control over the given device
	(i.e GPU,keyboard,mouse,etc). This functions returns a file
	descriptor to the device that has been acquired.

	Parameters:
		| **session**
		| Pointer to a ``struct`` :c:struct:`kmr_session` stores information about the current session
		| **devpath**
		| Path to a given character device associated with a connected device

	Returns:
		| **on success:** an open file descriptor
		| **on failure:** -1

==========================
kmr_session_release_device
==========================

.. c:function:: void kmr_session_release_device(struct kmr_session *session, int fd);

	The ReleaseDevice systemd-logind D-Bus interface function allows for one
	to release control over a given device (i.e GPU,keyboard,mouse,etc). The
	function also closes the passed file descriptor.

	Parameters:
		| **session**
		| Must pass a pointer to a ``struct`` :c:struct:`kmr_session`
		| **fd**
		| Open file descriptor associated with a given character device file

=========================================================================================================================================

.. _libseat_switch_session(): https://git.sr.ht/~kennylevinsen/seatd/tree/master/item/include/libseat.h#L109
