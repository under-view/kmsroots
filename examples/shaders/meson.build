shaders = [
  'triangle-shader.vert'        ,  'triangle-shader.frag'        ,
  'spin-rectangle-shader.vert'  ,  'spin-rectangle-shader.frag'  ,
  'texture-mapping-shader.vert' ,  'texture-mapping-shader.frag' ,
  'model-load-shader.vert'      ,  'model-load-shader.frag'
]

spriv_byte_code = []

shader_build_dir = meson.current_build_dir()

glslangValidator = find_program('glslangValidator', required: true)
gen_spriv_byte_code = generator(glslangValidator,
                                capture: true,
                                output: '@PLAINNAME@',
                                arguments: ['-H', '@INPUT@', '-o', '@EXTRA_ARGS@'])
foreach shader : shaders
  if shader.contains('.vert')
    shader_extension = 'vert'
    substring_size = -5
  elif shader.contains('.frag')
    shader_extension = 'frag'
    substring_size = -5
  endif

  shader_name_prefix = shader.substring(0,substring_size) # remove .vert/.frag extension from name
  spriv_shader_name = shader_name_prefix + '-' + shader_extension + '.spv'

  infile = '@0@'.format(meson.current_source_dir()) + '/' + shader
  outfile = '@0@'.format(shader_build_dir) + '/' + spriv_shader_name

  spriv_byte_code += [gen_spriv_byte_code.process(infile, extra_args: outfile)]
endforeach

index=0
foreach shader : shaders
  if (index == shaders.length()) # Fixes array out of bounds issues
    break
  endif

  example_deps += declare_dependency(sources:
    [
      spriv_byte_code[index],
      spriv_byte_code[index + 1]
    ]
  )

  index += 2
endforeach
