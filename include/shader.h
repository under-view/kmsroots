#ifndef KMR_SHADER_H
#define KMR_SHADER_H

#include "utils.h"


/*
 * struct kmr_shader_spirv (kmsroots Shader SPIRV [Standard Portable Intermediate Representation - Vulkan])
 *
 * members:
 * @result   - Pointer to an opaque handle to the results of a call to any shaderc_compile_into_*()
 *             Unfortunately we can't release/free until after the shader module is created.
 * @bytes    - Buffer that stores a spirv byte code.
 * @byteSize - Byte size of spirv byte code buffer.
 */
struct kmr_shader_spirv {
	void                *result;
	const unsigned char *bytes;
	unsigned long       byteSize;
};


/*
 * struct kmr_shader_spirv_create_info (kmsroots Shader SPIRV Create Information)
 *
 * members:
 * More information can be found at
 * https://github.com/google/shaderc/blob/main/libshaderc/include/shaderc/shaderc.h
 * @kind        - Used to specify what type of shader to create SPIR-V bytes from
 *                key: VkShaderStageFlagBits, value: shaderc_shader_kind
 * @source      - Pointer to a buffer containing actual shader code
 * @filename    - Used as a tag to identify the source string
 * @entryPoint  - Used to define the function name in the GLSL source that acts
 *                as an entry point for the shader
 */
struct kmr_shader_spirv_create_info {
	unsigned int kind;
	const char   *source;
	const char   *filename;
	const char   *entryPoint;
};


/*
 * kmr_shader_spirv_create: Takes in a character buffer containing shader code, it then compiles
 *                          char buff into SPIRV-bytes at runtime. These SPIRV-bytes can later be
 *                          passed to vulkan.
 *
 * parameters:
 * @spirvInfo - Pointer to a struct kmr_shader_spirv_create_info
 * returns:
 *	on success: pointer to struct kmr_shader_spirv
 *	on failure: NULL
 */
struct kmr_shader_spirv *
kmr_shader_spirv_create (struct kmr_shader_spirv_create_info *spirvInfo);


/*
 * kmr_shader_spirv_destroy: Frees any allocated memory and closes FD’s (if open) created after
 *                           kmr_shader_spirv_create() call.
 *
 * parameters:
 * @spirv - Must pass a valid pointer to a struct kmr_shader_spirv
 *        
 *          Free'd members with fd's closed
 *          struct kmr_shader_spirv {
 *              void *result;
 *          };
 */
void
kmr_shader_spirv_destroy (struct kmr_shader_spirv *spirv);


#endif
