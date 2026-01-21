#include "../include/bootpack.h"

// 한글 음소 인덱스 테이블
unsigned char HangulCode[3][32] = {
    { 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    { 0, 0, 0, 1, 2, 3, 4, 5, 0, 0, 6, 7, 8, 9, 10, 11, 0, 0, 12, 13, 14, 15, 16, 17, 0, 0, 18, 19, 20, 21, 0, 0},
    { 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 0, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 0, 0}
};
// 한글 초성 인덱스 테이블
unsigned char First[2][20] = {
    // 종성 없음
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1},
    // 종성 있음
    {0, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3}
};
// 한글 중성 인덱스 테이블
unsigned char Middle[3][22] = {
    // 중성 모양에 따른 종성 벌수
    {0, 0, 2, 0, 2, 1, 2, 1, 2, 3, 0, 2, 1, 3, 3, 1, 2, 1, 3, 3, 1, 1},
    // 종성 없을 때 중성에 따른 초성 벌수
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 3, 3, 3, 1, 2, 4, 4, 4, 2, 1, 3, 0},
    // 종성 있을 때 중성에 따른 초성 벌수
    {0, 5, 5, 5, 5, 5, 5, 5, 5, 6, 7, 7, 7, 6, 6, 7, 7, 7, 6, 6, 7, 5}
};

// 1. 문자열로부터 1바이트 읽은 후 가장 상위 비트를 검사하여 1인지 아닌지 확인한다.
// 2. 상위 비트가 0일 경우 한글이 아니기 때문에 일반 알파벳 혹은 숫자를 출력하고 1일 경우 2바이트로 구성된 한글 문자이므로 다음 1바이트를 더 읽어서 저장한다.
// 3. 저장된 2바이트의 데이터를 초성 중성 종성 각각 5비트의 크기로 나눈 후 HangulCode, First, Middle 테이블을 통해서 각 음소별 벌 수를 파악한다.
// 4. 벌수와 음소별 인덱스를 이용해서 폰트로부터 문자의 데이터를 읽어온다.
// 5. 초성, 중성, 종성별로 읽어 들인 폰트 데이터를 OR 연산을 이용해서 하나의 문자 이미지로 조합을 완성한다.
// 6. 완성된 문자 이미지를 화면에 출력한다.
// @param sht: 출력할 시트
// @param x: 출력할 x 좌표
// @param y: 출력할 y 좌표
// @param color: 출력할 색상
// @param *font: 한글 폰트 주소
// @param code: 조합형 코드 (2바이트)
// @return: void
void put_johab(struct SHEET *sht, int x, int y, char color, unsigned char *font, unsigned short code)
{
    // 이 코드는 반드시 한글 문자에 대해서만 호출되어야 한다는 가정 하에 1, 2번 과정이 생략되었음

    // code: 2바이트 조합형 코드 (MSB(1) + 초성(5) + 중성(5) + 종성(5))
    // 조합형 코드 분해
    int c1 = (code >> 8) & 0xFF; // 상위 8비트
    int c2 = code & 0xFF; // 하위 8비트

    // 비트 마스킹을 통한 음소별 값 추출
    int cho_val = (c1 & 0x7C) >> 2;                         // 초성 5비트(c1 MSB 다음부터 5비트) 남기기
    int jung_val = ((c1 & 0x03) << 3) | ((c2 & 0xE0) >> 5); // 중성 5비트(c1 하위 2비트, c2 상위 3비트) 남기기
    int jong_val = c2 & 0x1F;                               // 종성 5비트(c2 하위 5비트) 남기기

    int cho_idx = HangulCode[0][cho_val];
    int jung_idx = HangulCode[1][jung_val];
    int jong_idx = HangulCode[2][jong_val];

    int jong_exist = (jong_idx != 0) ? 1 : 0; // 종성 존재 여부 (인덱스 0 -> 없음, 인덱스 !0 -> 있음)
    
    // 초성 모양 결정
    // 중성 모음이 가로냐 세로냐에 따라 모양이 변하기 때문에 Middle 배열을 참조하여 결정
    // Row: 1(받침 X), 2(받침 O) / Col: 중성 인덱스
    int cho_type = Middle[1+jong_exist][jung_idx];
    
    // 중성 모양 결정
    // 초성의 크기나 위치에 따라 모양이 변하기 때문에 First 배열을 참조하여 결정
    // Row: 0(받침 X), 1(받침 O) / Col: 초성 인덱스
    int jung_type = First[jong_exist][cho_idx];

    // 종성 모양 결정
    // 중성 모양(길게 내려오냐 아니냐)에 따라 모양이 변하기 때문에 Middle 배열을 참조하여 결정
    // Row: 0(종성용) / Col: 중성 인덱스
    int jong_type = Middle[0][jung_idx];

    // 폰트 데이터는 1 글자당 32바이트
    unsigned char *p_cho = font + (cho_type * 20 + cho_idx) * 32;
    // 160 <- 초성 영역이므로 건너뜀
    unsigned char *p_jung = font + (160 + jung_type * 22 + jung_idx) * 32;
    // 248 <- 초성 + 중성 영역이므로 건너뜀
    unsigned char *p_jong = font + (248 + jong_type * 28 + jong_idx) * 32;

    // 그리기 (16x16)
    int i, b;
    for (i=0; i<16; i++) {
        // 초성 데이터 | 중성 데이터
        // i*2: 왼쪽 8픽셀
        // i*2+1: 오른쪽 8픽셀
        unsigned char line1 = p_cho[i*2] | p_jung[i*2];
        unsigned char line2 = p_cho[i*2+1] | p_jung[i*2+1];
        
        // 받침 있으면 종성 데이터도 OR 연산
        if (jong_exist) {
            line1 |= p_jong[i*2];
            line2 |= p_jong[i*2+1];
        }
        
        // VRAM 주소 계산
        // 화면 메모리 주소: y좌표 * 화면 너비 + x좌표
        unsigned char *p = sht->buf + (y + i) * sht->bxsize + x;

        // 픽셀 찍기
        for (b=0; b<8; b++) {
            if (line1 & (0x80 >> b)) p[b] = color;      // 왼쪽 8픽셀
            if (line2 & (0x80 >> b)) p[b+8] = color;    // 오른쪽 8픽셀
        }
    }
    return;
}

