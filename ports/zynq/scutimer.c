#include "xstatus.h"
#include "xscutimer.h"
#include "scutimer.h"

// All private timers and watchdog timers are always clocked at 1/2 of the CPU frequency (CPU_3x2x)
// It's probably a better idea to calculate the the CPU clk frequency based on the known PS_CLK
// instead of using a predifend value from xparameters.h
#define SCUTIMER_CLK_FREQ_HZ (XPAR_CPU_CORE_CLOCK_FREQ_HZ >> 1)

static XScuTimer Timer;	/* Cortex A9 SCU Private Timer Instance */
// Use this directly instead using lookup config table
static XScuTimer_Config scutimer_config = {
    .BaseAddr = XPAR_SCUTIMER_BASEADDR,
};

int scutimer_init(void) {
    int Status;

    // Zero out scutimer struct to avoid problems after resetting the SoC with a button
    // (Memory is not reset so the values of globals are the same as before the reset)
    Timer = (XScuTimer){0};

    // Initialize timer
    Status = XScuTimer_CfgInitialize(&Timer, &scutimer_config, scutimer_config.BaseAddr);
    if (Status != XST_SUCCESS) return XST_FAILURE;

    // Set prescaler so timer's period is 100ns
    XScuTimer_SetPrescaler(&Timer, SCUTIMER_CLK_FREQ_HZ / 10000000);

    // Enable Auto Reload and load counter with max value
    XScuTimer_EnableAutoReload(&Timer);
    XScuTimer_LoadTimer(&Timer, SCUTIMER_COUNTER_MAX_VALUE);

    return Status;
}

uint32_t scutimer_get_count(void) {
    return XScuTimer_GetCounterValue(&Timer);
}

void scutimer_start(void) {
    XScuTimer_Start(&Timer);
}

void scutimer_stop(void) {
    XScuTimer_Stop(&Timer);
}
