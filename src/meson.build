################################################################################
# Core files and dependency
################################################################################
vulkan = dependency('vulkan', version: '>=1.3.213', required: true)
libmath = cc.find_library('m', required: true)
# Needed by `utils.c` for shm_{open/close}
librt = cc.find_library('rt', required: true)

fs = [ 'vulkan.c', 'shader.c', 'utils.c' ]
lib_uvr_deps = [vulkan, libmath, librt]


################################################################################
# Shaderc libs, files, & extra compiler args
################################################################################
libshaderc = dependency('shaderc', required: get_option('shaderc'))
if libshaderc.found()
  pargs += ['-DINCLUDE_SHADERC=1']
  lib_uvr_deps += [libshaderc]
endif


################################################################################
# KMS libs, files, & extra compiler args
################################################################################
libdrm = dependency('libdrm', required: get_option('kms'))
libsystemd = dependency('libsystemd', required: get_option('sd-bus'))
libgbm = dependency('gbm', required: get_option('kms'))
if libdrm.found()
  pargs += ['-DINCLUDE_KMS=1']
  lib_uvr_deps += [libdrm]
  fs += ['kms.c']
endif

if libsystemd.found()
  pargs += ['-DINCLUDE_SDBUS=1']
  lib_uvr_deps += [libsystemd]
  fs += ['sd-dbus.c']
endif

if libgbm.found()
  lib_uvr_deps += [libgbm]
  fs += ['buffer.c']
endif


################################################################################
# X11 libs, files, & extra compiler args
################################################################################
libxcb = dependency('xcb', required: get_option('xcb'))
libxcbewmh = dependency('xcb-ewmh', required: get_option('xcb'))

if libxcb.found() and libxcbewmh.found()
  pargs += ['-DINCLUDE_XCB=1']
  fs += ['xclient.c']
  lib_uvr_deps += [libxcb, libxcbewmh]
endif


################################################################################
# Wayland libs, files, & extra compiler args
################################################################################
wayland_client = dependency('wayland-client', required: get_option('wayland'))
wayland_server = dependency('wayland-server', required: get_option('wayland-compositor'))
wlroots = dependency('wlroots', required: get_option('wayland-compositor'))

# Needed to stored corresponding C glue code
# from wayland xml protocls extension files
cglue_code = []

if wlroots.found() and wayland_server.found()
  pargs += ['-DWLR_USE_UNSTABLE=1', '-DINCLUDE_WAYLAND_COMP=1']
  fs += ['wserver.c']
  lib_uvr_deps += [wayland_server, wlroots]
endif

if wayland_client.found()
  wayland_scanner = find_program('wayland-scanner', required: true)
  wayland_protos = dependency('wayland-protocols', required: true)

  wayland_protos_dir = wayland_protos.get_pkgconfig_variable('pkgdatadir')
  wayland_protos = [
    [wayland_protos_dir, 'stable/xdg-shell/xdg-shell.xml'],
    [wayland_protos_dir, 'unstable/fullscreen-shell/fullscreen-shell-unstable-v1.xml' ],
  ]

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

  pargs += ['-DINCLUDE_WAYLAND=1']
  fs += ['wclient.c']
  lib_uvr_deps += [wayland_client]
endif


lib_underview_renderer_static = static_library(
  'uvr', cglue_code, files(fs),
  dependencies: lib_uvr_deps,
  include_directories: inc,
  c_args: pargs
)
