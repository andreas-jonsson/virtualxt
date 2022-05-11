; Copyright (c) 2019-2022 Andreas T Jonsson
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
org 256

start:
    jmp init

    align 4

int10_org:
    dd 0
int10_handler:
    out 0xB3, al
    jmp far [cs:int10_org]

int21_org:
    dd 0

int21_handler:
    out 0xB4, al
    jmp far [cs:int21_org]

intFC_handler:
    out 0xB2, al
    iret

end_of_resident:

init_text:
    db 'VirtualXT guest driver initialized!', '$'

init:
    ; Initialize video extensions.

    mov ax, 3510h ; Get interrupt vector.
    int 21h
    mov word [int10_org + 2], es
    mov word [int10_org], bx

    mov ax, 2510h ; Set interrupt vector.
    mov dx, int10_handler
    int 21h

    ; Initialize DOS extensions.

    mov ax, 3521h
    ;int 21h
    mov word [int21_org + 2], es
    mov word [int21_org], bx

    mov ax, 2521h
    mov dx, int21_handler
    ;int 21h

    ; Initialize network.

    in al, 0xB2
    cmp al, 0
    jne no_network

    mov ax, 25fch
    mov dx, intFC_handler
    int 21h

no_network:

    lea dx, init_text
    mov ax, 900h
    int 21h

    mov ax, 3100h ; Terminate and stay resident.
    mov dx, (end_of_resident - start + 256 + 15) >> 4
    int 21h
