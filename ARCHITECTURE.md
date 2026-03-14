# SwayFX Architecture Deep Dive

This document provides detailed technical information about SwayFX's internal architecture for AI assistants working on complex features.

## Core Data Structures

### Server Structure (`struct sway_server`)

Located in `include/sway/server.h` - The root context containing all subsystems:

```c
struct sway_server {
    struct wl_display *wl_display;
    struct wl_event_loop *wl_event_loop;

    struct wlr_backend *backend;
    struct wlr_compositor *compositor;
    struct wlr_scene *scene;
    struct wlr_scene_presentation *presentation;

    struct sway_input_manager *input;
    struct wl_list outputs;  // sway_output::link

    struct wlr_idle_notifier_v1 *idle_notifier_v1;
    struct sway_idle_inhibit_manager_v1 *idle_inhibit_manager_v1;

    struct wlr_layer_shell_v1 *layer_shell;
    struct wlr_xdg_shell *xdg_shell;
    struct wlr_tablet_manager_v2 *tablet_v2;

    struct wlr_xwayland *xwayland;
    struct wlr_xwayland_shell_v1 *xwayland_shell_v1;

    bool debug_txn_timings;
    bool debug_tree;

    list_t *dirty_nodes;  // Containers needing updates
    list_t *transactions;
};
```

**Key Subsystems:**
- `wl_display` - Main Wayland display server
- `backend` - Hardware backend (DRM, Wayland nested, X11)
- `scene` - wlroots scene graph root
- `input` - Input device manager
- `outputs` - List of connected monitors
- `dirty_nodes` - Containers needing transaction updates

### Container Structure (`struct sway_container`)

Located in `include/sway/tree/container.h` - Represents a tiling window or container group:

```c
struct sway_container {
    struct sway_node node;
    struct sway_view *view;  // NULL if split container

    struct wlr_scene_tree *scene_tree;
    struct wlr_scene_tree *content_tree;
    struct wlr_scene_tree *title_bar_tree;

    struct {
        struct wlr_scene_tree *tree;
        struct wlr_scene_rect *top, *bottom, *left, *right;
    } border;

    struct sway_container_state current;
    struct sway_container_state pending;

    char *title;
    char *formatted_title;

    enum sway_container_layout layout;
    enum sway_container_layout prev_split_layout;

    bool urgent;
    bool sticky;
    bool fullscreen_mode;

    // SwayFX-specific effect properties
    bool blur_enabled;
    bool shadow_enabled;
    int corner_radius;

    double alpha;

    struct wl_listener surface_commit;
    struct wl_listener surface_destroy;

    list_t *marks;  // char *

    struct wl_list link;  // workspace::tiling/floating
    struct wl_list workspace_link;
};
```

**State System:**
- `current` - Currently rendered state
- `pending` - State being prepared in transaction
- Transactions atomically swap pending → current

**Scene Trees:**
- `scene_tree` - Root of container's visual tree
- `content_tree` - Actual window content
- `title_bar_tree` - Title bar decorations
- `border.*` - Border decorations (top, bottom, left, right)

### Output Structure (`struct sway_output`)

Located in `include/sway/output.h` - Represents a physical or virtual display:

```c
struct sway_output {
    struct sway_node node;

    struct wlr_output *wlr_output;
    struct wlr_scene_output *scene_output;
    struct sway_server *server;
    struct wl_list link;

    struct wl_list layers[4];  // layer_surface::link
    // [0] = BACKGROUND, [1] = BOTTOM, [2] = TOP, [3] = OVERLAY

    struct wlr_box usable_area;

    struct wl_list workspaces;
    struct sway_workspace *current_workspace;

    bool enabled;
    bool enabling;

    list_t *current_modes;  // Output modes

    // SwayFX-specific: optimized blur for static backgrounds
    struct wlr_scene_optimized_blur *blur_layer;

    struct {
        struct wl_signal disable;
    } events;

    struct wl_listener layout_destroy;
    struct wl_listener destroy;
    struct wl_listener commit;
    struct wl_listener present;
    struct wl_listener damage;
    struct wl_listener frame;
    struct wl_listener needs_frame;

    struct {
        pid_t pid;
        // ...
    } test_data;
};
```

