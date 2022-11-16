#ifndef UVR_WSERVER
#define UVR_WSERVER

#include "utils.h"
#include <stdbool.h>

/*
 * struct uvr_ws_core (Underview Renderer Wayland Server Core)
 *
 * members:
 * @wlDisplay              - Wayland Display object is managed by libwayland. It handles accepting
 *                           clients from the Unix socket '$XDG_RUNTIME_DIR/(wayland display unix socket name)',
 *                           managing Wayland globals, and so on. XDG_RUNTIME_DIR="/run/user/1000"
 * @wlrBackend             - A backend provides a set of input and output devices. A display output chain suitable
 *                           for modesetting typically looks like fb (plane)->CRTC->encoder->connector.
 * @wlrRenderer            - Provides an instance to a renderer Implementation. Renderer implementations
 *                           contained in wlroots include Pixman, Vulkan, & GLES2.
 * @wlrRendererAllocator   - Stores create and destroy buffer function pointers, to some provided implemenatation.
 */
struct uvr_ws_core {
  struct wl_display    *wlDisplay;
  struct wlr_backend   *wlrBackend;
  struct wlr_renderer  *wlrRenderer;
  struct wlr_allocator *wlrRendererAllocator;
};


/*
 * struct uvr_ws_core_create_info (Underview Renderer Wayland Server Core Create Info)
 *
 * members:
 * @includeWlrDebugLogs - Determines whether or not wlroots DEBUG logs are enabled/disabled
 */
struct uvr_ws_core_create_info {
  bool includeWlrDebugLogs;
};


/*
 * uvr_ws_core_create: Creates a wayland display unix domain socket to communicate on and connects to
 *                     a given wlroots based backend {DRM/KMS, Wayland, X11, Headless} implementation.
 *                     Based upon current display environment. The backend is created here but not started.
 *
 * args:
 * @uvrws - pointer to a struct uvr_ws_core_create_info contains members that determine
 *          functions logic
 * return:
 *    on success struct uvr_ws_core
 *    on failure struct uvr_ws_core { with members nulled }
 */
struct uvr_ws_core uvr_ws_core_create(struct uvr_ws_core_create_info *uvrws);


/*
 * struct uvr_ws_destroy (Underview Renderer Wayland Server Destroy)
 *
 * members:
 * @uvr_ws_core - Must pass an array of valid struct uvr_ws_core { free'd members: struct wl_display ref, struct wlr_backend ref }
 */
struct uvr_ws_destroy {
  struct uvr_ws_core uvr_ws_core;
};


/*
 * uvr_ws_destroy: frees all allocated memory contained in struct uvrxcb
 *
 * args:
 * @uvrws - pointer to a struct uvr_ws_destroy contains addresses
 *          created during compositor lifetime in needed of destroying
 */
void uvr_ws_destroy(struct uvr_ws_destroy *uvrws);


#endif
