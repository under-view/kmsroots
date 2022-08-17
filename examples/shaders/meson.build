shaders = [
  'triangle-shader.vert', 'triangle-shader.frag',
  'spin-rectangle-shader.vert', 'spin-rectangle-shader.frag'
]

foreach shader : shaders
  if shader.contains('.vert')
    shader_extension = 'vert'
    substring_size = -5
  elif shader.contains('.frag')
    shader_extension = 'frag'
    substring_size = -5
  endif

  shader_name = shader.substring(0,substring_size) + '-' + shader_extension

  run_command('glslangValidator', '-H', '@0@'.format(meson.current_source_dir()) + '/' + shader,
                                  '-o', '@0@'.format(meson.current_build_dir()) + '/' + shader_name + '.spv', check: true)
endforeach

# Set project args to 'name of shader file - type.spv'
pargs += [
  '-DTRIANGLE_VERTEX_SHADER_SPIRV='           + '"@0@'.format(meson.current_build_dir()) + '/triangle-shader-vert.spv"',
  '-DTRIANGLE_FRAGMENT_SHADER_SPIRV='         + '"@0@'.format(meson.current_build_dir()) + '/triangle-shader-frag.spv"',
  '-DTRIANGLE_UNIFORM_VERTEX_SHADER_SPIRV='   + '"@0@'.format(meson.current_build_dir()) + '/spin-rectangle-shader-vert.spv"',
  '-DTRIANGLE_UNIFORM_FRAGMENT_SHADER_SPIRV=' + '"@0@'.format(meson.current_build_dir()) + '/spin-rectangle-shader-frag.spv"'
]
