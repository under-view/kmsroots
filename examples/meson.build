gpu = get_option('gpu')
if gpu == 'integrated'
  pargs += ['-DVK_PHYSICAL_DEVICE_TYPE=1']
elif gpu == 'discrete'
  pargs += ['-DVK_PHYSICAL_DEVICE_TYPE=2']
elif gpu == 'cpu'
  pargs += ['-DVK_PHYSICAL_DEVICE_TYPE=4']
endif

examples_dir = meson.current_source_dir()

subdir('shaders')
subdir('kms')
subdir('wayland')
subdir('xcb')
