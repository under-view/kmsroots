# libunderview-renderer

[wlroots](https://gitlab.freedesktop.org/wlroots/wlroots) wrapper library with an out-of-tree vulkan based renderer.

For better dependency version control underview builds all packages required from source. Follow
[build-underview-depends](https://github.com/under-view/build-underview-depends) repo README to get
going.

**Building**
```sh
$ meson -Dkms=true -Dxcb=true -Dwayland=false -Dgpu="integrated" build
$ ninja -C build
```

**Running Examples**
```sh
$ ./build/examples/underview-renderer-xcb
$ ./build/examples/underview-renderer-wayland
$ WLR_BACKENDS="drm" ./build/examples/underview-renderer-wayland-comp

# https://github.com/swaywm/wlroots/wiki/DRM-Debugging
# Enable verbose DRM logging
$ echo "0x19F" | sudo tee "/sys/module/drm/parameters/debug"
# Clear kernel logs
$ sudo dmesg -C
$ dmesg -w > "underview.log" &
$ ./build/examples/underview-renderer-kms
```