**Layer Organization:**
- Outputs have 4 layer shell lists (background, bottom, top, overlay)
- Each layer contains layer_surface objects (panels, notifications, etc.)
- Strict z-ordering: background < bottom < tiling < top < overlay

### Layer Surface (`struct sway_layer_surface`)

Located in `include/sway/layers.h` - Represents system UI elements:

```c
struct sway_layer_surface {
    struct wlr_layer_surface_v1 *layer_surface;
    struct wl_list link;

    struct wl_listener destroy;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener surface_commit;
    struct wl_listener output_destroy;
    struct wl_listener node_destroy;
    struct wl_listener new_popup;

    struct wlr_box geo;
    bool mapped;
    struct wlr_scene_layer_surface_v1 *scene;
    struct wlr_scene_tree *tree;
    struct wlr_scene_tree *popups;
    struct sway_output *output;

    // SwayFX effect nodes
    struct wlr_scene_blur *blur_node;
    struct wlr_scene_shadow *shadow_node;

    // Effect configuration
    bool blur_enabled, shadow_enabled;
    bool blur_ignore_transparent;
    int corner_radius;
};
```

### View Structure (`struct sway_view`)

Located in `include/sway/tree/view.h` - Represents an individual client window:

```c
struct sway_view {
    enum sway_view_type type;
    const struct sway_view_impl *impl;

    struct sway_container *container;
    struct wlr_surface *surface;

    pid_t pid;

    char *title_format;
    char *app_id;  // Wayland app identifier
    char *class;   // X11 class
    char *instance;

    bool using_csd;  // Client-side decorations

    struct {
        struct wl_signal unmap;
    } events;

    struct wl_listener surface_new_subsurface;
    struct wl_listener container_reparent;

    union {
        struct wlr_xdg_toplevel *wlr_xdg_toplevel;
        struct wlr_xwayland_surface *wlr_xwayland_surface;
    };

    // XDG decoration negotiation
    struct wlr_xdg_toplevel_decoration_v1 *xdg_decoration;
    struct wl_listener xdg_decoration_destroy;
    struct wl_listener xdg_decoration_request_mode;
};
```

## Subsystem Deep Dives

### Input Management

**Architecture:**
```
Input Event
    ↓
libinput (physical device)
    ↓
wlroots input abstraction
    ↓
sway_input_manager (routes to seats)
    ↓
sway_seat (focus management)
    ↓
seat operation (seatop_*)
    ↓
Action (move, resize, default)
```

**Key files:**
- `sway/input/input-manager.c` - Device hotplug, seat management
- `sway/input/seat.c` - Focus tracking, input routing (53K lines)
- `sway/input/cursor.c` - Mouse cursor handling (46K lines)
- `sway/input/keyboard.c` - Keyboard input, bindings (37K lines)
- `sway/input/seatop_*.c` - Input operation states

**Seat Operations:**
- `seatop_default` - Normal mode (focus, bindings)
- `seatop_move` - Moving floating window
- `seatop_resize_floating` - Resizing floating window
- `seatop_resize_tiling` - Resizing tiled window
- `seatop_down` - Mouse button held

### Transaction System

**Purpose:** Atomic state updates to prevent visual glitches during complex operations.

**Flow:**
```
1. User action (e.g., resize window)
       ↓
2. Modify container tree (set pending state)
       ↓
3. Mark containers dirty
       ↓
4. Create transaction
       ↓
5. Calculate layout (arrange_*)
       ↓
6. Commit transaction
       ↓
7. Wait for client acknowledgment (configure → ack)
       ↓
8. Apply transaction atomically
       ↓
9. Update rendering (scene graph)
```

