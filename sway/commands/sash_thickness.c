#include <stdlib.h>
#include "sway/commands.h"
#include "sway/config.h"
#include "sway/tree/arrange.h"

struct cmd_results *cmd_sash_thickness(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "sash_thickness", EXPECTED_EQUAL_TO, 1))) {
		return error;
	}

	char *end;
	int thickness = strtol(argv[0], &end, 10);
	if (*end || thickness < 1 || thickness > 20) {
		return cmd_results_new(CMD_INVALID,
			"sash_thickness must be between 1 and 20");
	}

	config->sash_thickness = thickness;

	if (config->active) {
		arrange_root();
	}

	return cmd_results_new(CMD_SUCCESS, NULL);
}
