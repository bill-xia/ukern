#include "errno.h"

const char* error_msg[E_MAX] = {
    "errno invalid",
    "no available memory",
    "image not ELF-64 format",
    "no available process control block"
};