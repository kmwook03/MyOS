#include "../include/apilib.h"

#define BUF_SIZE 256

void HariMain(void)
{
    int fh;
    char c, cmdline[30], *p;
    char buf[BUF_SIZE];
    int len;

    api_cmdline(cmdline, 30);
    for (p=cmdline; *p>' '; p++) { } // wait until space or end
    for (; *p==' '; p++) { }    // skip spaces
    
    fh = api_fopen(p);
    
    if (fh != 0) {
        for (;;) {
            len = api_fread(buf, BUF_SIZE, fh);

            if (len == 0) { break; }
            
            api_putstr_len(buf, len);
        }
    } else {
        api_putstr("File not found.\n");
    }
    api_end();
}
