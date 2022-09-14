
if wayland_client.found()
  progs = ['shm-buffer.c', 'triangle.c', 'triangle-index.c', 'spin-rectangle.c', 'texture-mapping.c']

  original_args = pargs
  foreach p : progs
    shader_name_prefix = p.substring(0,-2) # remove .c extension from name

    if p == 'triangle-index.c'
      shader_name_prefix = 'triangle'
    endif

    if p == 'texture-mapping.c'
      pargs += ['-DTEXTURE_IMAGE="' + examples_dir + '/textures/tuxedo-penguin.png"']
    endif

    pargs += [
      '-DVERTEX_SHADER_SPIRV='   + '"@0@'.format(shader_build_dir) + '/' + shader_name_prefix + '-shader-vert.spv"',
      '-DFRAGMENT_SHADER_SPIRV=' + '"@0@'.format(shader_build_dir) + '/' + shader_name_prefix + '-shader-frag.spv"',
    ]

    exec_name = 'underview-renderer-wayland-client-' + p.substring(0,-2) # remove .c extension from name
    executable(exec_name, p,
               link_with: lib_underview_renderer,
               dependencies: lib_uvr_deps,
               include_directories: [inc, examples_inc],
               c_args: pargs,
               install: false)

    pargs = original_args
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
