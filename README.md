# libunderview-renderer

Library for building X11, Wayland, & KMS Vulkan Applications

For better dependencies version control underview builds all packages required from source. Follow
[build-underview-depends](https://github.com/under-view/build-underview-depends) repo README to get
going.

**Building**
```sh
$ . ./config-env.sh "$(pwd)/../build-underview-depends/working/build_output"
$ meson -Dkms=true -Dxcb=true -Dwayland=false build
$ ninja -C build
```

**Running Examples**
```sh
$ ./build/examples/underview-renderer-xcb
$ ./build/examples/underview-renderer-wayland

# https://github.com/swaywm/wlroots/wiki/DRM-Debugging
# Enable verbose DRM logging
$ echo "0x19F" | sudo tee "/sys/module/drm/parameters/debug"
# Clear kernel logs
$ sudo dmesg -C
$ dmesg -w > "underview.log" &
$ ./build/examples/underview-renderer-kms
```
