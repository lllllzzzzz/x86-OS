
; ======================================================================												    												 ;
; 	Global Descriptor Table Subroutines
;
;		- CreateGDT(): 		Load GDT
;									 												    		
; ----------------------------------------------------------------------

bits 16

; GDT constants
;--------------
%define NULL_DESC 0
%define CODE_DESC 0x8
%define DATA_DESC 0x10

; ----------------------------------------------------------------------												
; 	CreateGDT():
;		- Loads the GDT into the GDT register (GDTR)
; ----------------------------------------------------------------------
CreateGDT:
	cli
	pusha
	lgdt	[loc]				; load GDTR with pointer to GDT
	sti
	popa
	ret

; ----------------------------------------												
; 	Define the GDT structure

gdt_start:
	; Null Descriptor
	dd 0
	dd 0
	; Kernel Space Code Descriptor
	dw 0xffff
	dw 0
	db 0
	db 10011010b
	db 11001111b
	db 0
	; Kernel Space Data Descriptor
	dw 0xffff
	dw 0
	db 0
	db 10010010b
	db 11001111b
	db 0
	; User Space Code Descriptor
	dw 0xffff
	dw 0
	db 0
	db 11111010b
	db 11001111b
	db 0
	; User Space Data Descriptor
	dw 0xffff
	dw 0
	db 0
	db 11110010b
	db 11001111b
	db 0

gdt_end:
loc:
	dw gdt_end - gdt_start - 1
	dd gdt_start