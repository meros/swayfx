/**
 * ext-foreign-toplevel-image-capture-source-v1 protocol manager.
 *
 * Vendored from wlroots master (post-0.19) and adapted for wlroots 0.19 API.
 * This implements the Wayland global that lets clients request an image
 * capture source for a foreign toplevel handle.
 */
#include <assert.h>
#include <stdlib.h>
#include <wlr/interfaces/wlr_ext_image_capture_source_v1.h>
#include <wlr/types/wlr_ext_foreign_toplevel_list_v1.h>
#include "ext-image-capture-source-v1-protocol.h"
#include "sway/ext_foreign_toplevel_capture.h"

#define FOREIGN_TOPLEVEL_IMAGE_SOURCE_MANAGER_V1_VERSION 1

static const struct ext_foreign_toplevel_image_capture_source_manager_v1_interface
	manager_impl;

static struct sway_foreign_toplevel_image_capture_manager *
manager_from_resource(struct wl_resource *resource) {
	assert(wl_resource_instance_of(resource,
		&ext_foreign_toplevel_image_capture_source_manager_v1_interface,
		&manager_impl));
	return wl_resource_get_user_data(resource);
}

bool sway_foreign_toplevel_image_capture_request_accept(
		struct sway_foreign_toplevel_image_capture_request *request,
		struct wlr_ext_image_capture_source_v1 *source) {
	return wlr_ext_image_capture_source_v1_create_resource(
		source, request->client, request->new_id);
}

static void manager_handle_create_source(struct wl_client *client,
		struct wl_resource *manager_resource, uint32_t new_id,
		struct wl_resource *foreign_toplevel_resource) {
	struct sway_foreign_toplevel_image_capture_manager *manager =
		manager_from_resource(manager_resource);
	struct wlr_ext_foreign_toplevel_handle_v1 *toplevel_handle =
		wlr_ext_foreign_toplevel_handle_v1_from_resource(
			foreign_toplevel_resource);
	if (toplevel_handle == NULL) {
		wlr_ext_image_capture_source_v1_create_resource(NULL, client, new_id);
		return;
	}

	struct sway_foreign_toplevel_image_capture_request *request =
		calloc(1, sizeof(*request));
	if (request == NULL) {
		wl_resource_post_no_memory(manager_resource);
		return;
	}

	request->toplevel_handle = toplevel_handle;
	request->client = client;
	request->new_id = new_id;

	wl_signal_emit_mutable(&manager->events.new_request, request);

	free(request);
}

static void manager_handle_destroy(struct wl_client *client,
		struct wl_resource *manager_resource) {
	wl_resource_destroy(manager_resource);
}

static const struct ext_foreign_toplevel_image_capture_source_manager_v1_interface
	manager_impl = {
	.create_source = manager_handle_create_source,
	.destroy = manager_handle_destroy,
};

static void manager_bind(struct wl_client *client, void *data,
		uint32_t version, uint32_t id) {
	struct sway_foreign_toplevel_image_capture_manager *manager = data;

	struct wl_resource *resource = wl_resource_create(client,
		&ext_foreign_toplevel_image_capture_source_manager_v1_interface,
		version, id);
	if (!resource) {
		wl_client_post_no_memory(client);
		return;
	}
	wl_resource_set_implementation(resource, &manager_impl, manager, NULL);
}

static void manager_handle_display_destroy(struct wl_listener *listener,
		void *data) {
	struct sway_foreign_toplevel_image_capture_manager *manager =
		wl_container_of(listener, manager, display_destroy);
	wl_signal_emit_mutable(&manager->events.destroy, NULL);
	wl_list_remove(&manager->display_destroy.link);
	wl_global_destroy(manager->global);
	free(manager);
}

struct sway_foreign_toplevel_image_capture_manager *
sway_foreign_toplevel_image_capture_manager_create(
		struct wl_display *display, uint32_t version) {
	assert(version <= FOREIGN_TOPLEVEL_IMAGE_SOURCE_MANAGER_V1_VERSION);

	struct sway_foreign_toplevel_image_capture_manager *manager =
		calloc(1, sizeof(*manager));
	if (manager == NULL) {
		return NULL;
	}

	manager->global = wl_global_create(display,
		&ext_foreign_toplevel_image_capture_source_manager_v1_interface,
		version, manager, manager_bind);
	if (manager->global == NULL) {
		free(manager);
		return NULL;
	}

	wl_signal_init(&manager->events.destroy);
	wl_signal_init(&manager->events.new_request);

	manager->display_destroy.notify = manager_handle_display_destroy;
	wl_display_add_destroy_listener(display, &manager->display_destroy);

	return manager;
}
