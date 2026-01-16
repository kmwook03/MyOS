// bootpack.c

#include "../include/bootpack.h"
#include <stdio.h>

#define KEYCMD_LED		0xed

void keywin_off(struct SHEET *key_win);
void keywin_on(struct SHEET *key_win);
struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal);
void close_constask(struct TASK *task);
void close_console(struct SHEET *sht);

void HariMain(void)
{
	// Declare Structures
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;  		// boot info from bootloader (0x0ff0 - 0x0fff)
	struct SHTCTL *shtctl; 												// sheet control
	struct FIFO32 fifo;                                         		// FIFO buffers for task communication
	struct FIFO32 keycmd;								 				// FIFO buffer for keyboard commands
	struct MOUSE_DEC mdec;												// mouse decoding data
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;				// memory manager
	struct SHEET *sht_back, *sht_mouse;									// sheets: background, mouse
	struct TASK *task_a, *task_cons[2], *task;							// tasks: task_a, console task, etc task
	struct CONSOLE *cons;												// console
	struct SHEET *sht = 0, *key_win, *sht2;								// window sheet

	// Declare Variables
	// 1. Buffers
	char s[40];															// string buffer
	int fifobuf[128], keycmd_buf[32], *cons_fifo[2];					// FIFO buffers
	unsigned char *buf_back, buf_mouse[256], *buf_cons[2];				// buffers for sheets: background, mouse, console
	// 2. Positions and Indexes
	int mx, my, i, new_mx = -1, new_my = 0, new_wx = 0x7fffffff, new_wy = 0; // variables for mouse and cursor
	unsigned int memtotal;												// total memory size
	int j, x, y, mmx = -1, mmy = -1, mmx2 = 0;							// variables for window movement mode by mouse
	// 3. Keyboard State Variables
	int key_shift = 0, key_ctrl = 0, key_leds = (binfo->leds >> 4) & 7, keycmd_wait = -1;	// keyboard state variables
	// 4. Key Tables
	static char keytable0[0x80] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0x08, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0x0a, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0x5c, 0,  0,   0,   0,   0,   0,   0,   0,   0,   0x5c, 0,  0
	};
	static char keytable1[0x80] = {
		0,   0,   '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0x08, 0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0x0a, 0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
	};

	// Initialize System
	init_gdtidt();  														// initialize GDT and IDT
	init_pic(); 															// initialize PIC
	io_sti(); 															    // enable CPU interrupts (set IF flag)
	fifo32_init(&fifo, 128, fifobuf, 0); 									// initialize FIFO buffer
	init_pit(); 															// initialize PIT

	init_keyboard(&fifo, 256); 											    // initialize keyboard
	enable_mouse(&fifo, 512, &mdec); 										// enable mouse
	io_out8(PIC0_IMR, 0xf8); 												// allow PIT, PIC1 and keyboard(11111000)
	io_out8(PIC1_IMR, 0xef); 												// allow mouse(11101111)

	fifo32_init(&keycmd, 32, keycmd_buf, 0); 								// keyboard command FIFO buffer
	*((int *) 0x0fec) = (int) &fifo;										// store main FIFO address (0x0fec)
	fifo32_put(&keycmd, KEYCMD_LED);										// keyboard LED command
	fifo32_put(&keycmd, key_leds);											// send current LED status

    // Initialize Memory Manager
	memtotal = memtest(0x00400000, 0xbfffffff);								// test memory size
	memman_init(memman);													// initialize memory manager
	memman_free(memman, 0x00001000, 0x0009e000); 							// free1: 0x00001000 - 0x0009efff 
	memman_free(memman, 0x00400000, memtotal - 0x00400000);					// free2: 0x00400000 - (memtotal - 0x00400000)

    // Initialize Graphics Display
	init_palette();															// initialize color palette
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);  // initialize sheet control
	task_a = task_init(memman);												// initialize main task
	fifo.task = task_a;														// set main task for FIFO buffer
    task_run(task_a, 1, 2);													// run main task
	*((int *) 0x0fe4) = (int) shtctl;										// store shtctl address (0x0fe4)

    // Draw Background Sheet
	sht_back  = sheet_alloc(shtctl);														// allocate background sheet
    buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);		// allocate background buffer
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);						// set background sheet buffer
    init_screen8(buf_back, binfo->scrnx, binfo->scrny);                                     // Initialize screen for background

    // Draw Console Sheets
	key_win = open_console(shtctl, memtotal);												// open first console window

    // Draw Mouse Sheet
	sht_mouse = sheet_alloc(shtctl);
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99); 					// mouse cursor
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;
    
	// Slide Sheets to Initial Positions and Set Display Order
	sheet_slide(sht_back, 0, 0);
	sheet_slide(key_win, 32, 4);
	sheet_slide(sht_mouse, mx, my);
	
	// Set Display Order of Sheets
	sheet_updown(sht_back,     0);
	sheet_updown(key_win,      1);
	sheet_updown(sht_mouse,    2);

	// Activate Keyboard Window
	keywin_on(key_win);

	// Main Loop
	for (;;) {
		// Send keyboard LED status
		if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
			keycmd_wait = fifo32_get(&keycmd);
			wait_KBC_sendready();
			io_out8(PORT_KEYCMD, keycmd_wait);
		}
		io_cli(); // disable CPU interrupts
		// Check FIFO buffer status
		if (fifo32_status(&fifo) == 0) { // if empty
			if (new_mx >= 0) { // if mouse moved
				io_sti(); // enable CPU interrupts
				sheet_slide(sht_mouse, new_mx, new_my); // move mouse sheet
				new_mx = -1; // reset new mouse x position
			} else if (new_wx != 0x7fffffff) { // if window moved
				io_sti(); // enable CPU interrupts
				sheet_slide(sht, new_wx, new_wy); // move window sheet
				new_wx = 0x7fffffff; // reset new window x position
			} else { // nothing to do, sleep
            	task_sleep(task_a); // sleep main task
				io_sti(); // enable CPU interrupts
			}
		} else { // if not empty
			i = fifo32_get(&fifo); // get data from FIFO buffer
			io_sti(); // enable CPU interrupts
			if (key_win != 0 && key_win->flags == 0) { // if key_win is closed
				if (shtctl->top == 1) { // only background sheet remains
					key_win = 0; // no active window
				} else { // switch to next top sheet
					key_win = shtctl->sheets[shtctl->top - 1]; // next top sheet
					keywin_on(key_win); // activate it
				}
			}
			// keyboard data processing logic
			if (256 <= i && i <= 511) {
                if (i < 256+0x80) { // convert to character
					if (key_shift == 0) { // Lowercase
						s[0] = keytable0[i - 256];
					} else { // Uppercase
						s[0] = keytable1[i - 256];
					}
				} else { // special keys
					s[0] = 0;
				}
				if ('A' <= s[0] && s[0] <= 'Z') { // alphabetic characters
					if (((key_leds & 4) == 0 && key_shift == 0) ||
						((key_leds & 4) != 0 && key_shift != 0)) {
							s[0] += 0x20; // convert to lowercase
					}
				}
				if (s[0] != 0 && key_win != 0) { // printable character
                    fifo32_put(&key_win->task->fifo, s[0] + 256);
                }
				if (i == 256 + 0x0f && key_win != 0) { // tab pressed
					keywin_off(key_win);
					j = key_win->height - 1;
					if (j == 0) {
						j = shtctl->top - 1;
					}
					key_win = shtctl->sheets[j];
					keywin_on(key_win);
				}
				if (i == 256 + 0x2a) { // left shift pressed
					key_shift |= 1;
				}
				if (i == 256 + 0x36) { // right shift pressed
					key_shift |= 2;
				}
				if (i == 256 + 0xaa) { // left shift released
					key_shift &= ~1;
				}
				if (i == 256 + 0xb6) { // right shift released
					key_shift &= ~2;
				}
				if (i == 256 + 0x1d) { // Ctrl pressed
					key_ctrl |= 1;
				}
				if (i == 256 + 0x9d) { // Ctrl released
					key_ctrl &= ~1;
				}
				if (i == 256 + 0x3a) { // CapsLock pressed
					key_leds ^= 4;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x45) { // NumLock pressed
					key_leds ^= 2;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x46) { // ScrollLock pressed
					key_leds ^= 1;
					fifo32_put(&keycmd, KEYCMD_LED);
					fifo32_put(&keycmd, key_leds);
				}
				if (i == 256 + 0x57) { // F11 pressed
					sheet_updown(shtctl->sheets[1], shtctl->top - 1); // bring console to top (under the mouse)
				}
				if (i == 256 + 0xfa) { // keyboard received data
					keycmd_wait = -1;
				}
				if (i == 256 + 0xfe) { // keyboard did not received data
					wait_KBC_sendready();
					io_out8(PORT_KEYCMD, keycmd_wait);
				}
				if (i == 256 + 0x2e && key_ctrl != 0 && key_win != 0) { // Ctrl + C pressed
					task = key_win->task;
					if (task != 0 && task->tss.ss0 != 0) {
						cons_putstr0(task->cons, "\nBreak(key) :\n");
						io_cli(); // disable CPU interrupts
						task->tss.eax = (int) &(task->tss.esp0);
						task->tss.eip = (int) asm_end_app;
						io_sti(); // enable CPU interrupts
						task_run(task, -1, 0);
					}
				}
				if (i == 256 + 0x3c && key_shift != 0) { // Shift + F2 pressed
					if (key_win != 0) {
						keywin_off(key_win);
					}
					key_win = open_console(shtctl, memtotal);
					sheet_slide(key_win, 32, 4);
					sheet_updown(key_win, shtctl->top);
					keywin_on(key_win);
				}
			} else if (512 <= i && i <= 767) { // mouse data
				if (mouse_decode(&mdec, i - 512) != 0) {
                    // cursor
					mx += mdec.x;
					my += mdec.y;
					if (mx < 0) {
						mx = 0;
					}
					if (my < 0) {
						my = 0;
					}
					if (mx > binfo->scrnx - 1) {
						mx = binfo->scrnx - 1;
					}
					if (my > binfo->scrny - 1) {
						my = binfo->scrny - 1;
					}
					sheet_slide(sht_mouse, mx, my);
					new_mx = mx;
					new_my = my;
                    if ((mdec.btn & 0x01) != 0) { // left button is pressed
						if (mmx < 0) {
							// normal mode
							// check which window is on the top at (mx, my)
							for (j=shtctl->top-1; j>0; j--) {
								sht = shtctl->sheets[j];
								x = mx - sht->vx0;
								y = my - sht->vy0;
								if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
									if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
										sheet_updown(sht, shtctl->top - 1);
										if (sht != key_win) {
											keywin_off(key_win);
											key_win = sht;
											keywin_on(key_win);
										}
										if (3<=x && x<sht->bxsize-3 && 3<=y && y<21) { // click title bar
											// move mode transition
											mmx = mx;
											mmy = my;
											mmx2 = sht->vx0;
											new_wy = sht->vy0;
										}
										if (sht->bxsize-21 <= x && x < sht->bxsize-5 && 5 <= y && y < 19) {
											// close button clicked
											if ((sht->flags & 0x10) != 0) { // is a application window
												task = sht->task;
												cons_putstr0(task->cons, "\nBreak(mouse) :\n");
												io_cli(); // disable CPU interrupts
												task->tss.eax = (int) &(task->tss.esp0);
												task->tss.eip = (int) asm_end_app;
												io_sti(); // enable CPU interrupts
												task_run(task, -1, 0);
											} else { // is a console window
												task = sht->task;
												sheet_updown(sht, -1); // hide window
												keywin_off(key_win);
												key_win = shtctl->sheets[shtctl->top-1];
												keywin_on(key_win);
												io_cli(); // disable CPU interrupts
												fifo32_put(&task->fifo, 4);
												io_sti(); // enable CPU interrupts
											}
										}
										break;
									}
								}
							}
						} else {
							// move mode
							x = mx - mmx;
							y = my - mmy;
							new_wx = (mmx2 + x + 2) & ~3;
							new_wy = new_wy + y;
							mmy = my;
						}
                    } else {
						mmx = -1;
						if (new_wx != 0x7fffffff) {
							sheet_slide(sht, new_wx, new_wy);
							new_wx = 0x7fffffff;
						}
					}
				}
			} else if (768 <= i && i <= 1023) {	// console close signal
				close_console(shtctl->sheets0 + (i - 768));
			} else if (1024 <= i && i <= 2023) { // constask close signal
				close_constask(taskctl->tasks0 + (i - 1024));
			} else if (2024 <= i && i <= 2279) { // close console only
				sht2 = shtctl->sheets0 + (i - 2024);
				memman_free_4k(memman, (int) sht2->buf, 256 * 165);
				sheet_free(sht2);
			}
		}
	}
}

