; Read all sectors in to video memory using BIOS functions.
; Post [FF][num sectors high][num sectors low][FF] on success.
; Post [num sectors][error num] on error.

%define PATTERN 0xC3
%define HEADS 2
%define TRACKS 80
%define SECTORS 18

cpu 8086
org 0x7C00

    mov ax, 0xB800
    mov es, ax
    mov bx, 0

    mov dx, 0 ; Total number of sectors

    mov ax, HEADS

heads:
    dec ax
    mov cx, TRACKS

tracks:
    dec cx

    push cx ; Store track counter
    push ax ; Store head counter
    push dx

    mov ch, cl  ; ch = Track 0-79
    mov cl, 1   ; cl = Sector
    mov dh, al  ; Head 0-1
    mov dl, 0

    mov ah, 2           ; BIOS read sector function
    mov al, SECTORS     ; Number of sectors to read (max 18)

    int 0x13
    jc error ; Stop if there was an error

    pop dx
    mov ah, 0
    add dx, ax ; Add to total number of sectors
    
    pop ax
    pop cx

    test cl, cl
    jnz tracks

    test al, al
    jnz heads

    ; This is the report
    mov al, 0xFF
    out 0x80, al
    mov al, dh
    out 0x80, al
    mov al, dl
    out 0x80, al
    mov al, 0xFF
    out 0x80, al
    jmp halt

error:
    out 0x80, al
    mov al, ah
    out 0x80, al

halt:
    hlt
    jmp halt

times 510 - ($-$$) db 0
dw 0xAA55

times 1474560 - ($-$$) db PATTERN ; 1.44MB with zeros.
;incbin "random.bin", 512, (1474560 - 512) ; 1.44MB with random bytes.