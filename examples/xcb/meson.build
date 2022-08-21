
if libxcb.found() and libxcbewmh.found()
  progs = ['triangle.c', 'triangle-index.c', 'spin-rectangle.c', 'texture-mapping.c']

  foreach p : progs
    if p == 'texture-mapping.c'
      pargs += ['-DTEXTURE_IMAGE="' + examples_dir + '/textures/tuxedo-penguin.png"']
    endif

    exec_name='underview-renderer-xcb-client-' + p.substring(0,-2) # remove .c extension from name
    executable(exec_name, p,
               link_with: lib_underview_renderer,
               dependencies: lib_uvr_deps,
               include_directories: [inc, examples_inc],
               c_args: pargs,
               install: false)
  endforeach
endif
