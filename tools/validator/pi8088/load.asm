cpu	8086
org	100h

dw 0

mov ax, 0
mov ss, ax
mov sp, ax
popf            ; Flags

mov bx, word 0  ; BX
mov cx, word 0  ; CX
mov dx, word 0  ; DX

mov ax, word 0  ; SS
mov ss, ax
mov ax, word 0  ; DS
mov ds, ax
mov ax, word 0  ; ES
mov es, ax
mov ax, word 0  ; SP
mov sp, ax
mov ax, word 0  ; BP
mov bp, ax
mov ax, word 0  ; SI
mov si, ax
mov ax, word 0  ; DI
mov di, ax

mov ax, word 0  ; AX
jmp 0:0         ; CS:IP
