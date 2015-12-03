
; ======================================================================
;   stage2.asm - Stage 2 Bootloader
;
;       - Set up GDT
;       - Switch CPU to protected mode
;       - Enable A20 address line
;
; ----------------------------------------------------------------------

bits    16                          ; Stage 2 is 16 bits
org     0x500                       ; Stage 2 loaded at 0x500
                                    ; 0x500 - 0x7bff unused above BIOS memory

jmp     Stage2

%include "Include/IO.inc"
%include "Include/GDT.inc"
%include "Include/A20.inc"
%include "Include/FAT12.inc"

%define IMAGE_PMODE_BASE 0x100000
%define IMAGE_RMODE_BASE 0x3000

Stage2:
; --------------------------------------------------                                                
;   Set up segments/stack
; --------------------------------------------------

cli                                 ; clear interrupts
xor     ax, ax
mov     ds, ax
mov     es, ax
mov     ax, 0x0                     ; stack: 0x9000 - 0xffff
mov     ss, ax                      ; 0x9000 is base of stack
mov     sp, 0xffff                  ; 0xffff is top of stack
sti                                 ; restore interrupts

; Stage 2 message
mov     si, msgStg2Done
call    Print

; --------------------------------------------------                                                
;   Set up the GDT
; --------------------------------------------------

call CreateGDT

; --------------------------------------------------                                                
;   Enable A20
; --------------------------------------------------

call EnableA20

; --------------------------------------------------                                                
;   Load Root Directory Table
; --------------------------------------------------

call LoadRoot

; --------------------------------------------------                                                
;   Load Kernel
; --------------------------------------------------

LoadKernel:
    mov     ebx, 0
    mov     bp, IMAGE_RMODE_BASE
    mov     si, ImageName
    call    LoadFile
    mov     dword [ImageSize], ecx
    cmp     ax, 0
    je      LoadStage3
    mov     si, errorKernel
    call    Print
    cli
    hlt

LoadStage3:
    mov     si, successKernel
    call    Print

; --------------------------------------------------                                                
;   Enter protected mode
; --------------------------------------------------

    cli                                 ; clear interrupts
    mov     eax, cr0                    ; cr0/control register 0
    or      eax, 1                      ; set bit 0 of cr0 to switch to protected mode
    mov     cr0, eax

    jmp     CODE_DESC:Stage3

; --------------------------------------------------                                                
;   Stage 3 Entry Point
; --------------------------------------------------

bits 32

Stage3:
    mov     ax, DATA_DESC
    mov     ds, ax
    mov     ss, ax
    mov     es, ax                      
    mov     esp, 0x90000                ; 0x90000 is base of stack

CopyImage:
    mov     eax, dword [ImageSize]
    movzx   ebx, word [bpbBytesPerSector]
    mul     ebx
    mov     ebx, 4
    div     ebx
    cld
    mov     esi, IMAGE_RMODE_BASE
    mov     edi, IMAGE_PMODE_BASE
    mov     ecx, eax
    rep     movsd

Execute:
    jmp     CODE_DESC:IMAGE_PMODE_BASE

    cli
    hlt

; constants
msgStg2Done     db 0x0a, 0X0d, "Stage 2 loaded successfully!", 0
errorKernel     db 0x0a, 0X0d, "Error loading kernel!", 0
successKernel   db 0x0a, 0X0d, "Successfully loaded kernel!", 0
ImageName       db "KERNEL  BIN"
ImageSize       db 0