// 유니코드 -> 조합형 코드 변환 테이블
static unsigned char U2J_cho[19] = {
    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20
};
static unsigned char U2J_jung[21] = {
    3, 4, 5, 6, 7,             // ㅏ, ㅐ, ㅑ, ㅒ, ㅓ
    10, 11, 12, 13, 14, 15,    // ㅔ, ㅕ, ㅖ, ㅗ, ㅘ, ㅙ
    18, 19, 20, 21, 22, 23,    // ㅚ, ㅛ, ㅜ, ㅝ, ㅞ, ㅟ
    26, 27, 28, 29             // ㅠ, ㅡ, ㅢ, ㅣ
};
static unsigned char U2J_jong[28] = {
    0, // 0: 받침 없음
    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 19, // ㄱ(2)~ㅂ(19) -> 값 2~19 (18 빠짐)
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29                      // ㅄ(18)~ㅎ(27) -> 값 20~29; (아직 구현 안됨)
};

// 초성 매핑 테이블 (ASCII 코드 -> 유니코드 인덱스)
// @param c: 입력된 키 값 (ASCII 코드)
// @return: 초성 인덱스 (0~18), 없으면 -1
int key2cho(char c) {
    switch(c) {
        case 'r': return 0;  // ㄱ
        case 'R': return 1;  // ㄲ
        case 's': return 2;  // ㄴ
        case 'e': return 3;  // ㄷ
        case 'E': return 4;  // ㄸ
        case 'f': return 5;  // ㄹ
        case 'a': return 6;  // ㅁ
        case 'q': return 7;  // ㅂ
        case 'Q': return 8;  // ㅃ
        case 't': return 9;  // ㅅ
        case 'T': return 10; // ㅆ
        case 'd': return 11; // ㅇ
        case 'w': return 12; // ㅈ
        case 'W': return 13; // ㅉ
        case 'c': return 14; // ㅊ
        case 'z': return 15; // ㅋ
        case 'x': return 16; // ㅌ
        case 'v': return 17; // ㅍ
        case 'g': return 18; // ㅎ
    }
    return -1;
}

