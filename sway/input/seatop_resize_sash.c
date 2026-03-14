#include <wlr/types/wlr_cursor.h>
#include <wlr/util/edges.h>
#include "sway/commands.h"
#include "sway/desktop/transaction.h"
#include "sway/input/cursor.h"
#include "sway/input/seat.h"
#include "sway/tree/arrange.h"
#include "sway/tree/container.h"
#include "sway/tree/sash.h"
#include "sway/tree/view.h"
#include "log.h"

struct seatop_resize_sash_event {
	struct sway_sash *sash;
	struct sway_container *before;
	struct sway_container *after;
	struct sway_container *parent;
	double ref_lx, ref_ly;                  // Starting cursor position
	double before_orig_fraction;
	double after_orig_fraction;
	enum sway_container_layout orientation;
};

static void handle_button(struct sway_seat *seat, uint32_t time_msec,
		struct wlr_input_device *device, uint32_t button,
		enum wl_pointer_button_state state) {
	struct seatop_resize_sash_event *e = seat->seatop_data;

	if (seat->cursor->pressed_button_count == 0) {
		// End resize
		container_set_resizing(e->before, false);
		container_set_resizing(e->after, false);

		if (e->parent) {
			arrange_container(e->parent);
		}

		transaction_commit_dirty();
		seatop_begin_default(seat);
	}
}

static void handle_pointer_motion(struct sway_seat *seat, uint32_t time_msec) {
	struct seatop_resize_sash_event *e = seat->seatop_data;
	struct sway_cursor *cursor = seat->cursor;

	// Calculate movement delta
	double moved_x = cursor->cursor->x - e->ref_lx;
	double moved_y = cursor->cursor->y - e->ref_ly;

	// Determine which dimension we're resizing
	double delta_pixels = 0;
	double total_size = 0;

	if (e->orientation == L_HORIZ) {
		// Horizontal layout - vertical sash - resize with X movement
		delta_pixels = moved_x;
		total_size = e->parent->child_total_width;
	} else {
		// Vertical layout - horizontal sash - resize with Y movement
		delta_pixels = moved_y;
		total_size = e->parent->child_total_height;
	}

	if (total_size <= 0) {
		return;
	}

	// Convert pixel delta to fraction delta
	double delta_fraction = delta_pixels / total_size;

	// Update container fractions
	double new_before_fraction = e->before_orig_fraction + delta_fraction;
	double new_after_fraction = e->after_orig_fraction - delta_fraction;

	// Clamp to minimum sizes (prevent containers from becoming too small)
	const double MIN_FRACTION = 0.1;  // At least 10% of available space
	if (new_before_fraction < MIN_FRACTION) {
		new_before_fraction = MIN_FRACTION;
		new_after_fraction = e->before_orig_fraction + e->after_orig_fraction - MIN_FRACTION;
	} else if (new_after_fraction < MIN_FRACTION) {
		new_after_fraction = MIN_FRACTION;
		new_before_fraction = e->before_orig_fraction + e->after_orig_fraction - MIN_FRACTION;
	}

	// Apply new fractions
	if (e->orientation == L_HORIZ) {
		e->before->width_fraction = new_before_fraction;
		e->after->width_fraction = new_after_fraction;
	} else {
		e->before->height_fraction = new_before_fraction;
		e->after->height_fraction = new_after_fraction;
	}

	// Trigger rearrange
	if (e->parent) {
		arrange_container(e->parent);
	}
	transaction_commit_dirty();
}

static void handle_unref(struct sway_seat *seat, struct sway_container *con) {
	struct seatop_resize_sash_event *e = seat->seatop_data;
	if (e->before == con || e->after == con || e->parent == con) {
		seatop_begin_default(seat);
	}
}

static const struct sway_seatop_impl seatop_impl = {
	.button = handle_button,
	.pointer_motion = handle_pointer_motion,
	.unref = handle_unref,
};

void seatop_begin_resize_sash(struct sway_seat *seat, struct sway_sash *sash) {
	if (!sash || !sash->before || !sash->after || !sash->parent) {
		sway_log(SWAY_ERROR, "Invalid sash for resize");
		return;
	}

	seatop_end(seat);

	struct seatop_resize_sash_event *e =
		calloc(1, sizeof(struct seatop_resize_sash_event));
	if (!e) {
		return;
	}

	e->sash = sash;
	e->before = sash->before;
	e->after = sash->after;
	e->parent = sash->parent;
	e->orientation = sash->orientation;

	// Record starting cursor position
	e->ref_lx = seat->cursor->cursor->x;
	e->ref_ly = seat->cursor->cursor->y;

	// Record original fractions
	if (e->orientation == L_HORIZ) {
		e->before_orig_fraction = e->before->width_fraction;
		e->after_orig_fraction = e->after->width_fraction;
	} else {
		e->before_orig_fraction = e->before->height_fraction;
		e->after_orig_fraction = e->after->height_fraction;
	}

	// Mark containers as resizing
	container_set_resizing(e->before, true);
	container_set_resizing(e->after, true);

	seat->seatop_impl = &seatop_impl;
	seat->seatop_data = e;

	sway_log(SWAY_DEBUG, "Started sash resize: orientation=%d, fractions=%.2f,%.2f",
		e->orientation, e->before_orig_fraction, e->after_orig_fraction);

	transaction_commit_dirty();
	wlr_seat_pointer_notify_clear_focus(seat->wlr_seat);
}
