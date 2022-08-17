
if libdrm.found() and libgbm.found()
  progs = ['main.c']

  foreach p : progs
    exec_name='underview-renderer-kms-' + p.substring(0,-2) # remove .c extension from name
    executable(exec_name, p,
              link_with: lib_underview_renderer,
              dependencies: lib_uvr_deps,
              include_directories: [inc, examples_inc],
              c_args: pargs,
              install: false)
  endforeach
endif
