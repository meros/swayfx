#ifndef _SWAY_EXT_FOREIGN_TOPLEVEL_CAPTURE_H
#define _SWAY_EXT_FOREIGN_TOPLEVEL_CAPTURE_H

#include <wayland-server-core.h>

struct wlr_allocator;
struct wlr_ext_foreign_toplevel_handle_v1;
struct wlr_ext_image_capture_source_v1;
struct wlr_renderer;
struct wlr_scene_node;

/**
 * Vendored from wlroots 0.20+ which adds ext-foreign-toplevel-image-capture-source-v1.
 * wlroots 0.19 only has the output capture source variant.
 */

struct sway_foreign_toplevel_image_capture_manager {
	struct wl_global *global;

	struct {
		struct wl_signal destroy;
		struct wl_signal new_request;
	} events;

	struct wl_listener display_destroy;
};

struct sway_foreign_toplevel_image_capture_request {
	struct wlr_ext_foreign_toplevel_handle_v1 *toplevel_handle;
	struct wl_client *client;
	uint32_t new_id;
};

struct sway_foreign_toplevel_image_capture_manager *
sway_foreign_toplevel_image_capture_manager_create(
	struct wl_display *display, uint32_t version);

bool sway_foreign_toplevel_image_capture_request_accept(
	struct sway_foreign_toplevel_image_capture_request *request,
	struct wlr_ext_image_capture_source_v1 *source);

/**
 * Scene-node-based image capture source (vendored from wlroots 0.20+).
 * Creates a virtual output that renders a scene node subtree for capture.
 */
struct wlr_ext_image_capture_source_v1 *
sway_image_capture_source_create_with_scene_node(
	struct wlr_scene_node *node, struct wl_event_loop *event_loop,
	struct wlr_allocator *allocator, struct wlr_renderer *renderer);

/**
 * Update the scale used by the capture source's virtual output.
 * This affects the buffer_size reported to capture clients and the
 * resolution of captured frames. Should be called when the view moves
 * to an output with a different scale.
 */
void sway_image_capture_source_set_scale(
	struct wlr_ext_image_capture_source_v1 *source, float scale);

#endif
