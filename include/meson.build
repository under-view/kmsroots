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

  headers = ''.join([
    '#include "drm-node.h"\n',
    '#include "buffer.h"\n',
    '#include "dma-buf.h"\n',
    '#include "pixel-format.h"',
  ])

  main_headers_dict += {'KMS_INTERFACE': headers}
endif

if shaderc.enabled()
  main_headers += ['shader.h']

  headers = '#include "shader.h"'

  main_headers_dict += {'SHADERC_INTERFACE': headers}
endif

conf_data = configuration_data(main_headers_dict)

conf = configure_file(input: 'kmsroots.h.in',
                      output: 'kmsroots.h',
                      configuration: conf_data,
                      install: false)

main_headers += [conf]
install_headers(main_headers, subdir: 'kmsroots')
