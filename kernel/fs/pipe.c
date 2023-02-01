#include "pipe.h"
#include "proc.h"
#include "printk.h"

// read from a pipe
// reader, rbuf and rsiz should be set properly before calling this function.
// rbuf is a pointer, must be converted into kernel space
// if invoke == 0, this function doesn't play with pipe->reader's state
// if invoke != 0, set pipe->reader's rax to # of bytes read, and state to READY
int
pipe_read(struct pipe *pipe, int invoke)
{
	// do the read
	size_t n = min(pipe->len, pipe->rsiz);
	if (n == 0)
		return 0;

	// convert rbuf into kernel space
	char *rbuf = (char *)U2K(pipe->reader, (u64)pipe->rbuf, 1);

	// workhorse
	size_t	i = 0,
		ind = 0,
		j = pipe->head,
		pipe_tail = (pipe->head + n) % PIPE_BUFLEN;
	while (j != pipe_tail) {
		rbuf[ind] = pipe->buf[j];
		++j;
		if (j == PIPE_BUFLEN)
			j = 0;
		++i;
		if ((u64)(pipe->rbuf + i) & PGMASK) {
			++ind;
			continue;
		}
		// convert rbuf into kernel space again
		rbuf = (char *)U2K(pipe->reader, (u64)pipe->rbuf + i, 1);
		ind = 0;
	}
	pipe->len -= n;
	pipe->head += n;
	if (pipe->head >= PIPE_BUFLEN)
		pipe->head -= PIPE_BUFLEN;
	if (invoke) {
		pipe->reader->context.rax = n;
		pipe->reader->state = READY;
	}

	pipe->rbuf = NULL;
	pipe->rsiz = 0;
	pipe->reader = NULL;
	// wake up writer if any
	if (pipe->writer != NULL) {
		pipe_write(pipe, 1);
	}
	return n;
}

// write to a pipe
// writer, wbuf and wsiz should be set properly before calling this function.
// wbuf is a pointer, must be converted into kernel space
// if invoke == 0, this function doesn't play with pipe->writer's state
// if invoke != 0 and all data is written out, set pipe->writer's rax to
// # of bytes written, and state to READY
int
pipe_write(struct pipe *pipe, int invoke)
{
	// do the write
	size_t n = min(PIPE_BUFLEN - pipe->len - 1, pipe->wsiz - pipe->wptr);
	// leave one slot, not a big problem
	// in case we want to test emptyness by comparing pointer some day.
	if (n == 0)
		return 0;

	// convert wbuf into kernel space
	char *wbuf = (char *)U2K(pipe->writer, (u64)pipe->wbuf + pipe->wptr, 0);
	
	// workhorse
	size_t	i = 0,
		ind = 0,
		j = (pipe->head + pipe->len) % PIPE_BUFLEN,
		new_pipe_head = (pipe->head + pipe->len + n) % PIPE_BUFLEN;
	while (j != new_pipe_head) {
		pipe->buf[j] = wbuf[ind];
		++j;
		if (j == PIPE_BUFLEN)
			j = 0;
		++i;
		if ((u64)(pipe->wbuf + pipe->wptr + i) & PGMASK) {
			++ind;
			continue;
		}
		// convert wbuf into kernel space again
		wbuf = (char *)U2K(pipe->writer,
			(u64)pipe->wbuf + pipe->wptr + i, 0);
		ind = 0;
	}
	pipe->wptr += n;
	pipe->len += n;
	if (invoke && pipe->wptr == pipe->wsiz) {
		pipe->writer->context.rax = pipe->wsiz;
		pipe->writer->state = READY;
	}
	if (pipe->wptr == pipe->wsiz) {
		pipe->wbuf = NULL;
		pipe->wptr = 0;
		pipe->wsiz = 0;
		pipe->writer = NULL;
	}
	// wake up reader if any
	// pipe_read() and pipe_write() calling each other never cause
	// recursion in execution, because they clear themselves(reader/writer)
	// before calling each other.
	if (pipe->reader != NULL) {
		pipe_read(pipe, 1);
	}
	return n;
}
