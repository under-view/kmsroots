
run_command('glslangValidator', '-H', '@0@'.format(meson.current_source_dir()) + '/triangle-shader.vert',
                                '-o', '@0@'.format(meson.current_build_dir()) + '/triangle-vert.spv', check: true)

run_command('glslangValidator', '-H', '@0@'.format(meson.current_source_dir()) + '/triangle-shader.frag',
                                '-o', '@0@'.format(meson.current_build_dir()) + '/triangle-frag.spv', check: true)

pargs += [
  '-DTRIANGLE_VERTEX_SHADER_SPIRV=' + '"@0@'.format(meson.current_build_dir()) + '/triangle-vert.spv"',
  '-DTRIANGLE_FRAGMENT_SHADER_SPIRV=' + '"@0@'.format(meson.current_build_dir()) + '/triangle-frag.spv"'
]