**Key files:**
- `sway/desktop/transaction.c` - Transaction lifecycle (1156 lines)
- `sway/tree/arrange.c` - Layout calculation

**Implementation details:**
```c
struct sway_transaction {
    struct wl_event_source *timer;
    list_t *instructions;  // Per-container state snapshots
    size_t num_waiting;    // Containers waiting for commit
    size_t num_configures; // Configure events sent
    struct timespec commit_time;
};

struct sway_transaction_instruction {
    struct sway_transaction *transaction;
    struct sway_node *node;
    struct sway_container_state state;  // Snapshot
    bool server_request;  // Wait for client ack?
};
```

**Transaction timing:**
- Timeout: 200ms maximum wait for slow clients
- Batching: Multiple changes combined into single transaction
- Serialization: Transactions applied in order

### IPC Protocol

**Architecture:** UNIX domain socket with JSON messages.

**Socket location:** `$XDG_RUNTIME_DIR/sway-ipc.$PID.sock`

**Message format:**
```
[Header: 14 bytes]
  - Magic: "i3-ipc" (6 bytes)
  - Payload length: uint32
  - Message type: uint32
[Payload: JSON]
```

**Message types:**
- `IPC_COMMAND` (0) - Execute command
- `IPC_GET_WORKSPACES` (1) - List workspaces
- `IPC_SUBSCRIBE` (2) - Subscribe to events
- `IPC_GET_OUTPUTS` (3) - List outputs
- `IPC_GET_TREE` (4) - Get container tree
- `IPC_GET_MARKS` (5) - Get marks
- `IPC_GET_BAR_CONFIG` (6) - Get bar configuration
- `IPC_GET_VERSION` (7) - Get version info
- `IPC_GET_BINDING_MODES` (8) - Get binding modes
- `IPC_GET_CONFIG` (9) - Get config
- `IPC_SEND_TICK` (10) - Send tick event
- `IPC_GET_INPUTS` (100) - Get input devices
- `IPC_GET_SEATS` (101) - Get seats

**Event types:**
- `workspace` - Workspace changes
- `mode` - Binding mode changes
- `window` - Window events (new, close, focus, title, etc.)
- `output` - Output changes
- `shutdown` - Compositor shutdown
- `tick` - Custom tick events
- `bar_state_update` - Bar visibility changes
- `input` - Input device changes

**Key files:**
- `sway/ipc-server.c` - Protocol implementation (977 lines)
- `sway/ipc-json.c` - JSON serialization (54K lines)

### Command System

**Registration:**
Commands are registered in `sway/commands.c` in the `handlers[]` array:

```c
static const struct cmd_handler handlers[] = {
    { "assign", cmd_assign },
    { "bar", cmd_bar },
    { "blur", cmd_blur },
    { "blur_brightness", cmd_blur_brightness },
    // ... 140+ commands, alphabetically sorted
};
```

**Execution flow:**
```
Config file / IPC command
    ↓
find_handler() in commands.c
    ↓
cmd_handler function
    ↓
Validate arguments (checkarg)
    ↓
Modify global/container state
    ↓
Return cmd_results
```

**Handler signature:**
```c
struct cmd_results *cmd_name(int argc, char **argv);
```

**Argument validation:**
```c
struct cmd_results *error = checkarg(argc, "command_name",
                                     EXPECTED_EQUAL, 2);
```

**Validation types:**
- `EXPECTED_AT_LEAST` - Minimum arguments
- `EXPECTED_AT_MOST` - Maximum arguments
- `EXPECTED_EQUAL` - Exact argument count
- `EXPECTED_MORE_THAN` - More than N arguments
- `EXPECTED_LESS_THAN` - Fewer than N arguments

