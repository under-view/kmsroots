#include <drm_fourcc.h>
#include <gbm.h>
#include <vulkan/vulkan.h>

#include "pixel-format.h"


struct kmr_pixel_format {
	uint32_t gbmFormat; // Just a redifinition of @drmFormat
	uint32_t drmFormat;
	uint32_t vkFormat;
};


// https://gitlab.freedesktop.org/wlroots/wlroots/-/blob/master/render/vulkan/pixel_format.c
static const struct kmr_pixel_format formats[] = {
	{
		.gbmFormat = GBM_FORMAT_R8,
		.drmFormat = DRM_FORMAT_R8,
		.vkFormat = VK_FORMAT_R8_SRGB,
	},
	{
		.gbmFormat = GBM_FORMAT_GR88,
		.drmFormat = DRM_FORMAT_GR88,
		.vkFormat = VK_FORMAT_R8G8_SRGB,
	},
	{
		.gbmFormat = GBM_FORMAT_RGB888,
		.drmFormat = DRM_FORMAT_RGB888,
		.vkFormat = VK_FORMAT_B8G8R8_SRGB,
	},
	{
		.gbmFormat = GBM_FORMAT_BGR888,
		.drmFormat = DRM_FORMAT_BGR888,
		.vkFormat = VK_FORMAT_R8G8B8_SRGB,
	},
	{
		.gbmFormat = GBM_FORMAT_ARGB8888,
		.drmFormat = DRM_FORMAT_ARGB8888,
		.vkFormat = VK_FORMAT_B8G8R8A8_SRGB,
	},
	{
		.gbmFormat = GBM_FORMAT_XRGB8888,
		.drmFormat = DRM_FORMAT_XRGB8888,
		.vkFormat = VK_FORMAT_B8G8R8A8_SRGB,
	},
	{
		.gbmFormat = GBM_FORMAT_XBGR8888,
		.drmFormat = DRM_FORMAT_XBGR8888,
		.vkFormat = VK_FORMAT_R8G8B8A8_SRGB,
	},
	{
		.gbmFormat = GBM_FORMAT_ABGR8888,
		.drmFormat = DRM_FORMAT_ABGR8888,
		.vkFormat = VK_FORMAT_R8G8B8A8_SRGB,
	},
	{
		.gbmFormat = GBM_FORMAT_RGBA4444,
		.drmFormat = DRM_FORMAT_RGBA4444,
		.vkFormat = VK_FORMAT_R4G4B4A4_UNORM_PACK16,
	},
	{
		.gbmFormat = GBM_FORMAT_RGBX4444,
		.drmFormat = DRM_FORMAT_RGBX4444,
		.vkFormat = VK_FORMAT_R4G4B4A4_UNORM_PACK16,
	},
	{
		.gbmFormat = GBM_FORMAT_BGRA4444,
		.drmFormat = DRM_FORMAT_BGRA4444,
		.vkFormat = VK_FORMAT_B4G4R4A4_UNORM_PACK16,
	},
	{
		.gbmFormat = GBM_FORMAT_BGRX4444,
		.drmFormat = DRM_FORMAT_BGRX4444,
		.vkFormat = VK_FORMAT_B4G4R4A4_UNORM_PACK16,
	},
	{
		.gbmFormat = GBM_FORMAT_RGB565,
		.drmFormat = DRM_FORMAT_RGB565,
		.vkFormat = VK_FORMAT_R5G6B5_UNORM_PACK16,
	},
	{
		.gbmFormat = GBM_FORMAT_BGR565,
		.drmFormat = DRM_FORMAT_BGR565,
		.vkFormat = VK_FORMAT_B5G6R5_UNORM_PACK16,
	},
	{
		.gbmFormat = GBM_FORMAT_RGBA5551,
		.drmFormat = DRM_FORMAT_RGBA5551,
		.vkFormat = VK_FORMAT_R5G5B5A1_UNORM_PACK16,
	},
	{
		.gbmFormat = GBM_FORMAT_RGBX5551,
		.drmFormat = DRM_FORMAT_RGBX5551,
		.vkFormat = VK_FORMAT_R5G5B5A1_UNORM_PACK16,
	},
	{
		.gbmFormat = GBM_FORMAT_BGRA5551,
		.drmFormat = DRM_FORMAT_BGRA5551,
		.vkFormat = VK_FORMAT_B5G5R5A1_UNORM_PACK16,
	},
	{
		.gbmFormat = GBM_FORMAT_BGRX5551,
		.drmFormat = DRM_FORMAT_BGRX5551,
		.vkFormat = VK_FORMAT_B5G5R5A1_UNORM_PACK16,
	},
	{
		.gbmFormat = GBM_FORMAT_ARGB1555,
		.drmFormat = DRM_FORMAT_ARGB1555,
		.vkFormat = VK_FORMAT_A1R5G5B5_UNORM_PACK16,
	},
	{
		.gbmFormat = GBM_FORMAT_XRGB1555,
		.drmFormat = DRM_FORMAT_XRGB1555,
		.vkFormat = VK_FORMAT_A1R5G5B5_UNORM_PACK16,
	},
	{
		.gbmFormat = GBM_FORMAT_ARGB2101010,
		.drmFormat = DRM_FORMAT_ARGB2101010,
		.vkFormat = VK_FORMAT_A2R10G10B10_UNORM_PACK32,
	},
	{
		.gbmFormat = GBM_FORMAT_XRGB2101010,
		.drmFormat = DRM_FORMAT_XRGB2101010,
		.vkFormat = VK_FORMAT_A2R10G10B10_UNORM_PACK32,
	},
	{
		.gbmFormat = GBM_FORMAT_ABGR2101010,
		.drmFormat = DRM_FORMAT_ABGR2101010,
		.vkFormat = VK_FORMAT_A2B10G10R10_UNORM_PACK32,
	},
	{
		.gbmFormat = GBM_FORMAT_XBGR2101010,
		.drmFormat = DRM_FORMAT_XBGR2101010,
		.vkFormat = VK_FORMAT_A2B10G10R10_UNORM_PACK32,
	},
	{
		.gbmFormat = GBM_FORMAT_ABGR16161616,
		.drmFormat = DRM_FORMAT_ABGR16161616,
		.vkFormat = VK_FORMAT_R16G16B16A16_UNORM,
	},
	{
		.gbmFormat = GBM_FORMAT_XBGR16161616,
		.drmFormat = DRM_FORMAT_XBGR16161616,
		.vkFormat = VK_FORMAT_R16G16B16A16_UNORM,
	},
	{
		.gbmFormat = GBM_FORMAT_ABGR16161616F,
		.drmFormat = DRM_FORMAT_ABGR16161616F,
		.vkFormat = VK_FORMAT_R16G16B16A16_SFLOAT,
	},
	{
		.gbmFormat = GBM_FORMAT_XBGR16161616F,
		.drmFormat = DRM_FORMAT_XBGR16161616F,
		.vkFormat = VK_FORMAT_R16G16B16A16_SFLOAT,
	},
	{
		.gbmFormat = GBM_FORMAT_NV12,
		.drmFormat = DRM_FORMAT_NV12,
		.vkFormat = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
	},
};


