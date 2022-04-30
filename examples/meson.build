example_inc = include_directories('common')

if build_kms
  if libdrm.found()
    executable('underview-renderer-kms',
      'kms/main.c',
      link_with: lib_underview_renderer,
      dependencies: lib_uvr_deps,
      include_directories: [inc, example_inc],
      install: true
    )
  endif
endif


if build_xcb
  if libxcb.found() and libxcbewmh.found()
    executable('underview-renderer-x11',
      'X11/main.c',
      link_with: lib_underview_renderer,
      dependencies: lib_uvr_deps,
      include_directories: [inc, example_inc],
      install: true
    )
  endif
endif


if build_wayland
  if wayland_client.found()
    executable('underview-renderer-wayland',
      'wayland/main.c',
      link_with: lib_underview_renderer,
      dependencies: lib_uvr_deps,
      include_directories: [inc, example_inc],
      install: true
    )
  endif
endif
