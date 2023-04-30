# libunderview-renderer

Library used to help build vulkan renderers that display render pass rendered framebuffers directly with DRM/KMS. Can also
be used to build xcb and wayland clients. Can also assists any [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots)
based compositors build their wlroots compatible out-of-tree vulkan renderers and DRM backends.

For better dependency version control underview builds all packages required from source. Follow
[build-underview-depends](https://github.com/under-view/build-underview-depends) repo README to get
going. Run the `setenvars.sh` to setup necessary enviroment variables for building.

**Tested Distro's**
- Ubuntu 20.04
- Ubuntu 22.04
- Arch Linux ([sway](https://github.com/swaywm/sway))

**Building**

All features are disabled by default

```sh
# Discrete GPU
$ meson setup -Dgpu="discrete" \
              -Dexamples="true" \
              -Dtests="true" \
              -Ddebugging="enabled" \
              -Dshaderc="enabled" \
              -Dxcb="enabled" \
              -Dwayland="enabled" \
              -Dwayland-compositor="enabled" \
              -Dkms="enabled" \
              -Dsd-bus="enabled" \
              build

# Embedded GPU (Normally test on the Udoo Bolt v3)
$ meson setup -Dgpu="integrated" \
              -Dexamples="true" \
              -Dtests="true" \
              -Ddebugging="disabled" \
              -Dshaderc="disabled" \
              -Dxcb="enabled" \
              -Dwayland="enabled" \
              -Dwayland-compositor="enabled" \
              -Dkms="enabled" \
              -Dsd-bus="enabled" \
              build

$ meson compile -C build
```

**Running Examples**
```sh
# Client examples
$ ./build/examples/xcb/underview-renderer-xcb-client-*
$ ./build/examples/wayland/underview-renderer-wayland-client-*
$ ./build/examples/wayland/underview-renderer-kms-*

# https://github.com/swaywm/wlroots/wiki/DRM-Debugging
# Enable verbose DRM logging
$ echo "0x19F" | sudo tee "/sys/module/drm/parameters/debug"
```

**Testing**

```sh
$ meson test -C build/
```
