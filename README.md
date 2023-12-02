# kmsroots

<p align="center">
    <a href="http://kmsroots.readthedocs.io/en/latest/?badge=latest">
        <img src="https://readthedocs.org/projects/kmsroots/badge/?version=latest" alt="Documentation Status">
    </a>
</p>

---

Library used to assists in building applications that require vulkan renderers. This implementation assists in displaying vulkan render pass
final framebuffers either directly to display via KMS atomic API or to one of the other linux display server clients (xcb, wayland). May also assists
any [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots) based compositor build their [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots)
compatible out-of-tree vulkan renderers.

**NOTE:** kmsroots KMS atomic page-flip implementation will only support a single output device (useful for kiosk's).
If more output devices required kmsroots may be used in conjunction with [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots).

#### 📚 Documentation

See documentation: https://kmsroots.readthedocs.io for build and install.

**Tested Distro's**
- Ubuntu 20.04
- Ubuntu 22.04
- Arch Linux ([sway](https://github.com/swaywm/sway))
