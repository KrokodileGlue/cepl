/*
 * compile.h - compile the generated source code
 *
 * AUTHOR: Joey Pabalinas <alyptik@protonmail.com>
 * See LICENSE.md file for copyright and license details.
 */

#ifndef COMPILE_H
#define COMPILE_H 1

/* silence linter */
#ifndef _GNU_SOURCE
#	define _GNU_SOURCE
#endif

#include "defs.h"
#include "errs.h"
#include <fcntl.h>
#include <sys/sendfile.h>

/* prototypes */
int compile(char const *restrict src, char *const cc_args[], char *const exec_args[]);

static inline void set_cloexec(int set_fd[static 2])
{
	if (fcntl(set_fd[0], F_SETFD, FD_CLOEXEC) == -1)
		WARN("fnctl()");
	if (fcntl(set_fd[1], F_SETFD, FD_CLOEXEC) == -1)
		WARN("fnctl()");
}

static inline void pipe_fd(int in_fd, int out_fd)
{
	/* pipe data in a loop */
	for (;;) {
		ptrdiff_t buf_len;
		if ((buf_len = splice(in_fd, NULL, out_fd, NULL, PAGE_SIZE, SPLICE_F_MOVE)) == -1) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			printf("%d\n", errno);
			WARN("error reading from input fd");
			break;
		}
		/* break on EOF */
		if (buf_len == 0)
			break;
	}
}

#endif
