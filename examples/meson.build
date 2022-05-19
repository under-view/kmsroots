gpu = get_option('gpu')
if gpu == 'other'
  gputype='-DVK_PHYSICAL_DEVICE_TYPE=0'
elif gpu == 'integrated'
  gputype='-DVK_PHYSICAL_DEVICE_TYPE=1'
elif gpu == 'discrete'
  gputype='-DVK_PHYSICAL_DEVICE_TYPE=2'
elif gpu == 'virtual'
  gputype='-DVK_PHYSICAL_DEVICE_TYPE=3'
elif gpu == 'cpu'
  gputype='-DVK_PHYSICAL_DEVICE_TYPE=4'
endif

example_inc = include_directories('common')

if build_kms
  if libdrm.found()
    executable('underview-renderer-kms',
      'kms/main.c',
      link_with: lib_underview_renderer,
      dependencies: lib_uvr_deps,
      include_directories: [inc, example_inc],
      c_args: gputype,
      install: true
    )
  endif
endif


if build_xcb
  if libxcb.found() and libxcbewmh.found()
    executable('underview-renderer-xcb',
      'xcb/main.c',
      link_with: lib_underview_renderer,
      dependencies: lib_uvr_deps,
      include_directories: [inc, example_inc],
      c_args: gputype,
      install: true
    )
  endif
endif


if build_wayland
  if wayland_client.found()
    executable('underview-renderer-wayland',
      'wayland/main.c',
      link_with: lib_underview_renderer,
      dependencies: lib_uvr_deps,
      include_directories: [inc, example_inc],
      c_args: gputype,
      install: true
    )
  endif
endif
