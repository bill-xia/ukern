#include "console.h"

static uint16_t *crt_buf = (uint16_t *)0xFFFF8000000B8000;
static uint32_t crt_ind = 0;

void
console_putch(char c)
{
    if (crt_ind == ROWS * COLS) {
        // screen full
        for (int i = 0; i < (ROWS - 1) * COLS; ++i) {
            crt_buf[i] = crt_buf[i + COLS];
        }
        for (int i = (ROWS - 1) * COLS; i < ROWS * COLS; ++i) {
            crt_buf[i] = ' ';
        }
        crt_ind -= COLS;
    }
    switch (c) {
    case '\n':
        do {
            crt_buf[crt_ind] = (' ' | (COLOR_FORE_WHITE | COLOR_BACK_BLACK));
            ++crt_ind;
        } while (crt_ind % COLS);
        break;
    case '\b':
        crt_ind--;
        break;
    case '\r':
        crt_ind -= crt_ind % COLS;
        break;
    case '\t':
        do {
            crt_buf[crt_ind] = (' ' | (COLOR_FORE_WHITE | COLOR_BACK_BLACK));
            ++crt_ind;
        } while (crt_ind % TAB_SIZE);
        break;
    default:
        crt_buf[crt_ind] = (c | (COLOR_FORE_WHITE | COLOR_BACK_BLACK));
        ++crt_ind;
        break;
    }
}