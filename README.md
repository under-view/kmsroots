# kmsroots

Library used to assists in building applications that require vulkan renderers. This implementation assists in displaying vulkan render pass
final framebuffers either directly to display via KMS atomic API or to one of the other linux display server clients (xcb, wayland). May also assists
any [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots) based compositor build their [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots)
compatible out-of-tree vulkan renderers.

**NOTE:** kmsroots KMS atomic page-flip implementation will only support a single output device (useful for kiosk's).
If more output devices required kmsroots may be used in conjunction with [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots).

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
# Discrete GPU (Normally test with AMDGPU [Radeon 6600])
$ meson setup -Dgpu="discrete" \
              -Dexamples="true" \
              -Dtests="true" \
              -Ddebugging="enabled" \
              -Dshaderc="disabled" \
              -Dxcb="enabled" \
              -Dwayland="enabled" \
              -Dkms="enabled" \
              -Dlibseat="enabled" \
              -Dlibinput="enabled" \
              build

# Embedded GPU (Normally test on the Udoo Bolt v3)
$ meson setup -Dgpu="integrated" \
              -Dexamples="true" \
              -Dtests="true" \
              -Ddebugging="disabled" \
              -Dshaderc="disabled" \
              -Dxcb="enabled" \
              -Dwayland="enabled" \
              -Dkms="enabled" \
              -Dlibseat="enabled" \
              -Dlibinput="enabled" \
              build

$ meson compile -C build
```

**Running Examples**
```sh
# Examples can exit with either CTRL-C, ESC, or Q.
$ ./build/examples/xcb/kmsroots-xcb-client-*
$ ./build/examples/wayland/kmsroots-wayland-client-*
$ ./build/examples/wayland/kmsroots-kms-*

# https://github.com/swaywm/wlroots/wiki/DRM-Debugging
# Enable verbose DRM logging
$ echo "0x19F" | sudo tee "/sys/module/drm/parameters/debug"
```

**Testing**

```sh
$ meson test -C build/
```