**Subcommands:**
Some commands have subcommand hierarchies:
- `bar` → `bar subcommand` (handled by `cmd_bar()` dispatcher)
- `input` → `input <device> subcommand` (handled by `cmd_input()`)
- `output` → `output <name> subcommand` (handled by `cmd_output()`)

**Config-only vs runtime commands:**
- Some commands only valid during config load (`workspace_layout`)
- Others only valid at runtime (some IPC commands)
- Most work in both contexts

### Rendering Pipeline

**SceneFX Integration:**

SwayFX replaces wlroots' `wlr_renderer` with `fx_renderer` from the scenefx library.

**Rendering flow:**
```
Output frame event
    ↓
output_render() in desktop/output.c
    ↓
wlr_scene_output_commit()
    ↓
SceneFX scene graph traversal
    ↓
For each node:
    - Render regular buffers
    - Apply blur nodes (multi-pass Gaussian)
    - Apply shadow nodes
    - Apply corner radius (via shader)
    ↓
Present to display (DRM/Wayland/X11)
```

**Effect node types:**
- `struct wlr_scene_blur` - Per-window/layer blur
- `struct wlr_scene_shadow` - Drop shadow effect
- `struct wlr_scene_optimized_blur` - Static background blur (faster)

**Blur implementation:**
1. Capture background into texture
2. Downsample (for performance)
3. Apply horizontal Gaussian pass
4. Apply vertical Gaussian pass
5. Repeat for `blur_passes` iterations
6. Composite with foreground

**Shadow implementation:**
1. Extract window shape
2. Apply blur to shape
3. Offset by shadow_offset
4. Tint with shadow_color
5. Render below window

**Performance considerations:**
- **Damage tracking:** Only re-render changed regions
- **Blur optimization:** Downsample blur texture (lower resolution)
- **Shadow caching:** Cache shadow texture until window resizes
- **Layer blur:** Separate blur for static vs dynamic content

### Scene Graph Organization

**Root structure:**
```
wlr_scene (root)
├── output_layout (all outputs)
└── per-output trees
    ├── background_layer (layer shell)
    ├── bottom_layer (layer shell)
    ├── tiling_layer
    │   ├── workspace 1
    │   │   └── container tree
    │   ├── workspace 2
    │   └── ...
    ├── fullscreen_layer
    ├── floating_layer
    ├── top_layer (layer shell)
    ├── overlay_layer (layer shell)
    └── session_lock_layer
```

**Container scene tree:**
```
container->scene_tree
├── border.tree
│   ├── border.top (rect)
│   ├── border.bottom (rect)
│   ├── border.left (rect)
│   └── border.right (rect)
├── title_bar_tree
│   └── title text (buffer)
└── content_tree
    ├── blur_node (if enabled)
    ├── shadow_node (if enabled)
    └── surface_tree (actual window content)
```

**Layer surface scene tree:**
```
layer_surface->tree
├── blur_node (if enabled)
├── shadow_node (if enabled)
└── wlr_scene_layer_surface_v1
    └── surface buffers
```

## Configuration System

### Config File Structure

Located at `~/.config/sway/config` (or `$SWAY_CONFIG_DIR/config`).

**Processing order:**
1. Parse config file line-by-line
2. Handle `include` directives recursively
3. Execute commands immediately
4. Store bindings for runtime
5. Handle `workspace_layout` defaults
6. Apply outputs configuration
7. Start bar processes

**Config reload:**
```bash
swaymsg reload
```

**Reload behavior:**
- Re-reads config file
- Re-executes all commands
- Does NOT restart server
- Client connections maintained
- Window positions preserved (unless layout commands change them)

### Criteria System

Criteria allow targeting specific windows for configuration.

**Syntax:**
```
[criteria] command
```

**Example:**
```
[app_id="firefox"] opacity 0.9
[class="Alacritty" title=".*vim.*"] blur disable
```

