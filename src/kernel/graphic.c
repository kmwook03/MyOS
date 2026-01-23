// graphic process
#include "../include/bootpack.h"

/**
 * @brief 팔레트 초기화 함수
 * 
 * @return: void
 */
void init_palette(void)
{
    // static char -> DB intruction
    // it can use only data
    static unsigned char table_rgb[16 * 3] = {
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
    unsigned char table2[216 * 3]; // 6 * 6 * 6 = 216 colors
    int r, g, b;
    for (b=0; b<6; b++) {
        for (g=0; g<6; g++) {
            for (r=0; r<6; r++) {
                table2[(r + g *6 + b * 36) * 3 + 0] = r * 51;
                table2[(r + g *6 + b * 36) * 3 + 1] = g * 51;
                table2[(r + g *6 + b * 36) * 3 + 2] = b * 51;
            }
        }
    }
    set_palette(16, 231, table2);
    return;
}

/**
 * @brief 팔레트 설정 함수
 * 
 * @param start: 시작 색상 인덱스
 * @param end: 끝 색상 인덱스
 * @param rgb: RGB 값 배열 포인터
 * @return: void
 */
void set_palette(int start, int end, unsigned char *rgb)
{
    int i, eflags;
    eflags = io_load_eflags();  // record interrupt enable flag
    io_cli();                   // disable interrupt
    io_out8(0x03c8, start);
    for (i=start; i<=end; i++) {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    io_store_eflags(eflags);    // restore interrupt enable flag
    return;
}

/** 
 * @brief vram의 (x0, y0) 부터 (x1, y1) 까지의 사각형을 색갈 c로 채움
 * 
 * @param vram: 비디오 메모리 주소
 * @param xsize: 화면 가로 크기
 * @param c: 채울 색상
 * @param x0: 시작 x 좌표
 * @param y0: 시작 y 좌표
 * @param x1: 끝 x 좌표
 * @param y1: 끝 y 좌표
 * @return: void
 */
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
    int x, y;
    for (y=y0; y<=y1; y++) {
        for (x = x0; x <= x1; x++) {
            vram[y * xsize + x] = c;
        }
    }
    return;
}

/** 
 * @brief 화면 초기화 함수
 * 
 * @param vram: 비디오 메모리 주소
 * @param x: 화면 가로 크기
 * @param y: 화면 세로 크기
 * @return: void
 */
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

/**
 * @brief 8x16 크기의 폰트 데이터를 비디오 메모리에 출력
 * 
 * @param vram: 비디오 메모리 주소
 * @param xsize: 화면 가로 크기
 * @param x: 출력할 x 좌표
 * @param y: 출력할 y 좌표
 * @param c: 출력할 색상
 * @param font: 폰트 데이터 주소
 * @return: void
 */
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

/**
 * @brief 영문 문자열을 비디오 메모리에 출력
 * 
 * @param vram: 비디오 메모리 주소
 * @param xsize: 화면 가로 크기
 * @param x: 출력할 x 좌표
 * @param y: 출력할 y 좌표
 * @param c: 출력할 색상
 * @param *s: 출력할 문자열
 * @return: void
 */
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s)
{
    extern char hankaku[4096];
    
    for (; *s!=0x00; s++) {
        putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
        x += 8;
    }
    
    return;
}

/**
 * @brief 마우스 커서 초기화
 * 
 * @param mouse: 마우스 커서 버퍼 주소
 * @param bc: 배경 색상
 * @return: void
 */
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

    for (y=0; y<16; y++) {
        for (x=0; x<16; x++) {
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

/** 
 * @brief VRAM에 버퍼 데이터를 복사하는 함수
 * 
 * @param vram: 비디오 메모리 주소
 * @param vxsize: 화면 가로 크기
 * @param pxsize: 복사할 버퍼의 가로 크기
 * @param pysize: 복사할 버퍼의 세로 크기
 * @param px0: 복사할 버퍼의 시작 x 좌표
 * @param py0: 복사할 버퍼의 시작 y 좌표
 * @param *buf: 복사할 버퍼 주소
 * @param bxsize: 복사할 버퍼의 가로 크기
 * @return: void
 */
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