// 중성 매핑 테이블 (ASCII 코드 -> 유니코드 인덱스)
// @param c: 입력된 키 값 (ASCII 코드)
// @return: 중성 인덱스 (0~20), 없으면 -1
int key2jung(char c) {
    switch(c) {
        case 'k': return 0;  // ㅏ
        case 'o': return 1;  // ㅐ
        case 'i': return 2;  // ㅑ
        case 'O': return 3;  // ㅒ
        case 'j': return 4;  // ㅓ
        case 'p': return 5;  // ㅔ
        case 'u': return 6;  // ㅕ
        case 'P': return 7;  // ㅖ
        case 'h': return 8;  // ㅗ
        case 'y': return 12; // ㅛ
        case 'n': return 13; // ㅜ
        case 'b': return 17; // ㅠ
        case 'm': return 18; // ㅡ
        case 'l': return 20; // ㅣ
    }
    return -1;
}

// 종성 매핑 테이블 (ASCII 코드 -> 유니코드 인덱스)
// @param c: 입력된 키 값 (ASCII 코드)
// @return: 종성 인덱스 (0~27), 없으면 -1
int key2jong(char c) {
    switch(c) {
        case 'r': return 1;  // ㄱ
        case 'R': return 2;  // ㄲ
        case 's': return 4;  // ㄴ
        case 'e': return 7;  // ㄷ
        case 'f': return 8;  // ㄹ
        case 'a': return 16; // ㅁ
        case 'q': return 17; // ㅂ
        case 't': return 19; // ㅅ
        case 'T': return 20; // ㅆ
        case 'd': return 21; // ㅇ
        case 'w': return 22; // ㅈ
        case 'c': return 23; // ㅊ
        case 'z': return 24; // ㅋ
        case 'x': return 25; // ㅌ
        case 'v': return 26; // ㅍ
        case 'g': return 27; // ㅎ
    }
    return -1; // 종성 없음
}

// 유니코드(UTF-8) -> 조합형 코드 변환
// 단지 개발 편의를 위한 함수.. sprintf(s, "한글한글"); 등을 가능하게 하기 위함.
// @param s: UTF-8 인코딩된 문자열 포인터 (3바이트 필요)
// @return: 조합형 코드 (2바이트, 최상위 비트 1), 한글이 아니면 0 반환
unsigned short utf8_to_johab(unsigned char *s)
{
    // UTF-8 포맷: 1110xxxx 10xxxxxx 10xxxxxx
    unsigned int unicode = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F);

    // 한글 영역인가? (가:0xAC00 ~ 힣:0xD7A3)
    if (unicode < 0xAC00 || unicode > 0xD7A3) return 0; // 한글 아님

    // 유니코드 인덱스 분해
    // 인덱스 = (초성 * 588) + (중성 * 28) + 종성
    unicode -= 0xAC00; // 0 부터 시작

    int cho = unicode / 588;
    unicode %= 588;
    int jung = unicode / 28;
    int jong = unicode % 28;

    // 조합형 코드 조립
    unsigned short johab = 0x8000; // 최상위 비트 1 설정

    // 테이블을 통해 비트값 구하기
    johab |= (U2J_cho[cho] & 0x1F) << 10; // 초성
    johab |= (U2J_jung[jung] & 0x1F) << 5; // 중성
    johab |= (U2J_jong[jong] & 0x1F); // 종성
    
    return johab;
}

// 유니코드 -> johab 변환 후 한글 출력 함수
// 단지 개발 편의를 위한 함수.. sprintf(s, "한글한글"); 등을 가능하게 하기 위함.
// @param sht: 출력할 시트
// @param x: 출력할 x 좌표
// @param y: 출력할 y 좌표
// @param color: 출력할 색상
// @param s: 출력할 문자열 (UTF-8 인코딩)
// @return: void
void putstr_utf8(struct SHEET *sht, int x, int y, char color, unsigned char *s)
{
    unsigned char *korean = (unsigned char *) *((int *) 0x0fe8); // 한글 폰트 주소
    char s_temp[2] = {0, 0}; // 문자 하나만 담는 임시 버퍼
    while (*s != 0x00) {
        if ((*s & 0x80) == 0) {
            // 1바이트 ASCII 문자
            s_temp[0] = *s;
            s_temp[1] = 0x00;
            putfonts8_asc(sht->buf, sht->bxsize, x, y, color, s_temp);
            x += 8;
            s++;
        } else if ((*s & 0xE0) == 0xE0) {
            // 3바이트 UTF-8 문자
            // UTF-8 -> 조합형 코드 변환
            if (s[1] == 0x00 || s[2] == 0x00) break; // 잘못된 문자열 처리

            unsigned short johab = utf8_to_johab(s);
            if (johab != 0) {
                put_johab(sht, x, y, color, korean, johab);
            }
            x += 16;
            s += 3; // 3바이트 문자이므로 포인터 3 증가
        } else {
            s++;
        }
    }
    sheet_refresh(sht, sht->vx0, sht->vy0, x, y + 16);
    return;
}

