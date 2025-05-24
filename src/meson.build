#############################
# Core files and dependency #
#############################

libcando = dependency('cando', version: '>= 0.1.7', required: true)
vulkan = dependency('vulkan', version: '>=1.3.213', required: true)
libmath = cc.find_library('m', required: true)
# Needed by `utils.c` for shm_{open/close}
librt = cc.find_library('rt', required: true)
# Needed by `gltf-loader.c`
libcglm = dependency('cglm', required: true)

fs = [ 'src/vulkan.c', 'src/utils.c', 'src/gltf-loader.c', 'src/stb_image.c' ]
lib_kmr_deps = [vulkan, libmath, librt, libcando]


################################################################
# Shader interface required libs, files, & extra compiler args #
################################################################

libshaderc = dependency('shaderc', required: shaderc.enabled())

if libshaderc.found()
  pargs += ['-DINCLUDE_SHADERC=1']
  lib_kmr_deps += [libshaderc]
  fs += ['src/shader.c']
endif


##########################################################################
# kms/session/buffer/input interfaces libs, files, & extra compiler args #
##########################################################################

libseat = dependency('libseat', required: session.enabled())
libinput = dependency('libinput', required: libinput.enabled())
libdrm = dependency('libdrm', required: kms.enabled())
libgbm = dependency('gbm', required: kms.enabled())
libudev = dependency('libudev', required: kms.enabled() or libinput.enabled())

if libdrm.found()
  pargs += ['-DINCLUDE_KMS=1']
  lib_kmr_deps += [libdrm]
  fs += ['src/drm-node.c', 'src/dma-buf.c']
endif

if libseat.found()
  pargs += ['-DINCLUDE_LIBSEAT=1']
  lib_kmr_deps += [libseat]
  fs += ['src/session.c']
endif

if libgbm.found()
  lib_kmr_deps += [libgbm]
  fs += ['src/buffer.c']
endif

if libudev.found()
  lib_kmr_deps += [libudev]
endif

if libinput.found()
  lib_kmr_deps += [libinput]
  fs += ['src/input.c']
endif

if libgbm.found() and libdrm.found() and vulkan.found()
  fs += ['src/pixel-format.c']
endif
