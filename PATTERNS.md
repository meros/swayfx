# SwayFX Code Patterns and Examples

This document provides concrete code examples for common development patterns in SwayFX. All examples are taken from actual codebase files.

## Command Implementation Patterns

### Basic Command Handler

**Example: `sway/commands/blur.c`**

```c
#include "sway/commands.h"
#include "sway/config.h"

struct cmd_results *cmd_blur(int argc, char **argv) {
    struct cmd_results *error = NULL;
    if ((error = checkarg(argc, "blur", EXPECTED_EQUAL, 1))) {
        return error;
    }

    if (strcmp(argv[0], "enable") == 0) {
        config->blur_enabled = true;
    } else if (strcmp(argv[0], "disable") == 0) {
        config->blur_enabled = false;
    } else {
        return cmd_results_new(CMD_INVALID,
            "Expected 'blur enable|disable'");
    }

    return cmd_results_new(CMD_SUCCESS, NULL);
}
```

**Pattern breakdown:**
1. Include necessary headers
2. Validate argument count with `checkarg()`
3. Parse arguments (strcmp for string matching)
4. Modify config/state
5. Return success or error result

### Command with Numeric Parameter

**Example: `sway/commands/blur_radius.c`**

```c
#include "sway/commands.h"
#include "sway/config.h"
#include <stdlib.h>

struct cmd_results *cmd_blur_radius(int argc, char **argv) {
    struct cmd_results *error = NULL;
    if ((error = checkarg(argc, "blur_radius", EXPECTED_EQUAL, 1))) {
        return error;
    }

    char *end;
    int value = strtol(argv[0], &end, 10);

    if (*end || value < 0 || value > 10) {
        return cmd_results_new(CMD_INVALID,
            "Invalid blur_radius value. Expected integer 0-10.");
    }

    config->blur_params.radius = value;
    return cmd_results_new(CMD_SUCCESS, NULL);
}
```

**Pattern breakdown:**
1. Use `strtol()` for string-to-int conversion
2. Check for parse errors (`*end != '\0'`)
3. Validate range
4. Update config structure
5. Return appropriate result

### Command with Float Parameter

**Example: `sway/commands/blur_noise.c`**

```c
#include "sway/commands.h"
#include "sway/config.h"
#include <stdlib.h>

struct cmd_results *cmd_blur_noise(int argc, char **argv) {
    struct cmd_results *error = NULL;
    if ((error = checkarg(argc, "blur_noise", EXPECTED_EQUAL, 1))) {
        return error;
    }

    char *end;
    float value = strtof(argv[0], &end);

    if (*end || value < 0.0f || value > 1.0f) {
        return cmd_results_new(CMD_INVALID,
            "Invalid blur_noise value. Expected float 0.0-1.0.");
    }

    config->blur_params.noise = value;
    return cmd_results_new(CMD_SUCCESS, NULL);
}
```

**Pattern breakdown:**
- Similar to numeric, but uses `strtof()` for float parsing
- Validate float range
- Store in float field

### Command with Variable Arguments

**Example: `sway/commands/assign.c`** (simplified)

```c
struct cmd_results *cmd_assign(int argc, char **argv) {
    struct cmd_results *error = NULL;
    if ((error = checkarg(argc, "assign", EXPECTED_AT_LEAST, 2))) {
        return error;
    }

    // First argument: criteria string
    char *criteria = argv[0];

    // Remaining arguments: workspace target
    char *target = join_args(argv + 1, argc - 1);

    // Create assignment rule
    struct sway_assignment *assignment = calloc(1, sizeof(*assignment));
    assignment->criteria = criteria_parse(criteria);
    assignment->target = target;

    list_add(config->workspace_assignments, assignment);
    return cmd_results_new(CMD_SUCCESS, NULL);
}
```

**Pattern breakdown:**
- `EXPECTED_AT_LEAST` for variable arguments
- `join_args()` to combine multiple arguments into single string
- Create structure, parse criteria, store in config list

