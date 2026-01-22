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

/** 
 * @brief 한글 조합형 코드 출력 함수
 * 
 * 1. 문자열로부터 1바이트 읽은 후 가장 상위 비트를 검사하여 1인지 아닌지 확인한다.
 * 
 * 2. 상위 비트가 0일 경우 한글이 아니기 때문에 일반 알파벳 혹은 숫자를 출력하고 1일 경우 2바이트로 구성된 한글 문자이므로 다음 1바이트를 더 읽어서 저장한다.
 * 
 * 3. 저장된 2바이트의 데이터를 초성 중성 종성 각각 5비트의 크기로 나눈 후 HangulCode, First, Middle 테이블을 통해서 각 음소별 벌 수를 파악한다.
 * 
 * 4. 벌수와 음소별 인덱스를 이용해서 폰트로부터 문자의 데이터를 읽어온다.
 * 
 * 5. 초성, 중성, 종성별로 읽어 들인 폰트 데이터를 OR 연산을 이용해서 하나의 문자 이미지로 조합을 완성한다.
 * 
 * 6. 완성된 문자 이미지를 화면에 출력한다.
 * 
 * @param sht: 출력할 시트
 * @param x: 출력할 x 좌표
 * @param y: 출력할 y 좌표
 * @param color: 출력할 색상
 * @param *font: 한글 폰트 주소
 * @param code: 조합형 코드 (2바이트)
 * @return: void
 */
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

/**
 * @brief 유니코드->초성조합코드 매핑 테이블
 * 
 * 주의: 테이블의 인덱스와 해당 인덱스에 저장된 조합형 코드 값을 혼동하면 안됨
 */
static unsigned char U2J_cho[19] = {
    2, 3, 4, 5, 6,          // 0:ㄱ, 1:ㄲ, 2:ㄴ, 3:ㄷ, 4:ㄸ
    7, 8, 9, 10, 11,        // 5:ㄹ, 6:ㅁ, 7:ㅂ, 8:ㅃ, 9:ㅅ
    12, 13, 14, 15, 16,     // 10:ㅆ, 11:ㅇ, 12:ㅈ, 13:ㅉ, 14:ㅊ
    17, 18, 19, 20          // 15:ㅋ, 16:ㅌ, 17:ㅍ, 18:ㅎ
};
/**
 * @brief 유니코드->중성조합코드 매핑 테이블
 * 
 * 주의: 테이블의 인덱스와 해당 인덱스에 저장된 조합형 코드 값을 혼동하면 안됨
 */
static unsigned char U2J_jung[21] = {
    3, 4, 5, 6, 7,             // 0:ㅏ, 1:ㅐ, 2:ㅑ, 3:ㅒ, 4:ㅓ
    10, 11, 12, 13, 14, 15,    // 5:ㅔ, 6:ㅕ, 7:ㅖ, 8:ㅗ, 9:ㅘ, 10:ㅙ
    18, 19, 20, 21, 22, 23,    // 11:ㅚ, 12:ㅛ, 13:ㅜ, 14:ㅝ, 15:ㅞ, 16:ㅟ
    26, 27, 28, 29             // 17:ㅠ, 18:ㅡ, 19:ㅢ, 20:ㅣ
};
/**
 * @brief 유니코드->종성조합코드 매핑 테이블
 * 
 * 주의: 테이블의 인덱스와 해당 인덱스에 저장된 조합형 코드 값을 혼동하면 안됨
 */
static unsigned char U2J_jong[28] = {
    0, // 0: 받침 없음
    2, 3, 4, 5, 6, 7,                       // 0:ㄱ, 1:ㄲ, 2:ㄳ, 3:ㄴ, 4:ㄵ, 5:ㄶ, 6:ㄷ
    8, 9, 10, 11, 12, 13, 14, 15, 16,       // 7:ㄹ, 8:ㄺ, 9:ㄻ, 10:ㄼ, 11:ㄽ, 12:ㄾ, 13:ㄿ, 14:ㅀ
    17, 19, 20, 21, 22,                     // 15:ㅁ, 16:ㅂ, 17:ㅄ, 18:ㅅ, 19:ㅆ 
    23, 24, 25, 26, 27, 28, 29              // 20:ㅇ, 21:ㅈ, 22:ㅊ, 23:ㅋ, 24:ㅌ, 25:ㅍ, 26:ㅎ
};

