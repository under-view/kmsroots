vulkan = dependency('vulkan', required: true)
libmath = cc.find_library('m', required: true)
libdrm = dependency('libdrm', required: false)
libxcb = cc.find_library('libxcb', required: false)
libxcbewmh = cc.find_library('libxcb-ewmh', required: false)

fs = [ 'vulkan.c', 'utils.c' ]
lib_uvc_deps = [vulkan, libmath]

if libdrm.found()
  lib_uvc_deps += [libdrm]
endif

if libxcb.found() and libxcbewmh.found()
  fs += ['xcb-client.c']
  lib_uvc_deps += [libxcb, libxcbewmh]
endif

lib_underviewcomp_static = static_library(
  'uvc', files(fs),
  dependencies: lib_uvc_deps,
  include_directories: inc
)
