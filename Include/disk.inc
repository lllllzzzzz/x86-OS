
; ==================================================                                                
;   Disk Subroutines
; ==================================================

bits 16

; --------------------------------------------------                                                
;   ReadSectors
;   - read sectors from disk
; --------------------------------------------------
ReadSectors:
    .MAIN
    push    ax                                  
    push    bx
    push    cx
    
    call    LBACHS                          ; convert the next sector into CHS
    mov     ah, 0x2                         
    mov     al, 0x1
    mov     ch, BYTE [absoluteTrack]        ; track
    mov     cl, BYTE [absoluteSector]       ; sector
    mov     dh, BYTE [absoluteHead]         ; head
    mov     dl, BYTE [bsDriveNumber]        ; drive number, 0 for floppy
    int     0x13                            ; load sector
    
    jnc     .SUCCESS                        ; no errors?
    xor     ax, ax
    int     0x13
    
    pop     cx                              ; error calling interrupt, exit
    pop     bx
    pop     ax
    mov     si, errFilesystem
    call    Print
    call    Exit

    .SUCCESS
    pop     cx
    pop     bx
    pop     ax
    add     bx, WORD [bpbBytesPerSector]    ; prepare to read next 512 bytes
    inc     ax                              ; next sector
    
    loop    .MAIN
    ret
    
; --------------------------------------------------                                                
;   ClusterLBA
;   - convert a cluster number to LBA
; --------------------------------------------------
ClusterLBA:
    sub     ax, 0x0002                      ; cluster - 2 = base cluster number
    xor     cx, cx
    mov     cl, BYTE [bpbSectorsPerCluster] 
    mul     cx                              ; base cluster * sectors per cluster
    add     ax, WORD [datasector]           ; ax = LBA cluster  
    ret
    
; --------------------------------------------------                                                
;   LBACHS
;   - convert LBA to CHS
; --------------------------------------------------
LBACHS:
    ; absolute sector:
    xor     dx, dx
    div     WORD [bpbSectorsPerTrack]       ; divide starting sector of root (19) by sectors per track
    inc     dl                              ; add boot sector (1)
    mov     BYTE [absoluteSector], dl
    ; absolute head:
    xor     dx, dx
    div     WORD [bpbHeadsPerCylinder]      ; modulus by number of heads
    mov     BYTE [absoluteHead], dl
    ; absolute track:
    mov     BYTE [absoluteTrack], al
    ret
