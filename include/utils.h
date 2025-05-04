#ifndef KMR_UTILS_H
#define KMR_UTILS_H

#include <stdio.h>
#include <inttypes.h>

#define UNUSED __attribute__((unused))
#define ARRAY_LEN(_arr) (sizeof(_arr) / sizeof(_arr[0]))
/* https://stackoverflow.com/a/3553321 */
#define STRUCT_MEMBER_SIZE(type, member) sizeof(((type *)0)->member)


/*
 * struct kmr_utils_aligned_buffer (kmsroots Utils Aligned Buffer)
 *
 * members:
 * @bufferAlignment     - Calculated memory buffer alignment value
 *                        @alignedBufferSize: 192 bytes then @bufferAlignment [ 64 | 64 | 64 ]
 * @alignedBufferSize   - Amount of bytes in @alignedBufferMemory
 * @alignedBufferMemory - Pointer to memory aligned range of addresses
 */
struct kmr_utils_aligned_buffer {
	uint32_t bufferAlignment;
	uint32_t alignedBufferSize;
	void     *alignedBufferMemory;
};


/*
 * struct kmr_utils_aligned_buffer_create_info (kmsroots Utils Aligned Buffer Create information)
 *
 * members:
 * @bytesToAlign      - sizeof of data that will be stored in memory aligned buffer
 * @bytesToAlignCount - Amount of @bytesToAlign. Used to allocate aligned buffer of size calculated (@bytesToAlign & bitmask @bufferAlignment) * @bytesToAlignCount
 * @bufferAlignment   - Multiple that is used to determine aligned requirments of a given buffer (block of memory).
 */
struct kmr_utils_aligned_buffer_create_info {
	size_t   bytesToAlign;
	uint32_t bytesToAlignCount;
	uint32_t bufferAlignment;
};


/*
 * kmr_utils_aligned_buffer_create: Function calculates the alignment of @bytesToAlign given that it is a multiple of @bufferAlignment
 *                                  and allocates an aligned memory buffer than can be used to store data.
 *                                  By default malloc(3) aligns buffers by 8 bytes function calls aligned_alloc(3) to change aligment value.
 *                                  Application must call free on struct kmr_utils_aligned_buffer { member: alignedBufferMemory }.
 *
 * parameters:
 * @kmsutils - pointer to a struct kmr_utils_aligned_buffer_create_info
 * returns:
 *	on success struct kmr_utils_aligned_buffer
 *	on failure struct kmr_utils_aligned_buffer { with members nulled }
 */
struct kmr_utils_aligned_buffer kmr_utils_aligned_buffer_create(struct kmr_utils_aligned_buffer_create_info *kmsutils);


/*
 * struct kmr_utils_image_buffer (kmsroots Utils Image Buffer)
 *
 * members:
 * @pixels            - Pointer to actual pixel data
 * @bitsPerPixel      - Stores information about amount of bits per pixel
 * @imageWidth        - Width of image in pixels/texels
 * @imageHeight       - Height of image in pixels/texels
 * @imageChannels     - Amount of color channels image has { RGBA(4): all images get converted to RGBA }
 *                      NOTE: Some images may have an @imageChannels value of 3, but when calculating @imageSize
 *                      the value of 4 is utilized.
 * @imageSize         - Byte size of the image (@imageWidth * @imageHeight) * @imageChannels
 * @imageBufferOffset - Special member used by kmr_gltf_loader_texture_image_create(3) to keep track of byte
 *                      offset in larger VkBuffer->VkDeviceMemory. Image assets and metadata of image assets
 *                      associated with GLTF file are loaded into an array of type struct kmr_utils_image_buffer.
 *                      This array can then be cycled through by the application to populate section of VkBuffer->VkDeviceMemory.
 *                      In order to save the application a bit of clock cycles (i.e by removing duplicate offset additions) compute
 *                      offset once and store value in @imageBufferOffset.
 */
struct kmr_utils_image_buffer {
	uint8_t  *pixels;
	uint8_t  bitsPerPixel;
	uint32_t imageWidth;
	uint32_t imageHeight;
	uint32_t imageChannels;
	size_t   imageSize;
	size_t   imageBufferOffset;
};


/*
 * struct kmr_utils_image_buffer_create_info (kmsroots Utils Image Buffer Create Information)
 *
 * members:
 * @directory - Directory or absolute path to a file that isn't @filename
 * @filename  - String containing file name to append to @directory.
 *              If NULL @directory needs to contain absolute path to file.
 * @maxStrLen - Allow customer to set maximum string len
 */
struct kmr_utils_image_buffer_create_info {
	const char *directory;
	const char *filename;
	uint16_t   maxStrLen;
};


/*
 * kmr_utils_image_buffer_create: Create pixel buffer for any given image and return its size, width, height,
 *                                color channel count, actual pixel buffer, and amount of bits per pixel.
 *                                Function converts RGB-only images to RGBA, as most devices don't support
 *                                RGB-formats in Vulkan.
 *
 * parameters:
 * @kmsutils - pointer to a struct kmr_utils_image_buffer_create_info
 * returns:
 *	on success struct kmr_utils_image_buffer
 *	on failure struct kmr_utils_image_buffer { with member nulled }
 */
struct kmr_utils_image_buffer kmr_utils_image_buffer_create(struct kmr_utils_image_buffer_create_info *kmsutils);


/*
 * kmr_utils_nanosecond: Function returns the current time in nanosecond
 *
 * returns:
 *	on success current time in nanosecond
 *	on failure no checks occur
 */
uint64_t kmr_utils_nanosecond();

/*
 * kmr_utils_concat_file_to_dir: Function acquires absolute path to a file, Given either the directory the file resides in
 *                               or the absolute path to another file that exists in the same directory. Function concatenates
 *                               @filename to @directory. Application must call free on returned pointer.
 *
 * parameters:
 * @directory - Directory or absolute path to a file that isn't @filename
 * @filename  - String containing file name to append to @directory.
 *              If NULL @directory needs to contain absolute path to file.
 * @maxStrLen - Allow customer to set maximum string len
 * returns:
 *	on success absolute path to file
 *	on failure NULL
 */
char *kmr_utils_concat_file_to_dir(const char *directory, const char *filename, uint16_t maxStrLen);

#endif