void keywin_off(struct SHEET *key_win)
{
	change_wtitle8(key_win, 0);
	if ((key_win->flags & 0x20) != 0) { 
		fifo32_put(&key_win->task->fifo, 3);	// cursor off 
	}
	return;
}

void keywin_on(struct SHEET *key_win)
{
	change_wtitle8(key_win, 1);
	if ((key_win->flags & 0x20) != 0) { 
		fifo32_put(&key_win->task->fifo, 2);	// cursor on 
	}
	return;
}

struct TASK *open_constask(struct SHEET *sht, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = task_alloc();
	int *cons_fifo = (int *) memman_alloc_4k(memman, 128 * 4);					// allocate console FIFO buffer
	task->cons_stack = memman_alloc_4k(memman, 64 * 1024);               		// allocate console stack
	task->tss.esp = task->cons_stack + 64 * 1024 - 12;
	task->tss.eip = (int) &console_task;
	task->tss.es = 1 * 8;
	task->tss.cs = 2 * 8;
	task->tss.ss = 1 * 8;
	task->tss.ds = 1 * 8;
	task->tss.fs = 1 * 8;
	task->tss.gs = 1 * 8;
	*((int *) (task->tss.esp + 4)) = (int) sht;									// argument: sht
	*((int *) (task->tss.esp + 8)) = memtotal;                                  // argument: memtotal
	task_run(task, 2, 2);														// run console task
	fifo32_init(&task->fifo, 128, cons_fifo, task);                 			// initialize console FIFO buffer
	return task;
}

struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SHEET *sht = sheet_alloc(shtctl);
	unsigned char *buf = (unsigned char *) memman_alloc_4k(memman, 256 * 165);	// allocate console buffer
	sheet_setbuf(sht, buf, 256, 165, -1);										// set console sheet buffer
	make_window8(buf, 256, 165, "console", 0);                                  // draw console window
	make_textbox8(sht, 8, 28, 240, 128, COL8_000000);                           // draw textbox area
	sht->task = open_constask(sht, memtotal);                                   // store task pointer in sheet
	sht->flags |= 0x20;                                                         // it has cursor

	return sht;
}

void close_constask(struct TASK *task)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	task_sleep(task);
	memman_free_4k(memman, task->cons_stack, 64 * 1024);
	memman_free_4k(memman, (int) task->fifo.buf, 128 * 4);
	task->flags = 0; // mark as unused
	return;
}

void close_console(struct SHEET *sht)
{
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct TASK *task = sht->task;
	memman_free_4k(memman, (int) sht->buf, 256 * 165);
	sheet_free(sht);
	close_constask(task);
	return;
}
