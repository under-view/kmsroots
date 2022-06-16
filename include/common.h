#ifndef UVR_COMMON_H
#define UVR_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/sysmacros.h>
#include <math.h>

#define UNUSED __attribute__((unused))
#define ARRAY_LEN(_arr) (sizeof(_arr) / sizeof(_arr[0]))

#define WINDOW_CNT 10

#endif
