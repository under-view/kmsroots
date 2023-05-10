#ifndef UVR_KMS_NODE_H
#define UVR_KMS_NODE_H

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
 * struct uvr_kms_node (Underview Renderer KMS Node)
 *
 * members:
 * @kmsfd          - A valid file descriptor to an open DRI device node
 * @vtfd           - File descriptor to open tty character device (i.e '/dev/tty0')
 * @keyBoardMode   - Integer saving the current keyboard mode. (man 2 ioctl_console for more info)
 * @session        - Stores address of struct uvr_session. Used when releasing a device
 */
struct uvr_kms_node {
	int                kmsfd;
	int                vtfd;
	int                keyBoardMode;
#ifdef INCLUDE_LIBSEAT
	struct uvr_session *session;
#endif
};


/*
 * struct uvr_kms_node_create_info (Underview Renderer KMS Node Create Information)
 *
 * members:
 * @session - Address of struct uvrsd_session. Which members are used to communicate
 *            with systemd-logind via D-Bus systemd-logind interface. Needed by
 *            kms_node_create to acquire and taken control of a device without the
 *            need of being root.
 * @kmsNode - Path to character device associated with GPU. If set to NULL. List of
 *            available kmsnode's will be queried and one will be automatically
 *            choosen for you.
 */
struct uvr_kms_node_create_info {
#ifdef INCLUDE_LIBSEAT
	struct uvr_session *session;
#endif
	const char         *kmsNode;
};


/*
 * uvr_kms_node_create: Function opens a DRI device node. If have a systemd-logind session available one can take
 *                      control of a device node. Returned fd is exposed to all planes (overlay, primary, and cursor)
 *                      and has access to the aspect ratio information in modes in userspace. In order
 *                      to drive KMS, we need to be 'master'. Function fails if we aren't DRM-Master more
 *                      info here: https://en.wikipedia.org/wiki/Direct_Rendering_Manager#DRM-Master_and_DRM-Auth
 *                      So, if a graphical session is already active on the current VT function fails.
 *
 * args:
 * @uvrkms - pointer to a struct uvr_kms_node_create_info used to determine pass DRI device file that we may want to use
 *           and information about the current sytemd-logind session
 * return:
 *	on success struct uvr_kms_node
 *	on failure struct uvr_kms_node { with members nulled, int's set to -1 }
 */
struct uvr_kms_node uvr_kms_node_create(struct uvr_kms_node_create_info *uvrkms);


/*
 * struct uvr_kms_node_device_capabilites (Underview Renderer KMS Node Device Capabilites)
 *
 * members:
 * @CAP_ADDFB2_MODIFIERS
 * @CAP_TIMESTAMP_MONOTONIC
 * @CAP_CRTC_IN_VBLANK_EVENT
 * @CAP_DUMB_BUFFER
 *
 * For more info see https://github.com/torvalds/linux/blob/master/include/uapi/drm/drm.h#L627
 */
struct uvr_kms_node_device_capabilites {
	bool CAP_ADDFB2_MODIFIERS;
	bool CAP_TIMESTAMP_MONOTONIC;
	bool CAP_CRTC_IN_VBLANK_EVENT;
	bool CAP_DUMB_BUFFER;
};


/*
 * uvr_kms_node_get_device_capabilities: Function takes in a DRM device fd and populates the struct uvrkms_node_device_capabilites
 *                                       to give details on what capabilites the particular kms device supports. Function is called
 *                                       by uvr_kms_node_create(3), but exposed is exposed application developer for their own use.
 *
 * args:
 * @kmsfd - Number associated with open KMS device node
 * return:
 *	struct uvr_kms_node_device_capabilites
 */
struct uvr_kms_node_device_capabilites uvr_kms_node_get_device_capabilities(int kmsfd);


/*
 * struct uvr_kms_node_object_props (Underview Renderer KMS Node Object Properties)
 *
 * It stores properties of certain KMS objects (connectors, CRTC and planes) that are
 * used in atomic modeset setup and also in atomic page-flips.
 *
 * members:
 * @props     - Stores list of property ids and their values
 * @propsData - Resources avaliable to a given KMS object
 * @id        - Driver assigned ID of the KMS object
 */
struct uvr_kms_node_object_props {
	drmModeObjectProperties *props;
	drmModePropertyRes      **propsData;
	uint32_t                id;
};


