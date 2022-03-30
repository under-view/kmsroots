# libunderview-renderer

Library for building X11, Wayland, & KMS Vulkan Applications

**Dependencies (Debian)**
```sh
$ sudo apt install -y aptitude
$ sudo aptitude install -y python3-pip pkg-config libdrm-dev libgbm-dev libudev-dev libsystemd-dev \
                           vulkan-validationlayers  ninja-build libxcb-dev libxcb-ewmh-dev \
                           wayland-protocols libwayland-client0 libwayland-bin libwayland-dev
$ sudo python3 -m pip install meson
```

**Building**
```sh
$ mkdir build
$ meson build
$ ninja -C build
```

**Running Examples**
```sh
$ ./build/examples/underview-renderer-x11
$ ./build/examples/underview-renderer-wayland
$ ./build/examples/underview-renderer-kms
```
