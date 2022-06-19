# libunderview-renderer

[wlroots](https://gitlab.freedesktop.org/wlroots/wlroots) based compositing library with an out-of-tree vulkan based renderer. Can also we used to build
xcb and wayland clients.

For better dependency version control underview builds all packages required from source. Follow
[build-underview-depends](https://github.com/under-view/build-underview-depends) repo README to get
going.

**Building**

All features are disabled by default

```sh
$ meson setup -Dgpu="discrete" \
              -Dshaderc="enabled" \
              -Dxcb="enabled" \
              -Dwayland="enabled" \
              -Dwayland-compositor="enabled" \
              build
$ meson compile -C build
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
