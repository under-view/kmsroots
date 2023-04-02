#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdint.h>
#include <libgen.h>    // dirname(3)
#define HAVE_POSIX_TIMER
#include <time.h>
#ifdef CLOCK_MONOTONIC
# define CLOCKID CLOCK_MONOTONIC
#else
# define CLOCKID CLOCK_REALTIME
#endif

#include "utils.h"


struct uvr_utils_aligned_buffer uvr_utils_aligned_buffer_create(struct uvr_utils_aligned_buffer_create_info *uvrutils)
{
	uint32_t bufferAlignment = 0, alignedBufferSize = 0;
	void *alignedBufferMemory = NULL;

	int64_t bitMask = ~(uvrutils->bufferAlignment - 1);
	bufferAlignment = (uvrutils->bytesToAlign + uvrutils->bufferAlignment - 1) & bitMask;

	alignedBufferSize = bufferAlignment * uvrutils->bytesToAlignCount;
	alignedBufferMemory = aligned_alloc(bufferAlignment, alignedBufferSize);
	if (!alignedBufferMemory) {
		uvr_utils_log(UVR_DANGER, "[x] aligned_alloc: %s", strerror(errno));
		goto exit_error_utils_aligned_buffer;
	}

	return (struct uvr_utils_aligned_buffer) { .bufferAlignment = bufferAlignment, .alignedBufferSize = alignedBufferSize, .alignedBufferMemory = alignedBufferMemory };

exit_error_utils_aligned_buffer:
	return (struct uvr_utils_aligned_buffer) { .bufferAlignment = 0, .alignedBufferSize = 0, .alignedBufferMemory = NULL };
}


struct uvr_utils_file uvr_utils_file_load(const char *filename)
{
	FILE *stream = NULL;
	unsigned char *bytes = NULL;
	long bsize = 0;

	/* Open the file in binary mode */
	stream = fopen(filename, "rb");
	if (!stream) {
		uvr_utils_log(UVR_DANGER, "[x] fopen(%s): %s", filename, strerror(errno));
		goto exit_error_utils_file_load;
	}

	/* Go to the end of the file */
	bsize = fseek(stream, 0, SEEK_END);
	if (bsize == -1) {
		uvr_utils_log(UVR_DANGER, "[x] fseek: %s", strerror(errno));
		goto exit_error_utils_file_load_fclose;
	}

	/*
	 * Get the current byte offset in the file.
	 * Used to read current position. Thus returns
	 * a number equal to the size of the buffer we
	 * need to allocate
	 */
	bsize = ftell(stream);
	if (bsize == -1) {
		uvr_utils_log(UVR_DANGER, "[x] ftell: %s", strerror(errno));
		goto exit_error_utils_file_load_fclose;
	}

	/* Jump back to the beginning of the file */
	rewind(stream);

	bytes = (unsigned char *) calloc(bsize, sizeof(unsigned char));
	if (!bytes) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_utils_file_load_fclose;
	}

	/* Read in the entire file */
	if (fread(bytes, bsize, 1, stream) == 0) {
		uvr_utils_log(UVR_DANGER, "[x] fread: %s", strerror(errno));
		goto exit_error_utils_file_load_free_bytes;
	}

	fclose(stream);

	return (struct uvr_utils_file) { .bytes = bytes, .byteSize = bsize };

exit_error_utils_file_load_free_bytes:
	free(bytes);
exit_error_utils_file_load_fclose:
	fclose(stream);
exit_error_utils_file_load:
	return (struct uvr_utils_file) { .bytes = NULL, .byteSize = 0 };
}


// https://www.roxlu.com/2014/047/high-resolution-timer-function-in-c-c--
uint64_t uvr_utils_nanosecond(void)
{
	static uint64_t isInit = 0;
	static struct timespec linux_rate;

	if (isInit == 0) {
		clock_getres(CLOCKID, &linux_rate);
		isInit = 1;
	}

	uint64_t now;
	struct timespec spec;
	clock_gettime(CLOCKID, &spec);
	now = spec.tv_sec * 1.0e9 + spec.tv_nsec;
	return now;
}


char *uvr_utils_concat_file_to_dir(const char *directory, const char *filename, int maxStrLen)
{
	char *filepath = NULL;
	struct stat s = {0};

	/* Load all images associated with GLTF file into memory */
	filepath = calloc(maxStrLen, sizeof(char));
	if (!filepath) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		return NULL;
	}

	if (stat(directory, &s) == -1) {
		uvr_utils_log(UVR_DANGER, "[x] stat: %s", strerror(errno));
		free(filepath);
		return NULL;
	}

	if (s.st_mode & S_IFDIR) {
		strncat(filepath, directory, maxStrLen);
	} else {
		char *str = strndup(directory, maxStrLen);
		strncat(filepath, dirname(str), maxStrLen);
		free(str);
	}

	if (strcmp(directory, "/"))
		strncat(filepath, "/", 2);
	strncat(filepath, filename, maxStrLen);

	return filepath;
}


/* https://wayland-book.com/surfaces/shared-memory.html */
static void randname(char *buf)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	long r = ts.tv_nsec;
	for (int i = 0; i < 6; ++i) {
		buf[i] = 'A'+(r&15)+(r&16)*2;
		r >>= 5;
	}
}


static int create_shm_file(void)
{
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


int allocate_shm_file(size_t size)
{
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


/* ANSI Escape Codes */
static const char *term_colors[] = {
	[UVR_NONE]    = "",
	[UVR_SUCCESS] = "\e[32;1m",
	[UVR_DANGER]  = "\e[31;1m",
	[UVR_INFO]    = "\e[35;1m",
	[UVR_WARNING] = "\e[33;1m",
	[UVR_RESET]   = "\x1b[0m"
};


/* Modified version of what was in wlroots/util/log.c */
const char *_uvr_utils_strip_path(const char *filepath)
{
	if (*filepath == '.')
		while (*filepath == '.' || *filepath == '/')
			filepath++;
	return filepath;
}


void _uvr_utils_log(enum uvr_utils_log_type type, FILE *stream, const char *fmt, ...)
{
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
