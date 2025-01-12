/*
 * This C source test the functions in three
 * different interfaces:
 * 	* buffer
 * 	* drm-node
 * 	* session.c
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <gbm.h>

/* Required by cmocka */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

#include <cando/cando.h>

#include "drm-node.h"
#include "buffer.h"
#ifdef INCLUDE_LIBSEAT
#include "session.h"
#endif /* INCLUDE_LIBSEAT */

struct app_kms
{
	struct kmr_drm_node *kmr_drm_node;
	struct kmr_buffer   *kmr_buffer;
#ifdef INCLUDE_LIBSEAT
	struct kmr_session  *kmr_session;
#endif /* INCLUDE_LIBSEAT */
};


/********************************************
 * Start of functions used by multiple test *
 ********************************************/

static void
cleanup (struct app_kms *kms)
{
	kmr_drm_node_destroy(kms->kmr_drm_node);
	kmr_buffer_destroy(kms->kmr_buffer);
#ifdef INCLUDE_LIBSEAT
	kmr_session_destroy(kms->kmr_session);
#endif /* INCLUDE_LIBSEAT */
}


static int
create_drm_context (struct app_kms *kms)
{
	int err = -1;

	struct kmr_drm_node_create_info kmsNodeCreateInfo;

#ifdef INCLUDE_LIBSEAT
	kms->kmr_session = kmr_session_create();
	if (!(kms->kmr_session))
		return -1;

	kmsNodeCreateInfo.session = kms->kmr_session;
#endif /* INCLUDE_LIBSEAT */

	kmsNodeCreateInfo.kmsNode = NULL;
	kms->kmr_drm_node = kmr_drm_node_create(&kmsNodeCreateInfo);
	if (!(kms->kmr_drm_node)) {
		cleanup(kms);
		return -1;
	}

	err = kmr_drm_node_set_display(kms->kmr_drm_node, NULL);
	if (err == -1) {
		cleanup(kms);
		return -1;
	}

	return 0;
}


static int
create_gbm_buffers (struct app_kms *kms)
{
	struct kmr_buffer_create_info bufferCreateInfo;
	memset(&bufferCreateInfo, 0, sizeof(bufferCreateInfo));

	bufferCreateInfo.bufferType = KMR_BUFFER_GBM_BUFFER;
	bufferCreateInfo.kmsfd = kmr_drm_node_get_kms_fd(kms->kmr_drm_node);
	bufferCreateInfo.bufferCount = 2;
	bufferCreateInfo.width = kmr_drm_node_get_display_width(kms->kmr_drm_node);
	bufferCreateInfo.height = kmr_drm_node_get_display_height(kms->kmr_drm_node);
	bufferCreateInfo.bitDepth = 24;
	bufferCreateInfo.bitsPerPixel = 32;
	bufferCreateInfo.gbmBoFlags = GBM_BO_USE_SCANOUT | GBM_BO_USE_WRITE;
	bufferCreateInfo.pixelFormat = GBM_BO_FORMAT_XRGB8888;
	bufferCreateInfo.modifiers = NULL;
	bufferCreateInfo.modifierCount = 0;

	kms->kmr_buffer = kmr_buffer_create(&bufferCreateInfo);
	if (!(kms->kmr_buffer)) {
		cleanup(kms);
		return -1;
	}

	return 0;
}

/******************************************
 * End of functions used by multiple test *
 ******************************************/


/**************************************
 * Start of test_kms_create functions *
 **************************************/

static void CANDO_UNUSED
test_kms_create_gbm_buffer (void CANDO_UNUSED **state)
{
	int ret = -1;

	struct app_kms app;
	memset(&app, 0, sizeof(app));

	cando_log_set_level(CANDO_LOG_ALL);

	ret = create_drm_context(&app);
	assert_int_equal(ret, 0);

	ret = create_gbm_buffers(&app);
	assert_int_equal(ret, 0);

	cleanup(&app);
}

/************************************
 * End of test_kms_create functions *
 ************************************/

int
main (void)
{
	const struct CMUnitTest tests[] =
	{
		cmocka_unit_test(test_kms_create_gbm_buffer),
	};

	return cmocka_run_group_tests(tests, NULL, NULL);
}
