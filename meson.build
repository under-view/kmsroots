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

pargs = []
so_version = ['0']
cc = meson.get_compiler('c')
inc = include_directories('include')

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
