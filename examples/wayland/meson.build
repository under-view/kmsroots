
if wayland_client.found()
  executable('underview-renderer-wayland-client-shm-buffer',
             'shm-buffer.c',
             link_with: lib_underview_renderer,
             dependencies: lib_uvr_deps,
             include_directories: [inc, examples_inc],
             c_args: pargs,
             install: false)

  executable('underview-renderer-wayland-client-triangle',
             'triangle.c',
             link_with: lib_underview_renderer,
             dependencies: lib_uvr_deps,
             include_directories: [inc, examples_inc],
             c_args: pargs,
             install: false)

  executable('underview-renderer-wayland-client-triangle-index',
             'triangle-index.c',
             link_with: lib_underview_renderer,
             dependencies: lib_uvr_deps,
             include_directories: [inc, examples_inc],
             c_args: pargs,
             install: false)

  executable('underview-renderer-wayland-client-spin-rectangle',
             'spin-rectangle.c',
             link_with: lib_underview_renderer,
             dependencies: lib_uvr_deps,
             include_directories: [inc, examples_inc],
             c_args: pargs,
             install: false)
endif


if wlroots.found() and wayland_server.found()
  executable('underview-renderer-wayland-comp',
             'compositor.c',
             link_with: lib_underview_renderer,
             dependencies: lib_uvr_deps,
             include_directories: [inc, examples_inc],
             c_args: pargs,
             install: false)
endif
