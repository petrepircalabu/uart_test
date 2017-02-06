/**
 * MIT License
 *
 * Copyright (c) 2017 Petre Pircalabu
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "cmd.h"
#include "help.h"

void help(void)
{
	int i;

	printf("Usage:\n"
		"\tuart_test <command> <parameters>\n\n"
		"Supported commands:\n");
	for (i = 0; i < cmd_count; i++) {
		printf("\t%s\t%s\n", cmds[i]->name,
			cmds[i]->description);
	}
	printf("\n");
}

static int help_init(struct cmd *cmd, int argc, char *argv[])
{
	if (argc > 1)
		cmd->priv = (void *) argv[1];

	return 0;
}

static int help_exec(struct cmd *cmd)
{
	if (cmd->priv) {
		struct cmd *pcmd;

		pcmd = find_cmd((const char *)cmd->priv);
		if (!pcmd) {
			help();
			return -EINVAL;
		}
		printf("%s\n", pcmd->help);
	} else {
		help();
	}
	return 0;
}

REGISTER_CMD(
	help,
	"displays help (generic or for a specific command)",
	"displays help (generic or for a specific command)",
	help_init,
	help_exec,
	NULL
);
