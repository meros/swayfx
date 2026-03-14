# SwayFX Quick Reference

Quick lookup guide for common tasks and file locations.

## File Locations

### Finding the Right File

**Want to modify...**
- **Blur effects** → `sway/commands/blur*.c`
- **Shadow effects** → `sway/commands/shadow*.c`
- **Corner radius** → `sway/commands/corner_radius.c`
- **Container management** → `sway/tree/container.c`
- **Window rendering** → `sway/desktop/output.c`
- **Layout calculation** → `sway/tree/arrange.c`
- **Input handling** → `sway/input/*.c`
- **IPC protocol** → `sway/ipc-server.c`
- **Config parsing** → `sway/config.c`
- **Command dispatcher** → `sway/commands.c`
- **Layer surfaces** → `sway/desktop/layer_shell.c`
- **XDG windows** → `sway/desktop/xdg_shell.c`

### Important Headers

**Data structures:**
- `include/sway/server.h` - Root server structure
- `include/sway/tree/container.h` - Container definition
- `include/sway/config.h` - Configuration structure
- `include/sway/output.h` - Output/monitor definition
- `include/sway/layers.h` - Layer surface definition
- `include/sway/commands.h` - Command handler declarations

**Utilities:**
- `include/list.h` - List operations
- `include/stringop.h` - String utilities
- `include/log.h` - Logging macros

## Common Tasks

### Add a New Command

1. Create `sway/commands/my_command.c`
2. Implement `struct cmd_results *cmd_my_command(int argc, char **argv)`
3. Add declaration to `include/sway/commands.h`
4. Register in `sway/commands.c` handlers array
5. Add to `sway/commands/meson.build`
6. Update `sway/sway.5.scd` man page

**Minimal template:**
```c
#include "sway/commands.h"

struct cmd_results *cmd_my_command(int argc, char **argv) {
    struct cmd_results *error = checkarg(argc, "my_command", EXPECTED_EQUAL, 1);
    if (error) return error;

    // Implementation here

    return cmd_results_new(CMD_SUCCESS, NULL);
}
```

### Add a New Effect

1. Add property to `struct sway_container` in `include/sway/tree/container.h`
2. Add property to `struct sway_container_state` (for transactions)
3. Create configuration commands (see "Add a New Command")
4. Apply effect in `sway/desktop/output.c` rendering
5. Handle in transaction system if state-dependent

### Modify Container Properties

**Pattern:**
```c
// Modify pending state
con->pending.property = new_value;

// Mark for transaction
container_set_dirty(con);

// Trigger layout recalculation
arrange_container(con);
```

### Debug a Crash

**Steps:**
1. Build with debug symbols: `meson build --buildtype=debug`
2. Run in GDB: `gdb ./build/sway/sway`
3. Set breakpoint: `break container_destroy`
4. Run: `run`
5. On crash: `backtrace`, `print *variable`

**Common crash causes:**
- Use-after-free (accessing freed memory)
- NULL pointer dereference
- Wayland listeners not removed
- Scene nodes accessed after parent destroyed

### Test Configuration Changes

**Quick iteration:**
```bash
# Edit config
vim ~/.config/sway/config

# Reload without restart
swaymsg reload

# Or test command directly
swaymsg blur enable
swaymsg blur_passes 3
```

**Check current state:**
```bash
swaymsg -t get_tree | jq '.nodes[0]'  # Output 0 tree
swaymsg -t get_outputs                # List outputs
```

## Command Reference

### Blur Commands

```bash
blur enable|disable                    # Toggle blur
blur_passes <0-10>                     # Render passes (quality)
blur_radius <0-10>                     # Blur intensity
blur_noise <0.0-1.0>                   # Noise overlay
blur_brightness <0.0-2.0>              # Brightness adjustment
blur_contrast <0.0-2.0>                # Contrast adjustment
blur_saturation <0.0-2.0>              # Saturation adjustment
blur_xray enable|disable               # Blur background only
```

### Shadow Commands

```bash
shadows enable|disable                 # Toggle shadows
shadow_blur_radius <0-99>              # Shadow softness
shadow_color <rrggbb[aa]>              # Active window shadow
shadow_inactive_color <rrggbb[aa]>     # Inactive window shadow
shadow_offset <x> <y>                  # Shadow position offset
```

### Other Effect Commands

```bash
corner_radius <0-N>                    # Rounded corners (pixels)
dim_inactive <0.0-1.0>                 # Dim unfocused windows
dim_inactive_colors.unfocused <rgba>   # Dim color tint
```

### Layer-Specific Effects

```bash
layer_effects "namespace" {
    blur enable
    blur_ignore_transparent enable
    shadows enable
    corner_radius 10
}
```

### IPC Commands

```bash
swaymsg -t get_workspaces             # List workspaces
swaymsg -t get_outputs                # List outputs
swaymsg -t get_tree                   # Container tree
swaymsg -t get_inputs                 # Input devices
swaymsg -t get_seats                  # Input seats
swaymsg -t subscribe '["window"]'     # Subscribe to events
```

## Build Commands

### Standard Build

```bash
meson setup build/
ninja -C build/
sudo ninja -C build/ install
```

### Build Options

```bash
meson setup build/ \
    -Dswaybar=true \
    -Dtray=enabled \
    -Dgdk-pixbuf=enabled \
    -Dman-pages=enabled \
    --buildtype=debug
```