// 한글 오토마타 구현부

// 유니코드 값을 UTF-8로 변환하여 dest에 저장
// @param val: 유니코드 값
// @param dest: 변환된 UTF-8 문자열을 저장할 버퍼 (최대 4바이트 필요)
// @return: void
void unicode_to_utf8(unsigned short val, char *dest)
{
    if (val < 0x80) {
        dest[0] = val;
        dest[1] = 0;
        dest[2] = 0;
        dest[3] = 0;
    } else if (val < 0x0800) {
        dest[0] = 0xc0 | (val >> 6);
        dest[1] = 0x80 | (val & 0x3f);
        dest[2] = 0;
        dest[3] = 0;
    } else {
        dest[0] = 0xe0 | (val >> 12);
        dest[1] = 0x80 | ((val >> 6) & 0x3f);
        dest[2] = 0x80 | (val & 0x3f);
        dest[3] = 0;
    }
    return;
}

// 종성 인덱스를 초성 인덱스로 변환하는 테이블
static int jong2cho[] = {
    -1, 0, 1, -1, 2, -1, -1, 3, 5, -1, -1, -1, -1, -1, -1, -1, 
    6, 7, -1, 9, 10, 11, 12, 14, 15, 16, 17, 18
};

// 조합 중인 글자 그리기 함수
// @param cons: 콘솔 구조체 포인터
// @param x: 출력할 x 좌표
// @param y: 출력할 y 좌표
// @param cho: 초성 인덱스
// @param jung: 중성 인덱스
// @param jong: 종성 인덱스
// @return: void
void draw_composing_char(struct CONSOLE *cons, int x, int y, int cho, int jung, int jong)
{
    unsigned char *korean = (unsigned char *) *((int *) 0x0fe8); // 한글 폰트 주소
    unsigned short johab = 0x8000; // 최상위 비트 1 설정
    
    if (cho != -1)johab |= (U2J_cho[cho] & 0x1F) << 10; // 초성
    if (jung != -1) johab |= (U2J_jung[jung] & 0x1F) << 5; // 중성
    if (jong != -1) johab |= (U2J_jong[jong] & 0x1F); // 종성

    boxfill8(cons->sht->buf, cons->sht->bxsize, COL8_000000, x, y, x+15, y+15); // 배경 지우기
    put_johab(cons->sht, x, y, COL8_FFFFFF, korean, johab);
    sheet_refresh(cons->sht, x, y, x+16, y+16);
    return;
}

// 중성 합성 함수
// 지금 조합이 합성 가능한 중성인지 확인(ㅢ, ㅘ, ㅚ, ㅙ, ㅟ, ㅝ, ㅞ)
// @param jung1: 첫 번째 중성 인덱스
// @param jung2: 두 번째 중성 인덱스
// @return: 합성된 중성 인덱스, 불가능하면 -1
int get_composite_jung(int jung1, int jung2)
{
    // 'ㅗ'(8) 계열
    if (jung1 == 8) {
        if (jung2 == 0) return 9;       // ㅗ + ㅏ = ㅘ
        if (jung2 == 1) return 10;      // ㅗ + ㅐ = ㅙ
        if (jung2 == 20) return 11;     // ㅗ + ㅣ = ㅚ
    }

    // 'ㅜ'(13) 계열
    if (jung1 == 13) {
        if (jung2 == 4) return 14;      // ㅜ + ㅓ = ㅝ
        if (jung2 == 5) return 15;      // ㅜ + ㅔ = ㅞ
        if (jung2 == 20) return 16;     // ㅜ + ㅣ = ㅟ
    }

    // 'ㅡ'(18) 계열
    if (jung1 == 18) {
        if (jung2 == 20) return 19;     // ㅡ + ㅣ = ㅢ
    }

    return -1; // 합성 불가
}

