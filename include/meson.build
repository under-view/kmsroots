main_headers_dict = {}

main_headers = [
  'vulkan.h',
  'gltf-loader.h',
]

if kms.enabled()
  main_headers += [
    'drm-node.h',
    'buffer.h',
    'dma-buf.h',
    'pixel-format.h'
  ]

  kms_headers = ''.join([
    '#include "drm-node.h"\n',
    '#include "buffer.h"\n',
    '#include "dma-buf.h"\n',
    '#include "pixel-format.h"',
  ])

  main_headers_dict += {'KMS_INTERFACE': kms_headers}
endif

if libseat.enabled()
  main_headers += ['session.h']

  headers = '#include "session.h"'

  main_headers_dict += {'SESSION_INTERFACE': headers}
endif

if libinput.enabled()
  main_headers += ['input.h']

  headers = '#include "input.h"'

  main_headers_dict += {'INPUT_INTERFACE': headers}
endif

if shaderc.enabled()
  main_headers += ['shader.h']

  headers = '#include "shader.h"'

  main_headers_dict += {'SHADERC_INTERFACE': headers}
endif

if xcb.enabled()
  main_headers += ['xclient.h']

  headers = '#include "xclient.h"'

  main_headers_dict += {'X_INTERFACE': headers}
endif

if wayland.enabled()
  main_headers += ['wclient.h']

  headers = '#include "wclient.h"'

  main_headers_dict += {'WAYLAND_INTERFACE': headers}
endif

conf_data = configuration_data(main_headers_dict)

conf = configure_file(input: 'kmsroots.h.in',
                      output: 'kmsroots.h',
                      configuration: conf_data,
                      install: false)

main_headers += [conf]
install_headers(main_headers, subdir: 'kmsroots')
