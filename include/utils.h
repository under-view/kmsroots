
#ifndef UVC_UTILS_H
#define UVC_UTILS_H

#include "common.h"

/* Used to help determine which ANSI Escape Codes to use */
typedef enum _uvc_log_type {
  UVC_NONE    = 0x0000,
  UVC_SUCCESS = 0x0001,
  UVC_DANGER  = 0x0002,
  UVC_INFO    = 0x0003,
  UVC_WARNING = 0x00004,
  UVC_RESET   = 0x0005,
  UVC_MAX_LOG_ENUM = 0xFFFF
} uvc_log_type;

void _uvc_utils_log(uvc_log_type type, FILE *stream, const char *fmt, ...);
const char *_uvc_utils_strip_path(const char *filepath);

/* Macros defined to help better structure the message */
#define uvc_utils_log(log_type, fmt, ...) \
  _uvc_utils_log(log_type, stdout, "[%s:%d] " fmt, _uvc_utils_strip_path(__FILE__), __LINE__, ##__VA_ARGS__)

#endif