// 중성 분해 함수
// 합성된 중성을 분해하여 원래의 중성 인덱스 반환
// @param jung: 합성된 중성 인덱스
// @return: 분해된 중성 인덱스 (첫 번째 중성 인덱스)
int split_composite_jung(int complex_jung)
{
    // 'ㅗ'로 복귀
    if (complex_jung == 9 || complex_jung == 10 || complex_jung == 11) return 8;
    // 'ㅡ'로 복귀
    if (complex_jung == 14 || complex_jung == 15 || complex_jung == 16) return 13;
    // 'ㅡ'로 복귀
    if (complex_jung == 19) return 18;

    return -1; // 분해 불가
}

// 한글 오토마타 함수
// @param cons: 콘솔 구조체 포인터
// @param task: 현재 작업 구조체 포인터
// @param key: 입력된 키 값 (ASCII 코드)
// @return: void
void hangul_automata(struct CONSOLE *cons, struct TASK *task, int key)
{
    char s[2];
    int idx_cho, idx_jung, idx_jong;

    idx_cho = key2cho(key);     // 초성 인덱스
    idx_jung = key2jung(key);   // 중성 인덱스
    idx_jong = key2jong(key);   // 종성 인덱스

    // 상태 전이
    switch(task->hangul_state) {
        // state 0: 아무 것도 입력되지 않은 상태
        case 0:
            if (idx_cho != -1) { // 초성이 입력 되어있다면?
                // 초성 입력 -> 조합 시작
                task->hangul_state = 1;         // 상태 1로 전이
                task->hangul_idx[0] = idx_cho;  // 초성 인덱스 저장
                task->hangul_idx[1] = -1;       // 중성 인덱스 초기화 (아직 입력 안됨)
                task->hangul_idx[2] = -1;       // 종성 인덱스 초기화 (아직 입력 안됨)
                draw_composing_char(cons, cons->cur_x, cons->cur_y, idx_cho, -1, -1);
                cons->cur_x += 16; // 커서 전진
            } else if (idx_jung != -1) { // 초성은 없는데 중성이 입력되었다면?
                // 모음 단독 입력 -> 바로 출력
                s[0] = key;
                s[1] = 0;
                cons_putstr0(cons, s);
            } else {
                // 한글 아님 -> 바로 출력
                s[0] = key;
                s[1] = 0;
                cons_putstr0(cons, s);
            }
            break;

        // state 1: 초성만 입력된 상태
        case 1:
            if (idx_jung != -1) { // 중성이 입력되었다면?
                // 중성 입력 -> 다음 상태로 전이
                task->hangul_state = 2;             // 상태 2로 전이
                task->hangul_idx[1] = idx_jung;     // 중성 인덱스 저장
                task->hangul_idx[2] = -1;           // 종성 인덱스 초기화 (아직 입력 안됨)
                draw_composing_char(cons, cons->cur_x - 16, cons->cur_y, task->hangul_idx[0], idx_jung, -1); // 조합 중인 글자 그리기
            } else if (idx_cho != -1) { // 또 다른 초성이 입력되었다면?
                // 초성 입력 -> 앞 글자 확정 후 새 글자 시작
                task->hangul_state = 1;             // 상태 1로 유지
                task->hangul_idx[0] = idx_cho;      // 새로운 초성 인덱스 저장
                task->hangul_idx[1] = -1;           // 중성 인덱스 초기화 (아직 입력 안됨)
                task->hangul_idx[2] = -1;           // 종성 인덱스 초기화 (아직 입력 안됨)

                draw_composing_char(cons, cons->cur_x, cons->cur_y, idx_cho, -1, -1);
                cons->cur_x += 16; // 커서 전진
            } else {
                // 한글 아님 -> 앞 글자 확정 후 새 글자 시작
                task->hangul_state = 0;
                s[0] = key;
                s[1] = 0;
                cons_putstr0(cons, s);
            }
            break;

        // state 2: 초성+중성 입력된 상태
        case 2:
            if (idx_jong != -1) {
                // 종성 입력 -> 글자 완성 + 다음 상태로 전이
                task->hangul_state = 3;
                task->hangul_idx[2] = idx_jong;
                draw_composing_char(cons, cons->cur_x - 16, cons->cur_y, task->hangul_idx[0], task->hangul_idx[1], idx_jong);
            } else if (idx_jung != -1) {
                // 모음 입력 -> 앞 글자 확정 후 새 글자 시작
                int complex = get_composite_jung(task->hangul_idx[1], idx_jung);

                if (complex != -1) {
                    task->hangul_idx[1] = complex;
                    draw_composing_char(cons, cons->cur_x - 16, cons->cur_y, task->hangul_idx[0], task->hangul_idx[1], -1);
                } else {
                    task->hangul_state = 0;
                    hangul_automata(cons, task, key);
                }
            } else {
                // 한글 아님 -> 앞 글자 확정 후 새 글자 시작
                task->hangul_state = 0;
                s[0] = key;
                s[1] = 0;
                cons_putstr0(cons, s);
            }
            break;
        // state 3: 초성+중성+종성 입력된 상태
        case 3:
            if (idx_jung != -1) {
                // 종성 분리 (예: 각ㅏ -> 가가)
                int prev_cho = task->hangul_idx[0];
                int prev_jung = task->hangul_idx[1];
                int prev_jong = task->hangul_idx[2];

                draw_composing_char(cons, cons->cur_x - 16, cons->cur_y, prev_cho, prev_jung, -1); // 앞 글자 다시 그리기

                int next_cho = jong2cho[prev_jong]; // 종성->초성 변환
                if (next_cho != -1) {
                    task->hangul_state = 2;
                    task->hangul_idx[0] = next_cho;
                    task->hangul_idx[1] = idx_jung;
                    task->hangul_idx[2] = -1;
                    draw_composing_char(cons, cons->cur_x, cons->cur_y, next_cho, idx_jung, -1);
                    cons->cur_x += 16; // 커서 전진
                } else {
                    task->hangul_state = 0;
                    hangul_automata(cons, task, key);
                }
            } else if (idx_cho != -1) {
                // 자음 입력 -> 앞 글자 확정 후 새 글자 시작
                task->hangul_state = 1;
                task->hangul_idx[0] = idx_cho;
                task->hangul_idx[1] = -1;
                task->hangul_idx[2] = -1;
                draw_composing_char(cons, cons->cur_x, cons->cur_y, idx_cho, -1, -1);
                cons->cur_x += 16; // 커서 전진
            } else {
                // 한글 아님 -> 앞 글자 확정 후 새 글자 시작
                task->hangul_state = 0;
                s[0] = key;
                s[1] = 0;
                cons_putstr0(cons, s);
            }
            break;
    }
}

