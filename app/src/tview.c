#include "../include/apilib.h"
#include <stdio.h>

int strtol(char *s, char **endp, int base);
char *skipspace(char *p);
void textview(int win, int w, int h, int xskip, char *p, int tab, int lang);
char *lineview(int win, int w, int y, int xskip, unsigned char *p, int tab, int lang);
int puttab(int x, int w, int xskip, char *s, int tab);

void HariMain(void)
{}

char *skipspace(char *p)
{
    for (; *p==' '; p++) { }
    return p;
}

void textview(int win, int w, int h, int xskip, char *p, int tab, int lang)
{
    int i;
    api_boxfilwin(win + 1, 8, 29, w * 8 + 7, h * 16 + 28, 7);
    for (i=0; i<h; i++) {
        p = lineview(win, w, i * 16 + 29, xskip, p, tab, lang);
    }
    api_refreshwin(win, 8, 29, w * 8 + 8, h * 16 + 29);
    return;
}

char *lineview(int win, int w, int y, int xskip, unsigned char *p, int tab, int lang)
{
    int x = -xskip;
    char s[130];
    for (;;) {
        if (*p == 0) {
            break;
        }
        if (*p == 0x0a) {
            p++;
            break;
        }
        if (lang == 0) { // ASCII
            if (*p == 0x09) {
                x = puttab(x, w, xskip, s, tab);
            } else {
                if (0 <= x && x < w) {
                    s[x] = *p;
                }
                x++;
            }
            p++;
        }
        //if (lang == 1) { // Korean
        //}
        if (x > w) { x = w; }
        if (x > 0) {
            s[x] = 0;
            api_putstrwin(win + 1, 8, y, 0, x, s);
        }
    }
    return p;
}

int puttab(int x, int w, int xskip, char *s, int tab)
{
    for (;;) {
        if (0 <= x && x < w) {
            s[x] = ' ';
        }
        x++;
        if ((x + xskip) % tab == 0) {
            break;
        }
    }
    return x;
}
