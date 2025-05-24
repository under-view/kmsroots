# Needed to store corresponding C glue code
# from wayland xml protocls extension files
cglue_code = []

wayland_client = dependency('wayland-client', required: true)
wayland_scanner = find_program('wayland-scanner', required: true)
wayland_protos = dependency('wayland-protocols', required: true)

examples_deps += [wayland_client]

progs = [
  'shm-buffer.c', 'triangle.c', 'triangle-index.c',
  'spin-rectangle.c', 'texture-mapping.c', 'model-load.c'
]

wayland_protos_dir = wayland_protos.get_variable(pkgconfig: 'pkgdatadir')
wayland_protos = [
  [wayland_protos_dir, 'stable/xdg-shell/xdg-shell.xml'],
  [wayland_protos_dir, 'unstable/fullscreen-shell/fullscreen-shell-unstable-v1.xml'],
]

# wayland-scanner process these wayland protocols defined
# in XML files and generate code from them
# processes xdg-shell.xml defines all the interfaces
# supported by a Wayland client.
wayland_scanner_code = generator(wayland_scanner,
  output: '@BASENAME@-protocol.c',
  arguments: ['private-code', '@INPUT@', '@OUTPUT@'],
)

wayland_scanner_client = generator(wayland_scanner,
  output: '@BASENAME@-client-protocol.h',
  arguments: ['client-header', '@INPUT@', '@OUTPUT@'],
)

# Create corresponding C glue code from wayland xml protocls extension files
foreach p : wayland_protos
  xml = join_paths(p)
  cglue_code += [
    wayland_scanner_client.process(xml),
    wayland_scanner_code.process(xml)
  ]
endforeach

original_args = pargs
foreach p : progs
  shader_name_prefix = p.substring(0,-2) # remove .c extension from name

  if p == 'triangle-index.c'
    shader_name_prefix = 'triangle'
  endif

  if p == 'texture-mapping.c'
    pargs += ['-DTEXTURE_IMAGE="' + examples_dir + '/textures/tuxedo-penguin.png"']
  endif

  if p == 'model-load.c'
    pargs += [
      '-DGLTF_MODEL="' + build_textures_dir + '/data/models/FlightHelmet/glTF/FlightHelmet.gltf"',
    ]
  endif

  if shaderc.disabled()
    pargs += [
      '-DVERTEX_SHADER_SPIRV='   + '"@0@'.format(shader_build_dir) + '/' + shader_name_prefix + '-shader-vert.spv"',
      '-DFRAGMENT_SHADER_SPIRV=' + '"@0@'.format(shader_build_dir) + '/' + shader_name_prefix + '-shader-frag.spv"',
    ]
  endif

  exec_name = 'kmsroots-wayland-client-' + p.substring(0,-2) # remove .c extension from name
  executable(exec_name,
             cglue_code,
             files([p,'wclient.c']),
             link_with: lib_kmsroots,
             dependencies: example_deps,
             include_directories: [inc],
             c_args: pargs,
             install: false)

  pargs = original_args
endforeach
