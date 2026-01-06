; hello-os
; TAB=4

CYLS	EQU	10

		ORG	0x7c00			; location which this program located in memory

; FAT12 Format
		JMP	entry
		DB	0x90
		DB	"HARIBOTE"		; Bootsector name
		DW	512				; size of 1 sector
		DB	1				; size of cluster (1 sector/cluster)
		DW	1				; numbers of reserved sectors
		DB	2				; numbers of FAT tables
		DW	224				; numbers of Entry for root directory
		DW	2880				; total numbers of sector in disk
		DB	0xf0				; midea type
		DW	9				; numbers of sector for one FAT table
		DW	18				; numbers of sector for one track
		DW	2				; numbers of head
		DD	0				; we don't use partition
		DD	2880				; drive size
		DB	0, 0, 0x29			; what..?
		DD	0xffffffff			; volume serial number
		DB	"HARIBOTEOS "			; name of disk
		DB	"FAT12   "			; name of format(8bytes)
		RESB	18				; remain 18bytes

; Program body

entry:
		MOV	AX, 0		; Register initialization
		MOV	SS, AX
		MOV	SP, 0x7c00
		MOV	DS, AX

		MOV AX, 0x0820
		MOV	ES, AX
		MOV	CH, 0		; cylinder 0
		MOV DH, 0		; head 0
		MOV CL, 2		; sector 2

readloop:
		MOV SI, 0		; fail counter
	
retry:
		MOV AH, 0x02	; AH = 0x02 -> read disk
		MOV AL, 1		; 1 sector
		MOV BX, 0
		MOV	DL, 0x00	; A drive
		INT	0x13		; call DISK BIOS
		JNC	next			; No error, jump to fin
		ADD	SI, 1
		CMP	SI, 5		; if SI >= 5, then go to error
		JAE error
		MOV	AH, 0x00
		MOV	DL, 0x00
		INT	0x13		; drive reset
		JMP	retry
	
next:
		MOV	AX, ES
		ADD	AX, 0x0020
		MOV	ES, AX		; ES + 0x0020
		ADD	CL, 1
		CMP	CL, 18
		JBE	readloop
		MOV	CL, 1
		ADD	DH, 1
		CMP	DH, 2
		JB	readloop
		MOV	DH, 0
		ADD	CH, 1
		CMP	CH, CYLS
		JB	readloop

		MOV	[0x0ff0], CH
		JMP	0xc200
	
error:
		MOV	SI, msg

putloop:
		MOV	AL, [SI]
		ADD	SI, 1
		CMP	AL, 0
		JE	fin
		MOV	AH, 0x0e		; character print
		MOV	BX, 15			; collor code
		INT	0x10			; video BIOS
		JMP putloop

fin:
		HLT					; halt CPU
		JMP fin
	
msg:
		DB	0x0a, 0x0a		; two line break
		DB	"load error"
		DB	0x0a
		DB 0

		RESB	0x7dfe-$			; fill 0x00 to 0x7dfe
		DB	0x55, 0xaa
