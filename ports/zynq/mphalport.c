#include "py/mpconfig.h"
#include "scutimer.h"
#include "xuartps_hw.h"
#include "py/runtime.h"
#include "py/mphal.h"

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

mp_uint_t mp_hal_ticks_100ns(void) {
    return (mp_uint_t)(SCUTIMER_COUNTER_MAX_VALUE - scutimer_get_count());
}

mp_uint_t mp_hal_ticks_us(void) {
    return mp_hal_ticks_100ns() / 10;
}

mp_uint_t mp_hal_ticks_ms(void) {
    return mp_hal_ticks_100ns() / 10000;
}

void mp_hal_delay_us(mp_uint_t us) {
}

void mp_hal_delay_ms(mp_uint_t ms) {
    mp_uint_t start = mp_hal_ticks_ms();
    mp_uint_t elapsed = 0;
    do {
        mp_event_wait_ms(ms - elapsed);
        elapsed = mp_hal_ticks_ms() - start;
    } while (elapsed < ms);
}

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    uintptr_t ret = 0;
    #if MICROPY_HW_USB_CDC
    ret |= mp_usbd_cdc_poll_interfaces(poll_flags);
    #endif
    #if MICROPY_HW_ENABLE_UART_REPL
    if (poll_flags & MP_STREAM_POLL_WR) {
        ret |= MP_STREAM_POLL_WR;
    }
    #endif
    #if MICROPY_PY_OS_DUPTERM
    ret |= mp_os_dupterm_poll(poll_flags);
    #endif
    return ret;
}

