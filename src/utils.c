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

#include <stb_image.h>

#include "utils.h"


struct kmr_utils_aligned_buffer kmr_utils_aligned_buffer_create(struct kmr_utils_aligned_buffer_create_info *kmsutils)
{
	uint32_t bufferAlignment = 0, alignedBufferSize = 0;
	void *alignedBufferMemory = NULL;

	int64_t bitMask = ~(kmsutils->bufferAlignment - 1);
	bufferAlignment = (kmsutils->bytesToAlign + kmsutils->bufferAlignment - 1) & bitMask;

	alignedBufferSize = bufferAlignment * kmsutils->bytesToAlignCount;
	alignedBufferMemory = aligned_alloc(bufferAlignment, alignedBufferSize);
	if (!alignedBufferMemory) {
		kmr_utils_log(KMR_DANGER, "[x] aligned_alloc: %s", strerror(errno));
		goto exit_error_utils_aligned_buffer;
	}

	return (struct kmr_utils_aligned_buffer) { .bufferAlignment = bufferAlignment, .alignedBufferSize = alignedBufferSize, .alignedBufferMemory = alignedBufferMemory };

exit_error_utils_aligned_buffer:
	return (struct kmr_utils_aligned_buffer) { .bufferAlignment = 0, .alignedBufferSize = 0, .alignedBufferMemory = NULL };
}


struct kmr_utils_image_buffer kmr_utils_image_buffer_create(struct kmr_utils_image_buffer_create_info *kmsutils)
{
	char *imageFile = NULL;
	uint8_t bitsPerPixel = 8;
	uint8_t *pixels = NULL;
	int imageWidth = 0, imageHeight = 0, imageChannels = 0;
	int imageSize = 0, requestedImageChannels = 0;
	struct kmr_utils_file loadedImageFile;

	/* Choosing to load image into memory using custom function versus using stbi_load */
	imageFile = kmr_utils_concat_file_to_dir(kmsutils->directory, kmsutils->filename, kmsutils->maxStrLen);
	if (!imageFile)
		goto exit_error_utils_image_buffer;

	loadedImageFile = kmr_utils_file_load(imageFile);
	if (!loadedImageFile.bytes)
		goto exit_error_utils_image_buffer_free_imageFile;

	/*
	 * force 32-bit textures for common Vulkan compatibility. It appears that
	 * some GPU drivers do not support 24-bit images for Vulkan
	 */
	requestedImageChannels = STBI_rgb_alpha;

	/*
	 * Comment taken from tiny_gltf.h in SaschaWillems/Vulkan
	 *
	 * It is possible that the image we want to load is a 16bit per channel image
	 * We are going to attempt to load it as 16bit per channel, and if it worked,
	 * set the image data accodingly. We are casting the returned pointer into
	 * unsigned char, because we are representing "bytes". But we are updating
	 * the Image metadata to signal that this image uses 2 bytes (16bits) per
	 * channel.
	 */
	if (stbi_is_16_bit_from_memory(loadedImageFile.bytes, loadedImageFile.byteSize)) {
		pixels = (uint8_t *) stbi_load_16_from_memory(loadedImageFile.bytes, loadedImageFile.byteSize, &imageWidth, &imageHeight, &imageChannels, requestedImageChannels);
		if (pixels) {
			bitsPerPixel = 16;
		}
	}

	/*
	 * Comment taken from tiny_gltf.h in SaschaWillems/Vulkan
	 * at this point, if data is still NULL, it means that the image wasn't
	 * 16bit per channel, we are going to load it as a normal 8bit per channel
	 * image as we used to do.
	 */
	if (!pixels)
		pixels = (uint8_t *) stbi_load_from_memory(loadedImageFile.bytes, loadedImageFile.byteSize, &imageWidth, &imageHeight, &imageChannels, requestedImageChannels);

	if (!pixels) {
		kmr_utils_log(KMR_DANGER, "[x] stbi_load_from_memory: Unknown image format. STB cannot decode image data for %s", imageFile);
		goto exit_error_utils_image_buffer_free_loadedImageFileBytes;
	}

	free(imageFile);
	free(loadedImageFile.bytes);

	imageSize += (imageWidth * imageHeight) * requestedImageChannels;

	return (struct kmr_utils_image_buffer) { .pixels = pixels, .bitsPerPixel = bitsPerPixel, .imageWidth = imageWidth, .imageHeight = imageHeight,
	                                         .imageChannels = imageChannels, .imageSize = imageSize, .imageBufferOffset = 0 };

//exit_error_utils_image_buffer_free_stbdata:
//	stbi_image_free(pixels);
exit_error_utils_image_buffer_free_loadedImageFileBytes:
	free(loadedImageFile.bytes);
exit_error_utils_image_buffer_free_imageFile:
	free(imageFile);
exit_error_utils_image_buffer:
	return (struct kmr_utils_image_buffer) { .pixels = NULL, .bitsPerPixel = 0, .imageWidth = 0, .imageHeight = 0, .imageChannels = 0, .imageSize = 0, .imageBufferOffset = 0 };
}


