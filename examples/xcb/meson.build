
if libxcb.found() and libxcbewmh.found()
  progs = [
    'triangle.c', 'triangle-index.c', 'spin-rectangle.c',
    'texture-mapping.c', 'model-load.c'
  ]

  original_args = pargs
  foreach p : progs
    shader_name_prefix = p.substring(0,-2) # remove .c extension from name

    if p == 'triangle-index.c'
      shader_name_prefix = 'triangle'
    endif

    if p == 'texture-mapping.c'
      pargs += ['-DTEXTURE_IMAGE="' + examples_dir + '/textures/snorlax-thinking.png"']
    endif

    if p == 'model-load.c'
      pargs += [
        '-DTEXTURE_IMAGE="' + examples_dir + '/textures/snorlax-thinking.png"',
        '-DGLTF_MODEL="' + build_textures_dir + '/data/models/FlightHelmet/glTF/FlightHelmet.gltf"',
      ]
    endif

    pargs += [
      '-DVERTEX_SHADER_SPIRV='   + '"@0@'.format(shader_build_dir) + '/' + shader_name_prefix + '-shader-vert.spv"',
      '-DFRAGMENT_SHADER_SPIRV=' + '"@0@'.format(shader_build_dir) + '/' + shader_name_prefix + '-shader-frag.spv"',
    ]

    exec_name = 'underview-renderer-xcb-client-' + p.substring(0,-2) # remove .c extension from name
    executable(exec_name, p,
               link_with: lib_underview_renderer,
               dependencies: lib_uvr_deps,
               include_directories: [inc],
               c_args: pargs,
               install: false)

    pargs = original_args
  endforeach
endif
