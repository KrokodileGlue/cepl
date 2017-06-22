/*
 * compile.c - compile the generated source code
 *
 * AUTHOR: Joey Pabalinas <alyptik@protonmail.com>
 * See LICENSE file for copyright and license details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/memfd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include "compile.h"

int compile(char const *cc, char *src, char *const ccargs[], char *const execargs[])
{
	int memfd, pipe_cc[2], pipe_exec[2];

	/* create pipes */
	pipe(pipe_cc);
	pipe(pipe_exec);
	set_cloexec(pipe_cc[0]);
	set_cloexec(pipe_cc[1]);
	set_cloexec(pipe_exec[0]);
	set_cloexec(pipe_exec[1]);

	/* fork compiler */
	switch (fork()) {
	/* error */
	case -1:
		err(EXIT_FAILURE, "%s", "error forking compiler");
		break;

	/* child */
	case 0:
		dup2(pipe_cc[0], 0);
		dup2(pipe_exec[1], 1);
		close(pipe_cc[1]);
		close(pipe_exec[0]);
		execvp(cc, ccargs);
		/* execvp() should never return */
		err(EXIT_FAILURE, "%s", "error forking compiler");
		break;

	/* parent */
	default:
		close(pipe_cc[0]);
		close(pipe_exec[1]);
		write(pipe_cc[1], src, strlen(src));
		close(pipe_cc[1]);
	}

	/* fork executable */
	switch (fork()) {
	/* error */
	case -1:
		err(EXIT_FAILURE, "%s", "error forking executable");
		break;

	/* child */
	case 0:
		if ((memfd = syscall(SYS_memfd_create, "cepl", MFD_CLOEXEC)) == -1)
			err(EXIT_FAILURE, "%s", "error creating memfd");
		if (pipe_fd(pipe_exec[0], memfd) == 0)
			err(EXIT_FAILURE, "%s", "zero bytes written by pipe_fd()");
		close(pipe_exec[0]);
		/* dup2(pipemain[1], STDOUT_FILENO); */
		fexecve(memfd, execargs, environ);
		/* fexecve() should never return */
		err(EXIT_FAILURE, "%s", "error forking executable");
		break;

	/* parent */
	default:
		close(pipe_exec[0]);
	}

	/* TODO: make return value useful */
	return 0;
}