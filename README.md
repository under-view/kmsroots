# libunderviewcomp

Library used to build Vulkan applications and compositors that utilize KMS directly.

**Dependencies (Debian)**
```sh
$ sudo apt install -y aptitude
$ sudo aptitude install -y python3-pip pkg-config libdrm-dev libgbm-dev libudev-dev libsystemd-dev vulkan-validationlayers  ninja-build libxcb-dev libxcb-ewmh-dev
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
$ ./build/examples/underviewcomp-x11
```