// 백스페이스 처리용 함수
// 현재 경우에 따라 지워도 커서가 뒤에 남는 버그가 있음..
// @param cons: 콘솔 구조체 포인터
// @param task: 현재 작업 구조체 포인터
// @return: 처리 여부 (1: 처리됨, 0: 처리 안됨)
int hangul_automata_delete(struct CONSOLE *cons, struct TASK *task)
{
    // 조합 중인 문자가 없으면 처리 안함 -> console_task에서 처리
    if (task->hangul_state == 0) return 0; // 처리 안됨

    int prev_jung; // 이전 중성 인덱스 저장용
    
    switch(task->hangul_state) {
        case 1: // 초성 삭제
            task->hangul_state = 0;
            task->hangul_idx[0] = -1;
            break;
        case 2: // 중성 삭제
            prev_jung = split_composite_jung(task->hangul_idx[1]);  // 중성 분해 시도
            if (prev_jung != -1) {  // 분해 가능하면 이전 형태로 되돌림
                task->hangul_idx[1] = prev_jung;
            } else {    // 분해 불가능하면 삭제
                task->hangul_state = 1;
                task->hangul_idx[1] = -1;
            }
            break;
        case 3: // 종성 삭제
            task->hangul_state = 2;
            task->hangul_idx[2] = -1;
            break;
    }

    if (task->hangul_state == 0) {                                                  // 백스페이스 결과 조합 중인 문자가 없으면 지우기
        draw_composing_char(cons, cons->cur_x - 16, cons->cur_y, -1, -1, -1);
        cons_putchar(&cons, ' ', 0); // 공백 문자로 덮어쓰기
        cons->cur_x -= 16;
    } else {                                                                        // 여전히 조합 중인 문자가 있으면 다시 그리기
        draw_composing_char(cons, cons->cur_x - 16, cons->cur_y, task->hangul_idx[0], task->hangul_idx[1], task->hangul_idx[2]);
    }

    return 1; // 처리됨
}
