#ifndef UVR_BUFFER_H
#define UVR_BUFFER_H

#include "utils.h"
#include "kms.h"

#include <gbm.h>

/*
 * Great Info https://afrantzis.com/pixel-format-guide/
 * https://github.com/afrantzis/pixel-format-guide
 */


/*
 * enum uvr_buffer_type (Underview Renderer Buffer Type)
 *
 * Buffer allocation options used by uvr_buffer_create
 */
enum uvr_buffer_type {
  UVR_BUFFER_DUMP_BUFFER               = 0,
  UVR_BUFFER_GBM_BUFFER                = 1,
  UVR_BUFFER_GBM_BUFFER_WITH_MODIFIERS = 2
};


/*
 * struct uvr_buffer_object (Underview Renderer Buffer Object)
 *
 * members:
 * @bo           - Handle to some GEM allocated buffer. Used to get GEM handles, DMA buffer fds (fd associate with GEM buffer),
 *                 pitches, and offsets for the buffer used by DRI device (GPU)
 * @fbid         - Framebuffer ID
 * @format       - The format of an image details how each pixel color channels is laid out in
 *                 memory: (i.e. RAM, VRAM, etc...). So, basically the width in bits, type, and
 *                 ordering of each pixels color channels.
 * @modifier     - The modifier details information on how pixels should be within a buffer for different types
 *                 operations such as scan out or rendering. (i.e linear, tiled, compressed, etc...)
 *                 https://01.org/linuxgraphics/Linux-Window-Systems-with-DRM
 * @planeCount   - Number of Planer Formats
 *                 More information can be found https://en.wikipedia.org/wiki/Planar_(computer_graphics)
 * @pitches      - pitch for each plane
 * @offsets      - offset of each plane
 *                 More information can be found https://gitlab.freedesktop.org/mesa/drm/-/blob/main/include/drm/drm_mode.h#L589
 * @gemHandles   - Stores GEM handles per plane used to query a DMA buf fd
 * @dmaBufferFds - (PRIME fd) Stores file descriptors to buffers that can be shared across hardware
 * @kmsfd        - File descriptor to open DRI device
 */
struct uvr_buffer_object {
  struct gbm_bo *bo;
  unsigned      fbid;
  unsigned      format;
  uint64_t      modifier;
  unsigned      planeCount;
  unsigned      pitches[4];
  unsigned      offsets[4];
  unsigned      gemHandles[4];
  int           dmaBufferFds[4];
  int           kmsfd;
};


/*
 * struct uvr_buffer (Underview Renderer Buffer)
 *
 * members:
 * @gbmDevice     - A handle used to allocate gbm buffers & surfaces
 * @bufferCount   - Array size of @bufferObjects
 * @bufferObjects - Stores an array of gbm_bo's and corresponding information
 *                  about the individual buffer.
 */
struct uvr_buffer {
  struct gbm_device        *gbmDevice;
  unsigned int             bufferCount;
  struct uvr_buffer_object *bufferObjects;
};


/*
 * struct uvr_buffer_create_info (Underview Renderer Buffer Create Information)
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
struct uvr_buffer_create_info {
  enum uvr_buffer_type bufferType;
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
 * uvr_buffer_create: Function creates multiple buffers that can store
 *
 * args:
 * @uvrbuff - Pointer to a struct uvrbuff_create_info
 * return:
 *    on success struct uvr_buffer
 *    on failure struct uvr_buffer { with members nulled }
 */
struct uvr_buffer uvr_buffer_create(struct uvr_buffer_create_info *uvrbuff);


/*
 * struct uvr_buffer_destroy (Underview Renderer Buffer Destroy)
 *
 * members:
 * @uvr_buffer_cnt - Must pass the amount of elements in struct uvr_buffer array
 * @uvr_buffer     - Must pass an array of valid struct uvr_buffer
 *                   {
 *                      free'd members:
 *                               struct gbm_device reference
 *                               struct uvr_buffer_object reference
 *                               int dmaBufferFds[4] - GEM handles closed
 *                               struct gbm_bo reference
 *                               unsigned fbid - KMS fd removed
 *                   }
 */
struct uvr_buffer_destroy {
  unsigned          uvr_buffer_cnt;
  struct uvr_buffer *uvr_buffer;
};


/*
 * uvr_buffer_destory: Function free's all allocate objects associated with a given buffer
 *
 * args:
 * @uvrbuff - Pointer to a struct uvr_buffer_destroy
 */
void uvr_buffer_destory(struct uvr_buffer_destroy *uvrbuff);


#endif
