// mouse controller implementation
#include "../include/bootpack.h"

struct FIFO32 *mousefifo;
int mousedata0;

void inthandler2c(int *esp)
{
    unsigned char data;
    io_out8(PIC1_OCW2, 0x64);
    io_out8(PIC0_OCW2, 0x62);
    data = io_in8(PORT_KEYDAT);
    fifo32_put(mousefifo, data + mousedata0);
    return;
}

#define KEYCMD_SENDTO_MOUSE    0xd4
#define MOUSECMD_ENABLE        0xf4

void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec)
{
    mousefifo = fifo;
    mousedata0 = data0;
    wait_KBC_sendready();
    io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
    wait_KBC_sendready();
    io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
    // if good, ACK(0xfa) will be sent from mouse
    mdec->phase = 0; // waiting for ACK(0xfa)
    return;
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
    if (mdec->phase == 0) {
        // receive ACK(0xfa)
        if (dat == 0xfa) {
            mdec->phase = 1;
        }
        return 0;
    }
    if (mdec->phase == 1) {
        // waiting for first byte of data
        if ((dat & 0xc8) == 0x08) {
            // valid first byte
            mdec->buf[0] = dat;
            mdec->phase = 2;
        }
        return 0;
    }
    if (mdec->phase == 2) {
        // waiting for second byte of data
        mdec->buf[1] = dat; // store second byte
        mdec->phase = 3;
        return 0;
    }
    if (mdec->phase == 3) {
        // waiting for third byte of data
        mdec->buf[2] = dat;
        mdec->phase = 1;
        mdec->btn = mdec->buf[0] & 0x07;
        mdec->x = mdec->buf[1];
        mdec->y = mdec->buf[2];
        if ((mdec->buf[0] & 0x10) != 0) {
            mdec->x |= 0xffffff00;
        }
        if ((mdec->buf[0] & 0x20) != 0) {
            mdec->y |= 0xffffff00;
        }
        mdec->y = -mdec->y; // y axis is inverted
        return 1;
    }
    return -1; // should not occur
}
