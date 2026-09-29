#include "sway/commands.h"
#include "sway/config.h"
#include "util.h"

struct cmd_results *cmd_gap_highlight_color(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "gap_highlight_color", EXPECTED_AT_LEAST, 1))) {
		return error;
	}

	uint32_t color;
	if (!parse_color(argv[0], &color)) {
		return cmd_results_new(CMD_INVALID, "Invalid %s color %s",
				"gap_highlight_color", argv[0]);
	}
	color_to_rgba(config->gap_highlight_color, color);

	return cmd_results_new(CMD_SUCCESS, NULL);
}
