; Copyright (c) 2019-2021 Andreas T Jonsson
; 
; This software is provided 'as-is', without any express or implied
; warranty. In no event will the authors be held liable for any damages
; arising from the use of this software.
; 
; Permission is granted to anyone to use this software for any purpose,
; including commercial applications, and to alter it and redistribute it
; freely, subject to the following restrictions:
; 
; 1. The origin of this software must not be misrepresented; you must not
;    claim that you wrote the original software. If you use this software
;    in a product, an acknowledgment in the product documentation would be
;    appreciated but is not required.
; 2. Altered source versions must be plainly marked as such, and must not be
;    misrepresented as being the original software.
; 3. This notice may not be removed or altered from any source distribution.

cpu 8086
org 0

db 0x55             ; The BIOS...
db 0xAA             ; Signature!
db 0x4              ; We're a 2k ROM(4*512 bytes)
jmp short init

init:
    pushf
    cli         ; We don't want to be interrupted now!

    call install_handlers

    popf
    retf

int_13_handler:
    out 0xB1, al

    push ax
    
    pushf
    pop ax

    push bp
    mov bp, sp
    mov [bp+8], ax
    pop bp

    pop ax

    iret

int_19_handler:

    ; Install network handler during bootstrap

    in al, 0xB2 
    cmp al, 0
    jne no_network

    mov word [INT_FC_OFFSET], int_fc_handler
    mov [INT_FC_SEGMENT], cs

  no_network:

    push bp
    mov bp, sp
    mov word [bp+4], 0
    mov word [bp+2], 0x7C00
    pop bp

    out 0xB0, al

    iret

int_fc_handler:
    out 0xB2, al
    iret

install_handlers:
    push ds
    push ax

    mov ax,0
    mov ds,ax   ; Point DS to the IVT

    ; Set up our own vectors!
    mov word [INT_13_OFFSET], int_13_handler
    mov [INT_13_SEGMENT], cs

    mov word [INT_19_OFFSET], int_19_handler
    mov [INT_19_SEGMENT], cs

    pop ax
    pop ds
    ret

;;;;;;;;;;;;;;;;;; Data ;;;;;;;;;;;;;;;;;;

INT_13_OFFSET  equ 0x4C
INT_13_SEGMENT equ 0x4E

INT_19_OFFSET  equ 0x64
INT_19_SEGMENT equ 0x66

INT_FC_OFFSET  equ 0x3F0
INT_FC_SEGMENT equ 0x3F2

db 'VXTX - VirtualXT BIOS Extensions', 0xA
db 'This work is licensed under the zlib license.', 0

times 0x7FF-($-$$) db 0
checksum db 0