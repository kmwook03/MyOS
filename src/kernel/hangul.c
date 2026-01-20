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
void put_johab(char *vram, int xsize, int x, int y, char color, unsigned char *font, unsigned short code)
{
    // code: 2바이트 조합형 코드 (MSB(1) + 초성(5) + 중성(5) + 종성(5))
    // 조합형 코드 분해
    int c1 = (code >> 8) & 0xFF; // 상위 8비트
    int c2 = code & 0xFF; // 하위 8비트

    // 비트 마스킹을 통한 음소별 값 추출
    int cho_val = (c1 & 0x7C) >> 2; // 초성 5비트(c1 MSB 다음부터 5비트) 남기기
    int jung_val = ((c1 & 0x03) << 3) | ((c2 & 0xE0) >> 5); // 중성 5비트(c1 하위 2비트, c2 상위 3비트) 남기기
    int jong_val = c2 & 0x1F; // 종성 5비트(c2 하위 5비트) 남기기

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
        unsigned char *p = vram + (y + i) * xsize + x;

        // 픽셀 찍기
        for (b=0; b<8; b++) {
            if (line1 & (0x80 >> b)) p[b] = color; // 왼쪽 8픽셀
            if (line2 & (0x80 >> b)) p[b+8] = color; // 오른쪽 8픽셀
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
    2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, // ㄱ(1)~ㅂ(17) -> 값 2~18
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29 // ㅄ(18)~ㅎ(27) -> 값 20~29 (19 건너뜀)
};

// 유니코드(UTF-8) -> 조합형 코드 변환
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

void putstr_utf8(unsigned char *vram, int xsize, int x, int y, char color, unsigned char *s)
{
    unsigned char *korean = (unsigned char *) *((int *) 0x0fe8); // 한글 폰트 주소
    while (*s != 0x00) {
        if ((*s & 0x80) == 0) {
            // 1바이트 ASCII 문자
            putfonts8_asc(vram, xsize, x, y, color, s);
            x += 8;
            s++;
        } else if ((*s & 0xE0) == 0xE0) {
            // 3바이트 UTF-8 문자
            // UTF-8 -> 조합형 코드 변환
            if (s[1] == 0x00 || s[2] == 0x00) break; // 잘못된 문자열 처리

            unsigned short johab = utf8_to_johab(s);
            if (johab != 0) {
                put_johab(vram, xsize, x, y, color, korean, johab);
            }
            x += 16;
            s += 3; // 3바이트 문자이므로 포인터 3 증가
        } else {
            s++;
        }
    }
}
