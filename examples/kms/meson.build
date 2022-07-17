
if libdrm.found() and libgbm.found()
  executable('underview-renderer-kms',
             'main.c',
             link_with: lib_underview_renderer,
             dependencies: lib_uvr_deps,
             include_directories: [inc],
             c_args: pargs,
             install: false)
endif
