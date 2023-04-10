
#if libdrm.found() and libgbm.found()
#  progs = ['simple-vk-drm.c']
#
#  foreach p : progs
#    exec_name='underview-renderer-kms-' + p.substring(0,-2) # remove .c extension from name
#    executable(exec_name, p,
#              link_with: lib_underview_renderer,
#              dependencies: lib_uvr_deps,
#              include_directories: [inc],
#              c_args: pargs,
#              install: false)
#  endforeach
#endif

if wlroots.found() and wayland_server.found()
  progs = ['compositor.c']

  foreach p : progs
    exec_name='underview-renderer-kms-' + p.substring(0,-2) # remove .c extension from name
    executable(exec_name, p,
               link_with: lib_underview_renderer,
               dependencies: lib_uvr_deps,
               include_directories: [inc],
               c_args: pargs,
               install: false)
  endforeach
endif
