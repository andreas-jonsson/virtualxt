; VXTX - VirtualXT BIOS Extensions
; Copyright (c) 2019-2020 Andreas T Jonsson (mail@andreasjonsson.se)
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

db 'VXTX - VirtualXT BIOS Extensions', 0xA
db 'This work is licensed under the GPL3 License.', 0

times 0x7FF-($-$$) db 0
checksum db 0