
executable('underview-renderer-x11',
  'X11/main.c',
  link_with: lib_underview_renderer,
  dependencies: lib_uvr_deps,
  include_directories: inc,
  install: true
)

executable('underview-renderer-wayland',
  'wayland/main.c',
  link_with: lib_underview_renderer,
  dependencies: lib_uvr_deps,
  include_directories: inc,
  install: true
)
