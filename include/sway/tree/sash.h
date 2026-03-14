#ifndef _SWAY_SASH_H
#define _SWAY_SASH_H
#include <scenefx/types/wlr_scene.h>
#include "sway/tree/container.h"

/**
 * Represents an interactive divider between tiled containers.
 * Sashes provide visual indication of resizable boundaries and
 * enable intuitive mouse-based resizing.
 */
struct sway_sash {
	struct wlr_scene_rect *rect;           // Visual scene node
	struct sway_container *parent;          // Parent container
	struct sway_container *before;          // Container before gap
	struct sway_container *after;           // Container after gap
	enum sway_container_layout orientation; // L_HORIZ or L_VERT
	bool hover;                             // Hover state for visual feedback
};

/**
 * Manages all sashes for a container.
 * Parent containers own sashes for gaps between their children.
 */
struct sway_sash_manager {
	struct wlr_scene_tree *tree;           // Scene tree parent for sashes
	list_t *sashes;                         // List of struct sway_sash*
};

/**
 * Initialize the sash manager for a container.
 */
void sash_manager_init(struct sway_container *con);

/**
 * Destroy all sashes and cleanup the manager.
 */
void sash_manager_destroy(struct sway_container *con);

/**
 * Create a sash between two containers.
 */
struct sway_sash *sash_create(struct sway_container *parent,
	struct sway_container *before, struct sway_container *after,
	enum sway_container_layout orientation);

/**
 * Destroy an individual sash.
 */
void sash_destroy(struct sway_sash *sash);

/**
 * Update sashes for a container based on current layout.
 * This is called during arrange_children to sync sashes with container positions.
 */
void update_sashes(struct sway_container *con);

/**
 * Get the scene descriptor data for a sash node.
 */
struct sway_sash *sash_try_get(struct wlr_scene_node *node);

#endif
