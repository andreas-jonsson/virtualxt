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
org 256

start:
    jmp init

intFC_handler:
    out 0xB2, al
    iret

packet_buffer:
	times 0x3F00 dd 0

end_of_resident:

init:
    lea dx, init_msg
    mov ax, 0x900
    int 0x21

    in al, 0xB2
    cmp al, 0
    jne .no_network

	; Setup buffer
	lea dx, [cs:packet_buffer]
	mov ah, 0xFF
	out 0xB2, al

    mov ax, 0x25FC
    mov dx, intFC_handler
    int 0x21

    lea dx, installed_msg
    mov ax, 0x900
    int 0x21

    mov ax, 0x3100 ; Terminate and stay resident.
    mov dx, (end_of_resident - start + 256 + 15) >> 4
    int 0x21

.no_network:
    lea dx, not_network_msg
    mov ax, 0x900
    int 0x21

    ; DOS "terminate" function.
    mov al, -1
    mov ah, 0x4C
    int 0x21

init_msg:
    db 'VirtualXT extension for Fake86 packet driver', 0xA, 0xD, '$'

installed_msg:
    db 'Driver installed!', 0xA, 0xD, '$'

not_network_msg:
    db 'No network module! Driver not installed.', 0xA, 0xD, '$'
