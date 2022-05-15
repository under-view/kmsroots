# libunderview-renderer

Library for building X11, Wayland, & KMS Vulkan Applications

For better dependencies version control underview builds all packages required from source. Follow
[build-underview-depends](https://github.com/under-view/build-underview-depends) repo README to get
going.

**Building**
```sh
$ meson -Dkms=true -Dxcb=true -Dwayland=false build
$ ninja -C build
```

**Running Examples**
```sh
$ ./build/examples/underview-renderer-xcb
$ ./build/examples/underview-renderer-wayland
$ ./build/examples/underview-renderer-kms
```
