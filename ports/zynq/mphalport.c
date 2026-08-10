#include "py/mpconfig.h"
#include "xuartps_hw.h"

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
    c = inbyte();
    return c;
}

// Send string of given length to stdout, converting \n to \r\n.
mp_uint_t mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    mp_uint_t ret = len;
    while (len--) {
        outbyte(*str++);
    }
    return ret;
}
