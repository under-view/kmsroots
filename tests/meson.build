progs = [ ]

original_args = pargs
foreach p : progs
  exec_name = 'test-' + p.substring(0,-2) # remove .c extension from name
  executable(exec_name, p,
            link_with: lib_underview_renderer,
            dependencies: lib_uvr_deps,
            include_directories: [inc],
            c_args: pargs,
            install: false)
  pargs = original_args
endforeach
