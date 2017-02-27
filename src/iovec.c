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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <termios.h>
#include <unistd.h>

#include "cmd.h"

#define MAX_BUFFERS 5
#define MAX_CHARS   10

const char iovec_help[] = "Usage:\n"
	"\t uart_test iovec [options] <ttyDevice>\n";

struct iovec_data {
	int receiver;
	int fd;
};

char *test_str[] = {
	"All ",
	"your ",
	"base ",
	"are belong ",
	"to us!\n"
};

static int iovec_init(struct cmd *cmd, int argc, char *argv[])
{
	int ret, c;
	struct iovec_data *pdata;

	pdata = (struct iovec_data *)calloc(1, sizeof(struct iovec_data));
	if (!pdata)
		return -ENOMEM;

	static struct option long_options[] = {
		{"receiver", no_argument, 0, 'r'},
		{"sender", no_argument, 0, 's'},
		{0, 0, 0, 0}
	};

	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "rs", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'r':
			pdata->receiver = 1;
			break;
		case 's':
			pdata->receiver = 0;
			break;
		default:
			fprintf(stderr, "iovec: Invalid option %s\n", optarg);
			ret = -EINVAL;
			goto e_exit;
		}
	}

	if (optind != argc - 1) {
		fprintf(stderr, "Please specify the tty device");
		ret = -EINVAL;
		goto e_exit;
	}

	pdata->fd = open(argv[optind], O_RDWR | O_NOCTTY);
	if (pdata->fd < 0) {
		ret = -ENOENT;
		goto e_exit;
	}

	tcflush(pdata->fd, TCIFLUSH);

	cmd->priv = (void *) pdata;

	return 0;

e_exit:
	free(pdata);
	return ret;
}

static int iovec_exec(struct cmd *cmd)
{
	struct iovec_data *pdata = (struct iovec_data *)cmd->priv;
	ssize_t count;
	struct iovec iov[MAX_BUFFERS];
	int i, ret = 0;

	if (!pdata)
		return -EINVAL;

	if (pdata->receiver) {
		ssize_t expected_count = 0;

		for (i = 0; i < MAX_BUFFERS; i++) {
			iov[i].iov_base = malloc(MAX_CHARS);
			iov[i].iov_len = strlen(test_str[i]);
			expected_count += iov[i].iov_len;
		}

		count = readv(pdata->fd, iov, MAX_BUFFERS);
		if (count < 0) {
			ret = -errno;
			goto e_exit;
		}

		if (count != expected_count) {
			ret = -EINVAL;
			goto e_exit;
		}

		for (i = 0; i < MAX_BUFFERS; i++) {
			if (strncmp(iov[i].iov_base, test_str[i],
						iov[i].iov_len)) {
				ret = -EINVAL;
				goto e_exit;
			}
		}
e_exit:
		for (i = 0; i < MAX_BUFFERS; i++)
			free(iov[i].iov_base);
	} else {
		for (i = 0; i < MAX_BUFFERS; i++) {
			iov[i].iov_base = test_str[i];
			iov[i].iov_len = strlen(test_str[i]);
		}

		count = writev(pdata->fd, iov, MAX_BUFFERS);
		if (count < 0)
			ret = -errno;
	}

	return ret;
}

static int iovec_cleanup(struct cmd *cmd)
{
	struct ping_data *pdata = (struct ping_data *)cmd->priv;

	if (!pdata)
		return -EINVAL;

	free(pdata);
	cmd->priv = NULL;
	return 0;
}

REGISTER_CMD(
	iovec,
	"iovec test over a serial line",
	iovec_help,
	iovec_init,
	iovec_exec,
	iovec_cleanup
);
