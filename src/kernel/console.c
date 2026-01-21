// console

#include "../include/bootpack.h"
#include <stdio.h>
#include <string.h>

// 콘솔 태스크 함수
// 키보드 입력 및 명령어 처리 담당
// 콘솔 시트 업데이트 및 커서 제어 포함
// @param sht: 콘솔 시트 포인터
// @param memtotal: 총 메모리 크기
// @return: void
void console_task(struct SHEET *sht, int memtotal)
{
    struct TASK *task = task_now();
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    int i, *fat = (int *) memman_alloc_4k(memman, 4 * 2880);
	struct CONSOLE cons;
    struct FILEHANDLE fhandle[8];
    char cmdline[30];

    cons.sht = sht;
    cons.cur_x = 8;
    cons.cur_y = 28;
    cons.cur_c = -1;
    task->cmdline = cmdline;
    cons.cur_width = 8;
    task->cons = &cons;

    if (cons.sht != 0) {
        cons.timer = timer_alloc();
        timer_init(cons.timer, &task->fifo, 1);
        timer_settime(cons.timer, 50);
    }
    file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));
    for (i=0; i<8; i++) {
        fhandle[i].buf = 0; // mark as unused
    }
    task->fhandle = fhandle;
    task->fat = fat;

    task->langmode = 0;

    cons_putchar(&cons, '>', 1);

    for (;;) {
        io_cli();
        if (fifo32_status(&task->fifo) == 0) {
            task_sleep(task);
            io_sti();
        } else {
            i = fifo32_get(&task->fifo);
            io_sti();
            if (i <= 1 && cons.sht != 0) { // 커서 깜빡임 처리
                if (i != 0) {
                    timer_init(cons.timer, &task->fifo, 0);
					if (cons.cur_c >= 0) {
                    	cons.cur_c = COL8_FFFFFF;
					}
                } else {
                    timer_init(cons.timer, &task->fifo, 1);
					if (cons.cur_c >= 0) {
                    	cons.cur_c = COL8_000000;
					}
                }
                timer_settime(cons.timer, 50);
            }
			if (i == 2) {
				cons.cur_c = COL8_FFFFFF;
			}
			if (i == 3) {
                if (cons.sht != 0) {
				    boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000, cons.cur_x, cons.cur_y, cons.cur_x + cons.cur_width - 1, cons.cur_y + 15);
                }
				cons.cur_c = -1;
			}
            if (i == 4) {
                cmd_exit(&cons, fat);
            }
			if (256 <= i && i <= 511) {
				if (i == 8 + 256) {                         // backspace
					if (cons.cur_x > 16) {
						cons_putchar(&cons, ' ', 0);
						cons.cur_x -= 8;
                        if (task->langmode == 1) {
                            cons.cur_x -= 8;
                        }
					}
				} else if (i == 10 + 256) {                  // enter
                    cons_putchar(&cons, ' ', 0);
					cmdline[cons.cur_x / 8 - 2] = 0;
					cons_newline(&cons);
                    cons_runcmd(cmdline, &cons, fat, memtotal); // execute command
                    if (cons.sht == 0) {
                        cmd_exit(&cons, fat);
                    }
                    cons_putchar(&cons, '>', 1);
				} else {
                    // normal character
                    if (task->langmode == 1) {
                        int key = i - 256;
                        if (cons.cur_x < 240) {
                            hangul_automata(&cons, task, key);
                        }
                    } else {
					    if (cons.cur_x < 240) {
						    cmdline[cons.cur_x / 8 - 2] = i - 256;
						    cons_putchar(&cons, i - 256, 1);
					    }
                    }
				}
			}
            if (cons.sht != 0) {
			    if (cons.cur_c >= 0) {
				    boxfill8(cons.sht->buf, cons.sht->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + cons.cur_width - 1, cons.cur_y + 15);
			    }
                sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + cons.cur_width, cons.cur_y + 16);
        
            }    
        }
    }
}

// 콘솔에 문자 출력
// @param cons: 콘솔 구조체 포인터
// @param chr: 출력할 문자
// @param move: 커서 이동 여부 (0: 이동 안함, 1: 이동함)
// @return: void
void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
    char s[2];
    s[0] = chr;
    s[1] = 0;
    if (s[0] == 0x09) {
        for (;;) {
            if (cons->sht != 0) {
                putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);
            }
            cons->cur_x += 8;
            if (cons->cur_x == 8 + 240) {
                cons_newline(cons);
            }
            if (((cons->cur_x - 8) & 0x1f) == 0) {
                break;
            }
        }
    } else if (s[0] == 0x0a) {
        cons_newline(cons);
    } else if (s[0] == 0x0d) {
        // do nothing
    } else {
        if (cons->sht != 0) {
            putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);
        }
        if (move != 0) {
            cons->cur_x += 8;
            if (cons->cur_x == 8 + 240) {
                cons_newline(cons);
            }
        }
    }
    return;
}

