#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "utils.h"


/* ANSI Escape Codes */
static const char *term_colors[] = {
  [UVR_NONE]    = "",
  [UVR_SUCCESS] = "\e[32;1m",
  [UVR_DANGER]  = "\e[31;1m",
  [UVR_INFO]    = "\e[37;1m",
  [UVR_WARNING] = "\e[33;1m",
  [UVR_RESET]   = "\x1b[0m"
};


void _uvr_utils_log(enum uvr_utils_log_type type, FILE *stream, const char *fmt, ...) {
  char buffer[26];
  va_list args; /* type that holds variable arguments */

  /* create message time stamp */
  time_t rawtime = time(NULL);

  /* generate time */
  strftime(buffer, sizeof(buffer), "%F %T - ", localtime_r(&rawtime, &(struct tm){}));
  fprintf(stream, "%s", buffer);

  /* Set terminal color */
  fprintf(stream, "%s", term_colors[type]);
  va_start(args, fmt);
  vfprintf(stream, fmt, args);
  va_end(args); /* Reset terminal color */
  fprintf(stream, "%s", term_colors[UVR_RESET]);

  /* Flush buffer */
  fprintf(stream, "\n");
}

/* Modified version of what was in wlroots/util/log.c */
const char *_uvr_utils_strip_path(const char *filepath) {
  if (*filepath == '.')
    while (*filepath == '.' || *filepath == '/')
      filepath++;
  return filepath;
}


/* https://wayland-book.com/surfaces/shared-memory.html */
static void randname(char *buf) {
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  long r = ts.tv_nsec;
  for (int i = 0; i < 6; ++i) {
    buf[i] = 'A'+(r&15)+(r&16)*2;
    r >>= 5;
  }
}


static int create_shm_file(void) {
  int retries = 100;
  do {
    char name[] = "/uvr-XXXXXX";
    randname(name + sizeof(name) - 7);
    --retries;
    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (fd >= 0) {
      shm_unlink(name);
      return fd;
    }
  } while (retries > 0 && errno == EEXIST);

  return -1;
}


int allocate_shm_file(size_t size) {
  int fd = create_shm_file();
  if (fd < 0)
    return -1;

  int ret;
  do {
    ret = ftruncate(fd, size);
  } while (ret < 0 && errno == EINTR);

  if (ret < 0) {
    close(fd);
    return -1;
  }

  return fd;
}
