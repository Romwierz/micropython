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
#include "py/compile.h"
#include "py/runtime.h"
#include "xstatus.h"
#include "xuartps.h"
#include "xparameters.h"

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

int UartPsHelloWorldExample(UINTPTR BaseAddress);

unsigned int * const XGPIOPS = (unsigned int *)0xe000a000;
volatile unsigned int * const SLCR = (unsigned int *)0xf8000000;
XUartPs Uart_Ps;

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

static void do_str(const char *src, mp_parse_input_kind_t input_kind) {
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

u32 calculate_uart_ref_clk(const u32 ps_clk)
{
    u32 uart_clk_ctrl;
    u32 io_pll_ctrl;
    // u32 io_pll_cfg;
    u32 divisor = 0;
    // u32 srcsel = 0;
    u32 pll_fdiv = 0;
    // u32 pll_lock_cnt = 0;
    // u32 pll_cp = 0;
    // u32 pll_res = 0;

    // todo:
    // 1) Check PLL source used to generate UART clock
    // 2) Check PLL configuration (bypass included)
    // For now, assume IO PLL is source and bypass is disabled

    uart_clk_ctrl = Xil_In32(XPAR_SLCR_BASEADDR + SLCR_UART_CLK_CTRL);
    divisor = (uart_clk_ctrl >> 8) & 0x6;
    // srcsel = (uart_clk_ctrl >> 4) & 0x2;

    io_pll_ctrl = Xil_In32(XPAR_SLCR_BASEADDR + SLCR_IO_PLL_CTRL);
    // io_pll_cfg = Xil_In32(XPAR_SLCR_BASEADDR + SLCR_IO_PLL_CFG);
    pll_fdiv = (io_pll_ctrl >> 12) & 0x7;
    // pll_lock_cnt = (io_pll_cfg >> 12) & 0xa;
    // pll_cp = (io_pll_cfg >> 8) & 0x4;
    // pll_res = (io_pll_cfg >> 4) & 0x4;

    return ps_clk * pll_fdiv / divisor;
}

// Write a character out to the UART.
static inline void uart_write_char(int c) {
    // XUartPs_Send(&Uart_Ps, (u8 *)&c, 1);
    outbyte(c);
}

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
    c = inbyte();
    return c;
}

// Send string of given length to stdout, converting \n to \r\n.
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    while (len--) {
        if (*str == '\n') {
            uart_write_char('\r');
        }
        uart_write_char(*str++);
    }
}

// Main entry point: initialise the runtime and execute demo strings.
void bare_main(void) {
    int c;

	UartPsHelloWorldExample(XUARTPS_BASEADDRESS);
    c = mp_hal_stdin_rx_chr();

    xil_printf("\n\rWaiting for input...\n\r");
    xil_printf("Received char: %c\n\r\n\r", c);

    xil_printf("SLCR_PLL_STATUS: 0x%08x\n\r", Xil_In32(XPAR_SLCR_BASEADDR + SLCR_PLL_STATUS));
    xil_printf("SLCR_IO_PLL_CTRL: 0x%08x\n\r", Xil_In32(XPAR_SLCR_BASEADDR + SLCR_IO_PLL_CTRL));
    xil_printf("SLCR_IO_PLL_CFG: 0x%08x\n\r", Xil_In32(XPAR_SLCR_BASEADDR + SLCR_IO_PLL_CFG));
    xil_printf("SLCR_UART_CLK_CTRL: 0x%08x\n\r", Xil_In32(XPAR_SLCR_BASEADDR + SLCR_UART_CLK_CTRL));
    xil_printf("Calculated UART_ref_clk: %d\n\r\n\r", calculate_uart_ref_clk(PS_CLK));


    mp_init();
    do_str(demo_single_input, MP_PARSE_SINGLE_INPUT);
    do_str(demo_file_input, MP_PARSE_FILE_INPUT);

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

int main(void) {
    bare_main();
}
// Called if an exception is raised outside all C exception-catching handlers.
void nlr_jump_fail(void *val) {
    for (;;) {
    }
}

#ifndef NDEBUG
// Used when debugging is enabled.
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    for (;;) {
    }
}
#endif

int UartPsHelloWorldExample(UINTPTR BaseAddress)
{
	//    xil_printf("--0--\n\r");
	// u8 HelloWorld[] = "\n\rHello World\n\r";
	u32 SentCount = 0;
	int Status;
	XUartPs_Config Config;

    // xil_printf("--1--\n\r");

    Config.BaseAddress = BaseAddress;
    Config.InputClockHz = calculate_uart_ref_clk(PS_CLK);

    // xil_printf("--2--\n\r");

	Status = XUartPs_CfgInitialize(&Uart_Ps, &Config, Config.BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

    // xil_printf("--3--\n\r");

	XUartPs_SetBaudRate(&Uart_Ps, 115200);

	//    xil_printf("--4--\n\r");
	//
	// while (SentCount < (sizeof(HelloWorld) - 1)) {
	// 	/* Transmit the data */
	// 	SentCount += XUartPs_Send(&Uart_Ps,
	// 				   &HelloWorld[SentCount], 1);
	// }

   // xil_printf("InputClkHz: %d\n\r", Uart_Ps.Config.InputClockHz);
   // xil_printf("RefClk: %d\n\r", Uart_Ps.Config.RefClk);
   // xil_printf("Calculated Baudrate: %d\n\r", Uart_Ps.BaudRate);

   return SentCount;
}
