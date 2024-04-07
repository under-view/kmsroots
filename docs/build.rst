Dependencies
============

For better dependency version control underview builds all packages required from source.

Build Underview Depends
~~~~~~~~~~~~~~~~~~~~~~~

Follow `build-underview-depends`_ repo README to get going.

.. code-block:: bash

	$ git clone https://github.com/under-view/build-underview-depends.git
	$ source setenvars.sh

Yocto Project SDK
~~~~~~~~~~~~~~~~~

Download SDK from `build-underview-depends (releases)`_

.. code-block:: bash

	$ ./x86_64-0.1.0-underview.sh
	sdk # Folder to place libs
	$ source environment-setup-zen1-underview-linux

Meson
=====

Options
~~~~~~~

All options/features are disabled by default.

.. code-block::
        :linenos:

        c_std=c11
        buildtype=release
        default_library=shared
        gpu=integrated    # Default [discrete] Options: [integrated, discrete, cpu]
        kms=enabled       # Default [disabled]
        libseat=enabled   # Default [disabled]
        libinput=enabled  # Default [disabled]
        xcb=enabled       # Default [disabled]
        wayland=enabled   # Default [disabled]
        shaderc=enabled   # Default [disabled]
        debugging=enabled # Default [disabled]
        examples=true     # Default [false]
        tests=true        # Default [false]
        docs=true         # Default [false]

Build (Normal)
~~~~~~~~~~~~~~

.. code-block:: bash

        $ meson setup [options] build
        $ ninja -C build -j$(nproc)

.. code-block:: bash

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

.. code-block:: bash

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

Build (SDK)
~~~~~~~~~~~

.. code-block:: bash

	# For Yocto SDK builds
	$ meson setup --prefix="${SDKTARGETSYSROOT}/usr" \
	              --libdir="${SDKTARGETSYSROOT}/usr/lib64" \
		      [options] \
	              build
        $ ninja -C build -j$(nproc)

.. code-block:: bash

	# Discrete GPU (Normally test with AMDGPU [Radeon 6600])
	$ meson setup --prefix="${SDKTARGETSYSROOT}/usr" \
	              --libdir="${SDKTARGETSYSROOT}/usr/lib64" \
	              -Dgpu="discrete" \
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

.. code-block:: bash

	# Embedded GPU (Normally test on the Udoo Bolt v3)
	$ meson setup --prefix="${SDKTARGETSYSROOT}/usr" \
	              --libdir="${SDKTARGETSYSROOT}/usr/lib64" \
	              -Dgpu="integrated" \
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

Include
~~~~~~~

.. code-block:: meson
        :linenos:

        # Clone kmsroots or create a kmsroots.wrap under <source_root>/subprojects
        project('name', 'c')

        kmsroots_dep = dependency('kmsroots', fallback : 'kmsroots', 'kmsroots_dep')

        executable('exe', 'src/main.c', dependencies : kmsroots_dep)

Documentation (Sphinx)
======================

kmsroots uses sphinx framework for documentation. Primarily utilizing `The C Domain`_.

https://www.sphinx-doc.org/en/master/man/sphinx-build.html

Dependencies
~~~~~~~~~~~~

- python3-pip

Build Docs
~~~~~~~~~~

.. code-block:: bash

        $ git clone https://github.com/under-view/kmsroots.git
        $ cd kmsroots
        $ sudo pip3 install -r docs/requirements.txt

        # If no build directory exists
        $ meson setup -Ddocs=true build

        # If build directory exists
        $ meson configure -Ddocs=true build

	$ ninja docs -C build

Running Examples
================

Normal
~~~~~~

.. code-block:: bash

	# Examples can exit with either CTRL-C, ESC, or Q.
	$ ./build/examples/xcb/kmsroots-xcb-client-*
	$ ./build/examples/wayland/kmsroots-wayland-client-*
	$ ./build/examples/wayland/kmsroots-kms-*

	# https://github.com/swaywm/wlroots/wiki/DRM-Debugging
	# Enable verbose DRM logging
	$ echo "0x19F" | sudo tee "/sys/module/drm/parameters/debug"

Yocto Project SDK
~~~~~~~~~~~~~~~~~

.. code-block:: bash

	# Vulkan loader can't find ic driver set LD_LIBRARY_PATH so it's discoverable
	# Examples can exit with either CTRL-C, ESC, or Q.
	$ LD_LIBRARY_PATH="${SDKTARGETSYSROOT}/usr/lib64" ./build/examples/xcb/kmsroots-xcb-client-*
	$ LD_LIBRARY_PATH="${SDKTARGETSYSROOT}/usr/lib64" ./build/examples/wayland/kmsroots-wayland-client-*
	$ LD_LIBRARY_PATH="${SDKTARGETSYSROOT}/usr/lib64" ./build/examples/wayland/kmsroots-kms-*

.. _build-underview-depends: https://github.com/under-view/build-underview-depends
.. _build-underview-depends (releases): https://github.com/under-view/build-underview-depends/releases
.. _The C Domain: https://www.sphinx-doc.org/en/master/usage/restructuredtext/domains.html#the-c-domain
