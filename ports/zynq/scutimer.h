#ifndef MICROPY_INCLUDED_ZYNQ_SCUTIMER_H
#define MICROPY_INCLUDED_ZYNQ_SCUTIMER_H

#define SCUTIMER_COUNTER_MAX_VALUE 0xffffffff

int scutimer_init(void);
uint32_t scutimer_get_count(void);
void scutimer_start(void);
void scutimer_stop(void);

#endif // MICROPY_INCLUDED_ZYNQ_SCUTIMER_H
