Build kmsroots
==============

Meson
~~~~~

.. code-block::
        :linenos:

        $ meson setup [options] build
        $ ninja -C build -j$(nproc)
        $ sudo ninja -C build install

**Meson Options**

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

**Use with your Meson project**

.. code-block::
        :linenos:

        # Clone kmsroots or create a kmsroots.wrap under <source_root>/subprojects
        project('name', 'c')

        kmsroots_dep = dependency('kmsroots', fallback : 'kmsroots', 'kmsroots_dep')

        executable('exe', 'src/main.c', dependencies : kmsroots_dep)

Documentation (Sphinx)
~~~~~~~~~~~~~~~~~~~~~~

kmsroots uses sphinx framework for documentation. Primarily utilizing `The C Domain`_.

https://www.sphinx-doc.org/en/master/man/sphinx-build.html

**Dependencies**

- python3-pip

.. code-block::

        $ git clone https://github.com/under-view/kmsroots.git
        $ cd kmsroots
        $ sudo pip3 install -r docs/requirements.txt

        # If no build directory exists
        $ meson setup -Ddocs=true build

        # If build directory exists
        $ meson configure -Ddocs=true build

	$ ninja docs -C build

.. _The C Domain: https://www.sphinx-doc.org/en/master/usage/restructuredtext/domains.html#the-c-domain
