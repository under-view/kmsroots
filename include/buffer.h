#ifndef KMR_BUFFER_H
#define KMR_BUFFER_H

#include "utils.h"

#include <gbm.h>

/*
 * Great Info https://afrantzis.com/pixel-format-guide/
 * https://github.com/afrantzis/pixel-format-guide
 */


/*
 * enum kmr_buffer_type (kmsroots Buffer Type)
 *
 * Buffer allocation options used by kmr_buffer_create
 */
enum kmr_buffer_type {
	KMR_BUFFER_DUMP_BUFFER               = 0,
	KMR_BUFFER_GBM_BUFFER                = 1,
	KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS = 2
};


/*
 * struct kmr_buffer_object (kmsroots Buffer Object)
 *
 * members:
 * @bo           - Handle to some GEM allocated buffer. Used to get GEM handles, DMA buffer fds
 *                 (fd associate with GEM buffer), pitches, and offsets for the buffer used by
 *                 DRI device (GPU)
 * @fbid         - Framebuffer ID
 * @format       - The format of an image details how each pixel color channels is laid out in
 *                 memory: (i.e. RAM, VRAM, etc...). So, basically the width in bits, type, and
 *                 ordering of each pixels color channels.
 * @modifier     - The modifier details information on how pixels should be within a buffer for different types
 *                 operations such as scan out or rendering. (i.e linear, tiled, compressed, etc...)
 *                 https://01.org/linuxgraphics/Linux-Window-Systems-with-DRM
 * @planeCount   - Number of Planar Formats. The number of @dmaBufferFds, @offsets, @pitches retrieved per plane.
 *                 More information can be found https://en.wikipedia.org/wiki/Planar_(computer_graphics)
 * @pitches      - width in bytes for each plane
 * @offsets      - offset of each plane
 *                 More information can be found https://gitlab.freedesktop.org/mesa/drm/-/blob/main/include/drm/drm_mode.h#L589
 * @dmaBufferFds - (PRIME fd) Stores file descriptors to buffers that can be shared across hardware
 * @kmsfd        - File descriptor to open DRI device
 */
struct kmr_buffer_object {
	struct gbm_bo *bo;
	unsigned      fbid;
	unsigned      format;
	uint64_t      modifier;
	unsigned      planeCount;
	unsigned      pitches[4];
	unsigned      offsets[4];
	int           dmaBufferFds[4];
	int           kmsfd;
};


/*
 * struct kmr_buffer (kmsroots Buffer)
 *
 * members:
 * @gbmDevice     - A handle used to allocate gbm buffers & surfaces
 * @bufferCount   - Array size of @bufferObjects
 * @bufferObjects - Stores an array of gbm_bo's and corresponding information about the individual buffer.
 */
struct kmr_buffer {
	struct gbm_device        *gbmDevice;
	unsigned int             bufferCount;
	struct kmr_buffer_object *bufferObjects;
};


/*
 * struct kmr_buffer_create_info (kmsroots Buffer Create Information)
 *
 * members:
 * @bufferType    - Determines what type of buffer to allocate (i.e Dump Buffer, GBM buffer)
 * @kmsfd         - Used by gbm_create_device. Must be a valid file descriptor
 *                  to a DRI device (GPU character device file)
 * @bufferCount   - The amount of buffers to allocate.
 *                  * 2 for double buffering
 *                  * 3 for triple buffering
 * @width         - Amount of pixels going width wise on screen. Need to allocate buffer of similar size.
 * @height        - Amount of pixels going height wise on screen. Need to allocate buffer of similar size.
 * @bitDepth      - Bit depth: https://petapixel.com/2018/09/19/8-12-14-vs-16-bit-depth-what-do-you-really-need/
 * @bitsPerPixel  - Pass the amount of bits per pixel
 * @gbmBoFlags    - Flags to indicate gbm_bo usage. More info here:
 *                  https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/gbm/main/gbm.h#L213
 * @pixelFormat   - The format of an image details how each pixel color channels is laid out in
 *                  memory: (i.e. RAM, VRAM, etc...). So basically the width in bits, type, and
 *                  ordering of each pixels color channels.
 * @modifiers     - List of drm format modifier
 * @modifierCount - Number of drm format modifiers passed
 */
struct kmr_buffer_create_info {
	enum kmr_buffer_type bufferType;
	unsigned int         kmsfd;
	unsigned int         bufferCount;
	unsigned int         width;
	unsigned int         height;
	unsigned int         bitDepth;
	unsigned int         bitsPerPixel;
	unsigned int         gbmBoFlags;
	unsigned int         pixelFormat;
	uint64_t             *modifiers;
	unsigned int         modifierCount;
};


/*
 * kmr_buffer_create: Function creates multiple GPU buffers
 *
 * parameters:
 * @kmrbuff - Pointer to a struct kmsbuff_create_info
 * returns:
 *	on success struct kmr_buffer
 *	on failure struct kmr_buffer { with members nulled }
 */
struct kmr_buffer kmr_buffer_create(struct kmr_buffer_create_info *kmrbuff);


/*
 * kmr_buffer_destroy: Function free's all allocate objects associated with a given buffer
 *
 * parameters:
 * @kmrbuff - Must pass an array of valid struct kmr_buffer
 * 	      Free’d and file descriptors closed members
 *	      struct kmr_buffer {
 *	          struct gbm_device *gbmDevice;
 *	          struct kmr_buffer_object *bufferObjects {
 *	              struct gbm_bo *bo;
 *	              unsigned dmaBufferFds[4];
 *	              unsigned fbid;
 *	          }
 *	      }
 * @count - Must pass the amount of elements in struct kmr_buffer array
 */
void kmr_buffer_destroy(struct kmr_buffer *kmrbuff, unsigned count);


#endif
