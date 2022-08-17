
if wayland_client.found()
  progs = ['shm-buffer.c', 'triangle.c', 'triangle-index.c', 'spin-rectangle.c']

  foreach p : progs
    exec_name='underview-renderer-wayland-client-' + p.substring(0,-2) # remove .c extension from name
    executable(exec_name, p,
               link_with: lib_underview_renderer,
               dependencies: lib_uvr_deps,
               include_directories: [inc, examples_inc],
               c_args: pargs,
               install: false)
  endforeach
endif


if wlroots.found() and wayland_server.found()
  progs = ['compositor.c']

  foreach p : progs
    exec_name='underview-renderer-wayland-' + p.substring(0,-2) # remove .c extension from name
    executable(exec_name, p,
               link_with: lib_underview_renderer,
               dependencies: lib_uvr_deps,
               include_directories: [inc, examples_inc],
               c_args: pargs,
               install: false)
  endforeach
endif
