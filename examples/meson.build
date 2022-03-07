
executable('underviewcomp-x11',
  'X11/main.c',
  link_with: lib_underviewcomp,
  dependencies: lib_uvc_deps,
  include_directories: inc,
  install: true
)

executable('underviewcomp-wayland',
  'wayland/main.c',
  link_with: lib_underviewcomp,
  dependencies: lib_uvc_deps,
  include_directories: inc,
  install: true
)
