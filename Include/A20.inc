; ======================================================================            
;   A20 Subroutines
;
;       - EnableA20():  Enable A20 address line
;                                                                                           
; ----------------------------------------------------------------------

bits 16

; ----------------------------------------------------------------------
;   EnableA20():                                                
;       - Enable A20 gate by writing command 0xDD to keyboard controller port 0x64
; ----------------------------------------------------------------------
EnableA20:
    cli
    push    ax
    mov     al, 0xdd                    ; 0xdd = enable A20
    out     0x64, al                    ; send command 0xdd to port 0x64
    pop     ax
    ret