/*
 * struct uvr_kms_node_display_output_chain (Underview Renderer KMS Node Display Output Chain)
 *
 * members:
 * @connector      - Anything that can display pixels in some form. (i.e HDMI). Connectors can
 *                   be hotplugged and unplugged at runtime
 * @connectorProps - Stores connector properties used during KMS atomic modesetting and page-flips.
 * @encoder        - (Deprecated) Takes pixel data from a crtc and converts it to an output that
 *                   the connector can understand. This is a Deprecated kms object
 * @crtc           - Represents a part of the chip that contains a pointer to a scanout buffer.
 * @crtcProps      - Stores crtc properties used during KMS atomic modesetting and page-flips.
 * @plane          - A plane represents an image source that can be blended with or overlayed on top of
 *                   a CRTC during the scanout process. Planes are associated with a frame buffer to crop
 *                   a portion of the image memory (source) and optionally scale it to a destination size.
 *                   The result is then blended with or overlayed on top of a CRTC.
 * @planeProps     - Stores plane properties used during KMS atomic modesetting and page-flips.
 * For more info see https://manpages.org/drm-kms/7
 * @width          - Highest mode (display resolution) width for @connector attached to display
 * @width          - Highest mode (display resolution) height for @connector attached to display
 * @kmsfd          - File descriptor associated with GPU device
 * @modeid         - Store the mode (resolution + refresh) property id. When we perform an atomic commit,
 *                   the driver expects a CRTC property named "MODE_ID", which points to the id given to one
 *                   of connceted display resolution & refresh rate. At the moment the highest mode it choosen.
 */
struct uvr_kms_node_display_output_chain {
	drmModeConnector                 *connector;
	struct uvr_kms_node_object_props connectorProps;
	drmModeCrtc                      *crtc;
	struct uvr_kms_node_object_props crtcProps;
	drmModePlane                     *plane;
	struct uvr_kms_node_object_props planeProps;
	uint16_t                         width;
	uint16_t                         height;
	int                              kmsfd;
	uint32_t                         modeid;
};


/*
 * struct uvr_kms_node_display_output_chain_create_info (Underview Renderer KMS Node display Output Chain Create Information)
 *
 * members:
 * @kmsfd - The file descriptor associated with open KMS device node.
 */
struct uvr_kms_node_display_output_chain_create_info {
	int kmsfd;
};


/*
 * uvr_kms_node_display_output_chain_create: Function takes in a pointer to a struct uvrkms_node_get_display_output_chain_create_info
 *                                           and produces one connector->encoder->CRTC->plane display output chain. Populating
 *                                           the members of struct uvr_kms_node_display_output_chain whose information will be
 *                                           later used in modesetting.
 *
 * args:
 * @uvrkms - pointer to a struct uvr_kms_node_display_output_chain_create_info used to determine what operation will happen in the function
 * return
 *	on success struct uvr_kms_node_display_output_chain
 *	on failure struct uvr_kms_node_display_output_chain { with members nulled }
 */
struct uvr_kms_node_display_output_chain uvr_kms_node_display_output_chain_create(struct uvr_kms_node_display_output_chain_create_info *uvrkms);


/*
 * struct uvr_kms_node_display_mode_info (Underview Renderer KMS Display Mode Information)
 *
 * members:
 * @fbid         - Id of framebuffer associated with gbm or dump buffer
 * @displayChain - Pointer to a plane->crtc->encoder->connector pair
 */
struct uvr_kms_node_display_mode_info {
	int                                      fbid;
	struct uvr_kms_node_display_output_chain *displayChain;
};


/*
 * uvr_kms_node_display_mode_set: Sets the display connected to @displayChain.connecter screen resolution
 *                                and refresh to the highest possible value.
 *
 * args:
 * @uvrkms - pointer to a struct uvr_kms_node_display_mode_info used to set highest display mode
 * return
 *	on success 0
 *	on failure -1
 */
int uvr_kms_node_display_mode_set(struct uvr_kms_node_display_mode_info *uvrkms);


/*
 * uvr_kms_node_display_mode_reset: Clears the current display mode setting
 *
 * args:
 * @uvrkms - pointer to a struct uvr_kms_node_display_mode_info used to set highest display mode
 * return
 *	on success 0
 *	on failure -1
 */
int uvr_kms_node_display_mode_reset(struct uvr_kms_node_display_mode_info *uvrkms);


