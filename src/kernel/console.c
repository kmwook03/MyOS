// console

#include "../include/bootpack.h"
#include "../include/utf8.h"
#include "../include/hangul.h"
#include <stdio.h>
#include <string.h>

void cons_debug(struct CONSOLE *cons, char *cmdline);

/** 
 * @brief 콘솔 태스크 함수
 * 
 * 키보드 입력 및 명령어 처리 담당
 *
 *  콘솔 시트 업데이트 및 커서 제어 포함
 * 
 * @param sht: 콘솔 시트 포인터
 * @param memtotal: 총 메모리 크기
 * @param langmode: 언어 모드 (0: 영어, 1: 한글)
 * 
 * @return: void
 */
void console_task(struct SHEET *sht, int memtotal, int langmode)
{
    struct TASK *task = task_now();                                     // 현재 태스크 포인터 얻기
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;              // 메모리 관리자 포인터
    int i, *fat = (int *) memman_alloc_4k(memman, 4 * 2880);            // FAT 테이블용 메모리 할당
	struct CONSOLE cons;                                                // 콘솔 구조체
    struct FILEHANDLE fhandle[8];                                       // 파일 핸들 구조체 배열
    char cmdline[128];                                                  // 명령어 입력 버퍼

    // 콘솔 구조체 초기화
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
    file_readfat(fat, (unsigned char *) (ADR_DISKIMG + 0x000200));      // FAT 테이블 읽기
    for (i=0; i<8; i++) {
        fhandle[i].buf = 0; // 미사용
    }
    task->fhandle = fhandle;        // 파일 핸들 배열 설정
    task->fat = fat;                // FAT 테이블 설정

    task->langmode = langmode;       // 언어모드 설정
    set_hangul(task, 0, -1, -1, -1); // 한글 오토마타 초기화
    cons_put_utf8(&cons, ">", 1, 1);     // 프롬프트 출력        
    cons.cmd_pos = 0;

    // 메인 루프
    for (;;) {
        io_cli(); // 인터럽트 금지
        if (fifo32_status(&task->fifo) == 0) {          // FIFO 버퍼가 비어있으면
            task_sleep(task);                           //    태스크 슬립
            io_sti();                                   //    인터럽트 허용
        } else {                                        // FIFO 버퍼에 데이터가 있으면
            i = fifo32_get(&task->fifo);                //    데이터 처리
            io_sti();                                   //    인터럽트 허용    
            if (i <= 1 && cons.sht != 0) { // 커서 깜빡임 처리
                if (i != 0) {
                    timer_init(cons.timer, &task->fifo, 0); // 커서 ON
					if (cons.cur_c >= 0) {
                    	cons.cur_c = COL8_FFFFFF;
					}
                } else {
                    timer_init(cons.timer, &task->fifo, 1); // 커서 OFF
					if (cons.cur_c >= 0) {
                    	cons.cur_c = COL8_000000;
					}
                }
                timer_settime(cons.timer, 50); // 0.5초 간격
            }
			if (i == 2) { // 커서 켜기
				cons.cur_c = COL8_FFFFFF;
			}
			if (i == 3) { // 커서 끄기
                if (cons.sht != 0) {
				    boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000, cons.cur_x, cons.cur_y, cons.cur_x + cons.cur_width - 1, cons.cur_y + 15);
                }
				cons.cur_c = -1;
			}
            if (i == 4) {
                cmd_exit(&cons, fat); // 콘솔 종료
            }
			if (256 <= i && i <= 511) {
				if (i == 8 + 256) {                                         // backspace: 지우기
                    if (cons.cur_x > 16) {
                        cons_put_utf8(&cons, " ", 1, 0);                    // 커서 지우기
                                            // 조합 중인 한글 삭제 시도
                        if (hangul_automata_delete(&cons, task) == 1) { 
                            // 한글 오토마타가 처리함
                            continue;
                        }

                        unsigned char last_char = (unsigned char)cmdline[cons.cmd_pos - 1];
                        if (last_char < 0x80) {     // ASCII
                            cons.cur_x -= 8;
                            cons.cmd_pos--;
                        } else {                    // UTF-8 한글
                            cons.cur_x -= 16;
                            cons.cmd_pos -= 3;
                            boxfill8(cons.sht->buf, cons.sht->bxsize, COL8_000000, cons.cur_x, cons.cur_y, cons.cur_x + 15, cons.cur_y + 15);
                            sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + 16, cons.cur_y + 16);
                        }
                    }
				} else if (i == 10 + 256) {                     // enter: 줄바꿈
                    if (task->hangul.state != 0) {
                        flush_hangul_to_cmdline(&cons, task, cmdline); // 조합 중인 한글 확정
                        set_hangul(task, 0, -1, -1, -1);                    // 한글 오토마타 초기화
                    }
                    cons_put_utf8(&cons, " ", 1, 0);                // 커서 지우기
					cmdline[cons.cmd_pos] = 0;            // 명령어 라인 종료 문자
                    cons.cmd_pos = 0;

                    cons_debug(&cons, cmdline);    // 디버그용
                    
                    cons_newline(&cons);                        // 줄바꿈
                    cons_runcmd(cmdline, &cons, fat, memtotal); // 명령어 실행
                    if (cons.sht == 0) {                        // 콘솔 시트가 없으면
                        cmd_exit(&cons, fat);                   // 콘솔 태스크 종료
                    }
                    cons_put_utf8(&cons, ">", 1, 1);                // 프롬프트 출력
				} else if (i == 256 + 0x3b) {	// F1 눌림
                    // 언어 모드 변경
                    task->langmode ^= 1;
                    flush_hangul_to_cmdline(&cons, task, cmdline); // 조합 중인 한글 확정
                    set_hangul(task, 0, -1, -1, -1);    // 한글 오토마타 초기화
                } else {
                    // 일반 문자 입출력
                    int key = i - 256;                      // 입력된 키 값 (ASCII 코드)
                    if (task->langmode == 1) {                  // 한글 모드
                        if (cons.cur_x < 8 + CONSOLE_TBOX_WIDTH) {
                            hangul_automata(&cons, task, key, cmdline);      // 한글 오토마타가 처리
                        }
                    } else {                                        // 영어 모드
					    if (cons.cur_x < 8 + CONSOLE_TBOX_WIDTH) {
						    cmdline[cons.cmd_pos] = key;  // 명령어 라인에 문자 저장
                            cons.cmd_pos++;
						    cons_put_utf8(&cons, (char *)&key, 1, 1);        // 문자 출력
					    }
                    }
				}
			}
            // 커서 초기화
            if (cons.sht != 0) {
			    if (cons.cur_c >= 0) {
				    boxfill8(cons.sht->buf, cons.sht->bxsize, cons.cur_c, cons.cur_x, cons.cur_y, cons.cur_x + cons.cur_width - 1, cons.cur_y + 15);
			    }
                sheet_refresh(cons.sht, cons.cur_x, cons.cur_y, cons.cur_x + cons.cur_width, cons.cur_y + 16);
        
            }    
        }
    }
}

