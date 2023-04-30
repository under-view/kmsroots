main_headers = [
  'vulkan.h', 'utils.h', 'gltf-loader.h'
]

if get_option('kms').enabled()
  main_headers += ['kms.h', 'buffer.h']
endif

if get_option('sd-bus').enabled()
  main_headers += ['sd-dbus.h']
endif

if get_option('shaderc').enabled()
  main_headers += ['shader.h']
endif

if get_option('xcb').enabled()
  main_headers += ['xclient.h']
endif

if get_option('wayland').enabled()
  main_headers += ['wclient.h']
endif

if get_option('wayland-compositor').enabled()
  main_headers += ['wserver.h']
endif

install_headers(main_headers, subdir: 'underview')
