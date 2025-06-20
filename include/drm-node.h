#ifndef KMR_DRM_NODE_H
#define KMR_DRM_NODE_H

#include <stdbool.h>

#ifdef INCLUDE_LIBSEAT
#include "session.h"
#endif /* INCLUDE_LIBSEAT */

/*
 * Stores information about the kmr_drm_node instance.
 */
struct kmr_drm_node;


/*
 * @brief struct kmr_drm_node_create_info (kmsroots DRM Node Create Information)
 *
 * @member kms_node - Path to character device associated with GPU.
 *                   If set to NULL. List of available kmsnode's will
 *                   be queried and one will be automatically choosen
 *                   for you.
 * @member session - Address of struct kmr_session. Which members are
 *                   used to communicate with systemd-logind via D-Bus
 *                   systemd-logind interface. Needed by kmr_drm_node_create(3)
 *                   to acquire and taken control of a device without the
 *                   need of being root.
 */
struct kmr_drm_node_create_info
{
	const char *kms_node;
	const void *session;
};


/*
 * @brief Function opens a DRI device node. If a systemd-logind session available
 *        one can take control of a device node. Returned fd is exposed to all
 *        planes (overlay, primary, and cursor) and has access to the aspect ratio
 *        information in modes in userspace. In order to drive KMS, we need to be
 *        'master'. Function fails if we aren't DRM-Master more info may be found
 *        here: https://en.wikipedia.org/wiki/Direct_Rendering_Manager#DRM-Master_and_DRM-Auth
 *        So, if a graphical session is already active on the current VT function fails.
 *
 * @param drm_node  - May be NULL or a pointer to a struct kmr_drm_node.
 *                    If NULL memory will be allocated and return to
 *                    caller. If not NULL address passed will be used
 *                    to store the newly created struct kmr_buffer
 *                    instance.
 * @param node_info - Pointer to a struct kmr_drm_node_create_info used
 *                    to pass a DRI/KMS device file that we may want to
 *                    use and to store information about the current
 *                    seatd/sytemd-logind D-bus session.
 *
 * @return
 *	on success: Pointer to a struct kmr_drm_node
 *	on failure: NULL
 */
struct kmr_drm_node *
kmr_drm_node_create (struct kmr_drm_node *drm_node,
                     const void *node_info);


/*
 * @brief Function produces one connector->encoder->CRTC->plane
 *        display output chain. Populating the members of
 *        struct kmr_drm_node_display whose information
 *        will be later used in modesetting.
 *
 * @param drm_node     - Pointer to a valid struct kmr_drm_node.
 * @param display_info - NULL for now.
 *
 * @return
 *	on success: 0
 *	on failure: -1
 */
int
kmr_drm_node_set_display (struct kmr_drm_node *drm_node,
                          const void *display_info);


/*
 * @brief kmsroots DRM Node Display Mode Enumeration.
 *
 * @macro KMR_DRM_NODE_DISPLAY_MODE_SET   - Set current display to values passed.
 * @macro KMR_DRM_NODE_DISPLAY_MODE_RESET - Resets current display to default.
 */
enum kmr_drm_node_display_mode
{
	KMR_DRM_NODE_DISPLAY_MODE_SET   = 0,
	KMR_DRM_NODE_DISPLAY_MODE_RESET = 1,
};


/*
 * @brief kmsroots DRM Display Mode Information Structure.
 *
 * @member fbid           - KMS ID of framebuffer associated with
 *                          gbm or dump buffer. This ID is used
 *                          during kms atomic modesetting.
 * @member display_action - Action function call will take. Must
 *                          be a enum kmr_drm_node_display_mode
 *                          macro.
 */
struct kmr_drm_node_display_mode_info
{
	int                            fbid;
	enum kmr_drm_node_display_mode display_action;
};


/*
 * @brief Sets the display connected to @display->connecter screen
 *        resolution and refresh to the highest possible value.
 *
 * @param drm_node     - Pointer to a valid struct kmr_drm_node.
 * @param display_info - Pointer to a struct kmr_drm_node_display_mode_info
 *                       used to set highest display mode.
 *
 * @return
 *	on success: 0
 *	on failure: -1
 */
int
kmr_drm_node_set_display_mode (struct kmr_drm_node *drm_node,
                               const void *display_info);


