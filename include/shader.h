#ifndef KMR_SHADER_H
#define KMR_SHADER_H

#include "utils.h"

#include <shaderc/shaderc.h>


/*
 * struct kmr_shader_spirv_create_info (kmsroots Shader SPIRV Create Information)
 *
 * members:
 * See: https://github.com/google/shaderc/blob/main/libshaderc/include/shaderc/shaderc.h#L478 for more information
 * @kind        - Used to specify what type of shader to create SPIR-V bytes from
 *                key: VkShaderStageFlagBits, value: shaderc_shader_kind
 * @source      - Pointer to a buffer containing actual shader code
 * @filename    - Used as a tag to identify the source string
 * @entryPoint  - Used to define the function name in the GLSL source that acts as an entry point for the shader
 */
struct kmr_shader_spirv_create_info {
	unsigned int kind;
	const char   *source;
	const char   *filename;
	const char   *entryPoint;
};


/*
 * struct kmr_shader_spirv (kmsroots Shader SPIRV [Standard Portable Intermediate Representation - Vulkan])
 *
 * members:
 * @result   - An opaque handle to the results of a call to any shaderc_compile_into_*()
 *             Unfortunately we can't release until after the shader module is created.
 * @bytes    - Buffer that stores a given file's content
 * @byteSize - Size of buffer storing a given file's content
 */
struct kmr_shader_spirv {
	shaderc_compilation_result_t result;
	const unsigned char          *bytes;
	unsigned long                byteSize;
};


/*
 * kmr_shader_compile_buffer_to_spirv: Takes in a character buffer containing shader code, it then compiles
 *                                     char buff into SPIRV-bytes at runtime. These SPIRV-bytes can later be
 *                                     passed to vulkan. struct kmr_shader_spirv member result can be free'd
 *                                     with a call to kmr_shader_destroy(3).
 *
 * args:
 * @kmsshader - Pointer to a struct kmr_shader_spirv_create_info containing infromation about what ops to do
 * return:
 *	on success struct kmr_shader_spirv
 *	on failure struct kmr_shader_spirv { with member nulled }
 */
struct kmr_shader_spirv kmr_shader_compile_buffer_to_spirv(struct kmr_shader_spirv_create_info *kmsshader);


/*
 * struct kmr_shader_destroy (kmsroots Shader Destroy)
 *
 * members:
 * @kmr_shader_spirv_cnt - Must pass the amount of elements in struct kmr_shader_spirv array
 * @kmr_shader_spirv     - Must pass a pointer to an array of valid struct kmr_shader_spirv { free'd  members: shaderc_compilation_result_t handle }
 */
struct kmr_shader_destroy {
	uint32_t                kmr_shader_spirv_cnt;
	struct kmr_shader_spirv *kmr_shader_spirv;
};


/*
 * kmr_shader_destroy: frees any allocated memory defined by customer
 *
 * args:
 * @kmsshader - pointer to a struct kmr_shader_destroy contains all objects created during
 *              application lifetime in need freeing
 */
void kmr_shader_destroy(struct kmr_shader_destroy *kmsshader);


#endif
