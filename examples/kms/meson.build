
if libdrm.found() and libgbm.found()
  progs = ['double-buffer-atomic-modeset.c']

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

if wlroots.found() and wayland_server.found()
  progs = ['model-load.c']

  original_args = pargs
  foreach p : progs
    shader_name_prefix = p.substring(0,-2) # remove .c extension from name

    if p == 'model-load.c'
      pargs += [
        '-DGLTF_MODEL="' + build_textures_dir + '/data/models/FlightHelmet/glTF/FlightHelmet.gltf"',
      ]
    endif

    if shaderc.disabled()
      pargs += [
        '-DVERTEX_SHADER_SPIRV='   + '"@0@'.format(shader_build_dir) + '/' + shader_name_prefix + '-shader-vert.spv"',
        '-DFRAGMENT_SHADER_SPIRV=' + '"@0@'.format(shader_build_dir) + '/' + shader_name_prefix + '-shader-frag.spv"',
      ]
    endif

    exec_name='underview-renderer-kms-' + p.substring(0,-2) # remove .c extension from name
    executable(exec_name, p,
               link_with: lib_underview_renderer,
               dependencies: example_deps,
               include_directories: [inc],
               c_args: pargs,
               install: false)

    pargs = original_args
  endforeach
endif
