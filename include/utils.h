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
 * struct kmr_utils_file (kmsroots Utils File)
 *
 * members:
 * @bytes    - Buffer that stores a given file's content
 * @byteSize - Size of buffer storing a given file's content
 */
struct kmr_utils_file {
	unsigned char *bytes;
	unsigned long byteSize;
};


/*
 * kmr_utils_file_load: Takes a file and loads its contents into a memory buffer.
 *                      Application's  must take up the mantel and call free on @bytes member.
 *
 * parameters:
 * @filename - Must pass path to file to load
 * returns:
 *	on success struct kmr_utils_file
 *	on failure struct kmr_utils_file { with member nulled }
 */
struct kmr_utils_file kmr_utils_file_load(const char *filename);


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


/*
 * kmr_utils_update_fd_flags: Updates the flags set on a file descriptor
 *
 * parameters:
 * @fd    - File descriptor to updates
 * @flags - Flags to update
 * returns:
 *	on success 0
 *	on failure -1
 */
int kmr_utils_update_fd_flags(int fd, int flags);


int allocate_shm_file(size_t size);


/*
 * enum kmr_utils_log_level_type (kmsroots Utils Log Level Type)
 *
 * Sets which messages of a given type to print and is used to
 * help determine which ANSI Escape Codes to utilize.
 */
typedef enum _kmr_utils_log_type {
	KMR_NONE     = 0x00000000,
	KMR_SUCCESS  = 0x00000001,
	KMR_DANGER   = 0x00000002,
	KMR_INFO     = 0x00000004,
	KMR_WARNING  = 0x00000008,
	KMR_RESET    = 0x00000010,
	KMR_ALL      = 0xFFFFFFFF
} kmr_utils_log_type;


void _kmr_utils_log(kmr_utils_log_type type, FILE *stream, const char *fmt, ...);
const char *_kmr_utils_strip_path(const char *filepath);


/* Macros defined to help better structure the message */
#define kmr_utils_log(logType, fmt, ...) \
	_kmr_utils_log(logType, stdout, "[%s:%d] " fmt, _kmr_utils_strip_path(__FILE__), __LINE__, ##__VA_ARGS__)


/*
 * kmr_utils_set_log_level: Sets which messages of kmr_utils_log_type allowed to be printed to stdout.
 *
 * parameters:
 * @level - 32-bit integer representing the logs to print to stdout.
 */
void kmr_utils_set_log_level(kmr_utils_log_type level);

#endif
