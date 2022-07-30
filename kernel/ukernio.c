#include "ukernio.h"

static uint16_t *crt_buf = (uint16_t *)0xB8000;
static uint32_t crt_ind = 0;

void print(char *s)
{
    while ((*s) != '\0') {
        crt_buf[crt_ind] = ((uint16_t)(*s) | (COLOR_FORE_WHITE | COLOR_BACK_BLACK));
        ++crt_ind;
        ++s;
    }
}