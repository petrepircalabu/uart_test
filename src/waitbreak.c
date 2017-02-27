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
#include <signal.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include "cmd.h"

const char waitbreak_help[] = "Usage:\n"
	"\tuart_test waitbreak [options] <ttyDevice>\n";

struct waitbreak_data {
	const char *ttyname;
	pid_t child_pid;	/* NOT VALID in the child process */
};

static int waitbreak_func(struct waitbreak_data *pdata)
{
	int fd;
	int ret;
	pid_t sid;
	struct termios oldtio, newtio;
	sigset_t mask;
	struct timespec timeout = {
		.tv_sec  = 10,
		.tv_nsec = 0
	};

	fd = open(pdata->ttyname, O_RDWR);
	if (fd < 0) {
		ret = -ENOENT;
		goto e_exit;
	}

	if (tcgetattr(fd, &oldtio)) {
		ret = -errno;
		goto e_close_fd;
	}

	newtio = oldtio;
	newtio.c_iflag &= ~IGNBRK;
	newtio.c_iflag |= BRKINT;

	if (tcsetattr(fd, TCSAFLUSH, &newtio)) {
		ret = -errno;
		goto e_restore_tio;
	}

	sid = setsid();
	if (sid < 0) {
		ret = -errno;
		goto e_restore_tio;
	}

	if (ioctl(fd, TIOCSCTTY, 1) == -1) {
		ret = -errno;
		goto e_restore_tio;
	}

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);

	do {
		if (sigtimedwait(&mask, NULL, &timeout) < 0) {
			if (errno == EINTR) {
				continue;
			} else if (errno == EAGAIN) {
				printf("Timeout\n");
				ret = -EAGAIN;
				goto e_restore_tio;
			}

			ret = -errno;
			goto e_restore_tio;
		}
		break;
	} while (1);


e_restore_tio:
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &oldtio);
e_close_fd:
	close(fd);
e_exit:
	return ret;
}

static int waitbreak_init(struct cmd *cmd, int argc, char *argv[])
{
	int c;
	int ret;
	struct waitbreak_data *pdata;

	cmd->priv = 0;

	pdata = (struct waitbreak_data *)
		calloc(1, sizeof(struct waitbreak_data));
	if (!pdata)
		return -ENOMEM;

	pdata->ttyname = argv[1];

	cmd->priv = (void *) pdata;
	return 0;
}

static int waitbreak_exec(struct cmd *cmd)
{
	struct waitbreak_data *pdata;

	if (!cmd->priv)
		return -EINVAL;

	pdata = (struct waitbreak_data *)cmd->priv;

	pdata->child_pid = fork();

	switch (pdata->child_pid) {
	case -1:
		return -EINVAL;

	case 0:
		exit(waitbreak_func(pdata));
		break;
	}

	return 0;
}

static int waitbreak_cleanup(struct cmd *cmd)
{
	pid_t ret;
	int status = 0;
	struct waitbreak_data *pdata;

	if (!cmd->priv)
		return -EINVAL;

	pdata = (struct waitbreak_data *)cmd->priv;

	if (pdata->child_pid) {
		ret = waitpid(pdata->child_pid, &status, 0);
		if (ret == -1)
			return -ECHILD;
	}
	return status;
}

REGISTER_CMD(
	waitbreak,
	"waits for BREAK condition on the specified port",
	waitbreak_help,
	waitbreak_init,
	waitbreak_exec,
	waitbreak_cleanup
);
