Build kmsroots
================================

Meson
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block::
        :linenos:

        $ meson setup [options] build
        $ ninja -C build -j$(nproc)
        $ sudo ninja -C build install

**Meson Options:**

All options/features are disabled by default.

.. code-block::
        :linenos:

        c_std=c11
        buildtype=release
        default_library=shared
        kms=enabled       # Default [disabled]
        libseat=enabled   # Default [disabled]
        libinput=enabled  # Default [disabled]
        xcb=enabled       # Default [disabled]
        wayland=enabled   # Default [disabled]
        shaderc=enabled   # Default [disabled]
        examples=true     # Default [false]
        tests=true        # Default [false]
        debugging=enabled # Default [disabled]
        gpu=integrated    # Default [discrete] Options: [integrated, discrete, cpu]

**Use with your Meson project**

.. code-block::
        :linenos:

        # Clone kmsroots or create a kmsroots.wrap under <source_root>/subprojects
        project('name', 'c')

        kmsroots_dep = dependency('kmsroots', fallback : 'kmsroots', 'kmsroots_dep')

        executable('exe', 'src/main.c', dependencies : kmsroots_dep)

Docs
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**Dependencies**

- python3-pip
- python3-clang

.. code-block::

        $ git clone https://github.com/under-view/kmsroots.git
        $ cd kmsroots/docs
        $ sudo pip3 install -r requirements.txt

