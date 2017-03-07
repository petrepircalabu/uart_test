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
const char rts_control_help[] = "Usage:\n"
	"\t uart_test rts_control [options] <ttyDevice>\n";

struct rts_control_data {
	int receiver;
	int fd;
	int timeout;
};

static const char *cmd1 = "CMD1";
static const char *cmd2 = "CMD2";
static const char *cmd3 = "CMD3";
static const char *resp_ok = "OKAY";
static const char *resp_timeout = "TOUT";

static const int default_timeout = 20;

static int rts_control_init(struct cmd *cmd, int argc, char *argv[])
{
	int ret, c;
	struct rts_control_data *pdata;

	pdata = (struct rts_control_data *)calloc(1,
		sizeof(struct rts_control_data));
	if (!pdata)
		return -ENOMEM;

	static struct option long_options[] = {
		{"receiver", no_argument, 0, 'r'},
		{"sender", no_argument, 0, 's'},
		{"timeout", required_argument, 0, 't'},
		{0, 0, 0, 0}
	};

	pdata->timeout = default_timeout;

	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "rst:", long_options,
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
		case 't':
			pdata->timeout = atoi(optarg);
			break;
		default:
			fprintf(stderr, "rts_control: Invalid option %s\n",
				optarg);
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

static int wait_command(int fd, char *buffer, int timeout)
{
	fd_set rfds;
	struct timeval tv;
	int retval;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	tv.tv_sec = timeout;
	tv.tv_usec = 0;

	retval = select(fd + 1, &rfds, NULL, NULL, &tv);
	switch (retval) {
	case -1:
		return -errno;
	case 0:
		return -EAGAIN;
	default:
		/* FIXME: Check FD_ISSET */
		return (read(fd, buffer, 4) == 4) ? 0 : -errno;
	}
	return 0;
}

static int set_rts(int fd, int level)
{
	int status;

	if (ioctl(fd, TIOCMGET, &status) == -1)
		return -errno;

	if (level)
		status |= TIOCM_RTS;
	else
		status &= ~TIOCM_RTS;

	if (ioctl(fd, TIOCMSET, &status) == -1)
		return -errno;

	return 0;
}

static int rts_control_exec(struct cmd *cmd)
{
	struct rts_control_data *pdata = (struct rts_control_data *)cmd->priv;
	char buffer[10];
	int retval;

	if (!pdata)
		return -EINVAL;

	if (pdata->receiver) {
		/* Wait for initial command */
		retval = wait_command(pdata->fd, buffer, pdata->timeout);
		if (retval != 0) {
			fprintf(stderr, "Failed to receive command1\n");
			return retval;
		}
		if (strncmp(buffer, cmd1, 4))
			return -EINVAL;

		/* Deassert RTS*/
		retval = set_rts(pdata->fd, 0);
		if (retval)
			return retval;

		/* Send Response */
		write(pdata->fd, resp_ok, 4);

		/* Poll on read - wait for timeout*/
		retval = wait_command(pdata->fd, buffer, pdata->timeout);
		if (retval != -EAGAIN) {
			fprintf(stderr, "Received data while RTS was 0\n");
			return -EINVAL;
		}

		/* restore RTS operation*/
		retval = set_rts(pdata->fd, 1);
		if (retval)
			return retval;

		/* Wait for command */
		retval = wait_command(pdata->fd, buffer, pdata->timeout);
		if (retval != 0)
			return retval;
		if (strncmp(buffer, cmd2, 4))
			return -EINVAL;
		/* Send Response */
		write(pdata->fd, resp_ok, 4);
	} else {
		/* Send initial command */
		write(pdata->fd, cmd1, 4);
		/* Wait for initial response */
		retval = wait_command(pdata->fd, buffer, pdata->timeout);
		if (retval != 0)
			return retval;
		if (strncmp(buffer, resp_ok, 4))
			return -EINVAL;

		/* Send second command */
		write(pdata->fd, cmd2, 4);

		/* Wait for response (should be timeout) */
		retval = wait_command(pdata->fd, buffer, pdata->timeout * 2);
		if (retval != 0)
			return retval;
		if (strncmp(buffer, resp_ok, 4))
			return -EINVAL;
	}
	return 0;
}

static int rts_control_cleanup(struct cmd *cmd)
{
	struct rts_control *pdata = (struct rts_control *)cmd->priv;

	if (!pdata)
		return -EINVAL;

	free(pdata);
	cmd->priv = NULL;

	return 0;
}

REGISTER_CMD(
	rts_control,
	"Test RTS control",
	rts_control_help,
	rts_control_init,
	rts_control_exec,
	rts_control_cleanup
);