// 콘솔에 문자열 포인터 사용하여 출력
// @param cons: 콘솔 구조체 포인터
// @param s: 출력할 문자열 포인터
// @return: void
void cons_putstr0 (struct CONSOLE *cons, char *s)
{
    unsigned char *korean = (unsigned char *) *((int *) 0x0fe8);
    unsigned short johab;
    int code;

    for (; *s!=0; s++) {
        if ((*s & 0x80) == 0) { // ASCII
            cons_putchar(cons, *s, 1);
        } else if ((*s & 0xe0) == 0xe0) {
            if (s[1] == 0 || s[2] == 0) break; // 문자열 끝 예외처리
            johab = utf8_to_johab((unsigned char *) s);
            if (johab != 0) {
                // 한글 출력
                if (cons->sht != 0) {
                    put_johab(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, korean, johab);
                    sheet_refresh(cons->sht, cons->cur_x, cons->cur_y, cons->cur_x + 16, cons->cur_y + 16);
                }
                cons->cur_x += 16;

                if (cons->cur_x + 16 > cons->sht->bxsize) {
                    cons_newline(cons);
                }
            }
            s += 2; // 3바이트 문자이므로 포인터 2 증가
        }
    }
    return;
}

// 콘솔에 문자열 포인터와 길이 사용하여 출력
// @param cons: 콘솔 구조체 포인터
// @param s: 출력할 문자열 포인터
// @param l: 출력할 문자열 길이
// @return: void
void cons_putstr1(struct CONSOLE *cons, char *s, int l)
{
    unsigned char *korean = (unsigned char *) *((int *) 0x0fe8);
    unsigned short johab;
    int i;

    for (i=0; i<l; i++) {
        if ((s[i] & 0x80) == 0) { // ASCII
            cons_putchar(cons, s[i], 1);
        } else if ((s[i] & 0xe0) == 0xe0) {
            if (s[i+1] == 0 || s[i+2] == 0) break; // 문자열 끝 예외처리
            johab = utf8_to_johab((unsigned char *) &s[i]);
            if (johab != 0) {
                // 한글 출력
                if (cons->sht != 0) {
                    put_johab(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, korean, johab);
                    sheet_refresh(cons->sht, cons->cur_x, cons->cur_y, cons->cur_x + 16, cons->cur_y + 16);
                }
                cons->cur_x += 16;

                if (cons->cur_x + 16 > cons->sht->bxsize) {
                    cons_newline(cons);
                }
            }
            i += 2; // 3바이트 문자이므로 인덱스 2 증가
        }
    }
    return;
}

// 콘솔 줄바꿈 처리 함수
// @param cons: 콘솔 구조체 포인터
// @return: void
void cons_newline(struct CONSOLE *cons)
{
	int x, y;
    struct SHEET *sht = cons->sht;
	if (cons->cur_y < 28 + 112) {
		cons->cur_y += 16;
	} else {
        // scroll
        if (sht != 0) {
		    for (y=28; y<28+112; y++) {
			    for (x=8; x<8+240; x++) {
				    sht->buf[x + y * sht->bxsize] = sht->buf[x + (y + 16) * sht->bxsize];
			    }
		    }
		
		    for (y=28+112; y<28+128; y++) {
			    for (x=8; x<8+240; x++) {
				    sht->buf[x + y * sht->bxsize] = COL8_000000;
			    }
		    }
		    sheet_refresh(sht, 8, 28, 8 + 240, 28 + 128);
        }
    }
    cons->cur_x = 8;
	return;
}

