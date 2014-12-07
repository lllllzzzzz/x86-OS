
;======================================================================;												    												 ;
; 	Protected Mode I/O Subroutines 									   ;
;																	   ;
;		- PrintCh: 		Print a character to VGA 					   ;
;		- PrintStr: 	Print a string to VGA                          ;
;		- MoveCursor: 	Move hardware CRT cursor                       ;
; 		- ClrScr: 		Clear screen via VGA                           ;
;										 							   ;				    												 ;
;----------------------------------------------------------------------;

; VGA constants
%define	VIDMEM		0xB8000					; start of VGA memory
%define	COLS		80						; character columns on screen
%define LINES		25						; character rows on screen
%define CHAR_ATTRIB	14						; character attribute = 14

posX db 0 									; current X position of cursor
posY db 0 									; current Y position of cursor

; 16 Bit Real Mode Subroutines 
bits 16							

;------------------------------------------------------------;													
; 	Print():												 ;
;	- Print a string										 ;
;															 ;
;	DS:SI = null terminated string							 ;
;------------------------------------------------------------;

Print:
	pusha
	.MAIN
	lodsb									; load byte from string in DS:SI into AL
	or		al, al							; AL = 0?
	jz		.DONE							; end of string, exit function
	mov 	ah, 0x0E						; going to print next character
	int 	0x10							; call video interrupt 0x10, print character
	jmp 	.MAIN							; repeat for next character until end of string
	
	.DONE
	popa
	ret										; done, return to boot loader

; 32 Bit Protected Mode Subroutines 
bits 32							

;------------------------------------------------------------;
; 	PrintCh():												 ;
;	- Print a character 									 ;
;															 ;
;	BL => character 										 ;
;------------------------------------------------------------;	

PrintCh:
	pusha									; push all registers
	mov		edi, VIDMEM						; i.e. mov edi, 0xB8000

	; ----------------------------------------
	; 	Get current cursor position

	xor		eax, eax						; null eax

	; ----------------------------------------
	; Current current position = x + y + COLS.
	; COLS = 80, which means 80 characters, but 2 bytes per character so COLS is multiplied by 2.

	mov		ecx, COLS*2 					; multiply COLS by 2 to get column bytes (160)
	mov		al, byte [posY]					; y position
	mul 	ecx								; multiply y position by 160 bytes
	push 	eax								; save y*COLS*2 on stack

	; ----------------------------------------
	; eax now contains current y cursor position*160 byters per row,
	; because COLS (number of characters per row)*2 bytes per character = 160 bytes.

	mov		al,	byte [posX]					; x position
	mov		cl,	2 							; going to multiply x by 2 because 1 character = 2 bytes
	mul 	cl								; get x*2
	pop 	ecx								; restore y*COLS*2 result into ecx
	add 	eax, ecx						; finally, add y*COLS*2 + x*2 to get absolute cursor position

	; ----------------------------------------
	; eax contains the offset from the video base address (0xB8000)
	; at which the character will be printed, so add the offset to the base address stored in EDI.

	xor		ecx, ecx
	add 	edi, eax						; 0xB8000 + offset = address to write character

	; ----------------------------------------
	; Check for newline

	cmp		bl, 0x0a						; is character a newline?
	je		.Row 							; if yes, go to next row

	; ----------------------------------------
	; Print the character

	mov		dl, bl 							; retrieve character from bl
	mov		dh, CHAR_ATTRIB					; retrive character attribute
	mov		word [edi], dx					; write character to VGA offset

	; ----------------------------------------
	; Update cursor position

	inc		byte [posX]						; increment X position to print next character
	cmp		byte [posX], COLS 				; check if X position is now at end of row
	je 		.Row 							; if out of space on row, move to next row below
	jmp		.Done 							; function complete, exit

	; ----------------------------------------
	; Move cursor to next row

	.Row:
	mov 	byte [posX], 0 					; reset posX
	inc 	byte [posY] 					; go to next row

	; ----------------------------------------
	; Restore registers and return

	.Done:
	popa
	ret

;------------------------------------------------------------;													
; 	PrintStr(): 											 ;
;	- Print a string 										 ;
;                     										 ;
;	EBX => address of null-terminated string 				 ;
;------------------------------------------------------------;	

PrintStr:
	pusha									; save registers
	push 	ebx
	pop 	edi 							; edi = ebx (address of string)

	.LOOP:
	; ----------------------------------------
	; Get character

	mov 	bl, byte [edi] 					; get next character
	cmp 	bl, 0 							; bl = null terminator 0?
	je 		.DONE 							; end of string, exit

	; ----------------------------------------
	; Print character to screen

	call PrintCh							; display the current character

	; ----------------------------------------
	; Move on to next character

	.NEXT:
	inc 	edi 							; next character
	jmp 	.LOOP 							; enter loop

	.DONE:
	; ----------------------------------------
	; Update hardware cursor

	mov 	bh, byte [posY] 				; get current cursor position
	mov 	bl, byte [posX]
	call 	MoveCursor 						; update cursor

	popa 									; restore registers
	ret

;------------------------------------------------------------;													
; 	MoveCursor():											 ;
;	- Move the hardware cursor after string output 			 ;
;															 ;
;	bh = y pos 												 ;
;	bl = x pos 												 ;
;------------------------------------------------------------;	

MoveCursor:
	pusha									; save all registers on stack

	; ----------------------------------------
	; Get current position of cursor
	;
	; posX and posY refer to cursor position in screen, NOT in memory,
	; so ignore the fact that every character is 2 bytes.
	; Cursor location: posX+posY*COLS

	xor		eax, eax
	mov 	ecx, COLS 						; going to multiply y with COLS
	mov 	al, bh							; bh = y position
	mul 	ecx 							; y*COLS
	add 	al, bl 							; add posX to y*COLS
	mov 	ebx, eax						; move cursor coordinates into ebx

	; ----------------------------------------
	; Write low order byte to CRT Controller Register (CRTC)

	mov 	al, 0x0f						; 0x0f = low byte index
	mov 	dx, 0x03d4						; 0x03d4 = port number of CRT index register
	out 	dx, al 							; write 0x0f to port 0x03d4

	mov 	al, bl							; low byte of ebx contains x position
	mov 	dx, 0x03d5						; 0x03d5 = port number of CRT data register
	out 	dx, al 							; write x position to port 0x03d5

	; ----------------------------------------
	; Write high order byte to CRT Controller Register (CRTC)

	xor 	eax, eax

	mov 	al, 0x0e						; 0x0f = high byte index
	mov 	dx, 0x03d4						; 0x03d4 = port number of CRT index register
	out 	dx, al 							; write 0x0e to port 0x03d4

	mov 	al, bh							; high byte of ebx contains y position
	mov 	dx, 0x03d5						; 0x03d5 = port number of CRT data register
	out 	dx, al 							; write y position to port 0x03d5

	popa 									; done, restore registers
	ret 									; exit

;------------------------------------------------------------;													
; 	ClrScr(): 												 ;
;	- Clear the screen 										 ;
;------------------------------------------------------------;	

ClrScr:
	pusha									; save interrupts on stack
	cld										; clear direction flag

	; ----------------------------------------
	; Clear the screen by writing empty characters

	mov 	edi, VIDMEM 					; VIDMEM = 0xb8000
	mov 	cx, 2000
	mov 	ah, CHAR_ATTRIB
	mov 	al, ' '
	rep 	stosw

	; ----------------------------------------
	; Screen is cleared, reset x and y coordinates

	mov 	byte [posX], 0 					; reset x to 0
	mov 	byte [posY], 0 					; reset y to 0
	popa 									; restore registers
	ret 									; exit