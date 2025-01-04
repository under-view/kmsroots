#ifndef KMR_BUFFER_H
#define KMR_BUFFER_H

#include "utils.h"

/*
 * Great Info https://afrantzis.com/pixel-format-guide/
 * https://github.com/afrantzis/pixel-format-guide
 */


/*
 * Stores information about the kmr_buffer instance.
 */
struct kmr_buffer;


/*
 * @brief enum kmr_buffer_type (kmsroots Buffer Type)
 *
 *        Buffer allocation options used by kmr_buffer_create(3)
 */
enum kmr_buffer_type
{
	KMR_BUFFER_DUMP_BUFFER               = 0,
	KMR_BUFFER_GBM_BUFFER                = 1,
	KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS = 2,
	KMR_BUFFER_MAX_TYPE                  = KMR_BUFFER_GBM_BUFFER_WITH_MODIFIERS+1,
};


/*
 * @brief struct kmr_buffer_create_info (kmsroots Buffer Create Information)
 *
 * @member bufferType    - Determines what type of buffer to allocate (i.e Dump Buffer, GBM buffer)
 * @member kmsfd         - Used by gbm_create_device. Must be a valid file descriptor
 *                         to a DRI device (GPU character device file)
 * @member bufferCount   - The amount of buffers to allocate.
 *                         	* 2 for double buffering
 *                         	* 3 for triple buffering
 *                         	* Max is set to 5
 * @member width         - Amount of pixels going width wise on screen.
 *                         Need to allocate buffer of similar size.
 * @member height        - Amount of pixels going height wise on screen.
 *                         Need to allocate buffer of similar size.
 * @member bitDepth      - Bit depth: https://petapixel.com/2018/09/19/8-12-14-vs-16-bit-depth-what-do-you-really-need/
 * @member bitsPerPixel  - Pass the amount of bits per pixel.
 * @member gbmBoFlags    - Flags to indicate gbm_bo usage. More info here:
 *                         https://gitlab.freedesktop.org/mesa/mesa/-/blob/main/src/gbm/main/gbm.h#L213
 * @member pixelFormat   - The format of an image details how each pixel color channels is laid out in
 *                         memory: (i.e. RAM, VRAM, etc...). So basically the width in bits, type, and
 *                         ordering of each pixels color channels.
 * @member modifierCount - Number of drm format modifiers passed
 * @member modifiers     - List of drm format modifier
 */
struct kmr_buffer_create_info
{
	enum kmr_buffer_type bufferType;
	unsigned int         kmsfd;
	unsigned int         bufferCount;
	unsigned int         width;
	unsigned int         height;
	unsigned int         bitDepth;
	unsigned int         bitsPerPixel;
	unsigned int         gbmBoFlags;
	unsigned int         pixelFormat;
	unsigned int         modifierCount;
	uint64_t             *modifiers;
};


/*
 * @brief Function creates multiple buffers that may be
 *        used to dump pixels into.
 *
 * @param bufferInfo - Pointer to a struct kmr_buffer_create_info
 *
 * @returns
 *	on success: Pointer to a struct kmr_buffer
 *	on failure: NULL
 */
struct kmr_buffer *
kmr_buffer_create (const void *bufferInfo);


/*
 * @brief Performs write operations into actual framebuffer
 *
 * @param buffer      - Pointer to a struct kmr_buffer.
 * @param bufferIndex - Array index to an individual buffer.
 * @param data        - Pointer to pixel buffer.
 * @param dataSize    - Size of @data pixel buffer.
 *
 * @returns
 * 	on success: 0
 * 	on failure: -1
 */
int
kmr_buffer_write (struct kmr_buffer *buffer,
                  const unsigned int bufferIndex,
                  const void *data,
                  const size_t dataSize);


/*
 * @brief File descriptor to an open KMS node.
 *        File descriptor is passed during call to
 *        kmr_buffer_create(3).
 *
 * @param buffer      - Pointer to a struct kmr_buffer
 * @param bufferIndex - Array index to an individual buffer
 *
 * @returns
 * 	on success: File descriptor to an open KMS node
 * 	on failure: -1
 */
int
kmr_buffer_get_kms_fd (struct kmr_buffer *buffer,
                       const unsigned int bufferIndex);


/*
 * @brief Amount of buffers associated with a struct kmr_buffer context.
 *
 * @param buffer - Pointer to a struct kmr_buffer
 *
 * @returns
 * 	on success: Buffer count
 * 	on failure: -1
 */