uint32_t kmr_pixel_format_convert_name(kmr_pixel_format_conv_type conv, uint32_t format)
{
	for (uint8_t i = 0; i < ARRAY_LEN(formats); i++) {
		switch (conv) {
			case KMR_PIXEL_FORMAT_CONV_GBM_TO_VK:
				if (format == formats[i].gbmFormat)
					return formats[i].vkFormat;
				break;
			case KMR_PIXEL_FORMAT_CONV_GBM_TO_DRM:
				if (format == formats[i].gbmFormat)
					return formats[i].drmFormat;
				break;
			case KMR_PIXEL_FORMAT_CONV_VK_TO_GBM:
				if (format == formats[i].vkFormat)
					return formats[i].gbmFormat;
				break;
			case KMR_PIXEL_FORMAT_CONV_VK_TO_DRM:
				if (format == formats[i].vkFormat)
					return formats[i].drmFormat;
				break;
			case KMR_PIXEL_FORMAT_CONV_DRM_TO_VK:
				if (format == formats[i].drmFormat)
					return formats[i].vkFormat;
				break;
			case KMR_PIXEL_FORMAT_CONV_DRM_TO_GBM:
				if (format == formats[i].drmFormat)
					return formats[i].gbmFormat;
				break;
			default:
				kmr_utils_log(KMR_DANGER, "[x] kmr_pixel_format_convert: Must specify correct kmr_pixel_format_conversion value");
				return UINT32_MAX;
		}
	}

	return UINT32_MAX;
}

