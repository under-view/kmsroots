#ifndef KMR_PIXEL_FORMAT_H
#define KMR_PIXEL_FORMAT_H

#include "utils.h"

/*
 * enum kmr_pixel_format_conv_type (kmsroots Pixel Format Conversion Type)
 *
 * Specifies the type of pixel format name conversion to perform
 */
typedef enum _kmr_pixel_format_conv_type {
	KMR_PIXEL_FORMAT_CONV_GBM_TO_VK  = 0,
	KMR_PIXEL_FORMAT_CONV_GBM_TO_DRM = 1, // Its the same format number
	KMR_PIXEL_FORMAT_CONV_VK_TO_GBM  = 2,
	KMR_PIXEL_FORMAT_CONV_VK_TO_DRM  = 3,
	KMR_PIXEL_FORMAT_CONV_DRM_TO_VK  = 4,
	KMR_PIXEL_FORMAT_CONV_DRM_TO_GBM = 5, // Its the same format number
} kmr_pixel_format_conv_type;


/*
 * kmr_pixel_format_convert_name: Converts the unsigned 32bit integer name given to one pixel format to
 *                                the unsigned 32bit name of another. GBM format name to VK format name.
 *                                The underlying buffer will be in the same pixel format. Makes it easier
 *                                to determine to format to create VkImage's with given the format of the
 *                                GBM buffer object.
 *
 * args:
 * @conv   - Enum constant specifying the format to convert from then to.
 * @format - Unsigned 32bit integer representing the type of pixel format.
 * return:
 * 	on success GBM/DRM/VK format (unsigned 32bit integer)
 * 	on failure UINT32_MAX
 */
uint32_t kmr_pixel_format_convert_name(kmr_pixel_format_conv_type conv, uint32_t format);


#endif
