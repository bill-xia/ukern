#ifndef ERRNO_H
#define ERRNO_H

#define E_INVALID 0
#define E_NOMEM 1

#define E_FORMAT 2 // for loading program image
#define E_NOPCB 3  // for creating proc

#define E_MAX 4

extern const char *error_msg[E_MAX];

#endif