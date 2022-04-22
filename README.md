# libunderview-renderer

Library for building X11, Wayland, & KMS Vulkan Applications

**Dependencies (Debian)**
```sh
$ sudo apt install -y aptitude
# Build depends
$ sudo aptitude install -y build-essential python3 python3-pip pkg-config ninja-build
$ sudo python3 -m pip install meson
# Vulkan depends
$ sudo aptitude install -y libvulkan-dev vulkan-validationlayers vulkan-utils vulkan-tools
# XCB depends
$ sudo aptitude install -y libxcb1-dev libxcb-ewmh-dev
# Wayland depends
$ sudo aptitude install -y wayland-protocols libwayland-client0 libwayland-bin libwayland-dev
# KMS depends
$ sudo aptitude install -y libdrm-dev libgbm-dev libudev-dev libsystemd-dev
```

**Building**
```sh
$ mkdir build
$ meson -Dkms=true -Dxcb=true -Dwayland=false build
$ ninja -C build
```

**Running Examples**
```sh
$ ./build/examples/underview-renderer-x11
$ ./build/examples/underview-renderer-wayland
$ ./build/examples/underview-renderer-kms
```
