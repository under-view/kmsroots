// https://www.roxlu.com/2014/047/high-resolution-timer-function-in-c-c--

/* ----------------------------------------------------------------------- */
/*
  Easy embeddable cross-platform high resolution timer function. For each
  platform we select the high resolution timer. You can call the 'ns()'
  function in your file after embedding this.
*/
#include <stdint.h>
#define HAVE_POSIX_TIMER
#include <time.h>
#ifdef CLOCK_MONOTONIC
# define CLOCKID CLOCK_MONOTONIC
#else
# define CLOCKID CLOCK_REALTIME
#endif

uint64_t nanosecond(void) {
  static uint64_t is_init = 0;
  static struct timespec linux_rate;

  if (is_init == 0) {
    clock_getres(CLOCKID, &linux_rate);
    is_init = 1;
  }

  uint64_t now;
  struct timespec spec;
  clock_gettime(CLOCKID, &spec);
  now = spec.tv_sec * 1.0e9 + spec.tv_nsec;
  return now;
}
/* ----------------------------------------------------------------------- */
