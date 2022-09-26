build_textures_dir = meson.current_build_dir()
vulkan_asset_pack = 'vulkan_asset_pack_gltf.zip'
vulkan_asset_pack_url = 'http://vulkan.gpuinfo.org/downloads'
vulkan_asset_pack_absolute_path = build_textures_dir + '/' + vulkan_asset_pack

if run_command('[', '-f', vulkan_asset_pack_absolute_path, ']').returncode() != 0
  warning('Running "wget ' + vulkan_asset_pack_url + '/' + vulkan_asset_pack + '"')

  run_command('wget',
              vulkan_asset_pack_url + '/' + vulkan_asset_pack,
              '-O',
              vulkan_asset_pack_absolute_path,
              check: true)
endif

if run_command('[', '-d', build_textures_dir + '/data', ']').returncode() != 0
  warning('Running "unzip ' + vulkan_asset_pack_absolute_path + '"')

  run_command('unzip',
              '-o',
              vulkan_asset_pack_absolute_path,
              '-d',
              build_textures_dir,
              check: true)
endif
