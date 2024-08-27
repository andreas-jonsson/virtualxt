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

%define VERSION '0.0.1'
%define DEBUG out 0xB3, al

start:
    jmp init

int0E_handler:
	push ax
	push bx
	push cx
	push dx
	push ds
	push es
	push si
	push di

	DEBUG

	; Get callback parameters es:di, bx and cx
	mov ah, 0xFE
	out 0xB2, al

	; Store callback
	mov word [cs:callback+2], es
	mov word [cs:callback], di

	; Get buffer from application.
	mov ax, 0
	call far [cs:callback]

	DEBUG

	; If es:di == 0 we should discard the buffer.
	mov ax, es
	cmp ax, 0
	jne .copy
	cmp di, 0
	jne .copy
	jmp .discard

.copy:
	; Do the copy to es:di buffer.
	; This call alters ds, es and si.
	mov ah, 0xFF
	out 0xB2, al

	DEBUG

	; Send the buffer.
	mov ax, 1
	call far [cs:callback]

	DEBUG

.discard:
	mov	al, 0x20	; Load nonspecific EOI value (0x20) into register al
	out	0x20, al	; Write EOI to PIC (port 0x20)

	pop di
	pop si
	pop es
	pop ds
	pop dx
	pop cx
	pop bx
	pop ax
	iret

int60_handler:
	; This is the required signature of a packet driver.
	; Reference: http://crynwr.com/packet_driver.html
	jmp short .handler
	nop
	db 'PKT DRVR', 0

.handler:
	cmp ah, 1	; driver_info
	jne .exec_command
	cmp ah, 5	;  terminate
	jne .terminate

	; Name
	mov bx, cs
	mov ds, bx
	lea si, name_text

	clc			; No error
	mov al, 1	; Functionality
	mov bx, 1	; Version
	mov ch, 1	; Class
	mov dx, 1	; Type
	mov cl, 0	; Number

	iret

.exec_command:
    out 0xB2, al
	iret

.terminate:
    mov dh, 7	; CANT_TERMINATE
    iret

name_text:
    db 'VirtualXT Packet Driver', 0

callback:
	dw 0, 0

; ---------------------------------------------------------------------
end_of_resident:

init:
	; Print copyright text.
    lea dx, copy_text
    mov ax, 0x900
    int 0x21

	; Check if network module is loaded.
    in al, 0xB2
    cmp al, 0
    jne .no_network

	; Install our handlers.
    mov ax, 0x2560
    mov dx, int60_handler
    int 0x21

    mov ax, 0x250E ; IRQ 6
    mov dx, int0E_handler
    int 0x21

    lea dx, driver_installed_text
    mov ax, 0x900
    int 0x21

    ; Terminate and stay resident.
    mov ax, 0x3100
    mov dx, (end_of_resident - start + 256 + 15) >> 4
    int 0x21

.no_network:

    lea dx, driver_not_installed_text
    mov ax, 0x900
    int 0x21

    ; DOS "terminate" function.
    mov ah, 0x4C
    int 0x21

copy_text:
    db 'VirtualXT Packet Driver v ', VERSION, 0xA, 0xD
    db 'Copyright (c) 2019-2024 Andreas T Jonsson', 0xA, 0xD, '$'

driver_installed_text:
    db 0xA, 'Driver installed!', 0xA, 0xD, '$'

driver_not_installed_text:
    db 0xA, 'Error! Could not find virtual network card.', 0xA, 0xD
    db 'Make sure the network module is loaded.', 0xA, 0xD, '$'