void cons_putchar(struct CONSOLE *cons, int chr, char move)
{
    char s[2];
    s[0] = chr;
    s[1] = 0;
    if (s[0] == 0x09) { // 탭 문자 처리
        for (;;) {
            if (cons->sht != 0) {
                putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);   // 공백 출력
            }
            cons->cur_x += 8;                                                                               // 커서 이동
            if (cons->cur_x == 8 + 240) {
                cons_newline(cons);                                                                         // 줄바꿈
            }
            if (((cons->cur_x - 8) & 0x1f) == 0) {                                                          // 탭 간격(32픽셀) 도달했으면 break;
                break;
            }
        }
    } else if (s[0] == 0x0a) { // 줄바꿈
        cons_newline(cons);
    } else if (s[0] == 0x0d) { // 뭐 없음
        // do nothing
    } else {
        if (cons->sht != 0) {
            putfonts8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, 1);        // 문자 출력
        }
        if (move != 0) { // 커서 이동
            cons->cur_x += 8;
            if (cons->cur_x == 8 + 240) {
                cons_newline(cons); // 줄바꿈
            }
        }
    }
    return;
}

/**
 * @brief 콘솔에 문자 출력
 * 
 * @param cons: 콘솔 구조체 포인터
 * @param chr: 출력할 문자
 * @param move: 커서 이동 여부 (0: 이동 안함, 1: 이동함)
 * @return: void
 */
