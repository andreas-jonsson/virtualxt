cpu	8086
org	0h

dw 0

mov [2], ax     ; AX
mov [4], bx     ; BX
mov [6], cx     ; CX
mov [8], dx     ; DX

mov ax, ss
mov [10], ax    ; SS
mov ax, sp
mov [12], ax    ; SP

mov ax, 0
mov ss, ax
mov ax, 2
mov sp, ax
pushf            ; Flags

mov ax, cs
mov [14], ax    ; CS
mov ax, ds
mov [16], ax    ; DS
mov ax, es
mov [18], ax    ; ES
mov ax, bp
mov [20], ax    ; BP
mov ax, si
mov [22], ax    ; SI
mov ax, di
mov [24], ax    ; DI

mov al, 0
out 0, al
