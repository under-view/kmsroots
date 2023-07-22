sphinx = find_program('sphinx-build-5', 'sphinx-build')

if sphinx.found()
  sphinx_sources = [ 'index.rst', 'build.rst', 'api.rst' ]

  # build docs and place them inside build/docsrc/src folder
  # after install files in build/docsrc/src into source_root(repo source)/docs
  custom_target('docs',
                command: [sphinx, '-W', '-b', 'html', meson.current_source_dir(), '@OUTPUT@'],
                input: sphinx_sources,
                output: 'docs',
                install: true,
                install_dir: '')
endif
