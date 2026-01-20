
// boot info struct (asmhead.nas)
struct BOOTINFO {
    char cyls, leds, vmode, reserve; // boot info
    short scrnx, scrny; // screen resolution
    char *vram; // video RAM address
};

#define ADR_BOOTINFO    0x00000ff0
#define ADR_DISKIMG     0x00100000

// naskfcunc.nas
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
int load_cr0(void);
void store_cr0(int cr0);
void load_tr(int tr);
void asm_inthandler0c(void);
void asm_inthandler0d(void);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
void farjmp(int eip, int cs);
void farcall(int eip, int cs);
void asm_hrb_api(void);
void start_app(int eip, int cs, int esp, int ds, int *tss_esp0);
void asm_end_app(void);

// fifo.c
struct FIFO32 {
    int *buf;
    int p, q, size, free, flags;
    struct TASK *task;
};

void fifo32_init(struct FIFO32 *fifo, int size, int *buf, struct TASK *task);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);

// graphic.c
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void init_screen8(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor8(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize,
                 int pysize, int px0, int py0, char *buf, int bxsize);

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

// dsctbl.c
// 8bytes structure which compose GDT
struct SEGMENT_DESCRIPTOR {
    short limit_low, base_low;
    char base_mid, access_right;
    char limit_high, base_high;

    // base = base_high << 24 + base_mid << 16 + base_low = 32bit
};

// 8bytes structure which compose IDT
struct GATE_DESCRIPTOR {
    short offset_low, selector;
    char dw_count, access_right;
    short offset_high;
};

// GDT/IDT functions implented in dsctbl.c
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);

#define ADR_IDT         0x0026f800
#define LIMIT_IDT       0x000007ff
#define ADR_GDT         0x00270000
#define LIMIT_GDT       0x0000ffff
#define ADR_BOTPAK      0x00280000
#define LIMIT_BOTPAK    0x0007ffff
#define AR_DATA32_RW    0x4092
#define AR_CODE32_ER    0x409a
#define AR_LDT          0x0082
#define AR_TSS32        0x0089
#define AR_INTGATE32    0x008e

// int.c
void init_pic(void);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1

// keyboard.c
void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *fifo, int data0);

#define PORT_KEYDAT    0x0060
#define PORT_KEYCMD    0x0064

// mouse.c
struct MOUSE_DEC {
    unsigned char buf[3], phase;
    int x, y, btn;
};

void inthandler2c(int *esp);
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

// memory.c
#define MEMMAN_FREES  4090  // max number of free memory blocks
#define MEMMAN_ADDR   0x003c0000

struct FREEINFO {   // information of free memory block
    unsigned int addr, size;
};

struct MEMMAN {
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

// sheet.c
#define MAX_SHEETS  256
struct SHEET {
    unsigned char *buf;
    int bxsize, bysize, vx0, vy0, col_inv, height, flags;
    struct SHTCTL *ctl;
    struct TASK *task;
};

struct SHTCTL {
    unsigned char *vram, *map;
    int xsize, ysize, top;
    struct SHEET *sheets[MAX_SHEETS]; // pointers to the sheets in display order
    struct SHEET sheets0[MAX_SHEETS]; // actual sheets
};

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc(struct SHTCTL *ctl);
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);

// timer.c
#define MAX_TIMER    500
struct TIMER {
    struct TIMER *next;
    unsigned int timeout;
    char flags, flags2;
    struct FIFO32 *fifo;
    int data;
};

struct TIMERCTL {
	unsigned int count, next;
	struct TIMER *t0;
    struct TIMER timers0[MAX_TIMER];
};
extern struct TIMERCTL timerctl;
void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
int timer_cancel(struct TIMER *timer);
void timer_cancelall(struct FIFO32 *fifo);
void inthandler20(int *esp);

// mtask.c
#define MAX_TASKS   1000    // max number of tasks
#define TASK_GDT0   3       // first TSS GDT number
#define MAX_TASKS_LV  100   // max number of tasks per level
#define MAX_TASKLEVELS  10  // max number of task levels

struct TSS32 {              // Task State Segment
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
};

struct TASK {
    int sel, flags;                     // selector(number of GDT) and status
    int level, priority;                // level and priority
    struct FIFO32 fifo;                 // FIFO for task
    struct TSS32 tss;                   // task state segment
    struct SEGMENT_DESCRIPTOR ldt[2];   // local descriptor table
    struct CONSOLE *cons;               // console associated with this task
    int ds_base, cons_stack;            // data segment base address
    struct FILEHANDLE *fhandle;         // file handle
    int *fat;                           // file allocation table
    char *cmdline;                      // command line
    char langmode;                      // language mode (0: English, 1: Korean)

};

struct TASKLEVEL {
    int running;                        // number of running tasks
    int now;                            // current running task index
    struct TASK *tasks[MAX_TASKS_LV];   // addresses of tasks
};

struct TASKCTL {
    int now_lv;                                 // current level
    char lv_change;                             // flag for level change
    struct TASKLEVEL level[MAX_TASKLEVELS];     // task levels
    struct TASK tasks0[MAX_TASKS];              // actual tasks
};
extern struct TASKCTL *taskctl;
extern struct TIMER *task_timer;
struct TASK *task_now(void);
struct TASK *task_init(struct MEMMAN *memman);
struct TASK *task_alloc(void);
void task_run(struct TASK *task, int level, int priority);
void task_switch(void);
void task_sleep(struct TASK *task);

// window.c
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act);
void change_wtitle8(struct SHEET *sht, char act);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);

// console.c
struct CONSOLE {
    struct SHEET *sht;
    int cur_x, cur_y, cur_c;
    struct TIMER *timer;
};
struct FILEHANDLE {
    char *buf;
    int size;
    int pos;
};
void console_task(struct SHEET *sht, int memtotal);
void cons_putchar(struct CONSOLE *cons, int chr, char move);
void cons_putstr0 (struct CONSOLE *cons, char *s);
void cons_putstr1(struct CONSOLE *cons, char *s, int l);
void cons_newline(struct CONSOLE *cons);
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal);
void cmd_mem(struct CONSOLE *cons, int memtotal);
void cmd_cls(struct CONSOLE *cons);
void cmd_dir(struct CONSOLE *cons);
void cmd_exit(struct CONSOLE *cons, int *fat);
void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal);
void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal);
void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col);
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline);
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax);
int *inthandler0c(int *esp);
int *inthandler0d(int *esp);

// file.c
struct FILEINFO {
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, data, clustno;
	unsigned int size;
};
void file_readfat(int *fat, unsigned char *img);
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img);
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max);
char *file_loadfile2(int clustno, int *psize, int *fat);

// tek.c
int tek_getsize(unsigned char *p);
int tek_decomp(unsigned char *p, char *q, int size);

// bootpack.c
struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal);
struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal);

// hangul.c
void put_johab(char *vram, int xsize, int x, int y, char color, unsigned char *font, unsigned short code);
unsigned short utf8_to_johab(unsigned char *s);
void putstr_utf8(unsigned char *vram, int xsize, int x, int y, char color, unsigned char *s);
