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

#endif