static char KeyToChoIdx[128] = {
    [0 ... 127] = -1,                                               // 기본값: 매핑 없음
    ['r'] = 0, ['R'] = 1,  ['s'] = 2,  ['e'] = 3,  ['E'] = 4,       // ㄱ, ㄲ, ㄴ, ㄷ, ㄸ
    ['f'] = 5,  ['a'] = 6,  ['q'] = 7,  ['Q'] = 8,  ['t'] = 9,      // ㄹ, ㅁ, ㅂ, ㅃ, ㅅ
    ['T'] = 10, ['d'] = 11, ['w'] = 12, ['W'] = 13, ['c'] = 14,     // ㅆ, ㅇ, ㅈ, ㅉ, ㅊ
    ['z'] = 15, ['x'] = 16, ['v'] = 17, ['g'] = 18                  // ㅋ, ㅌ, ㅍ, ㅎ
};

static char KeyToJungIdx[128] = {
    [0 ... 127] = -1,                                   // 기본값: 매핑 없음
    ['k'] = 0,  ['o'] = 1,  ['i'] = 2,  ['O'] = 3,      // ㅏ, ㅐ, ㅑ, ㅒ
    ['j'] = 4,  ['p'] = 5,  ['u'] = 6,  ['P'] = 7,      // ㅓ, ㅔ, ㅕ, ㅖ
    ['h'] = 8,                          ['y'] = 12,     // ㅗ,       ㅛ
    ['n'] = 13,                         ['b'] = 17,     // ㅜ,       ㅠ
    ['m'] = 18,                         ['l'] = 20      // ㅡ,       ㅣ
};

static char KeyToJongIdx[128] = {
    [0 ... 127] = -1,                                               // 기본값: 매핑 없음
    ['r'] = 1,  ['R'] = 2,  ['s'] = 4,  ['e'] = 7,  ['f'] = 8,      // ㄱ, ㄲ, ㄴ, ㄷ, ㄹ
    ['a'] = 16, ['q'] = 17, ['t'] = 19, ['T'] = 20, ['d'] = 21,     // ㅁ, ㅂ, ㅅ, ㅆ, ㅇ
    ['w'] = 22, ['c'] = 23, ['z'] = 24, ['x'] = 25, ['v'] = 26,     // ㅈ, ㅊ, ㅋ, ㅌ, ㅍ
    ['g'] = 27                                                      // ㅎ
};

/** 
 * @brief 초성 매핑 함수 (ASCII 코드 -> 유니코드 인덱스)
 * 
 * @param c: 입력된 키 값 (ASCII 코드)
 * @return: 초성 인덱스 (0~18), 없으면 -1
 */
int key2cho(char c) {
    unsigned char idx = (unsigned char)c;
    if (idx > 127) return -1;
    return KeyToChoIdx[idx];
}

/** 
 * @brief 중성 매핑 함수 (ASCII 코드 -> 유니코드 인덱스)
 * 
 * @param c: 입력된 키 값 (ASCII 코드)
 * @return: 중성 인덱스 (0~20), 없으면 -1
 */
int key2jung(char c) {
    unsigned char idx = (unsigned char)c;
    if (idx > 127) return -1;
    return KeyToJungIdx[idx];
}

/**
 * @brief 종성 매핑 함수 (ASCII 코드 -> 유니코드 인덱스) 
 * 
 * @param c: 입력된 키 값 (ASCII 코드)
 * @return: 종성 인덱스 (0~27), 없으면 -1
 */
int key2jong(char c) {
    unsigned char idx = (unsigned char)c;
    if (idx > 127) return -1;
    return KeyToJongIdx[idx];
}

/**
 * @brief 유니코드(UTF-8) -> 조합형 코드 변환.
 * 
 * 단지 개발 편의를 위한 함수.. 
 * sprintf(s, "한글한글"); 등을 가능하게 하기 위함.
 * 
 * @param s: UTF-8 인코딩된 문자열 포인터 (3바이트 필요)
 * @return: 조합형 코드 (2바이트, 최상위 비트 1), 한글이 아니면 0 반환
*/
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

