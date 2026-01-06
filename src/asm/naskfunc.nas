;   naskfunc
;   TAB=4

[FORMAT "WCOFF"]        ; obj make mode
[INSTRSET   "i486p"]
[BITS   32]             ; for 32-bit

;   info for obj
[FILE   "naskfunc.nas"]

    GLOBAL  _io_hlt, _write_mem8

; function
[SECTION    .text]
_io_hlt:                ; void io_hlt(void);
    HLT
    RET

_write_mem8:            ; void write_mem8(int addr, int data);
    MOV ECX, [ESP+4]    ; read address in [ESP+4]
    MOV AL, [ESP+8]     ; read data in [ESP+8]
    MOV [ECX], AL
    RET