void cons_put_utf8(struct CONSOLE *cons, char *s, int len, char move)
{
    if (len == 1) {
        if (s[0] == 0x09) { // 탭 문자 처리
            for (;;) {
                if (cons->sht != 0) {
                    putfonts_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, " ", 1);        // 공백 출력
                }
                cons->cur_x += 8;                                                                               // 커서 이동
                if (cons->cur_x == 8 + CONSOLE_TBOX_WIDTH) {                                                    
                    cons_newline(cons);                                                                         // 줄바꿈
                }
                if (((cons->cur_x - 8) & 0x1f) == 0) {                                                          // 탭 간격(32픽셀) 도달했으면 break;
                    break;
                }
            }
            return;
        } else if (s[0] == 0x0a) { // 줄바꿈
            cons_newline(cons);
            return;
        } else if (s[0] == 0x0d) { // 뭐 없음
            // do nothing
            return;
        }
    }
    if (cons->sht != 0) {
        putfonts_sht(cons->sht, cons->cur_x, cons->cur_y, COL8_FFFFFF, COL8_000000, s, len);               // 문자 출력
    }
    if (move != 0) { // 커서 이동
            int width = (len == 1) ? 8 : 16;
            cons->cur_x += width;
            if (cons->cur_x >= 8 + CONSOLE_TBOX_WIDTH) {
                cons_newline(cons); // 줄바꿈
            }
    }
    return;
}

void cons_putstr(struct CONSOLE *cons, char *s)
{
    int len;
    unsigned int unicode;
    while (*s != 0x00) {
        unicode = utf8_to_unicode(s, &len);
        if (len == 0) break; // 변환 실패 시 종료
        cons_put_utf8(cons, s, len, 1);
        s += len;
    }
    return;
}

/** 
 * @brief 콘솔에 문자열 포인터 사용하여 출력
 *
 * @param cons: 콘솔 구조체 포인터
 * @param s: 출력할 문자열 포인터
 * @return: void
*/
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
                    put_johab(cons->sht->buf, cons->sht->bxsize, cons->cur_x, cons->cur_y, COL8_FFFFFF, korean, johab);
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

/**
 * @brief 콘솔에 문자열 포인터와 길이 사용하여 출력
 *
 *  @param cons: 콘솔 구조체 포인터
 * @param s: 출력할 문자열 포인터
 * @param l: 출력할 문자열 길이
 * @return: void
 */
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
                    put_johab(cons->sht->buf, cons->sht->bxsize, cons->cur_x, cons->cur_y, COL8_FFFFFF, korean, johab);
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

/**
 * @brief 콘솔 줄바꿈 처리 함수
 * 
 * @param cons: 콘솔 구조체 포인터
 * @return: void
 */
void cons_newline(struct CONSOLE *cons)
{
	int x, y;
    struct SHEET *sht = cons->sht;
	if (cons->cur_y < 28 + CONSOLE_TBOX_HEIGHT - 16) {
		cons->cur_y += 16;
	} else {
        // 스크롤
        if (sht != 0) {
		    for (y=28; y<28+CONSOLE_TBOX_HEIGHT-16; y++) {
			    for (x=8; x<8+CONSOLE_TBOX_WIDTH; x++) {
				    sht->buf[x + y * sht->bxsize] = sht->buf[x + (y + 16) * sht->bxsize];
			    }
		    }
		
		    for (y=28+CONSOLE_TBOX_HEIGHT-16; y<28+CONSOLE_TBOX_HEIGHT; y++) {
			    for (x=8; x<8+CONSOLE_TBOX_WIDTH; x++) {
				    sht->buf[x + y * sht->bxsize] = COL8_000000;
			    }
		    }
		    sheet_refresh(sht, 8, 28, 8 + CONSOLE_TBOX_WIDTH, 28 + CONSOLE_TBOX_HEIGHT);
        }
    }
    cons->cur_x = 8;
	return;
}

/**
 * @brief 콘솔 명령어 실행 함수
 * 
 * @param cmdline: 명령어 문자열
 * @param cons: 콘솔 구조체 포인터
 * @param fat: FAT 테이블 포인터
 * @param memtotal: 총 메모리 크기
 * @return: void 
 */