/**
 * @brief 유니코드 -> johab 변환 후 한글 출력 함수.
 * 
 * 단지 개발 편의를 위한 함수.. 
 * sprintf(s, "한글한글"); 등을 가능하게 하기 위함.
 * 
 * @param sht: 출력할 시트
 * @param x: 출력할 x 좌표
 * @param y: 출력할 y 좌표
 * @param color: 출력할 색상
 * @param s: 출력할 문자열 (UTF-8 인코딩)
 * @return: void
 */
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

// --- 한글 오토마타 구현부 ---

/** 
 * @brief 유니코드 값을 UTF-8로 변환하여 dest에 저장
 * 
 * @param val: 유니코드 값
 * @param dest: 변환된 UTF-8 문자열을 저장할 버퍼 (최대 4바이트 필요)
 * @return: void
 */
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

/**
 * @brief 조합 중인 글자 그리기 함수
 * 
 * @param cons: 콘솔 구조체 포인터
 * @param x: 출력할 x 좌표
 * @param y: 출력할 y 좌표
 * @param cho: 초성 인덱스
 * @param jung: 중성 인덱스
 * @param jong: 종성 인덱스
 * @return: void
 */
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

/**
 * @brief 중성 합성 함수.
 * 
 * 지금 조합이 합성 가능한 중성인지 확인
 * (ㅢ, ㅘ, ㅚ, ㅙ, ㅟ, ㅝ, ㅞ)
 * 
 * @param jung1: 첫 번째 중성 인덱스
 * @param jung2: 두 번째 중성 인덱스
 * @return: 합성된 중성 인덱스, 불가능하면 -1
 */
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

/**
 * @brief 중성 분해 함수.
 * 
 * 합성된 중성을 분해하여 원래의 중성 인덱스 반환
 * 
 * @param jung: 합성된 중성 인덱스
 * @return: 분해된 중성 인덱스 (첫 번째 중성 인덱스)
 */
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

// 인덱스 참고용 주석
// static unsigned char U2J_cho[19] = {
//     2, 3, 4, 5, 6,          // 0:ㄱ, 1:ㄲ, 2:ㄴ, 3:ㄷ, 4:ㄸ
//     7, 8, 9, 10, 11,        // 5:ㄹ, 6:ㅁ, 7:ㅂ, 8:ㅃ, 9:ㅅ
//     12, 13, 14, 15, 16,     // 10:ㅆ, 11:ㅇ, 12:ㅈ, 13:ㅉ, 14:ㅊ
//     17, 18, 19, 20          // 15:ㅋ, 16:ㅌ, 17:ㅍ, 18:ㅎ
// };
//
// static unsigned char U2J_jong[28] = {
//     0,                                      // 0: 받침 없음
//     2, 3, 4, 5, 6, 7,                       // 1:ㄱ, 2:ㄲ, 3:ㄳ, 4:ㄴ, 5:ㄵ, 6:ㄶ, 7:ㄷ
//     8, 9, 10, 11, 12, 13, 14, 15, 16,       // 8:ㄹ, 9:ㄺ, 10:ㄻ, 11:ㄼ, 12:ㄽ, 13:ㄾ, 14:ㄿ, 15:ㅀ
//     17, 19, 20, 21, 22,                     // 16:ㅁ, 17:ㅂ, 18:ㅄ, 19:ㅅ, 20:ㅆ 
//     23, 24, 25, 26, 27, 28, 29              // 21:ㅇ, 22:ㅈ, 23:ㅊ, 24:ㅋ, 25:ㅌ, 26:ㅍ, 27:ㅎ
// };

/**
 * @brief 겹받침 합성 함수.
 * 
 * 지금 조합이 합성 가능한 겹받침인지 확인
 * (ㄳ, ㄵ, ㄶ, ㄺ, ㄻ, ㄼ, ㄽ, ㄾ, ㄿ, ㅀ, ㅄ)
 * 
 * @param cur_jong: 현재 종성 인덱스
 * @param next_cho: 다음 입력된 초성 인덱스
 * @return: 합성된 종성 인덱스, 불가능하면 -1
 */
