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
subdir('kms')
subdir('wayland')
subdir('xcb')
