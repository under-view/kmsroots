#ifndef UVR_WSERVER
#define UVR_WSERVER

#include "utils.h"

#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/util/log.h>


/*
 * struct uvr_ws_core (Underview Renderer Wayland Server Core)
 *
 * members:
 * @display - Represents a connection to the compositor and acts as a proxy to the wl_display singleton object.
 * @backend - Represents a connection to any given backend {DRM/KMS, Wayland, X11, Headless} implementation.
 *            Utilized by wlroots when initializing compositor outputs.
 */
struct uvr_ws_core {
  struct wl_display  *display;
  struct wlr_backend *backend;
};


/*
 * struct uvr_ws_core_create_info (Underview Renderer Wayland Server Core Create Info)
 *
 * members:
 * @inc_wlr_debug_logs - Determines whether or not wlroots DEBUG logs are enabled/disabled
 */
struct uvr_ws_core_create_info {
  bool inc_wlr_debug_logs;
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
