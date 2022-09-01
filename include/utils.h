#ifndef UVR_UTILS_H
#define UVR_UTILS_H

#include <stdio.h>
#include <inttypes.h>

#define UNUSED __attribute__((unused))
#define ARRAY_LEN(_arr) (sizeof(_arr) / sizeof(_arr[0]))

/* Used to help determine which ANSI Escape Codes to use */
enum uvr_utils_log_type {
  UVR_NONE    = 0x0000,
  UVR_SUCCESS = 0x0001,
  UVR_DANGER  = 0x0002,
  UVR_INFO    = 0x0003,
  UVR_WARNING = 0x00004,
  UVR_RESET   = 0x0005,
  UVR_MAX_LOG_ENUM = 0xFFFF
};

/* in api function calls */
int allocate_shm_file(size_t size);
void _uvr_utils_log(enum uvr_utils_log_type type, FILE *stream, const char *fmt, ...);
const char *_uvr_utils_strip_path(const char *filepath);

/* Macros defined to help better structure the message */
#define uvr_utils_log(log_type, fmt, ...) \
  _uvr_utils_log(log_type, stdout, "[%s:%d] " fmt, _uvr_utils_strip_path(__FILE__), __LINE__, ##__VA_ARGS__)


/*
 * uvr_utils_nanosecond: Function returns the current time in nanosecond
 *
 * return:
 *    on success current time in nanosecond
 *    on failure nothing happens
 */
uint64_t uvr_utils_nanosecond();


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
 *    on success struct uvr_utils_aligned_buffer
 *    on failure struct uvr_utils_aligned_buffer { with members nulled }
 */
struct uvr_utils_aligned_buffer uvr_utils_aligned_buffer_create(struct uvr_utils_aligned_buffer_create_info *uvrutils);


#endif
