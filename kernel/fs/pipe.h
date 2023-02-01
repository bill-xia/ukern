#ifndef PIPE_H
#define PIPE_H

#include "types.h"

#define PIPE_BUFLEN (4096 - sizeof(struct pipe))
// each pipe should occupy a page in kernel space
struct pipe {
	size_t		rref,
			wref,
			head,
			len,
			rsiz,	// rptr is omitted, because we return to read()
				// immediately once any data is available
			wptr,	// wptr is needed, because it may take many
				// trials to write all the data
			wsiz;	// wsiz is always the write()'s third argument,
				// not updated with wptr
	struct proc	*reader,
			*writer;
	char		*rbuf,
			*wbuf;
	u8		buf[0];
};

int pipe_read(struct pipe *pipe, int invoke);
int pipe_write(struct pipe *pipe, int invoke);

#endif