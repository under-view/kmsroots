project(
  'underview-renderer',
  'c',
  version: '0.0.1',
  license: 'MIT',
  meson_version: '>=0.53.0',
  default_options: [
    'warning_level=2',
    'werror=true',
    'prefix=/usr/'
  ],
)

so_version = ['0']
cc = meson.get_compiler('c')
inc = include_directories('include')

pargs = []
build_kms = get_option('kms')
build_xcb = get_option('xcb')
build_wayland = get_option('wayland')

if build_kms
  pargs += ['-DINCLUDE_KMS=1']
  build_sdbus = get_option('sd-bus')
  if build_sdbus
    pargs += ['-DINCLUDE_SDBUS=1']
  endif
endif

if build_xcb
  pargs += ['-DINCLUDE_XCB=1']
endif

if build_wayland
  pargs += ['-DINCLUDE_WAYLAND=1']
endif

subdir('src')

lib_underview_renderer = library(
  meson.project_name(),
  version: '.'.join(so_version),
  link_whole: lib_underview_renderer_static,
  include_directories: inc,
  c_args: pargs,
  install: true
)

subdir('examples')
