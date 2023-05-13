gpu = get_option('gpu')
if gpu == 'integrated'
  pargs += ['-DVK_PHYSICAL_DEVICE_TYPE=1']
elif gpu == 'discrete'
  pargs += ['-DVK_PHYSICAL_DEVICE_TYPE=2']
elif gpu == 'cpu'
  pargs += ['-DVK_PHYSICAL_DEVICE_TYPE=4']
endif

if debuging.enabled()
  pargs += ['-DINCLUDE_VULKAN_VALIDATION_LAYERS=1']
endif

examples_dir = meson.current_source_dir()
example_deps = [lib_kmr_deps]

if shaderc.disabled()
  subdir('shaders')
endif

subdir('textures')
subdir('kms')
subdir('wayland')
subdir('xcb')
