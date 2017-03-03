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
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/uio.h>
#include <asm/termios.h>
#include <unistd.h>

#include "cmd.h"

const char set_baud_help[] = "Usage:\n"
	"\t uart_test set_baud [options] <ttyDevice>\n";

struct set_baud_data {
	int receiver;
	int fd;
	int baudrate;
};

static const char *test_str = "All your base are belong to us!\n";

static int set_baud_init(struct cmd *cmd, int argc, char *argv[])
{
	int ret, c;
	struct set_baud_data *pdata;

	pdata = (struct set_baud_data *)calloc(1, sizeof(struct set_baud_data));
	if (!pdata)
		return -ENOMEM;

	static struct option long_options[] = {
		{"receiver", no_argument, 0, 'r'},
		{"sender", no_argument, 0, 's'},
		{"baudrate", no_argument, 0, 'b'},
		{0, 0, 0, 0}
	};

	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "rsb:", long_options,
			&option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'r':
			pdata->receiver = 1;
			break;
		case 's':
			pdata->receiver = 0;
			break;
		case 'b':
			pdata->baudrate = atoi(optarg);
			break;
		default:
			fprintf(stderr, "set_baud: Invalid option %s\n",
				optarg);
			ret = -EINVAL;
			goto e_exit;
		}
	}

	if (!pdata->baudrate)
		pdata->baudrate = 115200;

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

static int set_baud_exec(struct cmd *cmd)
{
	struct set_baud_data *pdata = (struct set_baud_data *)cmd->priv;
	char *pbuffer;
	ssize_t count;
	struct termios2 tio;
	int ret = 0;

	if (!pdata)
		return -EINVAL;

	pbuffer = calloc(1, 100);
	if (!pbuffer)
		return -EINVAL;

	ret = ioctl(pdata->fd, TCGETS2, &tio);
	if (ret != 0)
		goto e_exit;

	tio.c_cflag &= ~CBAUD;
	tio.c_cflag |= BOTHER;
	tio.c_ispeed = pdata->baudrate;
	tio.c_ospeed = pdata->baudrate;

	ret = ioctl(pdata->fd, TCSETS2, &tio);
	if (ret != 0)
		goto e_exit;

	if (pdata->receiver) {
		count = read(pdata->fd, pbuffer, strlen(test_str));
		if (count < 0) {
			ret = -errno;
			goto e_exit;
		}

		if (strcmp(pbuffer, test_str) != 0) {
			ret = -EINVAL;
			goto e_exit;
		}

	} else {
		count = write(pdata->fd, test_str, strlen(test_str));
		if (count < 0) {
			ret = -errno;
			goto e_exit;
		}
	}

e_exit:
	free(pbuffer);
	return ret;
}

static int set_baud_cleanup(struct cmd *cmd)
{
	struct ping_data *pdata = (struct ping_data *)cmd->priv;

	if (!pdata)
		return -EINVAL;

	free(pdata);
	cmd->priv = NULL;
	return 0;
}

REGISTER_CMD(
	set_baud,
	"Set baudrate",
	set_baud_help,
	set_baud_init,
	set_baud_exec,
	set_baud_cleanup
);
