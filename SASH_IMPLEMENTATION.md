# Interactive Window Sashes Implementation

## Overview

This implementation adds interactive sashes (visible dividers) between tiled windows in SwayFX. Sashes provide visual indication of resizable boundaries and enable intuitive mouse-based resizing.

## Building

### Prerequisites
- Meson build system
- Ninja
- Required dependencies (see main README.md)

### Build Instructions

```bash
# From the SwayFX root directory

# Configure the build (if not already done)
meson setup build

# Compile
meson compile -C build

# Install (optional, for system-wide installation)
sudo meson install -C build
```

### Development Build
For faster iteration during development:

```bash
# Reconfigure if needed
meson setup --reconfigure build

# Compile only changed files
ninja -C build

# Run from build directory without installing
./build/sway -c ~/.config/sway/config
```

## Configuration

Add these commands to your sway config file (`~/.config/sway/config`):

```
# Enable sashes (enabled by default)
sash enabled

# Customize sash appearance
sash_color #808080          # Gray sash color
sash_thickness 3             # Sash width in pixels (1-20)
sash_opacity 0.3             # Opacity (0.0-1.0)

# Disable sashes
# sash disabled
```

## Testing

### Basic Functionality Test

1. **Start SwayFX:**
   ```bash
   # Run from build directory
   ./build/sway
   ```

2. **Create horizontal split:**
   ```bash
   # In sway, open a terminal
   # Then split horizontally and open another
   $mod+h
   $mod+Return
   ```
   - **Expected:** Vertical sash appears between the two windows

3. **Create vertical split:**
   ```bash
   # Split vertically
   $mod+v
   $mod+Return
   ```
   - **Expected:** Horizontal sash appears between the two windows

4. **Test configuration:**
   ```bash
   # Via swaymsg
   swaymsg sash disabled
   # Verify sashes disappear

   swaymsg sash enabled
   swaymsg sash_color "#FF0000"
   swaymsg sash_thickness 5
   # Verify sashes reappear with new settings
   ```

### Visual Verification Checklist

- [ ] Sashes appear in horizontal splits (vertical dividers)
- [ ] Sashes appear in vertical splits (horizontal dividers)
- [ ] Sashes centered in gap between containers
- [ ] Sashes hidden in stacked/tabbed layouts
- [ ] Sashes hidden when gap size < sash_thickness + 2
- [ ] Sashes hidden in fullscreen mode
- [ ] Configuration commands work without errors
- [ ] Sashes update after config reload
- [ ] No visual glitches during window operations

## Current Implementation Status

### ✅ Completed Features
- Core sash data structures and lifecycle
- Visual rendering in gaps between containers
- Scene graph integration
- Configuration system (enable/disable, color, thickness, opacity)
- Command registration and handlers
- Build system integration

### 🚧 Not Yet Implemented
- **Mouse hover detection** - Cursor does not change when hovering over sash
- **Drag-to-resize functionality** - Sashes are visual only, not interactive
- **Hover visual feedback** - Brightness/opacity changes on hover
- **seatop_resize_sash** - Mouse drag handler for resizing

### Next Steps (for full interactivity)

1. **Hover Detection** (`sway/input/seatop_default.c`):
   - Add sash detection in `handle_motion()`
   - Change cursor to "col-resize" or "row-resize"
   - Set sash hover state for visual feedback

2. **Mouse Click Handler** (`sway/input/seatop_default.c`):
   - Detect sash clicks in `handle_button()`
   - Initiate resize operation

3. **Resize Operation** (`sway/input/seatop_resize_sash.c`):
   - Create new file based on `seatop_resize_tiling.c`
   - Implement drag handling
   - Update container fractions during drag
   - Commit transaction on release

## Debugging

### Enable Debug Logging

```bash
# Run sway with debug logging
SWAYSOCK=/tmp/sway-debug.sock ./build/sway -d 2>&1 | grep -i sash
```

### Common Issues

**Sashes not appearing:**
- Check if gaps are large enough: `gaps_inner >= sash_thickness + 2`
- Verify layout is horizontal or vertical (not tabbed/stacked)
- Check `sash enabled` is set in config

**Compile errors:**
- Ensure all new files are in `sway/meson.build`
- Check for missing includes
- Verify function declarations in headers

**Runtime crashes:**
- Check sash_manager initialization in `container_create()`
- Verify cleanup in `container_destroy()`
- Look for NULL pointer dereferences in `update_sashes()`

## Architecture Notes

### Sash Ownership
- Parent containers own sashes for gaps between their children
- Sashes are created/destroyed during `arrange_children()`
- Scene graph automatically handles sash lifecycle

### Key Files
- `sway/tree/sash.c` - Core sash implementation
- `sway/desktop/transaction.c` - Integration with layout engine
- `sway/tree/container.c` - Lifecycle management
- `sway/commands/sash*.c` - Configuration commands

### Design Decisions
- Sashes are `wlr_scene_rect` nodes for GPU-accelerated rendering
- Positioned in gap center for even spacing
- Use scene descriptors for type identification
- Opacity separate from color for easier customization

## Contributing

When working on sash features:

1. Test with various layouts (horizontal, vertical, nested)
2. Verify proper cleanup (no memory leaks)
3. Check visual appearance with different gap sizes
4. Ensure compatibility with fullscreen/floating modes
5. Update this document with any changes

## References

- Implementation Plan: `/home/meros/.claude/projects/-home-meros-git-personal-swayfx/8c61a7f8-604a-4956-af3b-eb6fce995723.jsonl`
- SwayFX Architecture: `ARCHITECTURE.md`
- Pattern Examples: `PATTERNS.md`