// 콘솔 명령어 실행 함수
// @param cmdline: 명령어 문자열
// @param cons: 콘솔 구조체 포인터
// @param fat: FAT 테이블 포인터
// @param memtotal: 총 메모리 크기
// @return: void
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal)
{
    if (strcmp(cmdline, "mem") == 0 && cons->sht != 0) {
        cmd_mem(cons, memtotal);
    } else if ((strcmp(cmdline, "cls") == 0 || strcmp(cmdline, "clear") == 0) && cons->sht != 0) {
        cmd_cls(cons);
    } else if ((strcmp(cmdline, "dir") == 0 || strcmp(cmdline, "ls") == 0) && cons->sht != 0) {
        cmd_dir(cons);
    } else if (strcmp(cmdline, "exit") == 0) {
        cmd_exit(cons, fat);
    } else if (strncmp(cmdline, "start ", 6) == 0) {
        cmd_start(cons, cmdline, memtotal);
    } else if (strncmp(cmdline, "ncst ", 5) == 0) {
        cmd_ncst(cons, cmdline, memtotal);
    } else if (strncmp(cmdline, "langmode ", 9) == 0) {
        cmd_langmode(cons, cmdline);
    } else if (cmdline[0] != 0) {			
        if (cmd_app(cons, fat, cmdline) == 0) {			
            cons_putstr0(cons, "Bad command.\n\n");
        }   
    }
    return;
}

