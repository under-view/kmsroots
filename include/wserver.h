#ifndef UVR_WSERVER
#define UVR_WSERVER

#include "utils.h"
#include <stdbool.h>

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/allocator.h>
#include <wlr/render/interface.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_output_layout.h>


/*
 * struct uvr_ws_core (Underview Renderer Wayland Server Core)
 *
 * members:
 * @kmsfd                  - KMS node file descriptor associated with a given backend.
 * @wlDisplay              - Wayland Display object is managed by libwayland. It handles accepting
 *                           clients from the Unix socket '$XDG_RUNTIME_DIR/(wayland display unix socket name)',
 *                           managing Wayland globals, and so on. XDG_RUNTIME_DIR="/run/user/1000"
 * @wlrBackend             - A backend provides a set of input and output devices. A display output chain suitable
 *                           for modesetting typically looks like fb (plane)->CRTC->encoder->connector.
 * @wlrRenderer            - Provides an instance to a renderer Implementation. Renderer implementations
 *                           contained in wlroots include Pixman, Vulkan, & GLES2.
 * @wlrRendererAllocator   - Stores create and destroy buffer function pointers, to some provided implemenatation.
 * @wlrSceneGraph          - Pointer to compositor scene graph. This is a wlroots abstraction that handles all
 *                           rendering and damage tracking.
 * @wlrOutputLayout        - A wlroots utility for working with an arrangement of screens in a physical layout.
 * @outputList             - Doubly Linked list of display output devices
 * @newOutputListener      - Listener used to notify when a new output is connected
 */
struct uvr_ws_core {
	int                      kmsfd;
	struct wl_display        *wlDisplay;
	struct wlr_backend       *wlrBackend;
	struct wlr_renderer      *wlrRenderer;
	struct wlr_allocator     *wlrRendererAllocator;
	struct wlr_scene         *wlrSceneGraph;

	struct wlr_output_layout *wlrOutputLayout;
	struct wl_list           outputList;
	struct wl_listener       newOutputListener;
};


/*
 * struct uvr_ws_core_create_info (Underview Renderer Wayland Server Core Create Info)
 *
 * members:
 * @includeWlrDebugLogs - Determines whether or not wlroots DEBUG logs are enabled/disabled
 * @unixSockName        - Determines that name of the unix domain socket that'll be created
 *                        and used to handle communication. If NULL is passed it will look
 *                        for WAYLAND_DISPLAY env variable for the socket name. The Unix socket
 *                        will be created in the directory pointed to by environment variable
 *                        XDG_RUNTIME_DIR. If XDG_RUNTIME_DIR is not set, then this function
 *                        fails.
 * @rendererImpl        - Way for applications to build custom wlroots renderer implementation.
 *                        Using both functions from src/renderers/underview.c and their own
 *                        implementations.
 */
struct uvr_ws_core_create_info {
	bool                     includeWlrDebugLogs;
	char                     *unixSockName;
#ifdef UNDERVIEW_RENDERER
	struct wlr_renderer_impl *rendererImpl;
#endif
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
 *	on success struct uvr_ws_core
 *	on failure struct uvr_ws_core { with members nulled }
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