int get_composite_jong(int cur_jong, int next_cho)
{
    // 'ㄱ' 계열
    if (cur_jong == 1) {
        if (next_cho == 9) return 3; // ㄱ + ㅅ = ㄳ
    }
    // 'ㄴ' 계열
    if (cur_jong == 4) {
        if (next_cho == 12) return 5; // ㄴ + ㅈ = ㄵ
        if (next_cho == 18) return 6; // ㄴ + ㅎ = ㄶ
    }
    // 'ㄹ' 계열
    if (cur_jong == 8) {
        if (next_cho == 0) return 9; // ㄹ + ㄱ = ㄺ
        if (next_cho == 6) return 10; // ㄹ + ㅁ = ㄻ
        if (next_cho == 7) return 11; // ㄹ + ㅂ = ㄼ
        if (next_cho == 9) return 12; // ㄹ + ㅅ = ㄽ
        if (next_cho == 16) return 13; // ㄹ + ㅌ = ㄾ
        if (next_cho == 17) return 14; // ㄹ + ㅍ = ㄿ
        if (next_cho == 18) return 15; // ㄹ + ㅎ = ㅀ
    }
    // 'ㅂ' 계열
    if (cur_jong == 17) {
        if (next_cho == 9) return 18; // ㅂ + ㅅ = ㅄ
    }

    return -1; // 합성 불가
}

/**
 * @brief 겹받침 분해 함수.
 * 
 * 합성된 겹받침 중 첫 번째 받침의 U2J 인덱스 반환
 * (주의: 첫 번째 받침은 현재 글자의 종성이 됨)
 * 
 * @param complex_jong: 합성된 종성 인덱스
 * @return: 분해된 종성 U2J 인덱스 (첫 번째 받침 인덱스)
 */
int get_first_jong(int complex_jong) 
{
    if (complex_jong == 3) return 1;   // ㄳ -> ㄱ
    if (complex_jong == 5 || complex_jong == 6) return 4;   // ㄵ, ㄶ -> ㄴ
    if (complex_jong >= 9 && complex_jong <= 15) return 8;  // ㄺ, ㄻ, ㄼ, ㄽ, ㄾ, ㄿ, ㅀ -> ㄹ
    if (complex_jong == 18) return 17; // ㅄ -> ㅂ

    return -1; // 분해 불가
}

/**
 * @brief 겹받침 분해 함수.
 * 
 * 합성된 겹받침 중 두 번째 받침의 U2J 인덱스 반환 
 * (주의: 두 번째 받침은 다음 글자의 초성이 됨)
 * 
 * @param complex_jong: 합성된 종성 인덱스
 * @return: 분해된 종성 U2J 인덱스 (두 번째 받침 인덱스)
 */
int get_second_jong(int complex_jong)
{
    // 아쉽게도 겹받침의 두 번째 받침은 get_first_jong과 달리 규칙성이 없어서 일일이 처리해야 함
    // 더 나은 방법이 있다면 교체하는 것으로...
    switch(complex_jong) {
        case 3: return 9;    // ㄳ -> ㅅ
        case 5: return 12;   // ㄵ -> ㅈ
        case 6: return 18;   // ㄶ -> ㅎ
        case 9: return 0;    // ㄺ -> ㄱ
        case 10: return 6;   // ㄻ -> ㅁ
        case 11: return 7;   // ㄼ -> ㅂ
        case 12: return 9;   // ㄽ -> ㅅ
        case 13: return 16;  // ㄾ -> ㅌ
        case 14: return 17;  // ㄿ -> ㅍ
        case 15: return 18;  // ㅀ -> ㅎ
        case 18: return 9;   // ㅄ -> ㅅ
    }
    return -1;
}