struct kmr_utils_file kmr_utils_file_load(const char *filename)
{
	FILE *stream = NULL;
	unsigned char *bytes = NULL;
	long bsize = 0;

	/* Open the file in binary mode */
	stream = fopen(filename, "rb");
	if (!stream) {
		kmr_utils_log(KMR_DANGER, "[x] fopen(%s): %s", filename, strerror(errno));
		goto exit_error_utils_file_load;
	}

	/* Go to the end of the file */
	bsize = fseek(stream, 0, SEEK_END);
	if (bsize == -1) {
		kmr_utils_log(KMR_DANGER, "[x] fseek: %s", strerror(errno));
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
		kmr_utils_log(KMR_DANGER, "[x] ftell: %s", strerror(errno));
		goto exit_error_utils_file_load_fclose;
	}

	/* Jump back to the beginning of the file */
	rewind(stream);

	bytes = (unsigned char *) calloc(bsize, sizeof(unsigned char));
	if (!bytes) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_utils_file_load_fclose;
	}

	/* Read in the entire file */
	if (fread(bytes, bsize, 1, stream) == 0) {
		kmr_utils_log(KMR_DANGER, "[x] fread: %s", strerror(errno));
		goto exit_error_utils_file_load_free_bytes;
	}

	fclose(stream);

	return (struct kmr_utils_file) { .bytes = bytes, .byteSize = bsize };

exit_error_utils_file_load_free_bytes:
	free(bytes);
exit_error_utils_file_load_fclose:
	fclose(stream);
exit_error_utils_file_load:
	return (struct kmr_utils_file) { .bytes = NULL, .byteSize = 0 };
}


// https://www.roxlu.com/2014/047/high-resolution-timer-function-in-c-c--
uint64_t kmr_utils_nanosecond(void)
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


char *kmr_utils_concat_file_to_dir(const char *directory, const char *filename, uint16_t maxStrLen)
{
	char *filepath = NULL;
	struct stat s = {0};

	filepath = calloc(maxStrLen, sizeof(char));
	if (!filepath) {
		kmr_utils_log(KMR_DANGER, "[x] calloc: %s", strerror(errno));
		return NULL;
	}

	if (filename) {
		if (stat(directory, &s) == -1) {
			kmr_utils_log(KMR_DANGER, "[x] stat: %s", strerror(errno));
			free(filepath);
			return NULL;
		}
		
		/*
		 * if @directory is an actual directory
		 * 	append directory path to @filepath string
		 * else
		 * 	if its a file [/home/user/folder/filename.png] get directory
		 * 	to file and append that to @filepath string
		 */
		if (s.st_mode & S_IFDIR) {
			strncat(filepath, directory, maxStrLen);
		} else {
			char *str = strndup(directory, maxStrLen);
			strncat(filepath, dirname(str), maxStrLen);
			free(str);
		}

		/*
		 * if @directory not equal "/"
		 * 	append "/" to @filepath so that when @filename gets appended
		 * 	to @filepath [/home/user/folder -> /home/user/folder/]
		 */
		if (strcmp(directory, "/"))
			strncat(filepath, "/", 2);
		strncat(filepath, filename, maxStrLen);
	} else {
		strncat(filepath, directory, maxStrLen);
	}

	return filepath;
}


int kmr_utils_update_fd_flags(int fd, int flags) {
	int opt;

	opt = fcntl(fd, F_GETFL);
	if (fd < 0) {
		kmr_utils_log(KMR_DANGER, "[x] fcntl(F_GETFL): %s", strerror(errno));
		return -1;
	}

	opt |= flags;
	if (fcntl(fd, F_SETFL, opt) < 0) {
		kmr_utils_log(KMR_DANGER, "[x] fcntl(F_SETFL): %s", strerror(errno));
		return -1;
	}

	return 0;
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
		char name[] = "/kms-XXXXXX";
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
	[KMR_NONE]    = "",
	[KMR_SUCCESS] = "\e[32;1m",
	[KMR_DANGER]  = "\e[31;1m",
	[KMR_INFO]    = "\e[35;1m",
	[KMR_WARNING] = "\e[33;1m",
	[KMR_RESET]   = "\x1b[0m"
};


/* Modified version of what was in wlroots/util/log.c */
const char *_kmr_utils_strip_path(const char *filepath)
{
	if (*filepath == '.')
		while (*filepath == '.' || *filepath == '/')
			filepath++;
	return filepath;
}


void _kmr_utils_log(enum kmr_utils_log_type type, FILE *stream, const char *fmt, ...)
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
	fprintf(stream, "%s", term_colors[KMR_RESET]);

	/* Flush buffer */
	fprintf(stream, "\n");
}
