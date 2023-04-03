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

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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


struct uvr_utils_image_buffer uvr_utils_image_buffer_create(struct uvr_utils_image_buffer_create_info *uvrutils)
{
	char *imageFile = NULL;
	uint8_t bitsPerPixel = 8;
	uint8_t *pixels = NULL, *data = NULL;
	int imageWidth = 0, imageHeight = 0, imageChannels = 0;
	int imageSize = 0, requestedImageChannels = 0;
	struct uvr_utils_file loadedImageFile;

	/* Choosing to load image into memory using custom function versus using stbi_load */
	imageFile = uvr_utils_concat_file_to_dir(uvrutils->directory, uvrutils->filename, uvrutils->maxStrLen);
	if (!imageFile)
		goto exit_error_utils_image_buffer;

	loadedImageFile = uvr_utils_file_load(imageFile);
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
		data = (uint8_t *) stbi_load_16_from_memory(loadedImageFile.bytes, loadedImageFile.byteSize, &imageWidth, &imageHeight, &imageChannels, requestedImageChannels);
		if (data) {
			bitsPerPixel = 16;
		}
	}

	/*
	 * Comment taken from tiny_gltf.h in SaschaWillems/Vulkan
	 * at this point, if data is still NULL, it means that the image wasn't
	 * 16bit per channel, we are going to load it as a normal 8bit per channel
	 * image as we used to do.
	 */
	if (!data)
		data = (uint8_t *) stbi_load_from_memory(loadedImageFile.bytes, loadedImageFile.byteSize, &imageWidth, &imageHeight, &imageChannels, requestedImageChannels);

	if (!data) {
		uvr_utils_log(UVR_DANGER, "[x] stbi_load_from_memory: Unknown image format. STB cannot decode image data for %s", imageFile);
		goto exit_error_utils_image_buffer_free_loadedImageFileBytes;
	}

	free(imageFile);
	free(loadedImageFile.bytes);

	imageSize += (imageWidth * imageHeight) * requestedImageChannels;
	pixels = calloc(imageSize, sizeof(uint8_t));
	if (!pixels) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		goto exit_error_utils_image_buffer_free_stbdata;
	}

	if (imageChannels == 3) {
		uint8_t *rgba = pixels;
		uint8_t *rgb = data;

		uint8_t colors[4];
		colors[3] = 1;

		for (int i = 0; i < imageSize; i+=4) {
			memcpy(colors, rgb, 3);
			memcpy(rgba, colors, 4);
			rgba += 4; rgb += 3;
		}
	} else {
		memcpy(pixels, data, imageSize);
	}

	stbi_image_free(data);

	return (struct uvr_utils_image_buffer) { .pixels = pixels, .bitsPerPixel = bitsPerPixel, .imageWidth = imageWidth, .imageHeight = imageHeight,
	                                         .imageChannels = requestedImageChannels, .imageSize = imageSize, .imageBufferOffset = 0 };

exit_error_utils_image_buffer_free_stbdata:
	stbi_image_free(data);
exit_error_utils_image_buffer_free_loadedImageFileBytes:
	free(loadedImageFile.bytes);
exit_error_utils_image_buffer_free_imageFile:
	free(imageFile);
exit_error_utils_image_buffer:
	return (struct uvr_utils_image_buffer) { .pixels = NULL, .bitsPerPixel = 0, .imageWidth = 0, .imageHeight = 0, .imageChannels = 0, .imageSize = 0, .imageBufferOffset = 0 };
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


char *uvr_utils_concat_file_to_dir(const char *directory, const char *filename, uint16_t maxStrLen)
{
	char *filepath = NULL;
	struct stat s = {0};

	filepath = calloc(maxStrLen, sizeof(char));
	if (!filepath) {
		uvr_utils_log(UVR_DANGER, "[x] calloc: %s", strerror(errno));
		return NULL;
	}

	if (filename) {
		if (stat(directory, &s) == -1) {
			uvr_utils_log(UVR_DANGER, "[x] stat: %s", strerror(errno));
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
