; Copyright (C) 2019-2020 Andreas T Jonsson
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
db 0x10             ; We're a 8k ROM(16*512 bytes)
jmp short init

init:
    pushf
    push ds
    push ax
    push es

    cli         ; We don't want to be interrupted now!

    mov ax,0
    mov ds,ax   ; Point DS to the IVT
    mov ax,0x40
    mov es,ax   ; Point ES to the BDA
  
    ; Back-up the original vector set up by the BIOS!
    mov ax,[ds:INT19_OFFSET]          ; BIOS INT19 offset
    mov [es:BDA_INT19_OFFSET],ax      ; Save to our save position!
    mov ax,[ds:INT19_SEGMENT]         ; BIOS INT19 segment
    mov [es:BDA_INT19_SEGMENT],ax     ; Save to our save position!

    ; Set up our own vector!
    mov word [ds:0x64], int19_handler  ; Hook our
    mov [ds:0x66],cs                   ; INT19 interrupt vector!

    call init_video

    pop es
    pop ax
    pop ds
    popf

    retf

int19_handler:
    pushf
    push ax
    push ds
    push si

    mov ax,cs
    mov ds,ax

    mov al,13
    out 0xE8,al

    mov si,msg_hey
    call print_string

    .hang:
      jmp .hang

    pop si
    pop ds
    pop ax
    popf
    iret

init_video:
    pushf
    push ax

    mov al,1
    out 0x3B8,al    ; Reset modectrl register

    pop ax
    popf
    ret

print_string:
    push ax

  .next:
    lodsb        ; grab a byte from SI
 
    or al,al  ; logical or AL by itself
    jz .done   ; if the result is zero, get out
 
    mov ah,0x0E
    int 0x10      ; otherwise, print out the character!
 
    jmp .next
 
  .done:
      pop ax
      ret

; Data

msg_hey db 'Hey says VXTVGA BIOS extension!',0

INT19_OFFSET equ 0x64
INT19_SEGMENT equ 0x66
BDA_INT19_OFFSET equ 0xAC
BDA_INT19_SEGMENT equ 0xAE

times 0x1FFF-($-$$) db 0
checksum db 0