# libunderview-renderer

If i'm correct can eventually turn this into a render for a [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots) based
compositor, however there may still be work to allow for out of tree renderers.

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

# https://github.com/swaywm/wlroots/wiki/DRM-Debugging
# Enable verbose DRM logging
$ echo "0x19F" | sudo tee "/sys/module/drm/parameters/debug"
# Clear kernel logs
$ sudo dmesg -C
$ dmesg -w > "underview.log" &
$ ./build/examples/underview-renderer-kms
```
