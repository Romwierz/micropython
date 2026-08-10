/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2021 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string.h>
#include "xuartps.h"
#include "xparameters.h"

#include "py/builtin.h"
#include "py/compile.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "shared/runtime/pyexec.h"
#include "xuartps_hw.h"

#define PS_CLK 33330000

// GPIO registers offsets
#define DATA0 0x00000040/4
#define DIRM0 0x00000204/4
#define OEN0  0x00000208/4

// MIO registers offsets
#define MIO_PIN_00 0x00000700/4 // User LED 1
#define MIO_PIN_09 0x00000724/4 // User LED 2
#define MIO_PIN_11 0x0000072c/4 // UART0 Tx

// SLCR registers offsets
#define SLCR_IO_PLL_CTRL   0x00000108
#define SLCR_PLL_STATUS    0x0000010C
#define SLCR_IO_PLL_CFG    0x00000118
#define SLCR_DCI_CLK_CTRL  0x00000128
#define SLCR_UART_CLK_CTRL 0x00000154

// MIO_PIN_x bits definition
#define MIO_PIN_DisableRcvr 1U << 13
#define MIO_PIN_PULLUP      1U << 12
#define MIO_PIN_IO_Type     6U << 11
#define MIO_PIN_Speed       1U << 8
#define MIO_PIN_L3_SEL      6U << 7
#define MIO_PIN_L2_SEL      6U << 4
#define MIO_PIN_L1_SEL      1U << 2
#define MIO_PIN_L0_SEL      1U << 1
#define MIO_PIN_TRI_ENABLE  1U << 0

#define MIO_PIN_IO_Type_LVCMOS18 1U << 11

#define	XUARTPS_BASEADDRESS	XPAR_XUARTPS_1_BASEADDR

int uart_init(UINTPTR BaseAddress);

unsigned int * const XGPIOPS = (unsigned int *)0xe000a000;
volatile unsigned int * const SLCR = (unsigned int *)0xf8000000;

static inline void set_bit(unsigned int nr, volatile unsigned int *addr)
{
    unsigned int mask = 1U << nr;
    *addr  |= mask;
}

static inline void clear_bit(unsigned int nr, volatile unsigned int *addr)
{
    unsigned int mask = 1U << nr;
    *addr &= ~mask;
}

void configure_mio0_9(void)
{
    SLCR[MIO_PIN_00] = MIO_PIN_DisableRcvr | MIO_PIN_IO_Type_LVCMOS18;
    SLCR[MIO_PIN_09] = MIO_PIN_DisableRcvr | MIO_PIN_IO_Type_LVCMOS18;
}

static void delay(volatile unsigned int cycles)
{
    while (cycles--);
}

static const char *demo_single_input =
    "print('hello world!', list(x + 1 for x in range(10)), end='eol\\n')";

static const char *demo_file_input =
    "import micropython\n"
    "\n"
    "print(dir(micropython))\n"
    "\n"
    "for i in range(10):\n"
    "    print('iter {:08}'.format(i))";

#if MICROPY_ENABLE_COMPILER
void do_str(const char *src, mp_parse_input_kind_t input_kind) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        // Compile, parse and execute the given string.
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // Uncaught exception: print it out.
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}
#endif

static char *stack_top;
#if MICROPY_ENABLE_GC
static char heap[MICROPY_HEAP_SIZE];
#endif

// Main entry point: initialise the runtime and execute demo strings.
int main(void) {
	uart_init(XUARTPS_BASEADDRESS);
    outbyte('x');
    outbyte('\n');

    int stack_dummy;
    stack_top = (char *)&stack_dummy;

    #if MICROPY_ENABLE_GC
    gc_init(heap, heap + sizeof(heap));
    #endif
    mp_init();
    do_str(demo_single_input, MP_PARSE_SINGLE_INPUT);
    do_str(demo_file_input, MP_PARSE_FILE_INPUT);
    #if MICROPY_REPL_EVENT_DRIVEN
    pyexec_event_repl_init();
    for (;;) {
        int c = mp_hal_stdin_rx_chr();
        if (pyexec_event_repl_process_char(c)) {
            break;
        }
    }
    #else
    pyexec_friendly_repl();
    #endif
    do_str("print('hello world!', list(x+1 for x in range(10)), end='eol\\n')", MP_PARSE_SINGLE_INPUT);
    do_str("for i in range(10):\r\n  print(i)", MP_PARSE_FILE_INPUT);

    // Congigure MIO0,9 as GPIO
    configure_mio0_9();

    // Configure MIO pins 0,9 (User LED 1,2) as output and enable it
    set_bit(0, &XGPIOPS[DIRM0]);
    set_bit(0, &XGPIOPS[OEN0]);
    set_bit(9, &XGPIOPS[DIRM0]);
    set_bit(9, &XGPIOPS[OEN0]);


    // Toggle the leds
    while(1) {
        clear_bit(0, &XGPIOPS[DATA0]);
        set_bit(9, &XGPIOPS[DATA0]);
        delay(4000000U);
        set_bit(0, &XGPIOPS[DATA0]);
        clear_bit(9, &XGPIOPS[DATA0]);
        delay(4000000U);
    };

    mp_deinit();
}

#if MICROPY_ENABLE_GC
void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    gc_dump_info(&mp_plat_print);
}
#endif

mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    mp_raise_OSError(MP_ENOENT);
}

// Called if an exception is raised outside all C exception-catching handlers.
void nlr_jump_fail(void *val) {
    for (;;) {
    }
}

void MP_NORETURN __fatal_error(const char *msg) {
    while (1) {
        ;
    }
}

#ifndef NDEBUG
// Used when debugging is enabled.
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    for (;;) {
    }
}
#endif
