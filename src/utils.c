
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

void _uvr_utils_log(uvr_log_type type, FILE *stream, const char *fmt, ...) {
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