### Reconfigure

```bash
meson configure build/ -Dswaybar=false
ninja -C build/
```

### Clean Build

```bash
rm -rf build/
meson setup build/
ninja -C build/
```

## Debugging Commands

### Run with Logging

```bash
sway -d                                # Debug level
sway -V                                # Verbose (all logs)
sway -d 2>&1 | tee sway.log           # Save to file
```

### GDB Debugging

```bash
gdb ./build/sway/sway
(gdb) break container_create
(gdb) run
(gdb) print *container
(gdb) backtrace
(gdb) continue
```

### Valgrind Memory Check

```bash
valgrind --leak-check=full ./build/sway/sway
```

## Common Code Patterns

### Check Null Pointer

```c
if (!pointer) {
    sway_log(SWAY_ERROR, "Null pointer");
    return;
}
```

### Iterate List

```c
for (int i = 0; i < list->length; i++) {
    struct my_type *item = list->items[i];
    // ...
}
```

### Parse Integer

```c
char *end;
int value = strtol(argv[0], &end, 10);
if (*end || value < 0 || value > 10) {
    return cmd_results_new(CMD_INVALID, "Invalid value");
}
```

### Parse Float

```c
char *end;
float value = strtof(argv[0], &end);
if (*end || value < 0.0f || value > 1.0f) {
    return cmd_results_new(CMD_INVALID, "Invalid value");
}
```

### Create Container

```c
struct sway_container *con = calloc(1, sizeof(*con));
if (!con) {
    sway_log(SWAY_ERROR, "Allocation failed");
    return NULL;
}
// Initialize fields...
return con;
```

### Destroy Container

```c
void destroy(struct sway_container *con) {
    if (!con) return;
    // Remove listeners
    wl_list_remove(&con->listener.link);
    // Destroy scene tree
    wlr_scene_node_destroy(&con->scene_tree->node);
    // Free allocations
    free(con->title);
    free(con);
}
```

### Register Wayland Listener

```c
layer->destroy.notify = handle_destroy;
wl_signal_add(&wlr_surface->events.destroy, &layer->destroy);
```

### Remove Wayland Listener

```c
wl_list_remove(&layer->destroy.link);
```

## Useful Macros

### Logging

```c
sway_log(SWAY_ERROR, "Error: %s", msg);
sway_log(SWAY_INFO, "Info: %s", msg);
sway_log(SWAY_DEBUG, "Debug: %s", msg);
```

### Assertions

```c
sway_assert(condition, "Error message");
```

### Container of Listener

```c
struct my_type *obj = wl_container_of(listener, obj, listener_field);
```

## Git Workflow

### Development Branch

```bash
git checkout -b feature/my-feature
# Make changes
git add .
git commit -m "feat: add my feature"
git push origin feature/my-feature
```

### Update from Upstream

```bash
git remote add upstream https://github.com/WillPower3309/swayfx.git
git fetch upstream
git merge upstream/master
```

### Commit Message Format

```
<type>: <subject>

[optional body]

[optional footer]
```

**Types:**
- `feat:` - New feature
- `fix:` - Bug fix
- `docs:` - Documentation change
- `refactor:` - Code refactoring
- `test:` - Test changes
- `chore:` - Build/tooling changes

## Useful Environment Variables

```bash
WLR_BACKENDS=wayland        # Run nested in Wayland
WLR_WL_OUTPUTS=2            # Number of outputs (nested)
WLR_RENDERER=gles2          # Force GLES2 renderer
SWAY_CURSOR_THEME=Adwaita   # Set cursor theme
SWAY_CURSOR_SIZE=24         # Set cursor size
```

## Performance Tips

### Reduce Blur Cost

- Lower `blur_passes` (2-3 usually sufficient)
- Reduce `blur_radius` (3-5 works well)
- Use `blur_ignore_transparent` on panels

### Reduce Shadow Cost

- Lower `shadow_blur_radius`
- Disable shadows on inactive windows

### General

- Disable effects on battery power (not implemented yet)
- Use optimized_blur for static backgrounds
- Reduce `corner_radius` value

## Common Issues

### Issue: Effects not appearing

**Check:**
1. Effect enabled in config? (`blur enable`)
2. Container property set? (`con->blur_enabled`)
3. Scene node created? (check rendering code)
4. scenefx compiled correctly?

### Issue: Crash on window close

**Check:**
1. Wayland listeners removed? (`wl_list_remove`)
2. Scene nodes destroyed properly?
3. No use-after-free?

### Issue: Layout calculation wrong

**Check:**
1. Pending state modified? (not current)
2. Container marked dirty?
3. `arrange_*()` called?
4. Transaction applied?

### Issue: Command not found

**Check:**
1. Command registered in `commands.c`?
2. Handler function declared in `commands.h`?
3. File added to `meson.build`?
4. Spelling correct?

## Documentation

**Man pages:**
```bash
man sway               # General usage
man sway.5             # Configuration file
man swaymsg            # IPC client
man swaybar            # Status bar
man swaynag            # Notification daemon
```

**Online resources:**
- SwayFX: https://github.com/WillPower3309/swayfx
- Sway: https://github.com/swaywm/sway
- wlroots: https://gitlab.freedesktop.org/wlroots/wlroots
- Wayland: https://wayland.freedesktop.org/

---

*For detailed explanations, see AGENTS.md and ARCHITECTURE.md*
