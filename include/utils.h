#ifndef UVR_UTILS_H
#define UVR_UTILS_H

#include <stdio.h>
#include <inttypes.h>

#define UNUSED __attribute__((unused))
#define ARRAY_LEN(_arr) (sizeof(_arr) / sizeof(_arr[0]))
/* https://stackoverflow.com/a/3553321 */
#define STRUCT_MEMBER_SIZE(type, member) sizeof(((type *)0)->member)


/*
 * struct uvr_utils_aligned_buffer (Underview Utils Aligned Buffer)
 *
 * members:
 * @bufferAlignment     - Calculated memory buffer alignment value
 *                        @alignedBufferSize: 192 bytes then @bufferAlignment [ 64 | 64 | 64 ]
 * @alignedBufferSize   - Amount of bytes in @alignedBufferMemory
 * @alignedBufferMemory - Pointer to memory aligned range of addresses
 */
struct uvr_utils_aligned_buffer {
	uint32_t bufferAlignment;
	uint32_t alignedBufferSize;
	void     *alignedBufferMemory;
};


/*
 * struct uvr_utils_aligned_buffer_create_info (Underview Utils Aligned Buffer Create information)
 *
 * members:
 * @bytesToAlign      - sizeof of data that will be stored in memory aligned buffer
 * @bytesToAlignCount - Amount of @bytesToAlign. Used to allocate aligned buffer of size calculated (@bytesToAlign & bitmask @bufferAlignment) * @bytesToAlignCount
 * @bufferAlignment   - Multiple that is used to determine aligned requirments of a given buffer (block of memory).
 */
struct uvr_utils_aligned_buffer_create_info {
	size_t   bytesToAlign;
	uint32_t bytesToAlignCount;
	uint32_t bufferAlignment;
};


/*
 * uvr_utils_aligned_buffer_create: Function calculates the alignment of @bytesToAlign given that it is a multiple of @bufferAlignment
 *                                  and allocates an aligned memory buffer than can be used to store data.
 *                                  By default malloc(3) aligns buffers by 8 bytes function calls aligned_alloc(3) to change aligment value.
 *                                  Application must call free on struct uvr_utils_aligned_buffer { member: alignedBufferMemory }.
 *
 * args:
 * @uvrutils - pointer to a struct uvr_utils_aligned_buffer_create_info
 * return:
 *	on success struct uvr_utils_aligned_buffer
 *	on failure struct uvr_utils_aligned_buffer { with members nulled }
 */
struct uvr_utils_aligned_buffer uvr_utils_aligned_buffer_create(struct uvr_utils_aligned_buffer_create_info *uvrutils);


/*
 * struct uvr_utils_image_buffer (Underview Renderer Utils Image Buffer)
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
 * @imageBufferOffset - Special member used by uvr_gltf_loader_texture_image_create(3) to keep track of byte
 *                      offset in larger VkBuffer->VkDeviceMemory. Image assets and metadata of image assets
 *                      associated with GLTF file are loaded into an array of type struct uvr_utils_image_buffer.
 *                      This array can then be cycled through by the application to populate section of VkBuffer->VkDeviceMemory.
 *                      In order to save the application a bit of clock cycles (i.e by removing duplicate offset additions) compute
 *                      offset once and store value in @imageBufferOffset.
 */
struct uvr_utils_image_buffer {
	uint8_t  *pixels;
	uint8_t  bitsPerPixel;
	uint32_t imageWidth;
	uint32_t imageHeight;
	uint32_t imageChannels;
	size_t   imageSize;
	size_t   imageBufferOffset;
};


/*
 * struct uvr_utils_image_buffer_create_info (Underview Renderer Utils Image Buffer Create Information)
 *
 * members:
 * @directory - Directory or absolute path to a file that isn't @filename
 * @filename  - String containing file name to append to @directory.
 *              If NULL @directory needs to contain absolute path to file.
 * @maxStrLen - Allow customer to set maximum string len
 */
struct uvr_utils_image_buffer_create_info {
	const char *directory;
	const char *filename;
	uint16_t   maxStrLen;
};


/*
 * uvr_utils_image_buffer_create: Create pixel buffer for any given image and return its size, width, height,
 *                                color channel count, actual pixel buffer, and amount of bits per pixel.
 *                                Function converts RGB-only images to RGBA, as most devices don't support
 *                                RGB-formats in Vulkan.
 *
 * args:
 * @uvrutils - pointer to a struct uvr_utils_image_buffer_create_info
 * return:
 *	on success struct uvr_utils_image_buffer
 *	on failure struct uvr_utils_image_buffer { with member nulled }
 */
struct uvr_utils_image_buffer uvr_utils_image_buffer_create(struct uvr_utils_image_buffer_create_info *uvrutils);


/*
 * struct uvr_utils_file (Underview Renderer Utils File)
 *
 * members:
 * @bytes    - Buffer that stores a given file's content
 * @byteSize - Size of buffer storing a given file's content
 */
struct uvr_utils_file {
	unsigned char *bytes;
	unsigned long byteSize;
};


/*
 * uvr_utils_file_load: Takes a file and loads its contents into a memory buffer.
 *                      Application's  must take up the mantel and call free on @bytes member.
 *
 * args:
 * @filename - Must pass path to file to load
 * return:
 *	on success struct uvr_utils_file
 *	on failure struct uvr_utils_file { with member nulled }
 */
struct uvr_utils_file uvr_utils_file_load(const char *filename);


/*
 * uvr_utils_nanosecond: Function returns the current time in nanosecond
 *
 * return:
 *	on success current time in nanosecond
 *	on failure no checks occur
 */
uint64_t uvr_utils_nanosecond();


/*
 * uvr_utils_concat_file_to_dir: Function acquires absolute path to a file, Given either the directory the file resides in
 *                               or the absolute path to another file that exists in the same directory. Function concatenates
 *                               @filename to @directory. Application must call free on returned pointer.
 *
 * args:
 * @directory - Directory or absolute path to a file that isn't @filename
 * @filename  - String containing file name to append to @directory.
 *              If NULL @directory needs to contain absolute path to file.
 * @maxStrLen - Allow customer to set maximum string len
 * return:
 *	on success absolute path to file
 *	on failure NULL
 */
char *uvr_utils_concat_file_to_dir(const char *directory, const char *filename, uint16_t maxStrLen);


int allocate_shm_file(size_t size);


/* Used to help determine which ANSI Escape Codes to use */
enum uvr_utils_log_type {
	UVR_NONE         = 0,
	UVR_SUCCESS      = 1,
	UVR_DANGER       = 2,
	UVR_INFO         = 3,
	UVR_WARNING      = 4,
	UVR_RESET        = 5,
	UVR_MAX_LOG_ENUM = 0xFFFFFFFF
};


void _uvr_utils_log(enum uvr_utils_log_type type, FILE *stream, const char *fmt, ...);
const char *_uvr_utils_strip_path(const char *filepath);


/* Macros defined to help better structure the message */
#define uvr_utils_log(log_type, fmt, ...) \
	_uvr_utils_log(log_type, stdout, "[%s:%d] " fmt, _uvr_utils_strip_path(__FILE__), __LINE__, ##__VA_ARGS__)

#endif
