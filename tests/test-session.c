/* Required by cmocka */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

#include <cando/cando.h>

#include "session.h"

/******************************************
 * Start of test_session_create functions *
 ******************************************/

static void CANDO_UNUSED
test_session_create (void CANDO_UNUSED **state)
{
	struct kmr_session *session = NULL;

	session = kmr_session_create();
	assert_non_null(session);

	kmr_session_destroy(session);
}

/****************************************
 * End of test_session_create functions *
 ****************************************/


/***************************************
 * Start of test_session_get functions *
 ***************************************/

static void CANDO_UNUSED
test_session_get_seat_name (void CANDO_UNUSED **state)
{
	struct kmr_session *session = NULL;

	const char *seatName = NULL;

	session = kmr_session_create();
	assert_non_null(session);

	seatName = kmr_session_get_seat_name(session);
	assert_non_null(seatName);

	seatName = kmr_session_get_seat_name(NULL);
	assert_null(seatName);

	kmr_session_destroy(session);
}


static void CANDO_UNUSED
test_session_get_seat_fd (void CANDO_UNUSED **state)
{
	int seatfd = -1;

	struct kmr_session *session = NULL;

	session = kmr_session_create();
	assert_non_null(session);

	seatfd = kmr_session_get_seat_fd(session);
	assert_int_not_equal(seatfd, -1);

	seatfd = kmr_session_get_seat_fd(NULL);
	assert_int_equal(seatfd, -1);

	kmr_session_destroy(session);
}

/*************************************
 * End of test_session_get functions *
 *************************************/

int
main (void)
{
	const struct CMUnitTest tests[] =
	{
		cmocka_unit_test(test_session_create),
		cmocka_unit_test(test_session_get_seat_name),
		cmocka_unit_test(test_session_get_seat_fd),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