void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal)
{
    if ((strcmp(cmdline, "mem") == 0 || strcmp(cmdline, "메모리") == 0) && cons->sht != 0) {        // 가능
        cmd_mem(cons, memtotal);
    } else if ((strcmp(cmdline, "cls") == 0 || strcmp(cmdline, "clear") == 0 || strcmp(cmdline, "지우기") == 0) && cons->sht != 0) {    // 가능
        cmd_cls(cons);
    } else if ((strcmp(cmdline, "dir") == 0 || strcmp(cmdline, "ls") == 0 || strcmp(cmdline, "목록") == 0) && cons->sht != 0) {   // 가능
        cmd_dir(cons);
    } else if ((strcmp(cmdline, "exit") == 0 || strcmp(cmdline, "종료") == 0)) {
        cmd_exit(cons, fat);
    } else if (strncmp(cmdline, "start ", 6) == 0 || strncmp(cmdline, "실행 ", 3) == 0) {
        cmd_start(cons, cmdline, memtotal, cons->sht->task->langmode);
    } else if (strncmp(cmdline, "ncst ", 5) == 0) {
        cmd_ncst(cons, cmdline, memtotal, cons->sht->task->langmode);
    } else if (strncmp(cmdline, "langmode ", 9) == 0) {
        cmd_langmode(cons, cmdline);
    } else if (cmdline[0] != 0) {			
        if (cmd_app(cons, fat, cmdline) == 0) {			
            cons_putstr(cons, "Bad command.\n\n");
        }   
    }

    return;
}

/**
 * @brief mem command (메모리 사용량 표시)
 *
 * @param cons: 콘솔 구조체 포인터
 * @param memtotal: 총 메모리 크기
 * @return: void 
 */
void cmd_mem(struct CONSOLE *cons, int memtotal)
{
    // mem command
    struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
    char s[128];
	sprintf(s, "총 %dMB\n빈 공간 %dKB\n\n", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	cons_putstr(cons, s);
    return;
}

/**
 * @brief cls/clear command (콘솔 화면 지우기)
 * 
 * @param cons: 콘솔 구조체 포인터
 * @return: void
 */
void cmd_cls(struct CONSOLE *cons)
{
    // cls/clear command
    struct SHEET *sht = cons->sht;
    int x, y;
    for (y=28; y<28+CONSOLE_TBOX_HEIGHT; y++) {
        for (x=8; x<8+CONSOLE_TBOX_WIDTH; x++) {
            sht->buf[x + y * sht->bxsize] = COL8_000000;
        }
    }
    sheet_refresh(sht, 8, 28, 8 + CONSOLE_TBOX_WIDTH, 28 + CONSOLE_TBOX_HEIGHT);
    cons->cur_y = 28;
    return;
}

/**
 * @brief dir/ls command (디렉토리 목록 표시)
 * @param cons: 콘솔 구조체 포인터
 * @return: void
 */
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
				cons_putstr(cons, s);
			}
		}
	}
	cons_newline(cons);
    return;
}

/**
 * @brief exit command (콘솔 종료 및 태스크 종료)
 * 
 * @param cons: 콘솔 구조체 포인터
 * @param fat: FAT 테이블 포인터
 * @return: void
 */
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

/**
 * @brief start command (새 콘솔 생성 후 앱 실행)
 * 
 * @param cons: 콘솔 구조체 포인터
 * @param cmdline: 명령어 문자열
 * @param memtotal: 총 메모리 크기
 * @return: void
 */
void cmd_start(struct CONSOLE *cons, char *cmdline, int memtotal, int langmode)
{
	struct SHTCTL *shtctl = (struct SHTCTL *) *((int *) 0x0fe4);
	struct SHEET *sht = open_console(shtctl, memtotal, langmode);
	struct FIFO32 *fifo = &sht->task->fifo;
	int i = 0;
	sheet_slide(sht, 32, 4);
	sheet_updown(sht, shtctl->top);

    while (cmdline[i] != ' ') {
        i++;
    }
    i++; // skip space
	for (; cmdline[i] != 0; i++) {
        fifo32_put(fifo, cmdline[i] + 256);
    }
    fifo32_put(fifo, 10 + 256);	// Enter
	return;
}

/**
 * @brief ncst command (새 콘솔에서 명령어 실행)
 *
 * @param cons: 콘솔 구조체 포인터
 * @param cmdline: 명령어 문자열
 * @param memtotal: 총 메모리 크기
 * @return: void
 */
