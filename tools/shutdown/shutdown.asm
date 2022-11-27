; Copyright (c) 2019-2022 Andreas T Jonsson <mail@andreasjonsson.se>
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
;    Portions Copyright (c) 2019-2022 Andreas T Jonsson <mail@andreasjonsson.se>
;
; 2. Altered source versions must be plainly marked as such, and must not be
;    misrepresented as being the original software.
;
; 3. This notice may not be removed or altered from any source distribution.

cpu 8086
org 256

    lea dx, message
    mov ax, 900h
    int 21h

    xor al, al      ; Reset command
    out 0xB4, al
    mov al, 1       ; Shutdown command
    out 0xB4, al

stop:
    hlt
    jmp stop

message:
    db 'Emulator shutdown...', '$'