// mem command (메모리 사용량 표시)
// @param cons: 콘솔 구조체 포인터
// @param memtotal: 총 메모리 크기
// @return: void
void cmd_mem(struct CONSOLE *cons, int memtotal)
{
    // mem command
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    char s[60];
	sprintf(s, "total %dMB\nfree %dKB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	cons_putstr0(cons, s);
    return;
}

// cls/clear command (콘솔 화면 지우기)
// @param cons: 콘솔 구조체 포인터
// @return: void
void cmd_cls(struct CONSOLE *cons)
{
    // cls/clear command
    struct SHEET *sht = cons->sht;
    int x, y;
    for (y=28; y<28+128; y++) {
        for (x=8; x<8+240; x++) {
            sht->buf[x + y * sht->bxsize] = COL8_000000;
        }
    }
    sheet_refresh(sht, 8, 28, 8 + 240, 28 + 128);
    cons->cur_y = 28;
    return;
}

// dir/ls command (디렉토리 목록 표시)
// @param cons: 콘솔 구조체 포인터
// @return: void
void cmd_dir(struct CONSOLE *cons)
{
    // dir/ls command
    struct FILEINFO *finfo = (struct FILEINFO *) (ADR_DISKIMG + 0x002600);
    int i, j;
    char s[30];
    for (i=0; i<224; i++) {
		if (finfo[i].name[0] == 0x00) {
			break;
		}
		if (finfo[i].name[0] != 0xe5) {
			if ((finfo[i].type & 0x18) == 0) {
				sprintf(s, "filename.ext   %7d\n", finfo[i].size);
				for (j=0; j<8; j++) {
			    	s[j] = finfo[i].name[j];
				}
				s[9] = finfo[i].ext[0];
				s[10] = finfo[i].ext[1];
				s[11] = finfo[i].ext[2];
				cons_putstr0(cons, s);
			}
		}
	}
	cons_newline(cons);
    return;
}

// exit command (콘솔 종료 및 태스크 종료)
// @param cons: 콘솔 구조체 포인터
// @param fat: FAT 테이블 포인터
// @return: void
void cmd_exit(struct CONSOLE *cons, int *fat)
{
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct TASK *task = task_now();
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
    struct FIFO32 *fifo = (struct FIFO32 *) *((int *) 0x0fec);
    if (cons->sht != 0) {
        timer_cancel(cons->timer);
    }
    memman_free_4k(memman, (int) fat, 4 * 2880);
    io_cli(); // disable CPU interrupts
    if (cons->sht != 0) {
        fifo32_put(fifo, cons->sht - shtctl->sheets0 + 768);    // 768 - 1023
    } else {
        fifo32_put(fifo, task - taskctl->tasks0 + 1024);          // 1024 - 2023
    }
    io_sti();
    for (;;) {
        task_sleep(task);
    }
}

// start command (새 콘솔 생성 후 앱 실행)
// @param cons: 콘솔 구조체 포인터
// @param cmdline: 명령어 문자열
// @param memtotal: 총 메모리 크기
// @return: void
void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal)
{
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht = open_console(shtctl, memtotal);
	struct FIFO32 *fifo = &sht->task->fifo;
	int i;
	sheet_slide(sht, 32, 4);
	sheet_updown(sht, shtctl->top);
	for (i = 6; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	// Enter
	cons_newline(cons);
	return;
}

// ncst command (새 콘솔 생성 후 명령어 실행)
// @param cons: 콘솔 구조체 포인터
// @param cmdline: 명령어 문자열
// @param memtotal: 총 메모리 크기
// @return: void
void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal)
{
	struct TASK *task = open_constask(0, memtotal);
	struct FIFO32 *fifo = &task->fifo;
	int i;
	for (i = 5; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	// Enter
	cons_newline(cons);
	return;
}

// langmode command (언어 모드 설정)
// @param cons: 콘솔 구조체 포인터
// @param cmdline: 명령어 문자열
// @return: void
void cmd_langmode(struct CONSOLE *cons, char *cmdline)
{
    struct TASK *task = task_now();
    unsigned char mode = cmdline[9] - '0';
    int i;
    if (mode <= 1) {
        task->langmode = mode;
        task->hangul_state = 0;
        for(i=0; i<3; i++) task->hangul_idx[i] = -1;
        if (mode == 0) {
            cons_putstr0(cons, "[English]\n");
        } else {
            cons_putstr0(cons, "[Korean]\n");
        }
    } else {
        cons_putstr0(cons, "langmode command error. (0: English, 1: Korean)\n");
    }
    cons_newline(cons);
    return;
}

// 앱 실행 함수
// @param cons: 콘솔 구조체 포인터
// @param fat: FAT 테이블 포인터
// @param cmdline: 명령어 문자열
// @return: 성공 시 1, 실패 시 0
int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline)
{
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    struct FILEINFO *finfo;
    char name[18], *p, *q;
    struct TASK *task = task_now();
    int segsiz, datsiz, esp, dathrb, appsiz; // metadata of .hrb file
    struct SHTCTL *shtctl;
    struct SHEET *sht;
    int i;
    
    for (i=0; i<13; i++) {
        if (cmdline[i] <= ' ') {
            break;
        }
        name[i] = cmdline[i];
    }
    name[i] = 0; // null-terminate

    finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    if (finfo == 0 && name[i-1] != '.') {
        // search with .HRB extension
        name[i] = '.';
        name[i+1] = 'H';
        name[i+2] = 'R';
        name[i+3] = 'B';
        name[i+4] = 0;
        finfo = file_search(name, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
    }

    if (finfo != 0) {
        appsiz = finfo->size;
        p = file_loadfile2(finfo->clustno, &appsiz, fat);
        if (appsiz >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
            segsiz = *((int *) (p + 0x0000));
            esp    = *((int *) (p + 0x000c));
            datsiz = *((int *) (p + 0x0010));
            dathrb = *((int *) (p + 0x0014));
            q = (char *) memman_alloc_4k(memman, segsiz);
            task->ds_base = (int) q;
            set_segmdesc(task->ldt + 0, finfo->size - 1, (int) p, AR_CODE32_ER + 0x60);
            set_segmdesc(task->ldt + 1, segsiz - 1,      (int) q, AR_DATA32_RW + 0x60);
            for (i=0; i<datsiz; i++) {
                q[esp + i] = p[dathrb + i];
            }
            start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));
            shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
            for (i=0; i<MAX_SHEETS; i++) {
                sht = &(shtctl->sheets0[i]);
                if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
                    sheet_free(sht);
                }
            }
            for (i=0; i<8; i++) {
                if (task->fhandle[i].buf != 0) {
                    memman_free_4k(memman, (int) task->fhandle[i].buf, task->fhandle[i].size);
                    task->fhandle[i].buf = 0;
                }
            }
            timer_cancelall(&task->fifo);
            memman_free_4k(memman, (int) q, segsiz);
        } else {
            cons_putstr0(cons, ".hrb file format error.\n");
        }
        memman_free_4k(memman, (int) p, appsiz);
        cons_newline(cons);
        return 1;
    }
    
    return 0;
}

