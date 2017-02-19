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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "cmd.h"

const char sendbreak_help[] = "Usage:\n"
	"\tuart_test sendbreak [options] <ttyDevice>\n";

struct sendbreak_data {
	int fd;
	int duration;
};

static int sendbreak_init(struct cmd *cmd, int argc, char *argv[])
{
	int c;
	int ret;
	struct sendbreak_data *pdata;
	static struct option long_options[] = {
		{"duration", optional_argument, 0, 'd'},
		{0, 0, 0, 0}
	};

	cmd->priv = NULL;

	pdata = (struct sendbreak_data *)
		calloc(1, sizeof(struct sendbreak_data));
	if (!pdata)
		return -ENOMEM;

	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "d:", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'd':
			pdata->duration = atoi(optarg);
			break;
		}
	}

	if (optind != argc - 1) {
		fprintf(stderr, "Please specify the tty device");
		return -EINVAL;
	}

	pdata->fd = open(argv[optind], O_RDWR | O_NOCTTY);
	if (pdata->fd < 0)
		return -ENOENT;

	cmd->priv = (void *) pdata;

	return 0;
}

static int sendbreak_exec(struct cmd *cmd)
{
	struct sendbreak_data *pdata;

	if (!cmd->priv)
		return -EINVAL;

	pdata = (struct sendbreak_data *)cmd->priv;

	if (tcsendbreak(pdata->fd, pdata->duration))
		return -errno;

	return 0;
}

static int sendbreak_cleanup(struct cmd *cmd)
{
	struct sendbreak_data *pdata;

	if (!cmd->priv)
		return -EINVAL;

	pdata = (struct sendbreak_data *)cmd->priv;

	if (pdata->fd)
		close(pdata->fd);

	free(cmd->priv);

	return 0;
}

REGISTER_CMD(
	sendbreak,
	"send BREAK to the specified port",
	sendbreak_help,
	sendbreak_init,
	sendbreak_exec,
	sendbreak_cleanup
);
