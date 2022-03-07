vulkan = dependency('vulkan', required: true)
libdrm = dependency('libdrm', required: false)
libmath = cc.find_library('m', required: true)

fs = [ 'vulkan.c', 'utils.c' ]
lib_uvc_deps = [vulkan, libdrm, libmath]

lib_underviewcomp_static = static_library(
  'uvc', files(fs),
  dependencies: lib_uvc_deps,
  include_directories: inc
)