void cmd_ncst(struct CONSOLE *cons, char *cmdline, int memtotal, int langmode)
{
	struct TASK *task = open_constask(0, memtotal, langmode);
	struct FIFO32 *fifo = &task->fifo;
	int i;
	for (i = 5; cmdline[i] != 0; i++) {
		fifo32_put(fifo, cmdline[i] + 256);
	}
	fifo32_put(fifo, 10 + 256);	// Enter
	cons_newline(cons);
	return;
}

/**
 * @brief langmode command (언어 모드 설정)
 * 
 * @param cons: 콘솔 구조체 포인터
 * @param cmdline: 명령어 문자열
 * @return: void
 */
void cmd_langmode(struct CONSOLE *cons, char *cmdline)
{
    struct TASK *task = task_now();
    unsigned char mode = cmdline[9] - '0';
    int i;
    if (mode <= 1) {
        task->langmode = mode;
        if (mode == 0) {
            cons_putstr(cons, "[English]\n");
        } else {
            cons_putstr(cons, "[Korean]\n");
        }
    } else {
        cons_putstr(cons, "langmode command error. (0: English, 1: Korean)\n");
    }
    cons_newline(cons);
    return;
}

/**
 * @brief 애플리케이션 실행 명령어 처리 함수
 *
 * @param cons: 콘솔 구조체 포인터
 * @param fat: FAT 테이블 포인터
 * @param cmdline: 명령어 문자열
 * @return: 성공 시 1, 실패 시 0
 */
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
            cons_putstr(cons, ".hrb file format error.\n");
        }
        memman_free_4k(memman, (int) p, appsiz);
        cons_newline(cons);
        return 1;
    }
    
    return 0;
}

/**
 * @brief HRB API 함수
 * 
 * 레지스터를 통해 호출되며 다양한 시스템 기능 제공
 * 
 * @param: edi, esi, ebp, esp, ebx, edx, ecx, eax
 * @return: eax 레지스터 값 포인터
 */
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
        cons_putstr(cons, (char *) ebx + ds_base);
    } else if (edx == 3) {
        int len = ecx;
        cons_put_utf8(cons, (char *) ebx + ds_base, len, 1);
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
        putfonts(sht->buf, sht->bxsize, esi, edi, eax, (char *) ebp + ds_base);
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
    } else if (edx == 27) {
        reg[7] = task->langmode;
    }

    return 0;
}

/** @brief HRB API 선 그리기 함수
 *
 * @param sht: 그릴 시트 포인터
 * @param x0, y0: 시작 좌표
 * @param x1, y1: 끝 좌표
 * @param col: 선 색상
 * @return: void
 */
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

/** 
 * @brief INT 0C 핸들러 (스택 예외)
 * 
 * @param esp: 스택 포인터
 * @return: 새로운 스택 포인터
 */
int *inthandler0c(int *esp) {
    struct TASK *task = task_now();
    struct CONSOLE *cons = task->cons;
    char s[30];
    cons_putstr(cons, "\nINT 0C : \n Stack Exception.\n");
    sprintf(s, "EIP = %08X\n", esp[11]);
    cons_putstr(cons, s);
    return &(task->tss.esp0); // ABEND
}

/** 
 * @brief INT 0D 핸들러 (일반 보호 예외)
 *
 * @param esp: 스택 포인터
 * @return: 새로운 스택 포인터 
 */
int *inthandler0d(int *esp)
{
    struct TASK *task = task_now();
    struct CONSOLE *cons = task->cons;
    char s[30];
    cons_putstr(cons, "\nINT 0D : General Protected Exception.\n");
    sprintf(s, "EIP = %08X\n", esp[11]);
    cons_putstr(cons, s);
    return &(task->tss.esp0); // ABEND
}

void cons_debug(struct CONSOLE *cons, char *cmdline)
{
    // --- 디버깅 용 ---
    cons_newline(cons);
    cons_putstr(cons, "Debug info:\n");
    unsigned char *c = (unsigned char *)cmdline;
    int i;
    for (i=0; cmdline[i] != 0; i++) {
        char s[40];
        sprintf(s, "%02X ", c[i]);
        cons_putstr(cons, s);
    }
	/// ----------------
}
