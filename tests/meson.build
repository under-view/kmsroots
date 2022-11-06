progs = [ 'shader-file-load.c', 'shader-buffer-load.c' ]

original_args = pargs
foreach p : progs
  exec_name = 'test-' + p.substring(0,-2) # remove .c extension from name

  if p == 'shader-file-load.c'
    pargs += [
      '-DVERTEX_SHADER_SPIRV='   + '"@0@'.format(shader_build_dir) + '/' + 'triangle-shader-vert.spv"',
      '-DFRAGMENT_SHADER_SPIRV=' + '"@0@'.format(shader_build_dir) + '/' + 'triangle-shader-frag.spv"',
    ]
  endif

  exec = executable(exec_name, p,
                    link_with: lib_underview_renderer,
                    dependencies: lib_uvr_deps,
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