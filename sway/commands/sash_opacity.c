#include <stdlib.h>
#include "sway/commands.h"
#include "sway/config.h"
#include "sway/tree/arrange.h"

struct cmd_results *cmd_sash_opacity(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "sash_opacity", EXPECTED_EQUAL_TO, 1))) {
		return error;
	}

	char *end;
	float opacity = strtof(argv[0], &end);
	if (*end || opacity < 0.0f || opacity > 1.0f) {
		return cmd_results_new(CMD_INVALID,
			"sash_opacity must be between 0.0 and 1.0");
	}

	config->sash_opacity = opacity;

	if (config->active) {
		arrange_root();
	}

	return cmd_results_new(CMD_SUCCESS, NULL);
}
