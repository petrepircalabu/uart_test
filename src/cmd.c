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

struct cmd *find_cmd(const char *name)
{
	int i;

	for (i = 0; i < cmd_count; i++) {
		if (strcmp(name, cmds[i]->name) == 0)
			return cmds[i];
	}

	return NULL;
}

int execute_cmd(struct cmd *p_cmd, int argc, char *argv[])
{
	int ret = 0;

	if (p_cmd->init) {
		ret = p_cmd->init(p_cmd, argc, argv);
		if (ret != 0) {
			fprintf(stderr, "Failed to init %s\n", p_cmd->name);
			return ret;
		}
	}

	if (p_cmd->exec) {
		ret = p_cmd->exec(p_cmd);
		if (ret != 0) {
			fprintf(stderr, "Failed to execute %s\n", p_cmd->name);
			return ret;
		}
	}

	if (p_cmd->cleanup) {
		ret = p_cmd->cleanup(p_cmd);
		if (ret != 0) {
			fprintf(stderr, "Failed to cleanup %s\n", p_cmd->name);
			return ret;
		}
	}

	return 0;
}

int run_cmd(int argc, char *argv[])
{
	struct cmd *p_cmd = find_cmd(argv[0]);

	if (!p_cmd) {
		fprintf(stderr, "Command \"%s\" was not found", argv[0]);
		return -EINVAL;
	}

	return execute_cmd(p_cmd, argc, argv);
}
