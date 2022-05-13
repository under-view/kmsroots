#ifndef UVR_BUFFER_H
#define UVR_BUFFER_H

#include "common.h"
#include "utils.h"
#include "kms.h"

#include <gbm.h>

/*
 * Great Info https://afrantzis.com/pixel-format-guide/
 * https://github.com/afrantzis/pixel-format-guide
 */


/*
 * enum uvrbuff_type (Underview Renderer Buffer Type)
 *
 * Buffer allocation options used by uvr_buffer_create
 */
enum uvrbuff_type {
  DUMP_BUFFER,
  GBM_BUFFER,
  GBM_BUFFER_WITH_MODIFIERS
};


/*
 * struct uvrbuff_object_info (Underview Renderer Buffer Object Information)
 *
 * members:
 * @bo          - Handle to some GBM backend allocated buffer. Used to get GEM handles, DMA buf fds,
 *                pitches, and offsets for the buffer used by DRI device (GPU)
 * @fbid        - Frame buffer ID
 * @format      - The format of an image details how each pixel color channels is laid out in
 *                memory: (i.e. RAM, VRAM, etc...). So, basically the width in bits, type, and
 *                ordering of each pixels color channels.
 * @modifier    - The modifier details information on how pixels should be within a buffer for different types
 *                operations such as scan out or rendering. (i.e linear, tiled, compressed, etc...)
 *                https://01.org/linuxgraphics/Linux-Window-Systems-with-DRM
 * @num_planes  - Number of Planer Formats
 *                More information can be found https://en.wikipedia.org/wiki/Planar_(computer_graphics)
 * @pitches     - pitch for each plane
 * @offsets     - offset of each plane
 *                More information can be found https://gitlab.freedesktop.org/mesa/drm/-/blob/main/include/drm/drm_mode.h#L589
 * @gem_handles - Stores GEM handles per plane used to query a DMA buf fd
 * @dma_buf_fds - (PRIME fd) Stores file descriptors to buffers that can be shared across hardware
 * @kmsfd       - File descriptor to open DRI device
 */
struct uvrbuff_object_info {
  struct gbm_bo *bo;
  unsigned fbid;
  unsigned format;
  uint64_t modifier;
  unsigned num_planes;
  unsigned pitches[4];
  unsigned offsets[4];
  unsigned gem_handles[4];
  int dma_buf_fds[4];
  int kmsfd;
};


/*
 * struct uvrbuff (Underview Renderer Buffer)
 *
 * members:
 * @gbmdev       - A handle used to allocate gbm buffers & surfaces
 * @info_buffers - Stores an array of gbm_bo's and corresponding information
 *                 about the individual buffer.
 */
struct uvrbuff {
  struct gbm_device *gbmdev;
  struct uvrbuff_object_info *info_buffers;
};


/*
 * struct uvrbuff (Underview Renderer Buffer Create Information)
 *
 * members:
 * @bType         - Determines what type of buffer to allocate (i.e Dump Buffer, GBM buffer)
 * @kmsfd         - Used by gbm_create_device. Must be a valid file descriptor
 *                  to a DRI device (GPU character device file)
 * @buff_cnt      - The amount of buffers to allocate.
 *                  * 2 for double buffering
 *                  * 3 for triple buffering
 * @width         - Amount of pixels going width wise on screen. Need to allocate buffer of similar size.
 * @height        - Amount of pixels going height wise on screen. Need to allocate buffer of similar size.
 * @bitdepth      - Bit depth: https://petapixel.com/2018/09/19/8-12-14-vs-16-bit-depth-what-do-you-really-need/
 * @bpp           - Pass the amount of bits per pixel
 * @gbm_bo_flags  - Flags to indicate gbm_bo usage. More info here:
 *                  https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/gbm/main/gbm.h#L213
 * @pixformat     - The format of an image details how each pixel color channels is laid out in
 *                  memory: (i.e. RAM, VRAM, etc...). So basically the width in bits, type, and
 *                  ordering of each pixels color channels.
 * @modifiers     - List of drm format modifier
 * @modifiers_cnt - Number of drm format modifiers passed
 */
struct uvrbuff_create_info {
  enum uvrbuff_type bType;
  unsigned int kmsfd;

  unsigned int buff_cnt;
  unsigned int width;
  unsigned int height;
  unsigned int bitdepth;
  unsigned int bpp;
  unsigned int gbm_bo_flags;
  unsigned int pixformat;
  uint64_t *modifiers;
  unsigned int modifiers_cnt;
};


/*
 * uvr_buffer_create: Function creates multiple buffers that can store
 *
 * args:
 * @uvrbuff - Pointer to a struct uvrbuff_create_info
 * return:
 *    on success struct uvrbuff
 *    on failure struct uvrbuff { with members nulled }
 */
struct uvrbuff uvr_buffer_create(struct uvrbuff_create_info *uvrbuff);


/*
 * struct uvrbuff_destroy (Underview Renderer Buffer Destroy)
 *
 * members:
 * @gbmdev       - A handle used to allocate gbm buffers & surfaces
 * @buff_cnt     - Amount of allocated buffers
 * @info_buffers - Stores an array of gbm_bo's and corresponding information
 *                 about the individual buffer.
 */
struct uvrbuff_destroy {
  struct gbm_device *gbmdev;

  unsigned int buff_cnt;
  struct uvrbuff_object_info *info_buffers;
};


/*
 * uvr_buffer_destory: Function free's all allocate objects associated with a given buffer
 *
 * args:
 * @uvrbuff - Pointer to a struct uvrbuff_destroy
 */
void uvr_buffer_destory(struct uvrbuff_destroy *uvrbuff);


#endif
