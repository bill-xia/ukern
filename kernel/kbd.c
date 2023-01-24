#include "kbd.h"

char kbd_buffer[4096];
int kbd_buf_beg = 0, kbd_buf_siz = 0;