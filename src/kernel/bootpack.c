// Booting package
#include <stdio.h> // This is not standard C library. it was created by Kawai Hidemi for Haribote OS.

// these functions are implented in asm/naskfunc.asm
void io_hlt(void);
void io_cli(void);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);

// these functions are implemented below
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);

// color constants
#define COL8_000000    0  // black
#define COL8_FF0000    1  // bright red
#define COL8_00FF00    2  // bright green
#define COL8_FFFF00    3  // bright yellow
#define COL8_0000FF    4  // bright blue
#define COL8_FF00FF    5  // bright purple
#define COL8_00FFFF    6  // bright light blue
#define COL8_FFFFFF    7  // white
#define COL8_C6C6C6    8  // light gray
#define COL8_840000    9  // dark red
#define COL8_008400   10  // dark green
#define COL8_848400   11  // dark yellow
#define COL8_000084   12  // dark blue
#define COL8_840084   13  // dark purple
#define COL8_008484   14  // dark light blue
#define COL8_848484   15  // dark gray

// boot info struct
struct BOOTINFO {
    char cyls, leds, vmode, reserve;
    short scrnx, scrny;
    char *vram;
};

// 8bytes structure which compose GDT
struct SEGMENT_DESCRIPTOR {
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;
};

// 8bytes structure which compose IDT
struct GATE_DESCRIPTOR {
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};

// GDT/IDT functions declaration
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);

// main function(entry point)
void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0; // load boot info from memory address 0x0ff0 (set in boot/asmhead.nas)
    char s[40], mcursor[256];
    int mx, my;

    init_palette();
    init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
    mx = (binfo->scrnx - 16) / 2; // center of screen
    my = (binfo->scrny - 28 - 16) / 2;
    init_mouse_cursor8(mcursor, COL8_008484);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    putfonts8_asc(binfo->vram, binfo->scrnx, 8, 8, COL8_FFFFFF, "Hello, World!");
    putfonts8_asc(binfo->vram, binfo->scrnx, 31, 31, COL8_FFFFFF, "Haribote OS.");
    sprintf(s, "scrnx = %d", binfo->scrnx);
    putfonts8_asc(binfo->vram, binfo->scrnx, 16, 64, COL8_FFFFFF, s);

    for (;;) {
        io_hlt();
    }
}

void init_palette(void)
{
    static unsigned char table_rgb[16 * 3] = {          // static -> DB intruction to set color palette
        0x00, 0x00, 0x00,   // 0: black
        0xff, 0x00, 0x00,   // 1: bright red
        0x00, 0xff, 0x00,   // 2: bright green
        0xff, 0xff, 0x00,   // 3: bright yellow
        0x00, 0x00, 0xff,   // 4: bright blue
        0xff, 0x00, 0xff,   // 5: bright purple
        0x00, 0xff, 0xff,   // 6: bright light blue
        0xff, 0xff, 0xff,   // 7: white
        0xc6, 0xc6, 0xc6,   // 8: light gray
        0x84, 0x00, 0x00,   // 9: dark red
        0x00, 0x84, 0x00,   // 10: dark green
        0x84, 0x84, 0x00,   // 11: dark yellow
        0x00, 0x00, 0x84,   // 12: dark blue
        0x84, 0x00, 0x84,   // 13: dark purple
        0x00, 0x84, 0x84,   // 14: dark light blue
        0x84, 0x84, 0x84    // 15: dark gray
    };
    set_palette(0, 15, table_rgb);
    return;
}

void set_palette(int start, int end, unsigned char *rgb)
{
    int i, eflags;
    eflags = io_load_eflags();  // record interrupt enable flag
    io_cli();                   // disable interrupt
    io_out8(0x03c8, start);
    for (i = start; i <= end; i++) {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    io_store_eflags(eflags);    // restore interrupt enable flag
    return;
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
    int x, y;
    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            vram[y * xsize + x] = c;
        }
    }
    return;
}

void init_screen8(char *vram, int x, int y)
{
    boxfill8(vram, x, COL8_008484, 0,      0,      x - 1, y - 29);
    boxfill8(vram, x, COL8_C6C6C6, 0,      y - 28, x - 1, y - 28);
    boxfill8(vram, x, COL8_FFFFFF, 0,      y - 27, x - 1, y - 27);
    boxfill8(vram, x, COL8_C6C6C6, 0,      y - 26, x - 1, y - 1);

    boxfill8(vram, x, COL8_FFFFFF, 3,      y - 24, 59,     y - 24);
    boxfill8(vram, x, COL8_FFFFFF, 2,      y - 24, 2,      y - 4);
    boxfill8(vram, x, COL8_848484, 3,      y - 4,  59,     y - 4);
    boxfill8(vram, x, COL8_848484, 59,     y - 23, 59,     y - 5);
    boxfill8(vram, x, COL8_000000, 2,      y - 3,  59,     y - 3);
    boxfill8(vram, x, COL8_000000, 60,     y - 24, 60,     y - 3);

    boxfill8(vram, x, COL8_848484, x - 47, y - 24, x - 4,  y - 24);
    boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y - 4);
    boxfill8(vram, x, COL8_FFFFFF, x - 47, y - 3,  x - 4,  y - 3);
    boxfill8(vram, x, COL8_FFFFFF, x - 3,  y - 24, x - 3,  y - 3);
    return;
}

