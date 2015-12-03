
; ======================================================================
;   bootloader.asm - Stage 1 Bootloader
;
;       - Load root directory and FAT on FAT12 disk
;       - Locate stage2.bin
;       - Run Stage 2 bootloader
;
; ----------------------------------------------------------------------

; --------------------------------------------------
;   Constants
; --------------------------------------------------

; Floppy disk constants
absoluteSector  db 0x00
absoluteHead    db 0x00
absoluteTrack   db 0x00
datasector      dw 0x0000
cluster         dw 0x0000
; Strings
imageName       db "STAGE2  BIN"
msgStg1         db "Stage 1 loader running!", 0
msgLoadStg2     db 0x0a, 0x0d, "Loading Stage 2...", 0
errFilesystem   db 0x0a, 0X0d, "Cannot read FAT12!", 0
errStg2         db 0x0a, 0X0d, "Stage 2 not found!", 0

; --------------------------------------------------
;   Import subroutines
; --------------------------------------------------

%include "Include/IO.inc"                   ; I/O functions
%include "Include/disk.inc"                 ; disk functions

bits 16                                     ; 16 bit flat binary
org 0                                       ; BIOS entry point

jmp Boot                                    ; jump past BPB and begin bootstrap

; --------------------------------------------------
;   BIOS Parameter Block
; --------------------------------------------------

bpbOEM:                 db "Luke OS "       ; OEM label (padded to 8 bytes)
bpbBytesPerSector:      dw 512              ; number of bytes per sector (512 bytes per sector in FAT12)
bpbSectorsPerCluster:   db 1                ; number of sectors per cluster (always one sector in one cluster)
bpbReservedSectors:     dw 1                ; number of reserved sectors (only reserved sector is the boot sector)
bpbNumberOfFATs:        db 2                ; number of FATs (always two FATs)
bpbRootEntries:         dw 224              ; number of entries in the root directory (always 224, because 512 * 14 / 32 = 224)
bpbTotalSectors:        dw 2880             ; number of sectors on disk (always 2880)
bpbMediaDescriptor:     db 0xF8             ; media descriptor byte is a bitmap: disk is single sided, 9 sectors per fat, 80 tracks, removable = 0xF0
bpbSectorsPerFAT:       dw 9                ; number of sectors for each FAT (always 9)
bpbSectorsPerTrack:     dw 18               ; number of sectors per track (always 18)
bpbHeadsPerCylinder:    dw 2                ; number of heads per cylinder (always 2)
bpbHiddenSectors:       dd 0                ; number of sectors between start of the physical disk and start of this volume, so 0
bpbTotalSectorsBig:     dd 0                ; number of sectors is < 65535, so this field is 0
bsDriveNumber:          db 0                ; drive number (floppy disk is 0)
bsReserved:             db 0                ; reserved byte used by Windows NT
bsExtBootSignature:     db 0x29             ; boot signature is 0x29, so MS-DOS/PC-DOS v4.0 BPB
bsSerialNumber:         dd 0x12345678       ; serial number is 0x12345678, will be overwritten so doesn't matter 
bsVolumeLabel:          db "MOS FLOPPY "    ; volume label (padded to 11 bytes)
bsFileSystem:           db "FAT12   "       ; file system is FAT12 (padded to 8 bytes)

