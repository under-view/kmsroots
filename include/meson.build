main_headers = [
  'vulkan.h', 'utils.h', 'gltf-loader.h'
]

if get_option('kms').enabled()
  main_headers += ['drm-node.h', 'buffer.h', 'dma-buf.h', 'pixel-format.h']
endif

if get_option('libseat').enabled()
  main_headers += ['session.h']
endif

if get_option('libinput').enabled()
  main_headers += ['input.h']
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

install_headers(main_headers, subdir: 'kmsroots')
