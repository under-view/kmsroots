vulkan = dependency('vulkan', required: true)
libmath = cc.find_library('m', required: true)
# Needed by `utils.c` for shm_{open/close}
librt = cc.find_library('rt', required: true)

fs = [ 'vulkan.c', 'utils.c' ]
cglue_code = []
lib_uvr_deps = [vulkan, libmath, librt]


##################################
# KMS libs and files
##################################
if build_kms
  libdrm = dependency('libdrm', required: true)
  if libdrm.found()
    lib_uvr_deps += [libdrm]
  endif

  if build_sdbus
    libsystemd = dependency('libsystemd', required: true)
    if libsystemd.found()
      lib_uvr_deps += [libsystemd]
    endif

    fs += ['sd-dbus.c']
  endif

  libgbm = dependency('gbm', required: true)
  if libgbm.found()
    lib_uvr_deps += [libgbm]
    fs += ['buffer.c']
  endif

  fs += ['kms.c']
endif


##################################
# X11 libs and files
##################################
if build_xcb
  libxcb = dependency('xcb', required: true)
  libxcbewmh = dependency('xcb-ewmh', required: true)

  if libxcb.found() and libxcbewmh.found()
    fs += ['xcb-client.c']
    lib_uvr_deps += [libxcb, libxcbewmh]
  endif
endif


##################################
# Wayland libs and files
##################################
if build_wayland
  wayland_client = dependency('wayland-client', required: true)

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
      cglue_code += [
        wayland_scanner_client.process(xml),
        wayland_scanner_code.process(xml)
      ]
    endforeach

    fs += ['wayland-client.c']
    lib_uvr_deps += [wayland_client]
  endif
endif

lib_underview_renderer_static = static_library(
  'uvr', cglue_code, files(fs),
  dependencies: lib_uvr_deps,
  include_directories: inc
)
