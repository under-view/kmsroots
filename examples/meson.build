gpu = get_option('gpu')
if gpu == 'other'
  pargs += ['-DVK_PHYSICAL_DEVICE_TYPE=0']
elif gpu == 'integrated'
  pargs += ['-DVK_PHYSICAL_DEVICE_TYPE=1']
elif gpu == 'discrete'
  pargs += ['-DVK_PHYSICAL_DEVICE_TYPE=2']
elif gpu == 'virtual'
  pargs += ['-DVK_PHYSICAL_DEVICE_TYPE=3']
elif gpu == 'cpu'
  pargs += ['-DVK_PHYSICAL_DEVICE_TYPE=4']
endif

subdir('shaders')

example_inc = include_directories('common')

if libdrm.found() and libgbm.found()
  executable('underview-renderer-kms',
             'kms/main.c',
             link_with: lib_underview_renderer,
             dependencies: lib_uvr_deps,
             include_directories: [inc, example_inc],
             c_args: pargs,
             install: false)
endif


if libxcb.found() and libxcbewmh.found()
  executable('underview-renderer-xcb',
             'xcb/main.c',
             link_with: lib_underview_renderer,
             dependencies: lib_uvr_deps,
             include_directories: [inc, example_inc],
             c_args: pargs,
             install: false)
endif


if wayland_client.found()
  executable('underview-renderer-wayland',
             'wayland/wayland-client-main.c',
             link_with: lib_underview_renderer,
             dependencies: lib_uvr_deps,
             include_directories: [inc, example_inc],
             c_args: pargs,
             install: false)

  executable('underview-renderer-wayland-vk',
             'wayland/wayland-client-vk.c',
             link_with: lib_underview_renderer,
             dependencies: lib_uvr_deps,
             include_directories: [inc, example_inc],
             c_args: pargs,
             install: false)
endif


if wlroots.found() and wayland_server.found()
  executable('underview-renderer-wayland-comp',
             'wayland/wayland-compositor-main.c',
             link_with: lib_underview_renderer,
             dependencies: lib_uvr_deps,
             include_directories: [inc, example_inc],
             c_args: pargs,
             install: false)
endif