; --------------------------------------------------
;   Entry Point
; --------------------------------------------------
Boot:
    ; set segment registers
    cli                                     ; disable interrupts
    mov     ax, 0x07c0                      ; segment = 0x7c0
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax

    ; set up stack
    mov     ax, 0x0000                      ; base of stack = 0x0000
    mov     ss, ax                          ; set stack segment
    mov     sp, 0xFFFF                      ; stack pointer
    sti                                     ; restore interrupts
    
    mov     si, msgStg1                     ; tell user stage 1 is running
    call    Print
    
    ; --------------------------------------------------
    ;   Load Root Directory Table
    ; --------------------------------------------------
    LoadRoot:
    ; get size of root directory
    xor     cx, cx
    xor     dx, dx
    mov     ax, 0x0020                      ; entry is 32 bytes
    mul     WORD [bpbRootEntries]           ; 32 bytes * 224 sectors = 7168 bytes
    div     WORD [bpbBytesPerSector]        ; 7168 bytes / 512 bytes = 14 sectors
    xchg    ax, cx                          ; store root sectors in cx
    
    ; get location of root directory
    mov     al, BYTE [bpbNumberOfFATs]      ; 9 sectors per FAT
    mul     WORD [bpbSectorsPerFAT]         ; 9 * 2 = 18 sectors for both FATs
    add     ax, WORD [bpbReservedSectors]   ; add boot sector = 19
    mov     WORD [datasector], ax           ; store start of root directory
    add     WORD [datasector], cx           ; add number of sectors to get end of root = sector 33
    
    mov     bx, 0x0200                      ; load root at 07C0:0200
    
    call    ReadSectors

    ; --------------------------------------------------
    ;   Search for Stage 2
    ; --------------------------------------------------
    Find:
    mov     cx, WORD [bpbRootEntries]       ; get number of root entries (224)
    mov     di, 0x0200                      ; the offset of root directory in memory

    ; See if the current entry is stage2.bin
    .LOOP
    push    cx                              ; store root entry count on stack               
    mov     cx, 11                          ; file names are 11 bytes (8 byte name + 3 byte extension)
    mov     si, imageName                   ; going to look for stage2.bin/"STAGE2  BIN"
    push    di
    rep     cmpsb                           ; compare the first 11 bytes of 32 byte directory entry with "STAGE2  BIN"
    pop     di
    je      LoadFAT                         ; if stage2.bin found, load the FAT to find its clusters
    pop     cx                              ; get count of root entries off the stack, need to decrement cx to iterate over the remaining entries
    add     di, 0x0020                      ; entry was not stage2.bin, going to the next one
    loop    .LOOP                           ; check if the next entry is stage2.bin
    mov     si, errStg2
    call    Print
    call    Exit
    
    ; --------------------------------------------------
    ;   Load FAT into RAM
    ; --------------------------------------------------
    LoadFAT:
    ; get first cluster of file
    mov     dx, WORD [di + 0x001A]          ; first byte of entry + 0x1A/26 = first cluster
    mov     WORD [cluster], dx              ; store bytes 26-27, 2 byte cluster address
    
    ; get size of FAT
    xor     ax, ax                          
    mov     al, BYTE [bpbNumberOfFATs]      ; 2 FATs
    mul     WORD [bpbSectorsPerFAT]         ; 2 * 9 = 18 sectors for both FATs
    mov     cx, ax
    
    mov     ax, WORD [bpbReservedSectors]   ; 512 byte bootsector + 18 = FATs start at sector 19
    
    ; load FAT into memory at 0x0200
    mov     bx, 0x0200                      ; load at 0x0200, same as root directory table
    call    ReadSectors
    
    ; going to load stage2.bin at 0050:0000
    mov     ax, 0x0050                      ; segment 0x0050
    mov     es, ax
    mov     bx, 0x0000                      ; offset 0x0000
    push    bx

    mov     si, msgLoadStg2
    call    Print
    
    ; Load stage2.bin
    LoadImage:
    .MAIN
    mov     ax, WORD [cluster]              ; read this cluster next
    pop     bx
    call    ClusterLBA                      ; convert cluster to LBA format
    xor     cx, cx
    mov     cl, BYTE [bpbSectorsPerCluster] ; how many sectors will be read (1)
    call    ReadSectors
    push    bx
    
    ; get next cluster of stage2.bin
    mov     ax, WORD [cluster]              ; previous cluster read into memory
    mov     cx, ax
    mov     dx, ax
    shr     dx, 0x0001                      ; divide by 2
    add     cx, dx                          ; cx is 3/2 of cluster
    mov     bx, 0x0200                      ; FAT is located at 0x0200
    add     bx, cx                          ; bx + cx = index of next cluster in FAT
    mov     dx, WORD [bx]                   ; read next cluster from FAT index (2 byte cluster)
    test    ax, 0x0001                      ; is cluster even or odd?
    jnz     .ODD
          
    .EVEN
    and     dx, 0x0FFF                      ; cluster is even, bitwise and the cluster to get the low 12 bits
    jmp .DONE
    .ODD
    shr     dx, 0x0004                      ; cluster is odd, get the most significant 12 bits only
    .DONE
    mov     WORD [cluster], dx              ; overwrite previous cluster with next cluster
    cmp     dx, 0x0FF0                      ; if cluster is 0x0FF0 end of file has been reached
    jb      .MAIN                           ; if not EOF more clusters to load
          
    LoadDone:
    push    WORD 0x0050
    push    WORD 0x0000
    retf
    
    ; cli/hlt to prevent triple fault if error
    Exit:
    cli                                     ; clear interrupts
    hlt                                     ; halt the system
    
    ; pad leftover space
    times 510 - ($ - $$) db 0
    ; MBR signature
    dw 0xAA55
