#ifndef KMR_DRM_NODE_H
#define KMR_DRM_NODE_H

#include "utils.h"

#ifdef INCLUDE_LIBSEAT
#include "session.h"
#endif

/*
 * TAKEN FROM Daniel Stone (gitlab/kms-quads)
 * Headers from the kernel's DRM uABI, allowing us to use ioctls directly.
 * These come from the kernel, via libdrm.
 */
#include <drm.h>
#include <drm_fourcc.h>
#include <drm_mode.h>

/*
 * TAKEN FROM Daniel Stone (gitlab/kms-quads)
 * Headers from the libdrm userspace library API, prefixed xf86*. These
 * mostly provide device and resource enumeration, as well as wrappers
 * around many ioctls, notably atomic modesetting.
 */
#include <xf86drm.h>
#include <xf86drmMode.h>


/*
 * struct kmr_drm_node (kmsroots DRM Node)
 *
 * members:
 * @kmsfd   - A valid file descriptor to an open DRI device node
 * @session - Stores address of struct kmr_session. Used when opening and releasing a device.
 */
struct kmr_drm_node {
	int                kmsfd;
#ifdef INCLUDE_LIBSEAT
	struct kmr_session *session;
#endif
};


/*
 * struct kmr_drm_node_create_info (kmsroots DRM Node Create Information)
 *
 * members:
 * @session - Address of struct kmssd_session. Which members are used to communicate
 *            with systemd-logind via D-Bus systemd-logind interface. Needed by
 *            kms_node_create to acquire and taken control of a device without the
 *            need of being root.
 * @kmsNode - Path to character device associated with GPU. If set to NULL. List of
 *            available kmsnode's will be queried and one will be automatically
 *            choosen for you.
 */
struct kmr_drm_node_create_info {
#ifdef INCLUDE_LIBSEAT
	struct kmr_session *session;
#endif
	const char         *kmsNode;
};


/*
 * kmr_drm_node_create: Function opens a DRI device node. If have a systemd-logind session available one can take
 *                      control of a device node. Returned fd is exposed to all planes (overlay, primary, and cursor)
 *                      and has access to the aspect ratio information in modes in userspace. In order
 *                      to drive KMS, we need to be 'master'. Function fails if we aren't DRM-Master more
 *                      info here: https://en.wikipedia.org/wiki/Direct_Rendering_Manager#DRM-Master_and_DRM-Auth
 *                      So, if a graphical session is already active on the current VT function fails.
 *
 * args:
 * @kmrdrm - pointer to a struct kmr_drm_node_create_info used to pass a DRI/KMS device file that we may want to use
 *           and to store information about the current seatd/sytemd-logind D-bus session.
 * return:
 *	on success struct kmr_drm_node
 *	on failure struct kmr_drm_node { with members nulled, int's set to -1 }
 */
struct kmr_drm_node kmr_drm_node_create(struct kmr_drm_node_create_info *kmrdrm);


/*
 * struct kmr_drm_node_device_capabilites (kmsroots DRM Node Device Capabilites)
 *
 * members:
 * @CAP_ADDFB2_MODIFIERS
 * @CAP_TIMESTAMP_MONOTONIC
 * @CAP_CRTC_IN_VBLANK_EVENT
 * @CAP_DUMB_BUFFER
 *
 * For more info see https://github.com/torvalds/linux/blob/master/include/uapi/drm/drm.h#L627
 */
struct kmr_drm_node_device_capabilites {
	bool CAP_ADDFB2_MODIFIERS;
	bool CAP_TIMESTAMP_MONOTONIC;
	bool CAP_CRTC_IN_VBLANK_EVENT;
	bool CAP_DUMB_BUFFER;
};


/*
 * kmr_drm_node_get_device_capabilities: Function takes in a DRM device fd and populates the struct kmrdrm_node_device_capabilites
 *                                       to give details on what capabilites the particular kms device supports. Function is called
 *                                       by kmr_drm_node_create(3), but exposed is exposed application developer for their own use.
 *
 * args:
 * @kmsfd - Number associated with open KMS device node
 * return:
 *	struct kmr_drm_node_device_capabilites
 */
struct kmr_drm_node_device_capabilites kmr_drm_node_get_device_capabilities(int kmsfd);


/*
 * struct kmr_drm_node_object_props_data (kmsroots DRM Node Object Properties Data)
 *
 * @id    - Id of the given property
 * @value - Enum value of given KMS object property. Can be used for instance to check if plane
 *          object is a primary plane (DRM_PLANE_TYPE_PRIMARY).
 */
struct kmr_drm_node_object_props_data {
	uint32_t id;
	uint64_t value;
};


/*
 * struct kmr_drm_node_object_props (kmsroots DRM Node Object Properties)
 *
 * It stores properties of certain KMS objects (connectors, CRTC and planes) that are
 * used in atomic modeset setup and also in atomic page-flips.
 *
 * members:
 * @id             - Driver assigned ID of the KMS object.
 * @propsData      - Stores data of properties used during KMS atomic operations.
 * @propsDataCount - Array size of @propsData.
 */
