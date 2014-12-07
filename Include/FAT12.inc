
; ======================================================================												    												 
; 	FAT12 Subroutines
;
;		- LoadRoot: 	Load Root Directory Table
;		- LoadFAT: 		Load File Allocation Table
;		- FindFile: 	Find a filename
; 		- LoadFile: 	Load a file into memory
;										 												    												 
; ----------------------------------------------------------------------

bits 16

; Includes:
; ---------
%include "Include/Floppy.inc"				; floppy driver

; Constants:
; ----------
%define ROOT_SEG 	0x2e0 					; load root directory at 0xe2e00
%define ROOT_OFFSET 0x2e00					; offset of root directory = 0xe2e00
%define FAT_SEG 	0x2c0					; load FAT at 0xe2c0

; ----------------------------------------------------------------------												
; 	LoadRoot():
;		- Load Root Directory Table to 0x7e00
; ----------------------------------------------------------------------

LoadRoot:
	pusha
	push 	es

    ; get size of root directory
	xor		cx, cx
	xor		dx, dx
	mov 	ax, 32							; entry is 32 bytes
	mul 	WORD [bpbRootEntries]			; 32 bytes * 224 sectors = 7168 bytes
	div 	WORD [bpbBytesPerSector]		; 7168 bytes / 512 bytes = 14 sectors
	xchg 	ax, cx							; store root sectors in cx
	
	; get location of root directory
	mov 	al,	BYTE [bpbNumberOfFATs]		; 9 sectors per FAT
	mul 	WORD [bpbSectorsPerFAT]			; 9 * 2 = 18 sectors for both FATs
	add 	ax, WORD [bpbReservedSectors]	; add boot sector = 19
	mov		WORD [datasector], ax			; store start of root directory
	add 	WORD [datasector], cx			; add number of sectors to get end of root = sector 33
	
	mov 	bx, 0x0200						; load root at 07C0:0200
	
	push 	word ROOT_SEG
	pop		es 								; es:bx = ROOT_SEG:0x0
	mov		bx, 0x0 						
	call 	ReadSectors 					; read root sectors
	pop		es
	popa
	ret

; ----------------------------------------------------------------------	
;	LoadFAT():
;		- Load File Allocation Table at 0x7c00, overwriting boot loader
;
;	es:di => root directory address
; ----------------------------------------------------------------------	

LoadFAT:
	pusha
	push 	es
	
	; get size of FAT
	xor 	ax, ax							
	mov		al, BYTE [bpbNumberOfFATs]		; 2 FATs
	mul		WORD [bpbSectorsPerFAT]			; 2 * 9 = 18 sectors for both FATs
	mov		cx, ax
	
	mov		ax, WORD [bpbReservedSectors]	; 512 byte bootsector + 18 = FATs start at sector 19
	
	; load FAT into memory at 0x0200
	push 	word FAT_SEG
	pop 	es 								; es:bx = FAT_SEG:0x0
	xor 	bx, bx
	call	ReadSectors						; read FAT sectors
	pop 	es
	popa
	ret

; ----------------------------------------------------------------------	
;	FindFile
;		- Search for filename in root directory
;
;	ds:si => filename
;	return: ax => index of file in root, -1 if error
; ----------------------------------------------------------------------	

FindFile:
	push 	cx 								; save registers on stack
	push 	dx
	push 	bx
	mov 	bx, si 							; save filename on stack

	mov 	cx, WORD [bpbRootEntries]		; get number of root entries (224)
	mov 	di, ROOT_OFFSET					; the offset of root directory in memory
	cld 									; clear direction flag

	; See if the current entry is stage2.bin
	.LOOP
	push	cx								; store root entry count on stack				
	mov 	cx,	11							; file names are 11 bytes (8 byte name + 3 byte extension)
	mov 	si, bx							; going to look for stage2.bin/"STAGE2  BIN"
	push 	di

	rep 	cmpsb							; compare the first 11 bytes of 32 byte directory entry with "STAGE2  BIN"

	pop 	di
	je 		.FOUND							; if stage2.bin found, load the FAT to find its clusters
	pop 	cx								; get count of root entries off the stack, need to decrement cx to iterate over the remaining entries
	add 	di,	32							; entry was not stage2.bin, going to the next one
	loop 	.LOOP							; check if the next entry is stage2.bin
	
	.NOTFOUND
	pop 	bx 								; restore registers
	pop 	dx
	pop 	cx
	mov		ax, -1
	ret

	.FOUND
	pop 	ax 								; restore registers
	pop 	bx
	pop 	dx
	pop 	cx
	ret

; ----------------------------------------------------------------------	
;	LoadFile
;		- Load file into memory
;
;	es:si => file to load
;	bx:bp => buffer to load file
;	return: ax => 0 if success, -1 if error
;	return: cx => number of sectors loaded
; ----------------------------------------------------------------------	

LoadFile:
	xor 	ecx, ecx
	push 	ecx

	push 	bx 								; bx:bp = buffer to load file into
	push 	bp

	call 	FindFile						; find file, es:si = filename

	cmp 	ax, -1 							; FindFile error?
	jne 	.LOAD_IMAGE_PRE 				; if success, load FAT
	pop 	bp 								; if error, restore interrupts, set error code, return
	pop 	bx
	pop 	ecx
	mov 	ax, -1
	ret

	.LOAD_IMAGE_PRE
	sub 	edi, ROOT_OFFSET
	sub 	eax, ROOT_OFFSET

	push 	word ROOT_SEG
	pop 	es
	mov 	dx, word [es:di + 0x001A]
	mov 	word [cluster], dx
	pop 	bx
	pop 	es
	push 	bx
	push 	es

	call LoadFAT 							; load the FAT to 0x7c00

	.LOAD_IMAGE:
	mov		ax, WORD [cluster]				; read this cluster next
	pop 	es
	pop		bx
	call	ClusterLBA						; convert cluster to LBA format
	xor		cx, cx
	mov		cl, BYTE [bpbSectorsPerCluster]	; how many sectors will be read (1)
	call 	ReadSectors

	pop 	ecx
	inc 	ecx
	push 	ecx

	push 	bx
	push 	es

	mov 	ax, FAT_SEG
	mov 	es, ax
	xor 	bx, bx

	; get next cluster of file
	mov		ax, WORD [cluster]				; previous cluster read into memory
	mov		cx, ax
	mov		dx, ax
	shr		dx, 0x0001						; divide by 2
	add		cx, dx							; cx is 3/2 of cluster

	mov		bx, 0							; FAT is located at 0x0200
	add 	bx, cx							; bx + cx = index of next cluster in FAT
	mov		dx, WORD [es:bx]					; read next cluster from FAT index (2 byte cluster)
	test	ax, 0x0001						; is cluster even or odd?
	jnz		.ODD

	.EVEN
	and		dx, 0x0FFF						; cluster is even, bitwise and the cluster to get the low 12 bits
	jmp .DONE
	.ODD
	shr		dx, 0x0004						; cluster is odd, get the most significant 12 bits only
	.DONE
	mov		WORD [cluster], dx				; overwrite previous cluster with next cluster
	cmp		dx, 0x0FF0						; if cluster is 0x0FF0 end of file has been reached
	jb		.LOAD_IMAGE						; if not EOF more clusters to load

	.COMPLETE
	pop 	es
	pop 	bx
	pop 	ecx
	xor 	ax, ax
	ret