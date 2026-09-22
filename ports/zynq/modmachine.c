#include "xparameters.h"
#include "modmachine.h"
#include "py/runtime.h"
#include "xil_io.h"

static mp_obj_t mp_machine_unique_id(void) {
    // Read PSS_IDCODE register
    uint32_t id = Xil_In32(XPAR_SLCR_BASEADDR + 0x00000530);
    return mp_obj_new_bytes((byte *)&id, sizeof(id));
}

static mp_obj_t mp_machine_get_freq(void) {
    // What is this macro's purpose and why use it here?
    return MP_OBJ_NEW_SMALL_INT(XPAR_CPU_CORE_CLOCK_FREQ_HZ);
}

static void mp_machine_set_freq(size_t n_args, const mp_obj_t *args) {
    mp_raise_NotImplementedError(NULL);
}

static void mp_machine_idle(void) {
    // Other options to consider: wfe, wfi, mp_event_wait_indefinite, MICROPY_EVENT_POLL_HOOK
    mp_event_wait_ms(1);
}

static void mp_machine_lightsleep(size_t n_args, const mp_obj_t *args) {
    mp_raise_NotImplementedError(NULL);
}

MP_NORETURN static void mp_machine_deepsleep(size_t n_args, const mp_obj_t *args) {
    mp_raise_NotImplementedError(NULL);
}
