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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#include <arpa/inet.h>

#include "cmd.h"

const char ping_help[] = "Usage:\n"
	"\tuart_test ping [options] <ttyDevice>\n";

enum {
	INVALID_REQ,
	SEND_REQ,
	SEND_RECV_REQ,
	OKAY,
	NOK
};

struct ping_data {
	int fd;
	int server;
	int count;
	int cmd;
	pthread_t sender_id;
	pthread_t receiver_id;
};

struct ping_response {
	int retval;
	struct timespec duration;
};

static int send_cmd(int fd, int cmd, int arg)
{
	int netcmd, netarg;

	netcmd = htonl(cmd);
	netarg = htonl(arg);

	/* TODO: Add err checking */
	write(fd, (const void *)&netcmd, sizeof(int));
	write(fd, (const void *)&netarg, sizeof(int));

	return 0;
}

static int read_cmd(int fd, int *cmd, int *arg)
{
	int netcmd, netarg;

	read(fd, (void *)&netcmd, sizeof(int));
	read(fd, (void *)&netarg, sizeof(int));

	*cmd = ntohl(netcmd);
	*arg = ntohl(netarg);

	return 0;
}

/* TODO: Implement proper return value */
static void *sender_func(void *arg)
{
	struct ping_data *pdata = (struct ping_data *)arg;
	struct ping_response *presp = NULL;
	char *buf;
	struct timespec start, stop;
	ssize_t retval;

	if (!pdata)
		return NULL;

	presp = (struct ping_response *)calloc(1, sizeof(struct ping_response));
	if (!presp)
		return NULL;

	buf = malloc(pdata->count);
	if (!buf) {
		presp->retval = -ENOMEM;
		return presp;
	}

	memset(buf, 'a', pdata->count);

	clock_gettime(CLOCK_MONOTONIC, &start);
	retval = write(pdata->fd, buf, pdata->count);
	clock_gettime(CLOCK_MONOTONIC, &stop);

	presp->duration.tv_sec = stop.tv_sec - start.tv_sec;
	presp->duration.tv_nsec = stop.tv_nsec - start.tv_nsec;

	printf("Sender DONE.\n");
	return presp;
}

static void *receiver_func(void *arg)
{
	struct ping_data *pdata = (struct ping_data *)arg;
	struct ping_response *presp = NULL;
	int i;
	char *buf;
	struct timespec start, stop;
	ssize_t read_bytes, read_count;

	if (!pdata)
		return NULL;

	presp = (struct ping_response *)calloc(1, sizeof(struct ping_response));
	if (!presp)
		return NULL;

	buf = (char *)malloc(pdata->count);
	if (!buf) {
		presp->retval = -ENOMEM;
		return presp;
	}

	clock_gettime(CLOCK_MONOTONIC, &start);

	read_count = 0;
	do {
		read_bytes = read(pdata->fd, buf + read_bytes,
				pdata->count - read_count);
		if (read_bytes < 0) {
			/* read error */
			presp->retval = -errno;
			return presp;
		}
		read_count += read_bytes;
	} while (read_count < pdata->count);

	clock_gettime(CLOCK_MONOTONIC, &stop);

	for (i = 0; i < pdata->count; i++)
		if (buf[i] != 'a') {
			presp->retval = -EINVAL;
			return presp;
		}

	presp->duration.tv_sec = stop.tv_sec - start.tv_sec;
	presp->duration.tv_nsec = stop.tv_nsec - start.tv_nsec;

	printf("Receiver DONE.\n");

	return presp;
}

static int ping_init(struct cmd *cmd, int argc, char *argv[])
{
	int ret;
	int c;

	struct ping_data *pdata = (struct ping_data *)calloc(1,
			sizeof(struct ping_data));
	if (!pdata)
		return -ENOMEM;

	static struct option long_options[] =  {
		{"server", no_argument, 0, 's'},
		{"count", required_argument, 0, 'n'},
		{"command", required_argument, 0, 'c'},
		{0, 0, 0, 0}
	};

	while (1) {
		int option_index = 0;

		c = getopt_long(argc, argv, "st:c:n:", long_options,
				&option_index);
		if (c == -1)
			break;

		switch (c) {
		case 'n':
			pdata->count = atoi(optarg);
			break;
		case 's':
			pdata->server = 1;
			break;
		case 'c':
			if (strcmp(optarg, "SEND") == 0) {
				pdata->cmd = SEND_REQ;
			} else if (strcmp(optarg, "SEND_RECV") == 0) {
				pdata->cmd = SEND_RECV_REQ;
			} else {
				ret = -EINVAL;
				goto e_exit;
			}
			break;
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

static int ping_exec(struct cmd *cmd)
{
	struct ping_data *pdata = (struct ping_data *)cmd->priv;
	int command;
	int arg;
	int ret = 0;
	pthread_attr_t attr;

	if (!pdata)
		return -EINVAL;

	/* TODO: Add error handling */
	pthread_attr_init(&attr);

	if (pdata->server) {
		read_cmd(pdata->fd, &pdata->cmd, &arg);
		pdata->count = arg;
		pthread_create(&pdata->receiver_id, &attr, &receiver_func,
			(void *)pdata);
		send_cmd(pdata->fd, OKAY, 0);
		if (pdata->cmd == SEND_RECV_REQ)
			/* TODO: Check if delay is required */
			pthread_create(&pdata->sender_id, &attr, &sender_func,
				(void *)pdata);
	} else {
		send_cmd(pdata->fd, pdata->cmd, pdata->count);
		if (pdata->cmd == SEND_RECV_REQ)
			pthread_create(&pdata->receiver_id, &attr,
				&receiver_func, (void *)pdata);
		read_cmd(pdata->fd, &command, &arg);
		if (command == OKAY)
			pthread_create(&pdata->sender_id, &attr, &sender_func,
				(void *)pdata);
	}

e_exit:
	pthread_attr_destroy(&attr);
	return ret;
}

static int ping_cleanup(struct cmd *cmd)
{
	void *sender_ret = NULL, *receiver_ret = NULL;
	int retval = 0;
	long duration;
	struct ping_response *resp;
	struct ping_data *pdata = (struct ping_data *)cmd->priv;


	if (!pdata) {
		return -EINVAL;
	}
	
	if (pdata->sender_id)
		pthread_join(pdata->sender_id, &sender_ret);

	if (pdata->receiver_id)
		pthread_join(pdata->receiver_id, &receiver_ret);

	if (!pdata->server) {
		if (!sender_ret) {
			retval = -EINVAL;
			goto e_exit;
		}

		resp = (struct ping_response *) sender_ret;
		if (resp->retval != 0) {
			retval = resp->retval;
			printf("Check3\n");
			goto e_exit;
		}

		duration = resp->duration.tv_sec * 1000000 +
			resp->duration.tv_nsec / 1000;

		printf("Receiver took %ld usec.\n", duration);
	}

e_exit:
	free(pdata);

	if (sender_ret)
		free(sender_ret);

	if (receiver_ret)
		free(receiver_ret);

	cmd->priv = NULL;

	return retval;
}

REGISTER_CMD(
	ping,
	"ping test over a serial line",
	ping_help,
	ping_init,
	ping_exec,
	ping_cleanup
);
