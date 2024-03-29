################################################################################
# Core files and dependency
################################################################################
vulkan = dependency('vulkan', version: '>=1.3.213', required: true)
libmath = cc.find_library('m', required: true)
# Needed by `utils.c` for shm_{open/close}
librt = cc.find_library('rt', required: true)
# Needed by `gltf-loader.c`
libcglm = dependency('cglm', required: true)

fs = [ 'vulkan.c', 'utils.c', 'gltf-loader.c', 'stb_image.c' ]
lib_kmr_deps = [vulkan, libmath, librt]


################################################################################
# Shaderc libs, files, & extra compiler args
################################################################################
libshaderc = dependency('shaderc', required: get_option('shaderc'))
if libshaderc.found()
  pargs += ['-DINCLUDE_SHADERC=1']
  lib_kmr_deps += [libshaderc]
  fs += ['shader.c']
endif


################################################################################
# KMS libs, files, & extra compiler args
################################################################################
libseat = dependency('libseat', required: get_option('libseat'))
libinput = dependency('libinput', required: get_option('libinput'))
libdrm = dependency('libdrm', required: get_option('kms'))
libgbm = dependency('gbm', required: get_option('kms'))
libudev = dependency('libudev', required: get_option('kms').enabled() or get_option('libinput').enabled())
if libdrm.found()
  pargs += ['-DINCLUDE_KMS=1']
  lib_kmr_deps += [libdrm]
  fs += ['drm-node.c', 'dma-buf.c']
endif

if libseat.found()
  pargs += ['-DINCLUDE_LIBSEAT=1']
  lib_kmr_deps += [libseat]
  fs += ['session.c']
endif

if libgbm.found()
  lib_kmr_deps += [libgbm]
  fs += ['buffer.c']
endif

if libudev.found()
  lib_kmr_deps += [libudev]
endif

if libinput.found()
  lib_kmr_deps += [libinput]
  fs += ['input.c']
endif

if libgbm.found() and libdrm.found() and vulkan.found()
  fs += ['pixel-format.c']
endif

################################################################################
# X11 libs, files, & extra compiler args
################################################################################
libxcb = dependency('xcb', required: get_option('xcb'))
libxcbewmh = dependency('xcb-ewmh', required: get_option('xcb'))

if libxcb.found() and libxcbewmh.found()
  pargs += ['-DINCLUDE_XCB=1']
  fs += ['xclient.c']
  lib_kmr_deps += [libxcb, libxcbewmh]
endif


################################################################################
# Wayland libs, files, & extra compiler args
################################################################################
wayland_client = dependency('wayland-client', required: get_option('wayland'))

# Needed to stored corresponding C glue code
# from wayland xml protocls extension files
cglue_code = []

if wayland_client.found()
  wayland_scanner = find_program('wayland-scanner', required: true)
  wayland_protos = dependency('wayland-protocols', required: true)

  wayland_protos_dir = wayland_protos.get_variable(pkgconfig: 'pkgdatadir')
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
  lib_kmr_deps += [wayland_client]
endif


lib_kmsroots_static = static_library(
  'kms', cglue_code, files(fs),
  dependencies: lib_kmr_deps,
  include_directories: inc,
  c_args: pargs
)