// 하리보테 API 함수
// @param edi, esi, ebp, esp, ebx, edx, ecx, eax: 레지스터 값
// @return: eax 레지스터 값 포인터
int *hrb_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax)
{
    struct TASK *task = task_now();
    int ds_base = task->ds_base;
    struct CONSOLE *cons = task->cons;
    struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
    struct SHEET *sht;
    struct FIFO32 *sys_fifo = (struct FIFO32 *) *((int *) 0x0fec);
    int *reg = &eax + 1; // next address of eax
    // reg[0] : edi, reg[1] : esi, reg[2] : ebp, reg[3] : esp
    // reg[4] : ebx, reg[5] : edx, reg[6] : ecx, reg[7] : eax
    int i;
    struct FILEINFO *finfo;
    struct FILEHANDLE *fh;
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;

    if (edx == 1) {
        cons_putchar(cons, eax & 0xff, 1);
    } else if (edx == 2) {
        cons_putstr0(cons, (char *) ebx + ds_base);
    } else if (edx == 3) {
        cons_putstr1(cons, (char *) ebx + ds_base, ecx);
    } else if (edx == 4) {
        return &(task->tss.esp0);
    } else if (edx == 5) {
        sht = sheet_alloc(shtctl);
        sht->task = task;
        sht->flags |= 0x10;
        sheet_setbuf(sht, (char *) ebx + ds_base, esi, edi, eax);
        make_window8((char *) ebx + ds_base, esi, edi, (char *) ecx + ds_base, 0);
        sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2); // center and 4 pixel align (for optimization)
        sheet_updown(sht, shtctl->top);
        reg[7] = (int) sht;
    } else if (edx == 6) {
        sht = (struct SHEET *) (ebx & 0xfffffffe);
        putfonts8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *) ebp + ds_base);
        if ((ebx & 1) == 0) {
            sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
        }
    } else if (edx == 7) {
        sht = (struct SHEET *) (ebx & 0xfffffffe);
        boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
        if ((ebx & 1) == 0) {
            sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
        }
    } else if (edx == 8) {
        memman_init((struct MEMMAN *) (ebx + ds_base));
        ecx &= 0xfffffff0; // 16 byte align
        memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
    } else if (edx == 9) {
        ecx = (ecx + 0x0f) & 0xfffffff0; // 16 byte align
        reg[7] = memman_alloc((struct MEMMAN *) (ebx + ds_base), ecx);
    } else if (edx == 10) {
        ecx = (ecx + 0x0f) & 0xfffffff0; // 16 byte align
        memman_free((struct MEMMAN *) (ebx + ds_base), eax, ecx);
    } else if (edx == 11) {
        sht = (struct SHEET *) (ebx & 0xfffffffe);
        sht->buf[sht->bxsize * edi + esi] = eax;
        if ((ebx & 1) == 0) {
            sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
        }
    } else if (edx == 12) {
        sht = (struct SHEET *) ebx;
        sheet_refresh(sht, eax, ecx, esi, edi);
    } else if (edx == 13) {
        sht = (struct SHEET *) (ebx & 0xfffffffe);
        hrb_api_linewin(sht, eax, ecx, esi, edi, ebp);
        if ((ebx & 1) == 0) {
            if (eax > esi) {
                i = eax;
                eax = esi;
                esi = i;
            }
            if (ecx > edi) {
                i = ecx;
                ecx = edi;
                edi = i;
            }
            sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
        }
    } else if (edx == 14) {
        sheet_free((struct SHEET *) ebx);
    } else if (edx == 15) {
        for (;;) {
            io_cli();
            if (fifo32_status(&task->fifo) == 0) {
                if (eax != 0) {
                    task_sleep(task);
                } else {
                    io_sti();
                    reg[7] = -1;
                    return 0;
                }
            }
            i = fifo32_get(&task->fifo);
            io_sti();
            if (i <= 1 && cons->sht != 0) {
                timer_init(cons->timer, &task->fifo, 1);
                timer_settime(cons->timer, 50);
            }
            if (i == 2) {
                cons->cur_c = COL8_FFFFFF;
            }
            if (i == 3) {
                cons->cur_c = -1;
            }
            if (i == 4) {
                timer_cancel(cons->timer);
                io_cli();
                fifo32_put(sys_fifo, cons->sht - shtctl->sheets0 + 2024); // 2024 - 2279
                cons->sht = 0;
                io_sti();
            }
            if (i >= 256) {
                reg[7] = i - 256;
                return 0;
            }
        }
    } else if (edx == 16) {
        reg[7] = (int) timer_alloc();
        ((struct TIMER *) reg[7])->flags2 = 1;
    } else if (edx == 17) {
        timer_init((struct TIMER *) ebx, &task->fifo, eax + 256);
    } else if (edx == 18) {
        timer_settime((struct TIMER *) ebx, eax);
    } else if (edx == 19) {
        timer_free((struct TIMER *) ebx);
    } else if (edx == 20) {
        if (eax == 0) {
            i = io_in8(0x61);
            io_out8(0x61, i & 0x0d); // stop speaker
        } else {
            i = 1193180000 / eax;
            io_out8(0x43, 0xb6);
            io_out8(0x42, i & 0xff);
            io_out8(0x42, i >> 8);
            i = io_in8(0x61);
            io_out8(0x61, (i | 0x03) & 0x0f);
        }
    } else if (edx == 21) {
        for (i=0; i<8; i++) {
            if (task->fhandle[i].buf == 0) { break; }
        }
        fh = &task->fhandle[i];
        reg[7] = 0;
        if (i < 8) {
            finfo = file_search((char *) ebx + ds_base, (struct FILEINFO *) (ADR_DISKIMG + 0x002600), 224);
            if (finfo != 0) {
                reg[7] = (int) fh;
                fh->size = finfo->size;
                fh->pos = 0;
                fh->buf = file_loadfile2(finfo->clustno, &fh->size, task->fat);
            }
        }
    } else if (edx == 22) {
        fh = (struct FILEHANDLE *) eax;
        memman_free_4k(memman, (int) fh->buf, fh->size);
        fh->buf = 0;
    } else if (edx == 23) {
        fh = (struct FILEHANDLE *) eax;
        if (ecx == 0) {
            fh->pos = ebx;
        } else if (ecx == 1) {
            fh->pos += ebx;
        } else if (ecx == 2) {
            fh->pos = fh->size + ebx;
        }

        if (fh->pos < 0) {
            fh->pos = 0;
        }
        if (fh->pos > fh->size) {
            fh->pos = fh->size;
        }
    } else if (edx == 24) {
        fh = (struct FILEHANDLE *) eax;
        if (ecx == 0) {
            reg[7] = fh->size;
        } else if (ecx == 1) {
            reg[7] = fh->pos;
        } else if (ecx == 2) {
            reg[7] = fh->pos - fh->size;
        }
    } else if (edx == 25) {
        fh = (struct FILEHANDLE *) eax;
        for (i=0; i<ecx; i++) {
            if (fh->pos == fh->size) {
                break;
            }
            *((char *) ebx + ds_base + i) = fh->buf[fh->pos];
            fh->pos++;
        }
        reg[7] = i;
    } else if (edx == 26) {
        i = 0;
        for (;;) {
            *((char *) ebx + ds_base + i) = task->cmdline[i];
            if (task->cmdline[i] == 0) {
                break;
            }
            if (i >= ecx) {
                break;
            }
            i++;
        }
        reg[7] = i;
    }

    return 0;
}

