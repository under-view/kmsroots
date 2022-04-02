vulkan = dependency('vulkan', required: true)
libmath = cc.find_library('m', required: true)
# Needed by `utils.c` for shm_{open/close}
librt = cc.find_library('rt', required: true)
libdrm = dependency('libdrm', required: false)
libxcb = cc.find_library('libxcb', required: false)
libxcbewmh = cc.find_library('libxcb-ewmh', required: false)
wayland_client = dependency('wayland-client', required: false)

fs = [ 'vulkan.c', 'utils.c' ]
lib_uvr_deps = [vulkan, libmath, librt]

##################################
# KMS libs and files
##################################
if libdrm.found()
  lib_uvr_deps += [libdrm]
endif

##################################
# X11 libs and files
##################################
if libxcb.found() and libxcbewmh.found()
  fs += ['xcb-client.c']
  lib_uvr_deps += [libxcb, libxcbewmh]
endif

##################################
# Wayland libs and files
##################################
wayland_cglue = []
if wayland_client.found()
  wayland_scanner = find_program('wayland-scanner', required: true)
  wayland_protos = dependency('wayland-protocols', required: true)

  wayland_protos_dir = wayland_protos.get_pkgconfig_variable('pkgdatadir')
  wayland_protos = [ [wayland_protos_dir, 'stable/xdg-shell/xdg-shell.xml'], ]

  # wayland-scanner process these wayland protocols defined
  # in XML files and generate code from them
  # processes xdg-shell.xml defines all the interfaces
  # supported by a Wayland client.
  wayland_scanner_code = generator(wayland_scanner,
    output: '@BASENAME@-protocol.c',
    arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
  )

  wayland_scanner_client = generator(wayland_scanner,
    output: '@BASENAME@-client-protocol.h',
    arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
  )

  # Create corresponding C glue code from wayland xml protocls extension files
  foreach p : wayland_protos
    xml = join_paths(p)
    wayland_cglue += [
      wayland_scanner_client.process(xml),
      wayland_scanner_code.process(xml)
    ]
  endforeach

  fs += ['wayland-client.c']
  lib_uvr_deps += [wayland_client]
endif

lib_underview_renderer_static = static_library(
  'uvr', wayland_cglue, files(fs),
  dependencies: lib_uvr_deps,
  include_directories: inc
)