## Container Management Patterns

### Creating a Container

**Example: `sway/tree/container.c:container_create()`**

```c
struct sway_container *container_create(struct sway_view *view) {
    struct sway_container *c = calloc(1, sizeof(struct sway_container));
    if (!c) {
        sway_log(SWAY_ERROR, "Unable to allocate sway_container");
        return NULL;
    }

    node_init(&c->node, N_CONTAINER, c);
    c->pending.layout = L_NONE;
    c->view = view;
    c->alpha = 1.0f;

    // SwayFX defaults
    c->blur_enabled = config->blur_enabled;
    c->shadow_enabled = config->shadows_enabled;
    c->corner_radius = config->corner_radius;

    // Initialize scene tree
    c->scene_tree = wlr_scene_tree_create(root->staging);
    c->content_tree = wlr_scene_tree_create(c->scene_tree);

    // Initialize Wayland listeners
    wl_signal_init(&c->events.destroy);

    return c;
}
```

**Pattern breakdown:**
1. `calloc()` for zero-initialized allocation
2. NULL check with error logging
3. Initialize node structure
4. Set default values (from config)
5. Create scene tree hierarchy
6. Initialize event signals
7. Return pointer

### Destroying a Container

**Example: `sway/tree/container.c:container_destroy()`**

```c
void container_destroy(struct sway_container *con) {
    if (!con) {
        return;
    }

    // Signal destruction to listeners
    wl_signal_emit_mutable(&con->events.destroy, con);

    // Remove from parent
    container_detach(con);

    // Clean up scene tree (automatically destroys children)
    if (con->scene_tree) {
        wlr_scene_node_destroy(&con->scene_tree->node);
    }

    // Free dynamic allocations
    free(con->title);
    free(con->formatted_title);
    list_free(con->marks);

    free(con);
}
```

**Pattern breakdown:**
1. NULL check first
2. Emit destroy signal (notify listeners)
3. Remove from parent/hierarchy
4. Destroy scene tree (cascades to children)
5. Free all dynamic allocations
6. Free structure itself

## Wayland Listener Patterns

### Setting Up a Listener

**Example: `sway/desktop/layer_shell.c`**

```c
struct sway_layer_surface {
    // ... fields ...
    struct wl_listener destroy;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener surface_commit;
};

static void handle_destroy(struct wl_listener *listener, void *data) {
    struct sway_layer_surface *layer =
        wl_container_of(listener, layer, destroy);

    sway_log(SWAY_DEBUG, "Layer surface destroyed");

    // Clean up
    layer_surface_cleanup(layer);
}

static void handle_map(struct wl_listener *listener, void *data) {
    struct sway_layer_surface *layer =
        wl_container_of(listener, layer, map);

    // Surface is now visible
    layer->mapped = true;
    arrange_layers(layer->output);
}

void create_layer_surface(struct wlr_layer_surface_v1 *wlr_layer_surface) {
    struct sway_layer_surface *layer = calloc(1, sizeof(*layer));
    layer->layer_surface = wlr_layer_surface;

    // Register listeners
    layer->destroy.notify = handle_destroy;
    wl_signal_add(&wlr_layer_surface->events.destroy, &layer->destroy);

    layer->map.notify = handle_map;
    wl_signal_add(&wlr_layer_surface->events.map, &layer->map);

    // ... more listeners ...
}
```

**Pattern breakdown:**
1. Declare `struct wl_listener` in parent structure
2. Create static handler functions with signature `void handler(struct wl_listener *, void *)`
3. Use `wl_container_of()` to get parent structure from listener
4. Set `.notify` function pointer
5. Use `wl_signal_add()` to register

### Cleaning Up Listeners

**Example: From any listener cleanup**

