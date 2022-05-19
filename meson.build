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

build_kms = get_option('kms')
build_xcb = get_option('xcb')
build_wayland = get_option('wayland')

if build_kms
  add_project_arguments(['-DINCLUDE_KMS=1'], language: 'c')

  build_sdbus = get_option('sd-bus')
  if build_sdbus
    add_project_arguments(['-DINCLUDE_SDBUS=1'], language: 'c')
  endif
endif

if build_xcb
  add_project_arguments(['-DINCLUDE_XCB=1'], language: 'c')
endif

if build_wayland
  add_project_arguments(['-DINCLUDE_WAYLAND=1'], language: 'c')
endif


inc = include_directories('include')

subdir('src')

lib_underview_renderer = library(
  meson.project_name(),
  version: '.'.join(so_version),
  link_whole: lib_underview_renderer_static,
  include_directories: inc,
  install: true
)

subdir('examples')