**Criteria types:**
- `app_id` - Wayland application ID (regex)
- `class` - X11 window class (regex)
- `instance` - X11 window instance (regex)
- `title` - Window title (regex)
- `workspace` - Current workspace (regex)
- `con_mark` - Container marks (regex)
- `con_id` - Container ID (exact match)
- `floating` - Is floating (no value)
- `tiling` - Is tiling (no value)
- `urgent` - Is urgent (no value)
- `pid` - Process ID (exact match)
- `shell` - Shell type ("xdg_shell" or "xwayland")

**Implementation:**
- Uses PCre2 for regex matching
- Multiple criteria are AND'ed together
- Criteria checked on window map and property changes

**Key files:**
- `sway/criteria.c` - Criteria parsing and matching
- `sway/config.c` - Config file parsing (includes criteria handling)

### Binding System

**Binding types:**
- Keyboard bindings (`bindsym`, `bindcode`)
- Mouse bindings (`bindmouse`)
- Switch bindings (`bindswitch`) - lid close, tablet mode
- Gestures (`bindgesture`) - swipe, pinch

**Modifiers:**
- `Mod1` - Alt
- `Mod4` - Super/Windows key
- `Shift`, `Control`, `Mod2`, `Mod3`, `Mod5`

**Binding flags:**
- `--release` - Trigger on key release
- `--locked` - Work even when locked
- `--inhibited` - Work even when inhibited
- `--no-repeat` - Don't repeat when held
- `--no-warn` - Suppress warnings

**Example:**
```
bindsym Mod4+Return exec alacritty
bindsym --release Print exec grim
bindsym Mod4+Shift+q kill
```

## Effect Implementation Details

### Blur System

**Configuration hierarchy:**
1. Global blur settings (applied to all windows by default)
2. Per-container criteria (override global)
3. Layer-specific criteria (panels, notifications)

**Blur parameters:**
- `blur_passes` (0-10) - Quality (2^passes total shader passes)
- `blur_radius` (0-10) - Kernel size
- `blur_noise` (0.0-1.0) - Noise texture alpha (hides banding artifacts)
- `blur_brightness` (0.0-2.0) - Brightness adjustment
- `blur_contrast` (0.0-2.0) - Contrast adjustment
- `blur_saturation` (0.0-2.0) - Saturation adjustment
- `blur_xray` - Blur background, not underlying windows

**Implementation notes:**
- Uses dual Kawase blur algorithm (separable Gaussian approximation)
- Multi-pass for quality vs single-pass for performance
- Downsampling reduces shader workload
- Noise texture breaks up banding (dithering)

**Key functions:**
- `container_update_blur()` - Apply blur to container
- `layer_surface_configure_blur()` - Apply blur to layer surface
- `output_configure_scene()` - Setup blur nodes in scene tree

### Shadow System

**Shadow parameters:**
- `shadow_blur_radius` (0-99) - Shadow softness
- `shadow_color` (hex RGBA) - Active window shadow color
- `shadow_inactive_color` (hex RGBA) - Inactive window shadow color
- `shadow_offset` (x, y) - Shadow position offset

**Implementation:**
1. Extract window alpha channel (shape)
2. Apply Gaussian blur to shape
3. Offset by (x, y) pixels
4. Tint with color
5. Render behind window content

**Performance:**
- Shadows cached until window resizes
- Uses same blur shader as window blur
- Separate shadow for active/inactive states

### Corner Radius

**Parameters:**
- `corner_radius` (0-N) - Radius in pixels

**Implementation:**
- Applied via fragment shader
- Anti-aliased edges (smooth)
- Works with borders and titlebars
- Clips window content to rounded shape

**Technical details:**
- Uses signed distance field (SDF) for anti-aliasing
- Pixel-perfect at any radius value
- Applied to container, border, and titlebar separately

### Dim Inactive

**Parameters:**
- `dim_inactive` (0.0-1.0) - Opacity reduction for unfocused windows
- `dim_inactive_colors.unfocused` (hex RGBA) - Custom dim color/tint

