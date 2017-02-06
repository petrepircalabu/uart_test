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
#ifndef CMD_H
#define CMD_H

#define MAX_CMDS 100

struct cmd;

struct cmd {
	int (*init)(struct cmd *, int, char *[]);
	int (*exec)(struct cmd *);
	int (*cleanup)(struct cmd *);
	const char *name;
	const char *description;
	const char *help;
	void *priv;
};

extern int cmd_count;
extern struct cmd *cmds[MAX_CMDS];

#define REGISTER_CMD(NAME, DESCRIPTION, HELP, INIT, EXEC, CLEANUP) \
static struct cmd NAME ## _cmd = {\
.init = INIT,\
.exec = EXEC,\
.cleanup = CLEANUP,\
.name = #NAME,\
.description = DESCRIPTION,\
.help = HELP, \
};\
static __attribute__((constructor)) void register_ ## NAME(void) \
{ \
	cmds[cmd_count++] = &NAME ## _cmd; \
}

int run_cmd(int argc, char *argv[]);

struct cmd *find_cmd(const char *name);

int execute_cmd(struct cmd *p_cmd, int argc, char *argv[]);

#endif /* CMD_H */
