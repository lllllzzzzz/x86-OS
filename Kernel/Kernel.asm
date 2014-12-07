
; ======================================================================												    												 ;
; 	OS Kernel Stub									 												    		
; ----------------------------------------------------------------------

bits 	32
org  	0x100000

jmp 	Entry

%include "Include/IO.inc"

Entry:
	mov 	ax, 0x10
	mov 	dx, ax
	mov 	ss, ax
	mov 	es, ax
	mov 	esp, 0x90000

	call	ClrScr
	mov 	ebx, msgKernel
	call	PrintStr

	cli
	hlt

msgKernel db 0x0a, 0x0d, "Kernel running!", 0