// 하리보테 API의 선 그리기 함수
// @param sht: 그릴 시트 포인터
// @param x0, y0: 시작 좌표
// @param x1, y1: 끝 좌표
// @param col: 선 색상
// @return: void
void hrb_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col)
{
    int i, x, y, len, dx, dy;

    dx = x1 - x0;
    dy = y1 - y0;
    x = x0 << 10;
    y = y0 << 10;
    if (dx < 0) { dx = -dx; }
    if (dy < 0) { dy = -dy; }
    if (dx >= dy) {
        len = dx + 1;
        if (x0 > x1) {
            dx = -1024;
        } else {
            dx = 1024;
        }
        if (y0 <= y1) {
            dy = ((y1 - y0 + 1) << 10) / len;
        } else {
            dy = ((y1 - y0 - 1) << 10) / len;
        }
    } else {
        len = dy + 1;
        if (y0 > y1) {
            dy = -1024;
        } else {
            dy = 1024;
        }
        if (x0 <= x1) {
            dx = ((x1 - x0 + 1) << 10) / len;
        } else {
            dx = ((x1 - x0 - 1) << 10) / len;
        }
    }
    for (i=0; i<len; i++) {
        sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
        x += dx;
        y += dy;
    }

    return;
}

// INT 0C 핸들러 (스택 예외)
// @param esp: 스택 포인터
// @return: 새로운 스택 포인터
int *inthandler0c(int *esp) {
    struct TASK *task = task_now();
    struct CONSOLE *cons = task->cons;
    char s[30];
    cons_putstr0(cons, "\nINT 0C : \n Stack Exception.\n");
    sprintf(s, "EIP = %08X\n", esp[11]);
    cons_putstr0(cons, s);
    return &(task->tss.esp0); // ABEND
}

// INT 0D 핸들러 (일반 보호 예외)
// @param esp: 스택 포인터
// @return: 새로운 스택 포인터 
int *inthandler0d(int *esp)
{
    struct TASK *task = task_now();
    struct CONSOLE *cons = task->cons;
    char s[30];
    cons_putstr0(cons, "\nINT 0D : General Protected Exception.\n");
    sprintf(s, "EIP = %08X\n", esp[11]);
    cons_putstr0(cons, s);
    return &(task->tss.esp0); // ABEND
}