**Implementation:**
- Applies alpha blending to unfocused containers
- Can also apply color tint (for custom dim color)
- Updated on focus changes

**Note:** Affects performance on large displays with many windows (overlapping transparency).

## Performance Optimization

### Damage Tracking

SwayFX uses wlroots' damage tracking to minimize rendering work.

**How it works:**
1. Window updates report damaged region (changed pixels)
2. Compositor accumulates damage across multiple windows
3. Only damaged regions re-rendered on next frame
4. Effects recalculated only for damaged areas

**Limitations:**
- Blur requires rendering behind window (expands damage region)
- Shadows extend damage region beyond window bounds
- Animations mark entire container as damaged each frame

### Effect Optimization Strategies

**Blur optimization:**
- Downsample blur texture (render blur at 50% or 25% resolution)
- Limit blur_passes (2-3 passes usually sufficient)
- Use blur_ignore_transparent for panels (skip transparent pixels)

**Shadow optimization:**
- Cache shadow texture (only regenerate on resize)
- Limit shadow_blur_radius (higher values = more shader work)
- Disable shadows on inactive windows if not needed

**General:**
- Reduce corner_radius (smaller radius = simpler shader)
- Disable effects on battery power (possible future feature)
- Use optimized_blur for static backgrounds

## Development Patterns

### Adding a New Wayland Protocol

1. **Add protocol XML**: Place in `protocols/` directory
2. **Register in build**: Add to `protocols/meson.build`
   ```python
   protocols = {
       'my-protocol': 'my-protocol.xml',
   }
   ```
3. **Generate bindings**: Meson automatically runs wayland-scanner
4. **Include generated headers**: `#include "my-protocol-protocol.h"`
5. **Implement protocol**: Create listener functions, handle requests

### Debugging Scene Graph Issues

**Dump scene graph:**
```bash
swaymsg -t get_tree > tree.json
```

**Enable debug logging:**
```bash
sway -d 2>&1 | grep -i scene
```

**Inspect with wlroots tools:**
```bash
# List scene nodes
# (requires custom debug code in output.c)
```

**Common issues:**
- Scene node not added to tree (invisible)
- Wrong z-order (node in wrong layer)
- Node destroyed prematurely (use-after-free)
- Damage not reported (stale rendering)

### Adding Custom Effect

To add a completely new effect type:

1. **Extend scenefx** (if needed):
   - Add new `wlr_scene_*` node type
   - Implement shader in scenefx library
   - Expose creation/configuration API

2. **Add configuration commands**:
   - Create command handler in `sway/commands/`
   - Register in `commands.c`
   - Add config struct fields

3. **Add container properties**:
   - Extend `struct sway_container` in `include/sway/tree/container.h`
   - Extend `struct sway_container_state` (for transactions)

4. **Apply effect in rendering**:
   - Modify `output.c` or relevant rendering function
   - Create effect node, add to scene tree
   - Handle in transaction system

5. **Document**:
   - Update man page `sway.5.scd`
   - Update README.md
   - Update this file (ARCHITECTURE.md)

## Testing and Debugging

### Manual Testing Workflow

1. **Build with debug symbols:**
   ```bash
   meson setup build/ --buildtype=debug
   ninja -C build/
   ```

2. **Run in nested Wayland session:**
   ```bash
   # Requires wlroots with Wayland backend
   WLR_BACKENDS=wayland WLR_WL_OUTPUTS=1 ./build/sway/sway
   ```

3. **Test configuration changes:**
   - Edit `~/.config/sway/config`
   - Run `swaymsg reload`
   - Observe effects

4. **Test IPC commands:**
   ```bash
   swaymsg blur enable
   swaymsg corner_radius 10
   swaymsg -t get_tree | jq '.nodes[0]'
   ```

### Debug Logging