```c
static void handle_destroy(struct wl_listener *listener, void *data) {
    struct sway_layer_surface *layer =
        wl_container_of(listener, layer, destroy);

    // IMPORTANT: Remove all other listeners first
    wl_list_remove(&layer->map.link);
    wl_list_remove(&layer->unmap.link);
    wl_list_remove(&layer->surface_commit.link);
    // Don't remove destroy.link - we're in the destroy handler

    // Then cleanup
    free(layer);
}
```

**Pattern breakdown:**
- Always `wl_list_remove()` before freeing
- Don't remove the current listener (we're in its callback)
- Remove in reverse order or all at once (doesn't matter)

## List Management Patterns

### Creating and Populating a List

**Example: From `common/list.c` usage**

```c
#include "list.h"

// Create list
list_t *my_list = create_list();

// Add items
list_add(my_list, item1);
list_add(my_list, item2);
list_add(my_list, item3);

// Insert at specific position
list_insert(my_list, 1, item_between);

// Iterate
for (int i = 0; i < my_list->length; i++) {
    struct my_type *item = my_list->items[i];
    // ... use item ...
}

// Find item
int index = list_find(my_list, search_item);
if (index != -1) {
    // Found at index
}

// Remove item (doesn't free)
list_del(my_list, index);

// Free list (doesn't free items!)
list_free(my_list);
```

**Important notes:**
- `list_free()` does NOT free items, only the list container
- Must manually free items before `list_free()` if needed
- Lists grow dynamically (no size limit)

### List with Cleanup Function

**Example: Freeing list items**

```c
// Free all items, then list
for (int i = 0; i < my_list->length; i++) {
    free_my_item(my_list->items[i]);
}
list_free(my_list);

// Or use list_free_items_and_destroy() if available
```

## Configuration Parsing Patterns

### Parsing Boolean Config

**Example: From `sway/commands/*.c`**

```c
bool parse_boolean(const char *arg, bool *result) {
    if (strcmp(arg, "enable") == 0 || strcmp(arg, "true") == 0 ||
        strcmp(arg, "yes") == 0 || strcmp(arg, "on") == 0) {
        *result = true;
        return true;
    } else if (strcmp(arg, "disable") == 0 || strcmp(arg, "false") == 0 ||
               strcmp(arg, "no") == 0 || strcmp(arg, "off") == 0) {
        *result = false;
        return true;
    }
    return false;
}

// Usage
struct cmd_results *cmd_my_feature(int argc, char **argv) {
    bool value;
    if (!parse_boolean(argv[0], &value)) {
        return cmd_results_new(CMD_INVALID,
            "Expected enable/disable");
    }
    config->my_feature = value;
    return cmd_results_new(CMD_SUCCESS, NULL);
}
```

### Parsing Color Values

**Example: Color parsing pattern**

```c
#include <stdint.h>

// Parse hex color: RRGGBB or RRGGBBAA
bool parse_color(const char *hex, float color[4]) {
    if (hex[0] == '#') {
        hex++;
    }

    int len = strlen(hex);
    if (len != 6 && len != 8) {
        return false;
    }

    uint32_t rgba = strtoul(hex, NULL, 16);

    if (len == 6) {
        // RGB, assume full alpha
        color[0] = ((rgba >> 16) & 0xFF) / 255.0f;  // R
        color[1] = ((rgba >> 8) & 0xFF) / 255.0f;   // G
        color[2] = (rgba & 0xFF) / 255.0f;          // B
        color[3] = 1.0f;                            // A
    } else {
        // RGBA
        color[0] = ((rgba >> 24) & 0xFF) / 255.0f;
        color[1] = ((rgba >> 16) & 0xFF) / 255.0f;
        color[2] = ((rgba >> 8) & 0xFF) / 255.0f;
        color[3] = (rgba & 0xFF) / 255.0f;
    }

    return true;
}
```

## Transaction Patterns

### Marking Container Dirty

**Example: From `sway/tree/container.c`**

```c
void container_set_geometry_from_content(struct sway_container *con) {
    if (!con->view) {
        return;
    }

    // Update pending state
    con->pending.content_x = con->current.content_x;
    con->pending.content_y = con->current.content_y;
    con->pending.content_width = con->view->geometry.width;
    con->pending.content_height = con->view->geometry.height;

    // Mark for transaction
    container_set_dirty(con);
}

void container_set_dirty(struct sway_container *con) {
    if (con->node.dirty) {
        return;  // Already dirty
    }
    con->node.dirty = true;
    list_add(server.dirty_nodes, &con->node);
}
```

**Pattern breakdown:**
1. Modify `pending` state (not `current`)
2. Call `container_set_dirty()` to mark for transaction
3. Transaction system will commit changes atomically

### Arranging Layout

**Example: Triggering layout recalculation**

```c
void handle_container_resize(struct sway_container *con, int width, int height) {
    // Update pending size
    con->pending.content_width = width;
    con->pending.content_height = height;

    // Mark dirty
    container_set_dirty(con);

    // Trigger layout recalculation
    arrange_container(con);

    // Transaction will commit on next idle
}
```

## Scene Graph Patterns

### Creating Effect Nodes

**Example: Adding blur to container (simplified)**

```c
void container_update_blur(struct sway_container *con) {
    // Remove existing blur node if present
    if (con->blur_node) {
        wlr_scene_node_destroy(&con->blur_node->node);
        con->blur_node = NULL;
    }

    // Create new blur node if enabled
    if (con->blur_enabled) {
        con->blur_node = wlr_scene_blur_create(con->scene_tree);

        // Configure blur parameters
        wlr_scene_blur_set_radius(con->blur_node, config->blur_params.radius);
        wlr_scene_blur_set_passes(con->blur_node, config->blur_params.passes);
        wlr_scene_blur_set_noise(con->blur_node, config->blur_params.noise);
    }
}
```

**Pattern breakdown:**
1. Destroy existing effect node (if any)
2. Create new effect node from scenefx
3. Configure effect parameters
4. Effect node automatically added to scene tree

### Scene Tree Hierarchy

**Example: Creating container scene tree**

```c
struct sway_container *container_create(struct sway_view *view) {
    struct sway_container *c = calloc(1, sizeof(*c));

    // Create root scene tree for container
    c->scene_tree = wlr_scene_tree_create(parent_tree);

    // Create border tree (child of scene_tree)
    c->border.tree = wlr_scene_tree_create(c->scene_tree);
    c->border.top = wlr_scene_rect_create(c->border.tree, 0, 0, border_color);
    c->border.bottom = wlr_scene_rect_create(c->border.tree, 0, 0, border_color);
    c->border.left = wlr_scene_rect_create(c->border.tree, 0, 0, border_color);
    c->border.right = wlr_scene_rect_create(c->border.tree, 0, 0, border_color);

    // Create title bar tree (sibling to border)
    c->title_bar_tree = wlr_scene_tree_create(c->scene_tree);

    // Create content tree (contains actual window)
    c->content_tree = wlr_scene_tree_create(c->scene_tree);

    return c;
}
```

**Hierarchy:**
```
scene_tree (container root)
├── border.tree
│   ├── top (rect)
│   ├── bottom (rect)
│   ├── left (rect)
│   └── right (rect)
├── title_bar_tree
│   └── title_buffer
└── content_tree
    ├── blur_node (if enabled)
    ├── shadow_node (if enabled)
    └── surface_tree (window content)
```

## IPC Patterns

### Handling IPC Commands

**Example: `sway/ipc-server.c:ipc_client_handle_command()`**

```c
void ipc_client_handle_command(struct ipc_client *client) {
    // Read message header
    char header[IPC_HEADER_SIZE];
    read(client->fd, header, IPC_HEADER_SIZE);

    uint32_t payload_length = *(uint32_t *)(header + 6);
    uint32_t message_type = *(uint32_t *)(header + 10);

    // Read payload
    char *payload = malloc(payload_length + 1);
    read(client->fd, payload, payload_length);
    payload[payload_length] = '\0';

    // Dispatch based on type
    switch (message_type) {
        case IPC_COMMAND:
            handle_command(client, payload);
            break;
        case IPC_GET_TREE:
            handle_get_tree(client);
            break;
        case IPC_GET_OUTPUTS:
            handle_get_outputs(client);
            break;
        // ... more cases ...
    }

    free(payload);
}
```

### JSON Serialization

**Example: Serializing container tree**

```c
#include <json-c/json.h>

json_object *ipc_json_describe_container(struct sway_container *con) {
    json_object *object = json_object_new_object();

    json_object_object_add(object, "id", json_object_new_int(con->node.id));
    json_object_object_add(object, "name", json_object_new_string(con->title));
    json_object_object_add(object, "type", json_object_new_string("con"));

    // Geometry
    json_object *rect = json_object_new_object();
    json_object_object_add(rect, "x", json_object_new_int(con->current.x));
    json_object_object_add(rect, "y", json_object_new_int(con->current.y));
    json_object_object_add(rect, "width", json_object_new_int(con->current.width));
    json_object_object_add(rect, "height", json_object_new_int(con->current.height));
    json_object_object_add(object, "rect", rect);

    // SwayFX properties
    json_object_object_add(object, "blur_enabled",
        json_object_new_boolean(con->blur_enabled));
    json_object_object_add(object, "corner_radius",
        json_object_new_int(con->corner_radius));

    return object;
}
```

## Logging Patterns

### Standard Logging

```c
#include "log.h"

// Error logging (always shown)
sway_log(SWAY_ERROR, "Failed to create container: %s", strerror(errno));

// Info logging (default level)
sway_log(SWAY_INFO, "Output %s connected", output->name);

// Debug logging (requires -d flag)
sway_log(SWAY_DEBUG, "Container %p blur_enabled=%d", con, con->blur_enabled);

// Verbose logging (requires -V flag)
sway_log(SWAY_DEBUG, "Detailed state: x=%d y=%d w=%d h=%d",
    con->x, con->y, con->width, con->height);
```

### Conditional Logging

```c
if (server.debug_txn_timings) {
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    float ms = (now.tv_sec - txn->commit_time.tv_sec) * 1000.0f +
               (now.tv_nsec - txn->commit_time.tv_nsec) / 1000000.0f;
    sway_log(SWAY_DEBUG, "Transaction %p took %.2fms", txn, ms);
}
```

## Error Handling Patterns

### Null Pointer Safety

```c
void some_function(struct sway_container *con) {
    // ALWAYS check pointers first
    if (!con) {
        sway_log(SWAY_ERROR, "Received NULL container");
        return;
    }

    // Safe to use con here
}
```

### Resource Cleanup on Error

```c
struct sway_container *create_container_safe(struct sway_view *view) {
    struct sway_container *con = calloc(1, sizeof(*con));
    if (!con) {
        return NULL;
    }

    con->scene_tree = wlr_scene_tree_create(root->staging);
    if (!con->scene_tree) {
        free(con);  // Clean up partial allocation
        return NULL;
    }

    con->content_tree = wlr_scene_tree_create(con->scene_tree);
    if (!con->content_tree) {
        wlr_scene_node_destroy(&con->scene_tree->node);  // Cleanup scene tree
        free(con);
        return NULL;
    }

    return con;
}
```

### Assertion for Invariants

```c
#include "sway/assert.h"

void container_add_child(struct sway_container *parent,
                         struct sway_container *child) {
    // Validate invariants
    sway_assert(parent != NULL, "Parent cannot be NULL");
    sway_assert(child != NULL, "Child cannot be NULL");
    sway_assert(parent != child, "Cannot add container as its own child");

    // Proceed with operation
    list_add(parent->children, child);
    child->parent = parent;
}
```

## String Handling Patterns

### Safe String Operations

```c
#include "stringop.h"

// Join array of strings
char *joined = join_args(argv, argc);

// Strip whitespace
char *trimmed = strip_whitespace(input);

// Format string (allocated)
char *formatted = format_str("Container: %s (%d)", name, id);

// Always free when done
free(joined);
free(trimmed);
free(formatted);
```

### String Comparison

```c
// Standard comparison
if (strcmp(str1, str2) == 0) {
    // Equal
}

// Case-insensitive
if (strcasecmp(str1, str2) == 0) {
    // Equal (ignoring case)
}

// Lenient comparison (handles NULL)
if (lenient_strcmp(str1, str2) == 0) {
    // Equal (NULL-safe)
}

// Prefix check
if (strncmp(str, "prefix", 6) == 0) {
    // Starts with "prefix"
}
```

## Criteria Matching Patterns

### Applying Configuration by Criteria

**Example: From `sway/criteria.c`**

```c
// Define criteria string
char *criteria_string = "[app_id=\"firefox\"]";

// Parse criteria
struct criteria *criteria = criteria_parse(criteria_string);

// Check if container matches
bool matches(struct sway_container *con, struct criteria *criteria) {
    if (criteria->app_id) {
        if (!con->view || !con->view->app_id) {
            return false;
        }
        if (!regex_match(criteria->app_id, con->view->app_id)) {
            return false;
        }
    }

    if (criteria->title) {
        if (!con->title) {
            return false;
        }
        if (!regex_match(criteria->title, con->title)) {
            return false;
        }
    }

    // ... more criteria checks ...

    return true;
}

// Apply command to matching containers
void for_each_matching_container(struct criteria *criteria,
                                   void (*callback)(struct sway_container *)) {
    for (int i = 0; i < root->outputs->length; i++) {
        struct sway_output *output = root->outputs->items[i];
        for (int j = 0; j < output->workspaces->length; j++) {
            struct sway_workspace *ws = output->workspaces->items[j];
            for (int k = 0; k < ws->tiling->length; k++) {
                struct sway_container *con = ws->tiling->items[k];
                if (matches(con, criteria)) {
                    callback(con);
                }
            }
        }
    }
}
```

## Common Gotchas and Solutions

### Gotcha: Forgetting to Remove Wayland Listeners

**Problem:**
```c
// BAD: Listeners not removed before free
struct sway_layer_surface *layer = ...;
free(layer);  // CRASH: listeners still registered!
```

**Solution:**
```c
// GOOD: Remove listeners first
wl_list_remove(&layer->destroy.link);
wl_list_remove(&layer->map.link);
wl_list_remove(&layer->unmap.link);
free(layer);
```

### Gotcha: List Ownership Confusion

**Problem:**
```c
// BAD: Assuming list_free() frees items
list_t *my_list = create_list();
list_add(my_list, malloc(100));
list_free(my_list);  // LEAK: item not freed!
```

**Solution:**
```c
// GOOD: Free items manually
for (int i = 0; i < my_list->length; i++) {
    free(my_list->items[i]);
}
list_free(my_list);
```

### Gotcha: Modifying Current State Directly

**Problem:**
```c
// BAD: Modifying current state (bypasses transactions)
con->current.width = 500;
```

**Solution:**
```c
// GOOD: Modify pending state and mark dirty
con->pending.width = 500;
container_set_dirty(con);
arrange_container(con);
// Transaction system applies change atomically
```

### Gotcha: Scene Node Lifecycle

**Problem:**
```c
// BAD: Accessing scene node after parent destroyed
wlr_scene_node_destroy(&con->scene_tree->node);
wlr_scene_node_destroy(&con->content_tree->node);  // CRASH: already destroyed!
```

**Solution:**
```c
// GOOD: Destroying parent destroys all children automatically
wlr_scene_node_destroy(&con->scene_tree->node);
// content_tree is automatically destroyed (child of scene_tree)
```

---

*This patterns guide is optimized for AI assistants. Refer to actual source files for complete implementations.*