/** 
 * @brief 한글 오토마타 함수.
 * 
 * state 0: 아무 것도 입력되지 않은 상태
 * 
 * state 1: 초성만 입력된 상태
 * 
 * state 2: 초성+중성 입력된 상태
 * 
 * state 3: 초성+중성+종성 입력된 상태
 * 
 * state 4: 초성+중성+겹받침 입력된 상태
 * @param cons: 콘솔 구조체 포인터
 * @param task: 현재 작업 구조체 포인터
 * @param key: 입력된 키 값 (ASCII 코드)
 * @return: void
 */
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
                    // 조합 가능
                    task->hangul_idx[1] = complex;
                    draw_composing_char(cons, cons->cur_x - 16, cons->cur_y, task->hangul_idx[0], task->hangul_idx[1], -1);
                } else {    
                    // 조합 불가
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
                // 자음 입력
                int complex_jong = get_composite_jong(task->hangul_idx[2], idx_cho);
                if (complex_jong != -1) {
                    task->hangul_state = 4;
                    task->hangul_idx[2] = complex_jong;
                    draw_composing_char(cons, cons->cur_x - 16, cons->cur_y, task->hangul_idx[0], task->hangul_idx[1], task->hangul_idx[2]);
                } else {
                    task->hangul_state = 1;
                    task->hangul_idx[0] = idx_cho;
                    task->hangul_idx[1] = -1;
                    task->hangul_idx[2] = -1;
                    draw_composing_char(cons, cons->cur_x, cons->cur_y, idx_cho, -1, -1);
                    cons->cur_x += 16; // 커서 전진
                }
            } else {
                // 한글 아님 -> 앞 글자 확정 후 새 글자 시작
                task->hangul_state = 0;
                s[0] = key;
                s[1] = 0;
                cons_putstr0(cons, s);
            }
            break;
        
        // state 4: 초성+중성+복합종성 입력된 상태
        case 4:
            if (idx_jung != -1) {
                // 모음 입력 -> 겹받침 분해
                // 예: 값 + ㅏ -> 갑사

                // 현재 겹받침 인덱스
                int complex_jong = task->hangul_idx[2];

                // 겹받침 분해
                int prev_jong_part = get_first_jong(complex_jong);
                int next_cho_part = get_second_jong(complex_jong);

                // 앞 글자 다시 그리기
                draw_composing_char(cons, cons->cur_x - 16, cons->cur_y, task->hangul_idx[0], task->hangul_idx[1], prev_jong_part);
                
                // 새 글자 그리기
                draw_composing_char(cons, cons->cur_x, cons->cur_y, next_cho_part, idx_jung, -1);
                cons->cur_x += 16; // 커서 전진
            } else if (idx_cho != -1) {
                // 자음 입력 -> 앞 글자 확정 후 새 글자 시작
                task->hangul_state = 1;
                task->hangul_idx[0] = idx_cho;
                task->hangul_idx[1] = -1;
                task->hangul_idx[2] = -1;

                draw_composing_char(cons, cons->cur_x, cons->cur_y, idx_cho, -1, -1);
                cons->cur_x += 16; // 커서 전진
            } else {
                task->hangul_state = 0;
                s[0] = key;
                s[1] = 0;
                cons_putstr0(cons, s);
            }
            break;
    }
}

/**
 * @brief 백스페이스 처리용 함수.
 * 
 * 오토마타 합성 과정을 거슬러 올라가며 삭제 처리.
 *
 * @param cons: 콘솔 구조체 포인터
 * @param task: 현재 작업 구조체 포인터
 * @return: 처리 여부 (1: 처리됨, 0: 처리 안됨)
 */
int hangul_automata_delete(struct CONSOLE *cons, struct TASK *task)
{
    // 조합 중인 문자가 없으면 처리 안함 -> console_task에서 처리
    if (task->hangul_state == 0) return 0; // 처리 안됨

    int prev_jung; // 이전 중성 인덱스 저장용
    
    // 상태 처리
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
        case 4: // 겹받침 삭제
            // 겹받침 분해하여 앞쪽 받침만 남김
            task->hangul_idx[2] = get_first_jong(task->hangul_idx[2]);
            task->hangul_state = 3;
            break;
    }

    // 화면 갱신
    if (task->hangul_state == 0) {   // 백스페이스 결과 조합 중인 문자가 없으면 지우기
        draw_composing_char(cons, cons->cur_x, cons->cur_y, -1, -1, -1); // 빈칸 그리기
        boxfill8(cons->sht->buf, cons->sht->bxsize, COL8_000000, cons->cur_x - 16, cons->cur_y, cons->cur_x - 1, cons->cur_y + 15); // 배경 지우기
        sheet_refresh(cons->sht, cons->cur_x - 16, cons->cur_y, cons->cur_x, cons->cur_y + 16);
        cons->cur_x -= 16; // 커서 뒤로 이동
    } else {                         // 여전히 조합 중인 문자가 있으면 다시 그리기
        draw_composing_char(cons, cons->cur_x - 16, cons->cur_y, task->hangul_idx[0], task->hangul_idx[1], task->hangul_idx[2]);
    }

    return 1; // 처리됨
}

void initialize_hangul(struct TASK *task)
{
    task->hangul_state = 0;
    task->hangul_idx[0] = -1;
    task->hangul_idx[1] = -1;
    task->hangul_idx[2] = -1;
    return;
}