struct kmr_drm_node_object_props {
	uint32_t                              id;
	struct kmr_drm_node_object_props_data *propsData;
	uint8_t                               propsDataCount;
};


/*
 * struct kmr_drm_node_display_mode_data (kmsroots DRM Node Display Mode Data)
 *
 * members:
 * @id       - Store the highest mode (resolution + refresh) property id. When we perform an atomic commit,
 *             the driver expects a CRTC property named "MODE_ID", which points to the id given to one
 *             of connceted display resolution & refresh rate. At the moment the highest mode it choosen.
 * @modeInfo - Store the highest mode data (display resolution + refresh) associated with display.
 */
struct kmr_drm_node_display_mode_data {
	uint32_t        id;
	drmModeModeInfo modeInfo;
};


/*
 * struct kmr_drm_node_display_output_chain (kmsroots DRM Node Display Output Chain)
 *
 * members:
 * @kmsfd      - Pollable file descriptor to an open KMS (GPU) device file.
 * @width      - Highest mode (display resolution) width for @connector attached to display
 * @height     - Highest mode (display resolution) height for @connector attached to display
 * @modeData   - Stores highest mode (display resolution & refresh) along with the modeid property
 *               used during KMS atomic operations.
 * @connector  - Anything that can display pixels in some form. (i.e HDMI). Connectors can
 *               be hotplugged and unplugged at runtime. Stores connector properties used
 *               during KMS atomic modesetting and page-flips.
 * @crtc       - Represents a part of the chip that contains a pointer to a scanout buffer.
 *               Stores crtc properties used during KMS atomic modesetting and page-flips.
 * @plane      - A plane represents an image source that can be blended with or overlayed on top of
 *               a CRTC during the scanout process. Planes are associated with a frame buffer to crop
 *               a portion of the image memory (source) and optionally scale it to a destination size.
 *               The result is then blended with or overlayed on top of a CRTC. Stores primary plane
 *               properties used during KMS atomic modesetting and page-flips.
 * For more info see https://manpages.org/drm-kms/7
 */
struct kmr_drm_node_display_output_chain {
	int                                   kmsfd;
	uint16_t                              width;
	uint16_t                              height;
	struct kmr_drm_node_display_mode_data modeData;
	struct kmr_drm_node_object_props      connector;
	struct kmr_drm_node_object_props      crtc;
	struct kmr_drm_node_object_props      plane;
};


/*
 * struct kmr_drm_node_display_output_chain_create_info (kmsroots DRM Node display Output Chain Create Information)
 *
 * members:
 * @kmsfd - The file descriptor associated with open KMS device node.
 */
struct kmr_drm_node_display_output_chain_create_info {
	int kmsfd;
};


/*
 * kmr_drm_node_display_output_chain_create: Function takes in a pointer to a struct kmrdrm_node_get_display_output_chain_create_info
 *                                           and produces one connector->encoder->CRTC->plane display output chain. Populating
 *                                           the members of struct kmr_drm_node_display_output_chain whose information will be
 *                                           later used in modesetting.
 *
 * args:
 * @kmrdrm - pointer to a struct kmr_drm_node_display_output_chain_create_info used to determine what operation will happen in the function
 * return
 *	on success struct kmr_drm_node_display_output_chain
 *	on failure struct kmr_drm_node_display_output_chain { with members nulled }
 */
struct kmr_drm_node_display_output_chain kmr_drm_node_display_output_chain_create(struct kmr_drm_node_display_output_chain_create_info *kmrdrm);


/*
 * struct kmr_drm_node_display_mode_info (kmsroots KMS Display Mode Information)
 *
 * members:
 * @fbid         - Id of framebuffer associated with gbm or dump buffer
 * @displayChain - Pointer to a plane->crtc->encoder->connector pair
 */
struct kmr_drm_node_display_mode_info {
	int                                      fbid;
	struct kmr_drm_node_display_output_chain *displayChain;
};


/*
 * kmr_drm_node_display_mode_set: Sets the display connected to @displayChain.connecter screen resolution
 *                                and refresh to the highest possible value.
 *
 * args:
 * @kmrdrm - pointer to a struct kmr_drm_node_display_mode_info used to set highest display mode
 * return
 *	on success 0
 *	on failure -1
 */
int kmr_drm_node_display_mode_set(struct kmr_drm_node_display_mode_info *kmrdrm);


/*
 * kmr_drm_node_display_mode_reset: Clears the current display mode setting
 *
 * args:
 * @kmrdrm - pointer to a struct kmr_drm_node_display_mode_info used to set highest display mode
 * return
 *	on success 0
 *	on failure -1
 */
int kmr_drm_node_display_mode_reset(struct kmr_drm_node_display_mode_info *kmrdrm);


