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
project_name = meson.project_name()
cc = meson.get_compiler('c')

debuging = get_option('debugging')
shaderc = get_option('shaderc')
kms = get_option('kms')

inc = include_directories('include', 'external/include')

subdir('include')
subdir('src')

lib_kmsroots = library(project_name,
                       cglue_code, files(fs),
                       version: '.'.join(so_version),
                       include_directories: inc,
                       dependencies: lib_kmr_deps,
                       c_args: pargs,
                       install: true)

pkg = import('pkgconfig')

pkglibs = []
pkgsubdirs = []
pkg.generate(libraries: [pkglibs, lib_kmsroots],
             subdirs : pkgsubdirs,
             version : so_version,
             name : project_name,
             filebase : 'kmsroots',
             description : 'Library to help write vulkan renderers.')

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
