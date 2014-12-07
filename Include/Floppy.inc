
; ======================================================================												    												 ;
; 	3.5" Floppy Disk Subroutines
;
;		- ReadSectors():	Read sectors from floppy disk
;		- ClusterLBA(): 	Convert cluster to LBA
;		- LBACHS(): 		Convert LBA to CHS
;									 												    		
; ----------------------------------------------------------------------

bits 16

; ----------------------------------------------------------------------
;
; 	BIOS Parameter Block
;
; ----------------------------------------------------------------------

bpbOEM:					db "Luke OS "		; OEM label
bpbBytesPerSector:		dw 512				; number of bytes per sector (512 bytes per sector in FAT12)
bpbSectorsPerCluster: 	db 1				; number of sectors per cluster (always one sector in one cluster)
bpbReservedSectors:		dw 1				; number of reserved sectors (only reserved sector is the boot sector)
bpbNumberOfFATs:		db 2				; number of FATs (always two FATs)
bpbRootEntries:			dw 224				; number of entries in the root directory (always 224, because 512 * 14 / 32 = 224)
bpbTotalSectors:		dw 2880				; number of sectors on disk (always 2880)
bpbMediaDescriptor:		db 0xF8				; media descriptor byte is a bitmap: disk is single sided, 9 sectors per fat, 80 tracks, removable = 0xF0
bpbSectorsPerFAT:		dw 9				; number of sectors for each FAT (always 9)
bpbSectorsPerTrack:		dw 18				; number of sectors per track (always 18)
bpbHeadsPerCylinder:	dw 2				; number of heads per cylinder (always 2)
bpbHiddenSectors:		dd 0				; number of sectors between start of the physical disk and start of this volume, so 0
bpbTotalSectorsBig:		dd 0				; number of sectors is < 65535, so this field is 0
bsDriveNumber:			db 0				; drive number (floppy disk is 0)
bsReserved:				db 0				; reserved byte used by Windows NT
bsExtBootSignature:		db 0x29				; boot signature is 0x29, so MS-DOS/PC-DOS v4.0 BPB
bsSerialNumber:			dd 0x12345678		; serial number is 0x12345678, will be overwritten so doesn't matter 
bsVolumeLabel:			db "MOS FLOPPY "	; volume label
bsFileSystem:			db "FAT12   "		; file system is FAT12

; constants
absoluteSector 			db 0x00
absoluteHead   			db 0x00
absoluteTrack  			db 0x00
datasector  			dw 0x0000
cluster     			dw 0x0000

; --------------------------------------------------												
; 	ReadSectors():
;		- read sectors from disk
;
;	cx => 		Number of sectors to read
;	ax => 		First sector
;	es:ebx => 	buffer to read to
; --------------------------------------------------
ReadSectors:
	.MAIN
	push 	ax									
	push 	bx
	push 	cx
	
	call	LBACHS							; convert the next sector into CHS
	mov 	ah, 0x2							
	mov 	al, 0x1
	mov		ch, BYTE [absoluteTrack]		; track
	mov 	cl, BYTE [absoluteSector]		; sector
	mov 	dh, BYTE [absoluteHead]			; head
	mov 	dl, BYTE [bsDriveNumber]		; drive number, 0 for floppy
	int		0x13							; load sector
	
	jnc		.SUCCESS						; no errors?
	xor 	ax, ax
	int 	0x13
	
	pop 	cx								; error calling interrupt, exit
	pop 	bx
	pop 	ax

	.SUCCESS
	pop 	cx
	pop 	bx
	pop 	ax
	add 	bx, WORD [bpbBytesPerSector]	; prepare to read next 512 bytes
	inc 	ax								; next sector
	
	loop 	.MAIN
	ret

; --------------------------------------------------												
; 	ClusterLBA():
;		- convert a cluster number to LBA
; --------------------------------------------------
ClusterLBA:
	sub		ax, 0x0002						; cluster - 2 = base cluster number
	xor		cx, cx
	mov		cl, BYTE [bpbSectorsPerCluster]	
	mul		cx								; base cluster * sectors per cluster
	add		ax, WORD [datasector]			; ax = LBA cluster	
	ret
	
; --------------------------------------------------												
; 	LBACHS():
;		- convert LBA to CHS
; --------------------------------------------------
LBACHS:
	; absolute sector:
	xor		dx, dx
	div 	WORD [bpbSectorsPerTrack]		; divide starting sector of root (19) by sectors per track
	inc		dl								; add boot sector (1)
	mov		BYTE [absoluteSector], dl
	; absolute head:
	xor		dx, dx
	div		WORD [bpbHeadsPerCylinder]		; modulus by number of heads
	mov		BYTE [absoluteHead], dl
	; absolute track:
	mov		BYTE [absoluteTrack], al
	ret