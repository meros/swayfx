# SwayFX - AI Development Guide

SwayFX is a feature-rich Wayland compositor that extends [Sway](https://github.com/swaywm/sway) with GPU-accelerated visual effects. It's a fork of Sway (i3-compatible tiling window manager) that adds blur effects, rounded corners, shadows, and animations while maintaining strict i3 compatibility.

## Project Overview

**Purpose**: Provide modern visual effects for Wayland tiling window managers without sacrificing i3's workflow
**Language**: C11 (POSIX-compliant, no GNU extensions)
**Architecture**: Event-driven Wayland compositor built on wlroots with custom scenefx renderer
**Version**: 0.5.3 (based on Sway 1.11.0)

## Tech Stack

- **wlroots** (0.19.x) - Wayland compositor library providing backend abstraction
- **scenefx** (0.4) - Custom GLES2 renderer for GPU-accelerated effects (blur, shadows)
- **Wayland** - Display server protocol (client-server architecture)
- **Meson** - Build system (C11, -Werror enabled)
- **Cairo + Pango** - Text and 2D graphics rendering
- **libinput** - Input device handling
- **json-c** - IPC protocol serialization
- **PCre2** - Criteria pattern matching

## Architecture Overview

### Core Architecture Pattern

SwayFX follows a **scene graph model** where all visual elements are organized in a tree hierarchy. The compositor manages:

1. **Server Layer** (`sway/server.c`) - Wayland display server and event loop
2. **Tree Layer** (`sway/tree/`) - Container hierarchy (workspaces → containers → views)
3. **Desktop Layer** (`sway/desktop/`) - Rendering coordination and output management
4. **Input Layer** (`sway/input/`) - Seat-based input device handling
5. **Command Layer** (`sway/commands/`) - Configuration command system (140+ commands)
6. **Effects Layer** (scenefx integration) - GPU-accelerated blur, shadows, rounded corners

### Scene Graph Hierarchy

```
Output (Monitor)
├── Background layer
├── Bottom layer (panels)
├── Tiling layer
│   ├── Workspace
│   │   └── Container tree
│   │       ├── Container (split)
│   │       │   ├── View (window) + effects
│   │       │   └── View (window) + effects
│   │       └── Container (tabbed/stacked)
│   └── Floating containers
├── Fullscreen layer
├── Top layer (panels)
├── Overlay layer
└── Session lock layer
```

### Data Flow

```
User Input/Config → Commands → Tree Manipulation → Transaction → Rendering
                                                         ↓
                                              Desktop Output Manager
                                                         ↓
                                              SceneFX Effects Pipeline
                                                         ↓
                                              Wayland Surface Presentation
```

## Codebase Organization

### Directory Structure

```
swayfx/
├── sway/                        # Main compositor daemon (~265 C files)
│   ├── commands/                # Command implementations (140+ files)
│   │   ├── blur.c, blur_*.c    # Blur effect commands
│   │   ├── corner_radius.c     # Rounded corner commands
│   │   ├── dim_inactive*.c     # Window dimming commands
│   │   ├── input/              # Input-specific commands (20+ files)
│   │   ├── bar/                # Swaybar commands
│   │   └── [layout/focus/...]  # Window management commands
│   ├── tree/                    # Window hierarchy management
│   │   ├── container.c         # Core container logic (2034 lines)
│   │   ├── workspace.c         # Workspace management
│   │   ├── view.c              # Individual window views
│   │   ├── arrange.c           # Layout calculation
│   │   └── root.c, node.c      # Tree structure
│   ├── desktop/                 # Rendering and output management
│   │   ├── output.c            # Output rendering coordination (799 lines)
│   │   ├── transaction.c       # Atomic state updates (1156 lines)
│   │   ├── layer_shell.c       # System UI layers (panels, notifications)
│   │   ├── xdg_shell.c         # Wayland window protocol
│   │   └── xwayland.c          # X11 compatibility
│   ├── input/                   # Input handling (14 files)
│   │   ├── seat.c              # Input focus coordination
│   │   ├── cursor.c            # Mouse cursor handling
│   │   ├── keyboard.c          # Keyboard input
│   │   └── seatop_*.c          # Input operations (move, resize, etc.)
│   ├── config.c                 # Config file parsing (1049 lines)
│   ├── server.c                 # Wayland server initialization
│   ├── main.c                   # Entry point
│   ├── ipc-server.c             # IPC protocol implementation (977 lines)
│   └── animation_manager.c      # Animation support
├── include/                     # Header files mirroring source structure
│   ├── sway/server.h           # Core server structure definitions
│   ├── sway/config.h           # Configuration structures
│   ├── sway/tree/container.h   # Container/window structures
│   ├── sway/output.h           # Output with scenefx blur layer
│   ├── sway/layers.h           # Layer surfaces with effect nodes
│   └── [...]
├── swaybar/                     # Status bar application
├── swaymsg/                     # IPC command-line client
├── swaynag/                     # Notification daemon
├── common/                      # Shared utilities (list, string ops, loop)
├── client/                      # Shared Wayland client code
├── protocols/                   # Wayland protocol definitions (.xml)
└── completions/                 # Shell completions

Key metrics: 265 C source files, 63 header files, ~100+ command implementations
```

### Critical Files Priority

**Must-read files (in order of importance):**

1. `sway/server.c` (800 lines) - Server initialization, main event loop, subsystem setup
2. `include/sway/server.h` - Core server structure with all subsystem pointers
3. `sway/tree/container.c` (2034 lines) - Container lifecycle, core window abstraction
4. `include/sway/tree/container.h` - Container structure definition
5. `sway/config.c` (1049 lines) - Configuration parsing and management
6. `sway/commands.c` (450 lines) - Command dispatcher and handler registry
7. `sway/desktop/output.c` (799 lines) - Output rendering coordination
8. `sway/desktop/transaction.c` (1156 lines) - Transaction system for atomic updates
9. `sway/ipc-server.c` (977 lines) - IPC protocol for external control
10. `include/sway/output.h` - Output structure with scenefx integration

**For SwayFX-specific features:**
- `sway/commands/blur*.c` - Blur effect parameters (passes, radius, noise, color adjustments)
- `sway/commands/corner_radius.c` - Rounded corner implementation
- `sway/commands/dim_inactive*.c` - Unfocused window dimming
- `include/sway/layers.h` - Layer surface effect structures
- `sway/desktop/layer_shell.c` - Layer surface rendering with effects

## Key Conventions

### Code Style (from CONTRIBUTING.md)

**Naming Conventions:**
- Structs: `struct sway_<component>` (e.g., `struct sway_container`, `struct sway_output`)
- Enums: `enum sway_<name>` or `enum <name>` (e.g., `enum cmd_status`)
- Functions: `snake_case` with module prefix (e.g., `container_create()`, `cmd_blur()`)
- Variables: `snake_case` (e.g., `binding_order`, `blur_enabled`)
- Constants/Flags: `UPPERCASE_WITH_UNDERSCORES` (e.g., `CMD_SUCCESS`, `BINDING_RELEASE`)

**File Organization:**
```c
// Order: system headers, wayland headers, local headers
#include <string.h>
#include <stdlib.h>
#include <wlr/types/wlr_cursor.h>
#include "sway/commands.h"
#include "sway/config.h"
```

**Function Pattern:**
```c
// Public API: module_action()
struct sway_container *container_create(struct sway_view *view) {
    struct sway_container *c = calloc(1, sizeof(struct sway_container));
    if (!c) {
        sway_log(SWAY_ERROR, "Unable to allocate sway_container");
        return NULL;
    }
    // ... initialization
    return c;
}

// Cleanup: free_module() or module_destroy()
void container_destroy(struct sway_container *con) {
    if (!con) {
        return;
    }
    // ... cleanup
    free(con);
}
```

### Command Handler Pattern

Every configuration command follows a strict signature:

```c
struct cmd_results *cmd_<name>(int argc, char **argv) {
    struct cmd_results *error = NULL;

    // Argument validation
    if ((error = checkarg(argc, "commandname", EXPECTED_AT_LEAST, 1))) {
        return error;
    }

    // Implementation
    // ...

    // Return status
    return cmd_results_new(CMD_SUCCESS, NULL);
}
```

**Status codes:**
- `CMD_SUCCESS` - Command executed successfully
- `CMD_FAILURE` - Command resulted in error
- `CMD_INVALID` - Unknown command or parser error
- `CMD_DEFER` - Execution deferred

**Registration (in `sway/commands.c`):**
```c
static const struct cmd_handler handlers[] = {
    { "blur", cmd_blur },
    { "blur_passes", cmd_blur_passes },
    // ... alphabetically sorted
};
```

### Resource Management

**Initialization patterns:**
- `create()` - Allocates and initializes (returns pointer)
- `init()` - Initializes pre-allocated structure (void)

**Cleanup patterns:**
- `destroy()` - Frees entire structure
- `finish()` - Cleans up internal fields (doesn't free structure itself)

**Null-safe cleanup:**
```c
void free_sway_binding(struct sway_binding *binding) {
    if (!binding) {
        return;  // Always check null first
    }
    free(binding->input);
    list_free(binding->keys);
    free(binding->command);
    free(binding);
}
```

### List Operations

Custom `list_t` abstraction (see `common/list.c`):
```c
list_t *list = create_list();
list_add(list, item);
list_insert(list, index, item);
for (int i = 0; i < list->length; i++) {
    void *item = list->items[i];
}
list_free(list);  // Does NOT free items, only container
```

### Wayland Listener Pattern

```c
// Listener registration
static void handle_output_enter(struct wl_listener *listener, void *data) {
    struct sway_container *con = wl_container_of(listener, con, output_enter);
    // ... handle event
}

// Setup
con->output_enter.notify = handle_output_enter;
wl_signal_add(&output->events.enter, &con->output_enter);

// Cleanup
wl_list_remove(&con->output_enter.link);
```

## SwayFX-Specific Features

### Visual Effects System

SwayFX extends Sway by replacing `wlr_renderer` with `fx_renderer` (from scenefx library). Effects are applied as scene nodes in the rendering tree.

**Effect Types:**
1. **Blur** - Gaussian blur on windows/layers
2. **Shadows** - Drop shadows with customizable color/offset/blur
3. **Rounded Corners** - Anti-aliased rounded corners on windows/titlebars
4. **Dim Inactive** - Darken unfocused windows
5. **Animations** - Smooth transitions for resize/movement

### Configuration Commands

**Blur configuration:**
```bash
blur enable|disable                    # Toggle blur
blur_passes <0-10>                     # Quality (more passes = smoother)
blur_radius <0-10>                     # Blur intensity
blur_noise <0.0-1.0>                   # Noise texture (hide artifacts)
blur_brightness <0.0-2.0>              # Adjust brightness
blur_contrast <0.0-2.0>                # Adjust contrast
blur_saturation <0.0-2.0>              # Adjust saturation
blur_xray enable|disable               # Blur background, not underlying windows
```

**Shadow configuration:**
```bash
shadows enable|disable                 # Toggle shadows
shadow_blur_radius <0-99>              # Shadow softness
shadow_color <rrggbb[aa]>              # Active window shadow color
shadow_inactive_color <rrggbb[aa]>     # Inactive window shadow color
shadow_offset <x> <y>                  # Shadow position offset
```

**Corner radius:**
```bash
corner_radius <0-N>                    # Pixel radius for corners
```

**Dim inactive:**
```bash
dim_inactive <0.0-1.0>                 # Opacity reduction for unfocused
dim_inactive_colors.unfocused <rrggbb[aa]>  # Custom dim color
```

**Layer-specific effects:**
```bash
layer_effects "namespace" {
    blur enable
    blur_ignore_transparent enable
    shadows enable
    corner_radius 10
}
```

### Effect Implementation Pattern

Effects are stored as properties on containers and layer surfaces:

```c
// In struct sway_container (include/sway/tree/container.h)
struct sway_container {
    struct wlr_scene_tree *scene_tree;
    bool blur_enabled;
    bool shadow_enabled;
    int corner_radius;
    // ...
};

// In struct sway_layer_surface (include/sway/layers.h)
struct sway_layer_surface {
    struct wlr_scene_blur *blur_node;       // SceneFX blur node
    struct wlr_scene_shadow *shadow_node;   // SceneFX shadow node
    bool blur_enabled, shadow_enabled;
    int corner_radius;
    // ...
};
```

**Effect application flow:**
1. User sets config command (e.g., `blur enable`)
2. Command handler updates global/container config
3. Container/layer is marked dirty
4. Transaction system applies changes atomically
5. `output.c` renders scene tree with scenefx effects

**Key functions for effects:**
- `container_update_blur()` - Apply blur to container
- `layer_shell_handle_commit()` - Apply effects to layer surfaces
- `output_configure_scene()` - Setup effect nodes in scene tree

## Development Workflow

### Building the Project

```bash
# Clone repository
git clone https://github.com/WillPower3309/swayfx.git
cd swayfx

# Build with Meson
meson build/
ninja -C build/

# Install (optional)
sudo ninja -C build/ install

# Run from build directory (for testing)
./build/sway/sway

# Build with specific options
meson build/ -Dswaybar=true -Dtray=enabled -Dgdk-pixbuf=enabled
```

### Build Options

```
-Ddefault-wallpaper=true|false    # Install default wallpaper
-Dswaybar=true|false               # Build swaybar component
-Dswaynag=true|false               # Build swaynag component
-Dtray=enabled|disabled|auto       # System tray support
-Dgdk-pixbuf=enabled|disabled|auto # Extended image format support
-Dman-pages=enabled|disabled|auto  # Generate man pages (needs scdoc)
-Dsd-bus-provider=auto|libsystemd|libelogind|basu  # D-Bus provider
```

### Testing Changes

SwayFX has no automated unit tests. Testing is manual:

1. **Configuration testing**: Modify `~/.config/sway/config` and reload (`swaymsg reload`)
2. **IPC testing**: Use `swaymsg` to send commands (e.g., `swaymsg blur enable`)
3. **Visual testing**: Launch sway with modified build and observe effects
4. **Logging**: Use `sway_log()` macros for debug output

**Log levels:**
```c
sway_log(SWAY_ERROR, "Error message");
sway_log(SWAY_INFO, "Info message");
sway_log(SWAY_DEBUG, "Debug message");
```

### Debugging Tips

**Run with logging:**
```bash
sway -d 2>&1 | tee sway.log  # Debug output to file
```

**IPC inspection:**
```bash
swaymsg -t get_tree          # Dump container tree
swaymsg -t get_outputs       # List outputs
swaymsg -t get_config        # Show current config
```

**GDB debugging:**
```bash
meson build/ --buildtype=debug
gdb ./build/sway/sway
```

## Common Task Patterns

### Adding a New Configuration Command

1. **Create command file**: `sway/commands/my_command.c`
   ```c
   #include "sway/commands.h"
   #include "sway/config.h"

   struct cmd_results *cmd_my_command(int argc, char **argv) {
       struct cmd_results *error = checkarg(argc, "my_command", EXPECTED_EQUAL, 1);
       if (error) {
           return error;
       }

       config->my_setting = parse_value(argv[0]);
       return cmd_results_new(CMD_SUCCESS, NULL);
   }
   ```

2. **Declare in header**: Add to `include/sway/commands.h`
   ```c
   struct cmd_results *cmd_my_command(int argc, char **argv);
   ```

3. **Register command**: Add to `handlers[]` array in `sway/commands.c` (alphabetically)
   ```c
   { "my_command", cmd_my_command },
   ```

4. **Add to build**: Update `sway/commands/meson.build`
   ```python
   sway_cmd_files = files(
       # ... existing files
       'my_command.c',
   )
   ```

5. **Update config struct**: Add field to `struct sway_config` in `include/sway/config.h`

6. **Document**: Add to man page `sway/sway.5.scd`

### Adding Effect to Container

To add a new visual effect:

1. **Add property to container**:
   ```c
   // In include/sway/tree/container.h
   struct sway_container {
       // ... existing fields
       bool my_effect_enabled;
   };
   ```

2. **Create configuration commands** (see "Adding a New Configuration Command")

3. **Apply effect in rendering**:
   ```c
   // In sway/desktop/output.c or relevant rendering function
   if (container->my_effect_enabled) {
       // Create/update scenefx effect node
       // Add to container's scene tree
   }
   ```

4. **Handle in transactions**: Ensure `desktop/transaction.c` accounts for effect state

### Modifying Existing Effect

Example: Changing blur behavior

1. **Locate command handler**: `sway/commands/blur.c` or `blur_*.c`
2. **Modify implementation**: Update parameter validation/application logic
3. **Update rendering**: Check `sway/desktop/output.c` for blur node application
4. **Test with config**: Use `blur_passes`, `blur_radius` commands to verify

## Domain-Specific Knowledge

### Wayland Concepts

**Display Server Architecture:**
- **Compositor**: Combines window surfaces and presents to display (SwayFX is a compositor)
- **Client**: Application that creates Wayland surfaces (windows)
- **Protocol**: XML-defined interfaces for communication (see `protocols/`)

**Surface Types:**
- **XDG Shell**: Standard desktop windows (`desktop/xdg_shell.c`)
- **Layer Shell**: System UI (panels, notifications) with z-ordering (`desktop/layer_shell.c`)
- **XWayland**: X11 compatibility surfaces (`desktop/xwayland.c`)

**Scene Graph (wlroots):**
- Tree of renderable nodes (buffers, rects, trees)
- Automatic damage tracking
- Coordinate space transformations
- SwayFX extends with scenefx effect nodes (blur, shadow)

### wlroots Integration

SwayFX depends on wlroots (0.19.x) for:
- **Backend Management**: DRM, Wayland, X11, headless backends
- **Input Handling**: libinput device abstraction
- **Output Management**: Monitor configuration, modes, transforms
- **Rendering**: Basic OpenGL ES 2 renderer (replaced by scenefx)
- **Protocols**: Implementation of core Wayland protocols

**Key wlroots types:**
- `struct wlr_backend` - Display backend (DRM, Wayland, etc.)
- `struct wlr_renderer` - Rendering backend (replaced by fx_renderer)
- `struct wlr_output` - Physical monitor
- `struct wlr_surface` - Client surface buffer
- `struct wlr_scene_*` - Scene graph nodes

### scenefx Integration

**Purpose**: Replace wlroots' simple renderer with GPU-accelerated effects

**Effect Node Types:**
- `struct wlr_scene_blur` - Gaussian blur effect
- `struct wlr_scene_shadow` - Drop shadow effect
- `struct wlr_scene_optimized_blur` - Optimized blur for static backgrounds

**Key differences from wlroots:**
- Effects applied as scene nodes (composable)
- GLES2 shaders for all effects
- Damage tracking aware (only re-render affected regions)
- Multi-pass rendering for quality

### Transaction System

SwayFX uses **atomic transactions** to prevent visual glitches during layout changes.

**How it works:**
1. User action triggers tree modification (e.g., resize window)
2. Changes are staged in a transaction
3. All affected containers are marked dirty
4. Transaction is committed atomically
5. Rendering applies all changes simultaneously

**Key files:**
- `sway/desktop/transaction.c` - Transaction implementation
- `sway/tree/arrange.c` - Layout recalculation

**Important**: Effects must be reconfigured when transactions apply (container size/position changes).

## Important Notes and Gotchas

### Memory Management

- **Always check NULL**: All cleanup functions must handle `NULL` pointers
- **List cleanup**: `list_free()` does NOT free items, only the list container
- **Wayland listeners**: Must remove listener with `wl_list_remove()` before freeing structure
- **Scene nodes**: Destroying parent scene tree node destroys all children

### Configuration System

- **Reload behavior**: `swaymsg reload` re-reads config but doesn't restart server
- **Criteria matching**: Uses PCre2 regex for window matching (app_id, class, title, etc.)
- **Command modes**: Some commands only valid in config file, others runtime-only
- **Bar config**: Separate command namespace (`bar_cmd_*`) for swaybar configuration

### Rendering Pipeline

- **Damage tracking**: Only re-render regions that changed (performance optimization)
- **Z-ordering**: Layer shell defines strict ordering (background < bottom < tiling < top < overlay)
- **Multi-output**: Same container can appear on multiple outputs (shared scene nodes)
- **Effect performance**: More blur passes = better quality but slower rendering

### Upstream Compatibility

SwayFX is a fork, not a drop-in replacement:
- **Diverging codebase**: Cannot simply merge upstream Sway changes
- **Config compatibility**: Most Sway configs work, but SwayFX adds extra commands
- **Protocol support**: Same Wayland protocol support as upstream Sway
- **Migration**: Can switch between Sway and SwayFX (config may need effect commands removed)

### Build System Quirks

- **wlroots version**: Requires exact 0.19.x (not 0.18 or 0.20)
- **scenefx dependency**: Must be compiled with scenefx support (not in standard repos)
- **Subproject fallback**: Meson can fetch wlroots/scenefx as subprojects if not found
- **Protocol generation**: Wayland protocols compiled at build time (requires wayland-scanner)

## Security Considerations

SwayFX is a privileged system component with security implications:

- **Capability requirements**: Needs CAP_SYS_ADMIN or DRM master for direct rendering
- **IPC socket**: Accepts commands from any process in user session
- **Input capture**: Can read all keyboard/mouse input (handle sensitive data carefully)
- **Screen capture**: Full access to all on-screen content
- **Wayland security model**: Stricter than X11 (no global key/screen capture by clients)

**When adding features:**
- Validate all IPC inputs (command parsing)
- Avoid shell injection in command execution
- Check bounds on array/buffer operations (blur passes, corner radius values)
- Be careful with file path handling (config loading, wallpaper paths)

## Resources and References

**Official Documentation:**
- [SwayFX GitHub](https://github.com/WillPower3309/swayfx)
- [Sway (upstream) Wiki](https://github.com/swaywm/sway/wiki)
- [wlroots Documentation](https://gitlab.freedesktop.org/wlroots/wlroots)
- [Wayland Documentation](https://wayland.freedesktop.org/docs/html/)

**Understanding the Codebase:**
- Start with `sway/main.c` (entry point)
- Read `sway/server.c` (server initialization)
- Understand `sway/tree/container.c` (core window abstraction)
- Study command handlers in `sway/commands/` (configuration system)

**Key Concepts to Learn:**
- Wayland protocol architecture
- wlroots compositor library
- Scene graph rendering model
- i3 tiling window manager concepts
- OpenGL ES 2 shader programming (for effects)

**Community:**
- GitHub Issues: Bug reports and feature requests
- Pull Requests: Follow CONTRIBUTING.md guidelines
- Upstream Sway: Many concepts applicable to SwayFX

---

*This documentation is optimized for AI coding assistants. For human-readable documentation, see README.md and man pages.*