void putfont8(char *vram, int xsize, int x, int y, char c, char *font)
{
    int i;
    char *p, d; // *p:address, d:data
    for (i=0; i<16; i++) {
        p = vram + (y + i) * xsize + x;
        d = font[i];
        if ((d & 0x80) != 0) { p[0] = c; }
        if ((d & 0x40) != 0) { p[1] = c; }
        if ((d & 0x20) != 0) { p[2] = c; }
        if ((d & 0x10) != 0) { p[3] = c; }
        if ((d & 0x08) != 0) { p[4] = c; }
        if ((d & 0x04) != 0) { p[5] = c; }
        if ((d & 0x02) != 0) { p[6] = c; }
        if ((d & 0x01) != 0) { p[7] = c; }
    }
    return;
}

void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s)
{
    extern char font[4096];
    for (; *s != 0x00; s++) {
        putfont8(vram, xsize, x, y, c, font + *s * 16);
        x += 8;
    }
    return;
}

void init_mouse_cursor8(char *mouse, char bc)
{
    static char cursor[16][16] = {
        "**************..",
        "*OOOOOOOOOOO*...",
        "*OOOOOOOOOO*....",
        "*OOOOOOOOO*.....",
        "*OOOOOOOO*......",
        "*OOOOOOO*.......",
        "*OOOOOOO*.......",
        "*OOOOOOOO*......",
        "*OOOO**OOO*.....",
        "*OOO*..*OOO*....",
        "*OO*....*OOO*...",
        "*O*......*OOO*..",
        "**........*OOO*.",
        "*..........*OOO*",
        "............*OO*",
        ".............***"
    };
    int x, y;

    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            if (cursor[y][x] == '*') {
                mouse[y * 16 + x] = COL8_000000;
            }
            if (cursor[y][x] == 'O') {
                mouse[y * 16 + x] = COL8_FFFFFF;
            }
            if (cursor[y][x] == '.') {
                mouse[y * 16 + x] = bc;
            }
        }
    }
    return;
}

void putblock8_8(char *vram, int vxsize, int pxsize,
                 int pysize, int px0, int py0, char *buf, int bxsize)
{
    int x, y;
    for (y=0; y<pysize; y++) {
        for (x=0; x<pxsize; x++) {
            vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
        }
    }
    return;
}

void init_gdtidt(void)
{
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) 0x00270000; // GDT range 0x00270000 - 0x0027ffff. It's possible if it's a address range that nobody don't use.
    struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) 0x0026f800; // IDT range 0x0026f800 - 0x0026ffff. It's possible if it's a address range that nobody don't use.
    int i;

    // initialize GDT
    // all segment descriptors(8192 entries) are set to 0 (limit=0, base=0, ar=0)
    for (i = 0; i < 8192; i++) {
        set_segmdesc(gdt + i, 0, 0, 0); // gdt + 1 = gdt + sizeof(SEGMENT_DESCRIPTOR) * 1
    }

    // first segment is representing entire memory area
    set_segmdesc(gdt + 1, 0xffffffff, 0x00000000, 0x4092);
    // second segment is representing area for bootpack
    set_segmdesc(gdt + 2, 0x0007ffff, 0x00280000, 0x409a);
    
    load_gdtr(0xffff, 0x00270000);

    // initialize IDT
    for (i = 0; i < 256; i++) {
        set_gatedesc(idt + i, 0, 0, 0);
    }
    load_idtr(0x7ff, 0x0026f800);

    return;
}

// set segment descriptor
// sd: segment descriptor address
// limit: segment limit
// base: segment base address
// ar: access right
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar)
{
    if (limit > 0xfffff) {
        ar |= 0x8000; // G_bit = 1
        limit /= 0x1000;
    }
    sd->limit_low    = limit & 0xffff;
    sd->base_low     = base & 0xffff;
    sd->base_mid     = (base >> 16) & 0xff;
    sd->access_right = ar & 0xff;
    sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
    sd->base_high    = (base >> 24) & 0xff;
    return;
}

void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar)
{
    gd->offset_low = offset & 0xffff;
    gd->selector = selector;
    gd->dw_count = (ar >> 8) & 0xff;
    gd->access_right = ar & 0xff;
    gd->offset_high = (offset >> 16) & 0xffff;
    return;
}
