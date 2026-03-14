#include <stdlib.h>
#include "sway/config.h"
#include "sway/scene_descriptor.h"
#include "sway/tree/container.h"
#include "sway/tree/sash.h"
#include "sway/tree/workspace.h"
#include "list.h"
#include "log.h"

void sash_manager_init(struct sway_container *con) {
	if (!con->content_tree) {
		return;
	}

	// Create sash tree as a child of content_tree (where children are placed)
	con->sash_manager.tree = wlr_scene_tree_create(con->content_tree);
	if (!con->sash_manager.tree) {
		sway_log(SWAY_ERROR, "Failed to create sash manager tree");
		return;
	}

	con->sash_manager.sashes = create_list();
	if (!con->sash_manager.sashes) {
		sway_log(SWAY_ERROR, "Failed to create sash list");
		wlr_scene_node_destroy(&con->sash_manager.tree->node);
		con->sash_manager.tree = NULL;
		return;
	}

	sway_log(SWAY_DEBUG, "Sash manager initialized for container");

	// Sashes will be above children by default since they're added after
	// But we can explicitly raise them
	wlr_scene_node_raise_to_top(&con->sash_manager.tree->node);
}

void sash_manager_destroy(struct sway_container *con) {
	if (!con->sash_manager.sashes) {
		return;
	}

	// Destroy all sashes
	for (int i = 0; i < con->sash_manager.sashes->length; i++) {
		struct sway_sash *sash = con->sash_manager.sashes->items[i];
		if (sash->rect) {
			scene_descriptor_destroy(&sash->rect->node, SWAY_SCENE_DESC_SASH);
			wlr_scene_node_destroy(&sash->rect->node);
		}
		free(sash);
	}
	list_free(con->sash_manager.sashes);
	con->sash_manager.sashes = NULL;

	// Destroy scene tree
	if (con->sash_manager.tree) {
		wlr_scene_node_destroy(&con->sash_manager.tree->node);
		con->sash_manager.tree = NULL;
	}
}

struct sway_sash *sash_create(struct sway_container *parent,
		struct sway_container *before, struct sway_container *after,
		enum sway_container_layout orientation) {
	if (!config->sash_enabled || !parent->sash_manager.tree) {
		return NULL;
	}

	struct sway_sash *sash = calloc(1, sizeof(struct sway_sash));
	if (!sash) {
		sway_log(SWAY_ERROR, "Failed to allocate sash");
		return NULL;
	}

	sash->parent = parent;
	sash->before = before;
	sash->after = after;
	sash->orientation = orientation;
	sash->hover = false;

	// Create visual rectangle with configured color and opacity
	float color[4] = {
		config->sash_color[0],
		config->sash_color[1],
		config->sash_color[2],
		config->sash_opacity  // Use configured opacity
	};

	sash->rect = wlr_scene_rect_create(parent->sash_manager.tree, 1, 1, color);
	if (!sash->rect) {
		sway_log(SWAY_ERROR, "Failed to create sash rectangle");
		free(sash);
		return NULL;
	}

	sway_log(SWAY_DEBUG, "Created sash rect: color=(%.2f,%.2f,%.2f,%.2f)",
		color[0], color[1], color[2], color[3]);

	// Attach scene descriptor
	scene_descriptor_assign(&sash->rect->node, SWAY_SCENE_DESC_SASH, sash);

	// Add to parent's sash list
	list_add(parent->sash_manager.sashes, sash);

	return sash;
}

void sash_destroy(struct sway_sash *sash) {
	if (!sash) {
		return;
	}

	// Remove from parent's list
	if (sash->parent && sash->parent->sash_manager.sashes) {
		for (int i = 0; i < sash->parent->sash_manager.sashes->length; i++) {
			if (sash->parent->sash_manager.sashes->items[i] == sash) {
				list_del(sash->parent->sash_manager.sashes, i);
				break;
			}
		}
	}

	// Destroy scene node
	if (sash->rect) {
		scene_descriptor_destroy(&sash->rect->node, SWAY_SCENE_DESC_SASH);
		wlr_scene_node_destroy(&sash->rect->node);
	}

	free(sash);
}

