project(
  'kmsroots',
  'c',
  version: '0.0.1',
  license: 'MIT',
  meson_version: '>=0.63.0',
  default_options: [
    'warning_level=2',
    'werror=true',
    'prefix=/usr/'
  ],
)

pargs = []
so_version = meson.project_version()
cc = meson.get_compiler('c')

debuging = get_option('debugging')
shaderc = get_option('shaderc')
kms = get_option('kms')

inc = include_directories('include', 'external/include')

subdir('include')
subdir('src')

lib_kmsroots = library(
  meson.project_name(),
  version: '.'.join(so_version),
  link_whole: lib_kmsroots_static,
  include_directories: inc,
  c_args: pargs,
  install: true
)

if get_option('examples')
  subdir('examples')
endif

if get_option('tests')
  subdir('tests')
endif

if get_option('docs')
  docs_dir = meson.current_source_dir() + '/docs'
  docs_build_dir = meson.current_build_dir() + '/docs'
  sphinx = find_program('sphinx-build-5', 'sphinx-build', required: true)

  if sphinx.found()
    subdir('docs')
    custom_target('docs',
                  command: [sphinx, '-W', '-b', 'html', docs_dir, '@OUTPUT@'],
                  input: docs_src, output: 'docs', install: true, install_dir: '')
  endif
endif