**Log levels:**
```c
SWAY_ERROR   - Critical errors
SWAY_INFO    - Informational messages
SWAY_DEBUG   - Verbose debugging
```

**Enable debug logging:**
```bash
sway -d               # Debug level
sway -V               # Verbose (all logging)
```

**Add logging in code:**
```c
#include "log.h"

sway_log(SWAY_DEBUG, "Container %p blur_enabled: %d", con, con->blur_enabled);
```

### GDB Debugging

```bash
# Attach to running sway process
gdb -p $(pgrep sway)

# Set breakpoints
(gdb) break container_create
(gdb) break output_render

# Continue execution
(gdb) continue

# Inspect state
(gdb) print *container
(gdb) print container->blur_enabled
```

### Common Debug Scenarios

**Effect not appearing:**
- Check if effect enabled in config
- Verify container has effect property set
- Check if effect node created in scene tree
- Verify scenefx support compiled in

**Crash on container destroy:**
- Check Wayland listener cleanup (wl_list_remove)
- Verify scene nodes destroyed properly
- Check for use-after-free (container accessed after free)

**IPC command not working:**
- Verify command registered in `commands.c`
- Check argument parsing logic
- Test with `swaymsg -t command <cmd>`
- Check return value and error messages

**Layout calculation wrong:**
- Debug `arrange_*()` functions
- Check container pending vs current state
- Verify transaction system applying changes
- Check for premature transaction commits

## Code Reading Guide

### Recommended Reading Order for New Contributors

1. **Start with main entry:**
   - `sway/main.c` - Understand startup sequence

2. **Server initialization:**
   - `sway/server.c` - Learn subsystem initialization
   - `include/sway/server.h` - Understand server structure

3. **Tree management:**
   - `include/sway/tree/container.h` - Container structure
   - `sway/tree/container.c` - Container lifecycle
   - `sway/tree/workspace.c` - Workspace management
   - `sway/tree/arrange.c` - Layout calculation

4. **Command system:**
   - `sway/commands.c` - Command dispatcher
   - `sway/commands/blur.c` - Example effect command
   - `sway/commands/focus.c` - Example layout command

5. **Configuration:**
   - `include/sway/config.h` - Config structure
   - `sway/config.c` - Config parsing
   - `sway/criteria.c` - Criteria matching

6. **Rendering:**
   - `sway/desktop/output.c` - Output rendering
   - `sway/desktop/transaction.c` - Transaction system
   - `sway/desktop/layer_shell.c` - Layer surfaces

7. **Input:**
   - `sway/input/input-manager.c` - Device management
   - `sway/input/seat.c` - Focus tracking
   - `sway/input/keyboard.c` - Keyboard handling

8. **IPC:**
   - `sway/ipc-server.c` - IPC protocol
   - `sway/ipc-json.c` - JSON serialization

### Understanding by Following a User Action

**Example: User resizes a window**

1. **Input event:**
   - Mouse button press → `cursor.c:handle_button()`
   - Checks if on window edge → starts resize operation
   - Enters resize seatop → `seatop_resize_tiling.c:handle_motion()`

2. **Tree modification:**
   - Calculate new size based on mouse position
   - Update container pending state
   - Mark container dirty → `container_set_geometry_from_content()`

3. **Transaction creation:**
   - `arrange_container()` calculates new layout
   - `transaction_commit_dirty()` creates transaction
   - Transaction includes all affected containers

4. **Client communication:**
   - Send XDG configure event with new size
   - Wait for client acknowledgment

5. **Transaction application:**
   - Client commits new surface size
   - Transaction detects ready state
   - `transaction_apply()` swaps pending → current

6. **Rendering update:**
   - Container scene tree resized
   - Damage region marked
   - Next frame renders new size

This flow touches: input → tree → transaction → rendering subsystems.

---

*This architecture document is intended for AI assistants working on complex features. For high-level overview, see AGENTS.md. For user documentation, see README.md.*