void update_sashes(struct sway_container *con) {
	if (!config->sash_enabled) {
		sway_log(SWAY_DEBUG, "Sashes disabled in config");
		return;
	}

	if (!con->sash_manager.sashes) {
		sway_log(SWAY_DEBUG, "No sash manager for container");
		return;
	}

	// Clear existing sashes
	while (con->sash_manager.sashes->length > 0) {
		struct sway_sash *sash = con->sash_manager.sashes->items[0];
		sash_destroy(sash);
	}

	// Only create sashes for horizontal and vertical splits
	enum sway_container_layout layout = con->current.layout;
	sway_log(SWAY_DEBUG, "update_sashes: layout=%d (L_HORIZ=%d, L_VERT=%d)",
		layout, L_HORIZ, L_VERT);

	if (layout != L_HORIZ && layout != L_VERT) {
		return;
	}

	// Don't create sashes if fullscreen
	if (con->current.fullscreen_mode != FULLSCREEN_NONE) {
		sway_log(SWAY_DEBUG, "Container is fullscreen, skipping sashes");
		return;
	}

	list_t *children = con->current.children;
	if (!children || children->length < 2) {
		sway_log(SWAY_DEBUG, "Not enough children (%d) for sashes",
			children ? children->length : 0);
		return;
	}

	// Get gap size
	int gaps = config->gaps_inner;
	if (con->current.workspace) {
		gaps = con->current.workspace->gaps_inner;
	}

	sway_log(SWAY_DEBUG, "Gaps: %d, sash_thickness: %d", gaps, config->sash_thickness);

	// Only show sashes if gap is large enough
	if (gaps < config->sash_thickness + 2) {
		sway_log(SWAY_DEBUG, "Gap too small for sashes");
		return;
	}

	// Create sashes between children
	sway_log(SWAY_DEBUG, "Creating %d sashes for %d children",
		children->length - 1, children->length);

	for (int i = 0; i < children->length - 1; i++) {
		struct sway_container *before = children->items[i];
		struct sway_container *after = children->items[i + 1];

		struct sway_sash *sash = sash_create(con, before, after, layout);
		if (!sash) {
			sway_log(SWAY_ERROR, "Failed to create sash %d", i);
			continue;
		}

		// Position and size the sash
		int sash_x, sash_y, sash_width, sash_height;

		if (layout == L_HORIZ) {
			// Vertical sash for horizontal layout
			sash_x = before->current.x + before->current.width +
				(gaps / 2) - (config->sash_thickness / 2) - con->current.x;
			sash_y = 0;
			sash_width = config->sash_thickness;
			sash_height = con->current.height;
		} else {
			// Horizontal sash for vertical layout
			sash_x = 0;
			sash_y = before->current.y + before->current.height +
				(gaps / 2) - (config->sash_thickness / 2) - con->current.y;
			sash_width = con->current.width;
			sash_height = config->sash_thickness;
		}

		sway_log(SWAY_DEBUG, "Sash %d: pos=(%d,%d) size=(%d,%d)",
			i, sash_x, sash_y, sash_width, sash_height);

		// Update rect size and position
		wlr_scene_rect_set_size(sash->rect, sash_width, sash_height);
		wlr_scene_node_set_position(&sash->rect->node, sash_x, sash_y);
		wlr_scene_node_set_enabled(&sash->rect->node, true);
	}

	sway_log(SWAY_DEBUG, "Created %d sashes successfully", con->sash_manager.sashes->length);
}

struct sway_sash *sash_try_get(struct wlr_scene_node *node) {
	return scene_descriptor_try_get(node, SWAY_SCENE_DESC_SASH);
}
