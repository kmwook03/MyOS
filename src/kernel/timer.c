// CPU timer management

#include "../include/bootpack.h"

#define PIT_CTRL    0x0043
#define PIT_CNT0    0x0040

struct TIMERCTL timerctl;

#define TIMER_FLAGS_ALLOC   1   // timer is allocated
#define TIMER_FLAGS_USING   2   // timer is running

void init_pit(void)
{
    int i;
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);
    timerctl.count = 0;
    timerctl.next = 0xffffffff; // initially set to maximum value
    timerctl.using = 0;
    for (i=0; i<MAX_TIMER; i++) {
        timerctl.timers0[i].flags = 0; // mark as unused
    }
    return;
}

struct TIMER *timer_alloc(void)
{
    int i;
    for (i = 0; i < MAX_TIMER; i++) {
        if (timerctl.timers0[i].flags == 0) {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
        }
    }
    return 0; // cannot allocate
}

void timer_free(struct TIMER *timer)
{
    timer->flags = 0; // mark as unused
    return;
}

void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data)
{
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout)
{
    int e, i, j;
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    e = io_load_eflags();
    
    // insert timer into timer list (sorted by timeout)
    for (i=0; i<timerctl.using; i++) {
        if (timerctl.timers[i]->timeout >= timer->timeout) {
            break;
        }
    }

    // move forward to make space
    for (j=timerctl.using; j>i; j--) {
        timerctl.timers[j] = timerctl.timers[j-1];
    }
    timerctl.using++;
    // insert the timer
    timerctl.timers[i] = timer;
    timerctl.next = timerctl.timers[0]->timeout;
    io_store_eflags(e);
    return;
}

void inthandler20(int *esp)
{
    int i, j;
    io_out8(PIC0_OCW2, 0x60); // notify PIC that IRQ-00 has been handled
    timerctl.count++;
    if (timerctl.next > timerctl.count) {
        return; // no timer has reached timeout yet
    }
    for (i=0; i<timerctl.using; i++) {
        if (timerctl.timers[i]->timeout > timerctl.count) {
            break;
        }
        // timeout
        timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
        fifo8_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
    }
    timerctl.using -= i;
    for (j=0; j<timerctl.using; j++) {
        timerctl.timers[j] = timerctl.timers[i + j];
    }
    if (timerctl.using > 0) {
        timerctl.next = timerctl.timers[0]->timeout;
    } else {
        timerctl.next = 0xffffffff;
    }

    return;
}