int
kmr_buffer_get_buffer_count (struct kmr_buffer *buffer);


/*
 * @brief Pointer to a struct gbm_device
 *
 * @param buffer      - Pointer to a struct kmr_buffer
 * @param bufferIndex - Array index to an individual buffer
 *
 * @returns
 * 	on success: Pointer to a struct gbm_device
 * 	on failure: NULL
 */
const void *
kmr_buffer_get_gbm_device (struct kmr_buffer *buffer,
                           const unsigned int bufferIndex);


/*
 * @brief Pointer to a struct gbm_bo
 *
 * @param buffer      - Pointer to a struct kmr_buffer
 * @param bufferIndex - Array index to an individual buffer
 *
 * @returns
 * 	on success: Pointer to a struct gbm_bo
 * 	on failure: NULL
 */
const void *
kmr_buffer_get_gbm_bo (struct kmr_buffer *buffer,
                       const unsigned int bufferIndex);


/*
 * @brief Returns the framebuffer id of the given buffer @bufferIndex
 *
 * @param buffer      - Pointer to a struct kmr_buffer
 * @param bufferIndex - Array index to an individual buffer
 *
 * @returns
 * 	on success: Framebuffer id
 * 	on failure: -1
 */
int
kmr_buffer_get_framebuffer_id (struct kmr_buffer *buffer,
                               const unsigned int bufferIndex);


/*
 * @brief Returns the pixel format of given buffer
 *
 * @param buffer      - Pointer to a struct kmr_buffer
 * @param bufferIndex - Array index to an individual buffer
 *
 * @returns
 * 	on success: Pixel format
 * 	on failure: -1
 */
int
kmr_buffer_get_pixel_format (struct kmr_buffer *buffer,
                             const unsigned int bufferIndex);


/*
 * @brief Returns the DRM format modifier of given buffer
 *
 * @param buffer      - Pointer to a struct kmr_buffer
 * @param bufferIndex - Array index to an individual buffer
 *
 * @returns
 * 	on success: DRM format modifier
 * 	on failure: -1
 */
int
kmr_buffer_get_format_modifier (struct kmr_buffer *buffer,
                                const unsigned int bufferIndex);


/*
 * @brief Returns the pitch/width/stride in bytes of a
 *        plane associated with a buffer.
 *
 * @param buffer      - Pointer to a struct kmr_buffer
 * @param bufferIndex - Array index to an individual buffer
 * @param planeIndex  - Array index to an individual plane
 *                      associated with a given buffer.
 *
 * @returns
 * 	on success: Pitch/width/stride in bytes of a plane
 *                  associated with a buffer.
 * 	on failure: -1
 */
int
kmr_buffer_get_plane_pitch (struct kmr_buffer *buffer,
                            const unsigned int bufferIndex,
                            const unsigned int planeIndex);


/*
 * @brief Returns the offset in bytes within the plane
 *        associated with a buffer.
 *
 * @param buffer      - Pointer to a struct kmr_buffer
 * @param bufferIndex - Array index to an individual buffer
 * @param planeIndex  - Array index to an individual plane
 *                      associated with a given buffer.
 *
 * @returns
 * 	on success: Offset in bytes within a plane associated with a buffer.
 * 	on failure: -1
 */
int
kmr_buffer_get_plane_offset (struct kmr_buffer *buffer,
                             const unsigned int bufferIndex,
                             const unsigned int planeIndex);


/*
 * @brief Returns file descriptor to a DMA Buffer.
 *
 * @param buffer      - Pointer to a struct kmr_buffer
 * @param bufferIndex - Array index to an individual buffer
 * @param dmaBufIndex - Array index of DMA buffer fd array
 *
 * @returns
 * 	on success: File descriptor to a DMA buffer
 * 	on failure: -1
 */
int
kmr_buffer_get_dma_buf_fd (struct kmr_buffer *buffer,
                           const unsigned int bufferIndex,
                           const unsigned int dmaBufIndex);


/*
 * @brief Frees any allocated memory and closes FD's (if open) created after
 *        kmr_buffer_create() call.
 *
 * @param buffer - Must pass a valid pointer to a struct kmr_buffer
 */
void
kmr_buffer_destroy (struct kmr_buffer *buffer);

#endif /* KMR_BUFFER_H */
