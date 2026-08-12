#include "xparameters_ps.h"

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

// utils
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

static inline void delay(volatile unsigned int cycles)
{
    while (cycles--);
}

// bspconfig.h
#define XPAR_STDIN_IS_UARTPS  
#define STDIN_BASEADDRESS XPS_UART1_BASEADDR
#define STDOUT_BASEADDRESS XPS_UART1_BASEADDR

// xil_printf.h
// extern void outbyte (char c); /**< To send byte */
// extern char inbyte(void); /**< To receive byte */
