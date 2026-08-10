#include "xstatus.h"
#include "xuartps.h"
#include "xuartps_hw.h"
#include "xparameters.h"
#include "py/mpconfig.h"

#define PS_CLK 33330000

// SLCR registers offsets
#define SLCR_IO_PLL_CTRL   0x00000108
#define SLCR_PLL_STATUS    0x0000010C
#define SLCR_IO_PLL_CFG    0x00000118
#define SLCR_DCI_CLK_CTRL  0x00000128
#define SLCR_UART_CLK_CTRL 0x00000154

#define	XUARTPS_BASEADDRESS	XPAR_XUARTPS_1_BASEADDR

XUartPs Uart_Ps;

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

int uart_init(UINTPTR BaseAddress)
{
	int Status;
	XUartPs_Config Config;

    Config.BaseAddress = BaseAddress;
    Config.InputClockHz = calculate_uart_ref_clk(PS_CLK);

	Status = XUartPs_CfgInitialize(&Uart_Ps, &Config, Config.BaseAddress);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	XUartPs_SetBaudRate(&Uart_Ps, 115200);

   return Status;
}
