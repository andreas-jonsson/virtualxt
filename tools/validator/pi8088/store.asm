cpu	8086
org	0h

dw 0

out 2, ax       ; AX
mov ax, bx
out 4, ax       ; BX
mov ax, cx
out 6, ax       ; CX
mov ax, dx
out 8, ax       ; DX

mov ax, ss
out 10, ax       ; SS
mov ax, sp
out 12, ax       ; SP

mov ax, 0
mov ss, ax
mov ax, 2
mov sp, ax
pushf            ; Flags

mov ax, cs
out 14, ax       ; CS
mov ax, ds
out 16, ax       ; DS
mov ax, es
out 18, ax       ; ES
mov ax, bp
out 20, ax       ; BP
mov ax, si
out 22, ax       ; SI
mov ax, di
out 24, ax       ; DI

mov al, 0xFF
out 0xFF, al    ; Done!