/*
 * @brief Returns file descriptor to an open DRM device.
 *
 * @param drm_node - Pointer to a valid struct kmr_drm_node.
 *
 * @return
 * 	on success: File descriptor to open DRM device node
 * 	on failure: -1
 */
int
kmr_drm_node_get_kms_fd (struct kmr_drm_node *drm_node);


/*
 * @brief Returns amount of pixels in width.
 *
 * @param drm_node - Pointer to a valid struct kmr_drm_node.
 *
 * @return
 * 	on success: Amount of pixels in width
 * 	on failure: -1
 */
int
kmr_drm_node_get_display_width (struct kmr_drm_node *drm_node);


/*
 * @brief Returns amount of pixels in height.
 *
 * @param drm_node - Pointer to a valid struct kmr_drm_node.
 *
 * @return
 * 	on success: Amount of pixels in height
 * 	on failure: -1
 */
int
kmr_drm_node_get_display_height (struct kmr_drm_node *drm_node);


/*
 * kmsroots Implementation
 * Function pointer used by struct kmr_drm_node_atomic_req_info
 * used to pass the address of an external function you want to run
 * Given that the arguments of the function are:
 * 	1. A pointer to a boolean determining if the renderer is running.
 *	   Used to exit rendering operations.
 * 	2. A pointer to an unsigned 8 bit integer determining current buffer
 *	   GBM/DUMP buffer being used.
 *	3. A pointer to an integer storing KMS framebuffer ID associated with
 *	   the GBM(GEM DMA Buf) or DUMP buffer. Used by the implementation
 *	   during atomic modesetting operations.
 *	4. A pointer to any arbitrary data the custom renderer may want pass
 *	   during rendering operations.
 */
typedef void (*kmr_drm_node_renderer_impl) (volatile bool*, unsigned int*, int*, void*);


/*
 * @brief struct kmr_drm_node_atomic_req_info
 *        (kmsroots DRM Node Atomic Request Create Information)
 *
 * @member renderer          - Function pointer that allows custom external
 *                             renderers to be executed by the api upon
 *                             struct kmr_drm_node { @kmsfd } polled events.
 * @member renderer_running  - Pointer to a boolean that determines if a
 *                             given renderer is running and in need of
 *                             stopping.
 * @member renderer_cur_buff - Pointer to an integer used by the api to
 *                             update the current displayable buffer.
 * @member renderer_fbid     - Pointer to an integer used as the value
 *                             of the FB_ID property for a plane related
 *                             to the CRTC during the atomic modeset operation.
 * @member renderer_data     - Pointer to an optional address. This address may be
 *                             the address of a struct. Reference/Address passed
 *                             depends on external renderer function.
 */
struct kmr_drm_node_atomic_req_info
{
	kmr_drm_node_renderer_impl renderer;
	volatile bool              *renderer_running;
	unsigned int               *renderer_cur_buff;
	int                        *renderer_fbid;
	void                       *renderer_data;
};


/*
 * @brief Function creates a KMS atomic request instance.
 *        Sets the interface that allows callers of API to
 *        setup custom renderer implementation. Performs the
 *        initial modeset operation. After all the application
 *        needs to do is wait for page-flip events to happen.
 *
 * @param drm_node    - Pointer to a valid struct kmr_drm_node.
 * @param atomic_info - Pointer to a struct kmr_drm_node_atomic_req_info
 *                      used to set external renderer and arguments of
 *                      the external renderer.
 *
 * @returns
 *	on success: 0
 *	on failure: -1
 */
int
kmr_drm_node_atomic_request (struct kmr_drm_node *drm_node,
                             const void *atomic_info);


/*
 * @brief Function calls drmHandleEvent() which processes
 *        outstanding DRM events on the DRM file-descriptor.
 *        This function should be called after the DRM
 *        file-descriptor has been polled readable.
 *
 * @param drm_node   - Pointer to a valid struct kmr_drm_node.
 * @param event_info - NULL for now.
 *
 * @returns
 *	on success: 0
 *	on failure: -1
 */
int
kmr_drm_node_handle_drm_event (struct kmr_drm_node *drm_node,
                               const void *event_info);


/*
 * @breif Frees any allocated memory and closes FD's (if open) created after
 *        kmr_drm_node_create() call.
 *
 * @param node - Pointer to a valid struct kmr_drm_node
 */
void
kmr_drm_node_destroy (struct kmr_drm_node *node);


#endif /* KMR_DRM_NODE_H */
