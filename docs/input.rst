.. default-domain:: C

input
=====

Header: kmsroots/input.h

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

1. :c:struct:`kmr_input`
#. :c:struct:`kmr_input_create_info`

=========
Functions
=========

1. :c:func:`kmr_input_create`
#. :c:func:`kmr_input_destroy`

=================
Function Pointers
=================

API Documentation
~~~~~~~~~~~~~~~~~

=========
kmr_input
=========

.. c:struct:: kmr_input

	.. c:member::
		struct libinput *inputInst;
		int             inputfd;

	:c:member:`inputInst`
		| Pointer to a libinput instance.

	:c:member:`inputfd`
		| Pollable file descriptor to a libinput instance. The libinput instance
		| is used to manage all incomming data from device files associated
		| with connected input devices.

=====================
kmr_input_create_info
=====================

.. c:struct:: kmr_input_create_info

	.. c:member::
		struct libinput *inputInst;
		int             inputfd;

	**Only included if libseat support enabled**

	:c:member:`session`
		| Address of ``struct`` :c:struct:`kmr_session`. Which members are used to communicate
		| with systemd-logind via D-Bus systemd-logind interface. Needed by
		| :c:func:`kmr_input_create` to acquire and taken control of a device without the
		| need of being root.

=================
kmr_input_create
=================

.. c:function:: struct kmr_input *kmr_input_create(struct kmr_input_create_info *inputInfo);

	Creates a libinput instance and pollable fd.

	Parameters:
		| **inputInfo**
		| Pointer to a struct kmr_input_create_info used store information about
		| the current seatd/sytemd-logind D-Bus session

	Returns:
		| **on success:** Pointer to a ``struct`` :c:struct:`kmr_input`
		| **on failure:** ``NULL``

=================
kmr_input_destroy
=================

.. c:function:: void kmr_input_destroy(struct kmr_input *input);

	Frees any allocated memory and closes FD's (if open) created after
	:c:func:`kmr_input_create` call.

	Parameters:
		| **input** - Must pass a valid pointer to a ``struct`` :c:struct:`kmr_input`

		.. code-block::

			/* Free'd and file descriptors closed members */
			struct kmr_input {
				struct libinput *inputInst;
				int             inputfd;
			};
