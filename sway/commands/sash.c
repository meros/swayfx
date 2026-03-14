#include "sway/commands.h"
#include "sway/config.h"
#include "sway/tree/arrange.h"
#include "util.h"

struct cmd_results *cmd_sash(int argc, char **argv) {
	struct cmd_results *error = NULL;
	if ((error = checkarg(argc, "sash", EXPECTED_EQUAL_TO, 1))) {
		return error;
	}

	if (parse_boolean(argv[0], config->sash_enabled)) {
		config->sash_enabled = parse_boolean(argv[0], config->sash_enabled);

		if (config->active) {
			arrange_root();
		}
	} else {
		return cmd_results_new(CMD_INVALID, "Expected 'sash enabled' or 'sash disabled'");
	}

	return cmd_results_new(CMD_SUCCESS, NULL);
}
