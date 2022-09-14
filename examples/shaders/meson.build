shaders = [
  'triangle-shader.vert'        ,  'triangle-shader.frag'        ,
  'spin-rectangle-shader.vert'  ,  'spin-rectangle-shader.frag'  ,
  'texture-mapping-shader.vert' ,  'texture-mapping-shader.frag' ,
  'model-load-shader.vert'      ,  'model-load-shader.frag'
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
                                  '-o', '@0@'.format(meson.current_build_dir()) + '/' + shader_name + '.spv', check: false)
endforeach

shader_build_dir = meson.current_build_dir()
