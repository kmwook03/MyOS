// file management(FAT12)

#include "../include/bootpack.h"

/**
 * @brief FAT 테이블 읽기 함수
 * 
 * @param fat: FAT 테이블 포인터
 * @param img: 디스크 이미지 포인터
 * @return: void
 */
void file_readfat(int *fat, unsigned char *img)
{
	int i, j = 0;
	for (i = 0; i < 2880; i += 2) {
		fat[i + 0] = (img[j + 0]      | img[j + 1] << 8) & 0xfff;
		fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
		j += 3;
	}
	return;
}

/**
 * @brief 파일 로드 함수
 * 
 * @param clustno: 시작 클러스터 번호
 * @param size: 파일 크기
 * @param buf: 파일 데이터를 저장할 버퍼 포인터
 * @param fat: FAT 테이블 포인터
 * @param img: 디스크 이미지 포인터
 * @return: void
 */
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img)
{
	int i;
	for (;;) {
		if (size <= 512) {
			for (i = 0; i < size; i++) {
				buf[i] = img[clustno * 512 + i];
			}
			break;
		}
		for (i = 0; i < 512; i++) {
			buf[i] = img[clustno * 512 + i];
		}
		size -= 512;
		buf += 512;
		clustno = fat[clustno];
	}
	return;
}

/**
 * @brief 파일 검색 함수
 * 
 * @param name: 검색할 파일 이름 문자열
 * @param finfo: 파일 정보 구조체 배열 포인터
 * @param max: 파일 정보 구조체 배열의 최대 개수
 * @return struct FILEINFO*: 검색된 파일 정보 구조체 포인터, 없으면 0 반환
 */
struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max)
{
    int x, y;
    char s[12];
    for (y = 0; y < 11; y++) {
		s[y] = ' ';
	}
	y = 0;
	for (x = 0; name[x] != 0; x++) {
        if (y >= 11) { return 0; } // 이름이 너무 김
		if (name[x] == '.' && y <= 8) {
			y = 8;
		} else {
			s[y] = name[x];
		    if ('a' <= s[y] && s[y] <= 'z') {
		    	s[y] -= 0x20;
			} 
			y++;
		}
	}
	for (x = 0; x < 224; ) {
		if (finfo[x].name[0] == 0x00) {
	    	break;
		}
		if ((finfo[x].type & 0x18) == 0) {
			for (y = 0; y < 11; y++) {
		    	if (finfo[x].name[y] != s[y]) {
					goto next;
				}
			}
			return finfo + x; 
		}
next:
		x++;
	}
    return 0;
}

/**
 * @brief 파일 로드 함수 (tek 압축 해제 지원)
 * 
 * @param clustno: 시작 클러스터 번호
 * @param psize: 파일 크기 포인터 (압축 해제 시 크기 변경됨)
 * @param fat: FAT 테이블 포인터
 * @return char*: 파일 데이터를 저장한 버퍼 포인터
 */
char *file_loadfile2(int clustno, int *psize, int *fat)
{
	int size = *psize, size2;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	char *buf, *buf2;
	buf = (char *) memman_alloc_4k(memman, size);
	file_loadfile(clustno, size, buf, fat, (char *) (ADR_DISKIMG + 0x003e00));
	// tek 압축 해제
	if (size >= 17) {
		size2 = tek_getsize(buf);
		if (size2 > 0) {
			buf2 = (char *) memman_alloc_4k(memman, size2);
			tek_decomp(buf, buf2, size2);
			memman_free_4k(memman, (int) buf, size);
			buf = buf2;
			*psize = size2;
		}
	}
	return buf;
}