/*
 * Underview Renderer Implementation
 * Function pointer used by struct uvr_kms_node_atomic_request_create_info*
 * Used to pass the address of an external function you want to run
 * Given that the arguments of the function are:
 * 	1. A pointer to a boolean determining if the renderer is running.
 *	   Used to exit rendering operations.
 * 	2. A pointer to an unsigned 8 bit integer determining current buffer
 *	   GBM/DUMP buffer being used.
 *	3. A pointer to an integer storing framebuffer ID associated with
 *	   the GBM(GEM DMA Buf) or DUMP buffer. Used by the implementation
 *	   during atomic modesetting operations.
 *	4. A pointer to any arbitrary data the custom renderer may want pass
 *	   during rendering operations.
 */
typedef void (*uvr_kms_node_renderer_impl)(bool*, uint8_t*, int*, void*);


/*
 * struct uvr_kms_node_atomic_request_create_info (Underview Renderer KMS Node Atomic Request Create Information)
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
struct uvr_kms_node_atomic_request_create_info {
	int                                      kmsfd;
	struct uvr_kms_node_display_output_chain *displayOutputChain;
	uvr_kms_node_renderer_impl               renderer;
	bool                                     *rendererRunning;
	uint8_t                                  *rendererCurrentBuffer;
	int                                      *rendererFbId;
	void                                     *rendererData;
};


/*
 * struct uvr_kms_node_atomic_request (Underview Renderer KMS Node Atomic Request)
 *
 * members:
 * @atomicRequest - Pointer to a KMS atomic request instance
 * @rendererInfo  - Used by the implementation. DO NOT MODIFY.
 */
struct uvr_kms_node_atomic_request {
	drmModeAtomicReq *atomicRequest;
	void             *rendererInfo;
};


/*
 * uvr_kms_node_atomic_request_create: Function creates a KMS atomic request instance. Sets the interface that allows
 *                                     users of API to setup custom renderer implementation.
 *
 * args:
 * @uvrkms - pointer to a struct uvr_kms_node_atomic_request_create_info used to set external renderer and arguments
 *           of the external renderer.
 * return
 *	on success struct uvr_kms_node_atomic_request
 *	on failure struct uvr_kms_node_atomic_request { with members nulled }
 */
struct uvr_kms_node_atomic_request uvr_kms_node_atomic_request_create(struct uvr_kms_node_atomic_request_create_info *uvrkms);


/*
 * struct uvr_kms_node_handle_drm_event_info (Underview Renderer KMS Node Handle DRM Event Information)
 *
 * members:
 * @kmsfd - File descriptor to an open KMS device node.
 */
struct uvr_kms_node_handle_drm_event_info {
	int kmsfd;
};


/*
 * uvr_kms_node_handle_drm_event: Function calls drmHandleEvent() which processes outstanding DRM events
 *                                on the DRM file-descriptor. This function should be called after the DRM
 *                                file-descriptor has polled readable.
 *
 * args:
 * @uvrkms - pointer to a struct uvr_kms_node_handle_drm_event_info
 * return
 *	on success 0
 *	on failure -1
 */
int uvr_kms_node_handle_drm_event(struct uvr_kms_node_handle_drm_event_info *uvrkms);


/*
 * struct uvr_kms_node_destroy (Underview Renderer KMS Node Destroy)
 *
 * members:
 * @uvr_kms_node                      - Must pass a valid struct uvr_kms_node. Which contains information about the
 *                                      file descriptor associated with open DRI device node to be closed. Stores
 *                                      information about logind session bus, id, and path needed to release control
 *                                      of a given device. That given device is a DRI device file.
 * @uvr_kms_node_display_output_chain - Must pass a valid struct uvr_kms_node_display_output_chain. Stores information
 *                                      about KMS device node connector->encoder->crtc->plane pair
 * @uvr_kms_node_atomic_request       - Must pass a valid struct uvr_kms_node_atomic_request { free'd members: @atomicRequest, @rendererInfo }
 */
struct uvr_kms_node_destroy {
	struct uvr_kms_node                      uvr_kms_node;
	struct uvr_kms_node_display_output_chain uvr_kms_node_display_output_chain;
	struct uvr_kms_node_atomic_request       uvr_kms_node_atomic_request;
};


/*
 * uvr_kms_node_destroy: Destroy any and all KMS node information
 *
 * args:
 * @uvrkms - pointer to a struct uvr_kms_node_destroy
 */
void uvr_kms_node_destroy(struct uvr_kms_node_destroy *uvrkms);

#endif