/*
 * kmsroots Implementation
 * Function pointer used by struct kmr_drm_node_atomic_request_create_info*
 * Used to pass the address of an external function you want to run
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
typedef void (*kmr_drm_node_renderer_impl)(volatile bool*, uint8_t*, int*, void*);


/*
 * struct kmr_drm_node_atomic_request_create_info (kmsroots DRM Node Atomic Request Create Information)
 *
 * members:
 * @kmsfd                 - File descriptor to an open KMS device node.
 * @displayOutputChain    - Pointer to a struct containing all plane->crtc->connector data used during
 *                          KMS atomic mode setting.
 * @renderer              - Function pointer that allows custom external renderers to be executed by the api
 *                          upon @kmsfd polled events.
 * @rendererRunning       - Pointer to a boolean that determines if a given renderer is running and in need
 *                          of stopping.
 * @rendererCurrentBuffer - Pointer to an integer used by the api to update the current displayable buffer.
 * @rendererFbId          - Pointer to an integer used as the value of the FB_ID property for a plane related to
 *                          the CRTC during the atomic modeset operation
 * @rendererData          - Pointer to an optional address. This address may be the address of a struct.
 *                          Reference/Address passed depends on external renderer function.
 */
struct kmr_drm_node_atomic_request_create_info {
	int                                      kmsfd;
	struct kmr_drm_node_display_output_chain *displayOutputChain;
	kmr_drm_node_renderer_impl               renderer;
	volatile bool                            *rendererRunning;
	uint8_t                                  *rendererCurrentBuffer;
	int                                      *rendererFbId;
	void                                     *rendererData;
};


/*
 * struct kmr_drm_node_atomic_request (kmsroots DRM Node Atomic Request)
 *
 * members:
 * @atomicRequest - Pointer to a KMS atomic request instance
 * @rendererInfo  - Used by the implementation to free data. DO NOT MODIFY.
 */
struct kmr_drm_node_atomic_request {
	drmModeAtomicReq *atomicRequest;
	void             *rendererInfo;
};


/*
 * kmr_drm_node_atomic_request_create: Function creates a KMS atomic request instance. Sets the interface that allows
 *                                     users of API to setup custom renderer implementation. Performs the initial modeset
 *                                     operation after all the application needs to do is wait for page-flip events to happen.
 *
 * args:
 * @kmrdrm - pointer to a struct kmr_drm_node_atomic_request_create_info used to set external renderer and arguments
 *           of the external renderer.
 * return
 *	on success struct kmr_drm_node_atomic_request
 *	on failure struct kmr_drm_node_atomic_request { with members nulled }
 */
struct kmr_drm_node_atomic_request kmr_drm_node_atomic_request_create(struct kmr_drm_node_atomic_request_create_info *kmrdrm);


/*
 * struct kmr_drm_node_handle_drm_event_info (kmsroots DRM Node Handle DRM Event Information)
 *
 * members:
 * @kmsfd - File descriptor to an open KMS device node.
 */
struct kmr_drm_node_handle_drm_event_info {
	int kmsfd;
};


/*
 * kmr_drm_node_handle_drm_event: Function calls drmHandleEvent() which processes outstanding DRM events
 *                                on the DRM file-descriptor. This function should be called after the DRM
 *                                file-descriptor has polled readable.
 *
 * args:
 * @kmrdrm - pointer to a struct kmr_drm_node_handle_drm_event_info
 * return
 *	on success 0
 *	on failure -1
 */
int kmr_drm_node_handle_drm_event(struct kmr_drm_node_handle_drm_event_info *kmrdrm);


/*
 * struct kmr_drm_node_destroy (kmsroots DRM Node Destroy)
 *
 * members:
 * @kmr_drm_node                      - Must pass a valid struct kmr_drm_node. Which contains information about the
 *                                      file descriptor associated with open DRI device node to be closed. Stores
 *                                      information about logind session bus, id, and path needed to release control
 *                                      of a given device. That given device is a DRI device file.
 * @kmr_drm_node_display_output_chain - Must pass a valid struct kmr_drm_node_display_output_chain. Stores information
 *                                      about KMS device node connector->encoder->crtc->plane pair
 * @kmr_drm_node_atomic_request       - Must pass a valid struct kmr_drm_node_atomic_request { free'd members: @atomicRequest, @rendererInfo }
 */
struct kmr_drm_node_destroy {
	struct kmr_drm_node                      kmr_drm_node;
	struct kmr_drm_node_display_output_chain kmr_drm_node_display_output_chain;
	struct kmr_drm_node_atomic_request       kmr_drm_node_atomic_request;
};


/*
 * kmr_drm_node_destroy: Destroy any and all KMS node information
 *
 * args:
 * @kmrdrm - pointer to a struct kmr_drm_node_destroy
 */
void kmr_drm_node_destroy(struct kmr_drm_node_destroy *kmrdrm);

#endif
