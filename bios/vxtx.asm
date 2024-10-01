; Copyright (c) 2019-2024 Andreas T Jonsson <mail@andreasjonsson.se>
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
;    claim that you wrote the original software. If you use this software in
;    a product, an acknowledgment (see the following) in the product
;    documentation is required.
;
;    This product make use of the VirtualXT software emulator.
;    Visit https://virtualxt.org for more information.
;
; 2. Altered source versions must be plainly marked as such, and must not be
;    misrepresented as being the original software.
;
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
    push bp
    mov bp, sp
    mov word [bp+4], 0
    mov word [bp+2], 0x7C00
    pop bp

    out 0xB0, al

    iret

install_handlers:
    push ds
    push ax

    mov ax,0
    mov ds,ax   ; Point DS to the IVT

    ; Set up our own vectors!
    mov word [INT_13_OFFSET], int_13_handler
    mov [INT_13_OFFSET+2], cs

    mov word [INT_19_OFFSET], int_19_handler
    mov [INT_19_OFFSET+2], cs
    
    ; Update equipment word to reflect virtual floppy controller.
    mov ax, 0x40 		; BDA
    mov ds, ax
    mov ax, [ds:0x10]	; Read equipment word.
	and ax, 0xC000 		; Indicate 1 floppy drive.
	or ax, 1 			; Indicate floppy drivers are present.
	mov [ds:0x10], ax	; Write new equipment word.
	
    pop ax
    pop ds
    ret

;;;;;;;;;;;;;;;;;; Data ;;;;;;;;;;;;;;;;;;

INT_13_OFFSET  equ 0x4C
INT_19_OFFSET  equ 0x64

db 'VXTX - VirtualXT BIOS Extensions', 0xA
db 'This work is licensed under the zlib-acknowledgement license.', 0

times 0x7FF-($-$$) db 0
checksum db 0
