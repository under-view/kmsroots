progs = [ 'gltf-file-loading.c' ]

if shaderc.enabled()
  progs += ['shader-buffer-load.c']
else
  progs += ['shader-file-load.c']
endif

if kms.enabled()
  progs += ['simple-vk-kms.c']
endif

original_args = pargs
foreach p : progs
  exec_name = 'test-' + p.substring(0,-2) # remove .c extension from name

  if p == 'shader-file-load.c' and shaderc.disabled()
    pargs += [
      '-DVERTEX_SHADER_SPIRV='   + '"@0@'.format(shader_build_dir) + '/' + 'triangle-shader-vert.spv"',
      '-DFRAGMENT_SHADER_SPIRV=' + '"@0@'.format(shader_build_dir) + '/' + 'triangle-shader-frag.spv"',
    ]
  endif

  if p == 'gltf-file-loading.c'
    pargs += [
      '-DGLTF_MODEL="' + build_textures_dir + '/data/models/FlightHelmet/glTF/FlightHelmet.gltf"',
    ]
  endif

  exec = executable(exec_name, p,
                    link_with: lib_kmsroots,
                    dependencies: lib_kmr_deps,
                    include_directories: [inc],
                    c_args: pargs,
                    install: false)

  if p == 'shader-file-load.c' or p == 'shader-buffer-load.c'
    test(exec_name, exec, suite: 'shader')
  else
    test(exec_name, exec)
  endif

  pargs = original_args
endforeach
