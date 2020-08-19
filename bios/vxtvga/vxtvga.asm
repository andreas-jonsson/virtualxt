; VXTVGA - Video BIOS for VirtualXT.
; Copyright (c) 2013-2014 Adrian Cable (adrian.cable@gmail.com)
; Copyright (c) 2014 Julian Olds
; Copyright (c) 2019-2020 Andreas T Jonsson (mail@andreasjonsson.se)
;
; This work is licensed under the MIT License. See included LICENSE file.

; DON'T TRY THIS CODE ON A REAL VIDEO CARD!

cpu 8086
org 0

db 0x55             ; The BIOS...
db 0xAA             ; Signature!
db 0x10             ; We're a 8k ROM(16*512 bytes)
jmp short init

init:
    pushf
    cli         ; We don't want to be interrupted now!

    call install_handler
    call init_video

    popf
    retf

int10_handler:
    cmp	ah, 0x00 ; Set video mode
    ;je	int10_set_vm
    cmp	ah, 0x01 ; Set cursor shape
    ;je	int10_set_cshape
    cmp	ah, 0x02 ; Set cursor position
    je	int10_set_cursor
    cmp	ah, 0x03 ; Get cursor position
    je	int10_get_cursor
    cmp	ah, 0x05 ; Set active display page
    ;je	int10_set_disp_page
    cmp	ah, 0x06 ; Scroll up window
    ;je	int10_scrollup
    cmp	ah, 0x07 ; Scroll down window
    ;je	int10_scrolldown
    cmp	ah, 0x08 ; Get character at cursor
    ;je	int10_charatcur
    cmp	ah, 0x09 ; Write char and attribute
    je int10_write_char_attrib
    cmp	ah, 0x0a ; Write char only
    je int10_write_char
    cmp	ah, 0x0b ; set colour
    je int10_set_colour
    cmp	ah, 0x0e ; Write character at cursor position, tty mode
    je int10_write_char_tty
    cmp	ah, 0x0f ; Get video mode
    ;je	int10_get_vm
    cmp	ah, 0x10 ; Get/Set Palette registers
    ;je	int10_palette
    cmp	ax, 0x1123 ; Set int43 to 8x8 double dot font
    ;je	int10_set_int43_8x8dd
    cmp	ax, 0x1130 ; get font information
    ;je	int10_get_font_info 
    cmp	ah, 0x12 ; Video sub-system configure
    ;je	int10_video_subsystem_cfg
    cmp	ah, 0x1a ; Video combination
    ;je	int10_vid_combination
    cmp	ah, 0x1b
    ;je	int10_get_state_info

    iret

int10_set_cursor:
    ; DH = row.
    ; DL = column.
    ; BH = page number (0..7).

    push ds
    push si
    push ax
    push dx

    mov	ax, 0x40
    mov	ds, ax

    cmp	bh, 0x07
    jg .set_cursor_done

    push bx
    mov	bl, bh
    mov	bh, 0
    shl	bx, 1
    mov	si, bx
    pop	bx

    mov	[ds:si+BDA_CURSOR_Y], dh
    mov	[ds:si+BDA_CURSOR_X], dl

    cmp	bh, [ds:BDA_ACTIVE_PAGE_SIZE]
    jne	.set_cursor_done

    mov	[ds:BDA_CURSOR_Y], dh
    mov	[ds:BDA_CURSOR_X], dl
    call set_crtc_cursor_pos

  .set_cursor_done:
    pop	dx
    pop	ax
    pop	si
    pop	ds
    iret

int10_get_cursor:
    ; On entry:
    ;   BH = page number (0..7).
    ; At exit:
    ;   CH, CL = beginning line of cursor, ending line of cursor
    ;   DH, DL = row, column

    push ds
    push si
    push ax

    mov	ax, 0x40
    mov	ds, ax

	  mov	cx, [ds:BDA_CURSOR_SHAPE_END]

	  cmp	bh, 0x07
	  jg .get_cursor_done

    push bx
    mov	bl, bh
    mov	bh, 0
    shl	bx, 1
    mov	si, bx
    pop	bx

    mov	dh, [ds:si+BDA_CURSOR_Y]
    mov	dl, [ds:si+BDA_CURSOR_X]

  .get_cursor_done:
    pop	ax
    pop	si
    pop	ds
    iret

int10_write_char:
    ; AL = character to display
    ; BH = page number
    ; BL = foreground colour (gfx mode only)
    ; CX = number of times to write character

    push si
    push di
    push ds
    push es
    push ax
    push bx
    push cx
    push bp

    push ax
    mov	ax, 0x40
    mov	ds, ax
    pop	ax

    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 0x00
    je .write_char_t40
    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 0x01
    je .write_char_t40
    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 0x02
    je .write_char_t80
    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 0x03
    je .write_char_t80

    ; Not a text mode, so only page 1 is valid.
    cmp	bh, 0
    jne	.write_char_done
    jmp	.write_char_page_ok

  .write_char_t40:
    cmp	bh, 0x07
    jg .write_char_done
    jmp	.write_char_page_ok

  .write_char_t80:
	  cmp	bh, 0x07
	  jg .write_char_done

  .write_char_page_ok:

    ; Get bios data cursor position offset for this page into SI
    push bx
    mov	bl, bh
    mov	bh, 0
    shl	bx, 1
    mov	si, bx
    pop	bx

    push ax

    ; Get video page offset into BP
    mov	al, [ds:BDA_ACTIVE_PAGE_SIZE+1]
    mul	byte bh
    mov	ah, al
    mov	al, 0
    mov	bp, ax

    ; Get offset for the cursor position within the page into DI
    mov	al, [ds:si+BDA_CURSOR_Y]
    mul	byte [ds:BDA_NUM_COLUMNS]
    add	al, [ds:si+BDA_CURSOR_X]
    adc	ah, 0
    shl	ax, 1
    mov	di, ax

    ; Get video RAM segment into ES
    mov	ax, 0xb800
    mov	es, ax

	  pop	ax

    mov	bx, 0xb800
    mov	es, bx

  .write_char_repeat:
	  cmp cx, 0
	  je .write_char_done        

    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 4
    je .write_char_cga320
    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 5
    je .write_char_cga320
    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 6
    je .write_char_cga640

	  jmp .write_char_text

  .write_char_cga320:
	  call put_cga320_char
	  jmp	.write_char_next

  .write_char_cga640:
	  call put_cga640_char 
	  jmp .write_char_next

  .write_char_text:
	  mov	[es:bp+di], al
 
  .write_char_next:
	  dec	cx
	  add	di, 2
	  jmp	.write_char_repeat    

  .write_char_done:
    pop	bp
    pop	cx
    pop	bx
    pop	ax
    pop	es
    pop	ds
    pop	di
    pop	si

    iret

int10_set_colour:
    ; BH = 0 => set background colour
    ;           BL = background/border colour (5 bits)
    ; BH = 1 => set palette
    ;           BL = palette id	
    ;              0 = red/green/brown
    ;              1 = cyan/magenta/white

    push dx
    push bx
    push ax

	  cmp	bh, 1
	  je .set_palette

    ; Set BG colour

    and	bl, 0x1f
    mov	dx, 0x3d9
    in al, dx
    and	al, 0xf0
    or al, bl
    out	dx, al

    jmp	.set_colour_done

  .set_palette:

	  cmp	bl, 0x00
	  jne	.set_palette1

	  jmp .set_palette_update

  .set_palette1:
	  mov	bl, 0x20

  .set_palette_update:	
    mov	dx, 0x3d9
    in al, dx
    and	al, 0xdF
    or al, bl
    out	dx, al

  .set_colour_done:
    pop	ax
    pop	bx
    pop	dx

    iret

int10_write_char_tty:
    ; AL = character to display
    ; BH = page number
    ; BL = foreground pixel colour (gfx mode only)

    push ds
    push es
    push ax
    push bx
    push cx
    push si
    push di
    push bp

    push	ax
    mov	ax, 0x40
    mov	ds, ax
    pop	ax

    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 0x0
    je .write_char_tty_t40
    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 0x1
    je .write_char_tty_t40
    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 0x2
    je .write_char_tty_t80
    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 0x3
    je .write_char_tty_t80

    ; Not a text mode, so only page 1 is valid.
    cmp	bh, 0
    jne	.write_char_tty_done
    jmp	.write_char_tty_page_ok

  .write_char_tty_t40:
    cmp	bh, 0x07
    jg .write_char_tty_done
    jmp	.write_char_tty_page_ok

  .write_char_tty_t80:
    cmp	bh, 0x07
    jg .write_char_tty_done

  .write_char_tty_page_ok:
    ; Get bios data cursor position offset for this page into SI
    push bx
    mov	bl, bh
    mov	bh, 0
    shl	bx, 1
    mov	si, bx
    pop	bx

    push	ax

    ; Get video page offset into BP
    mov	al, [ds:BDA_ACTIVE_PAGE_SIZE+1]
    mul	byte bh
    mov	ah, al
    mov	al, 0
    mov	bp, ax

    ; Get offset for the cursor position within the page into DI
    mov	al, [ds:si+BDA_CURSOR_Y]
    mul	byte [ds:BDA_NUM_COLUMNS]
    add	al, [ds:si+BDA_CURSOR_X]
    adc	ah, 0
    shl	ax, 1
    mov	di, ax

    ; Get video RAM segment into ES
    mov	ax, 0xb800
    mov	es, ax

    pop	ax

    cmp	al, 0x08
    je .write_char_tty_bs

    cmp al, 0x0A
    je .write_char_tty_nl

    cmp al, 0x0D
    je .write_char_tty_cr

    jmp .write_char_tty_printable

  .write_char_tty_bs:
    mov	byte [es:bp+di], 0x20
    dec	byte [ds:si+BDA_CURSOR_X]
    cmp	byte [ds:si+BDA_CURSOR_X], 0
    jg .write_char_tty_set_crtc_cursor

    mov	byte [ds:si+BDA_CURSOR_X], 0    
    jmp	.write_char_tty_set_crtc_cursor

  .write_char_tty_nl:
    mov	byte [ds:si+BDA_CURSOR_X], 0
    inc	byte [ds:si+BDA_CURSOR_Y]

    mov	cl, [ds:BDA_NUM_ROWS]
    cmp	byte [ds:si+BDA_CURSOR_Y], cl
    jbe	.write_char_tty_done
    mov	byte [ds:si+BDA_CURSOR_Y], cl

    push ax
    push cx
    push dx

    mov al, 1
    mov	bx, 0x0700
    mov	cx, 0
    mov	dh, [ds:BDA_NUM_ROWS]
    mov	dl, [ds:BDA_NUM_COLUMNS]
    dec	dl

    call scroll_up_window

    pop	dx
    pop	cx
    pop	ax

    jmp	.write_char_tty_set_crtc_cursor

  .write_char_tty_cr:
    mov	byte [ds:(si+BDA_CURSOR_X)],0
    jmp	.write_char_tty_set_crtc_cursor

  .write_char_tty_printable:
    cmp byte [ds:BDA_ACTIVE_VIDEOMODE], 4
    je .write_char_tty_cga320
    cmp byte [ds:BDA_ACTIVE_VIDEOMODE], 5
    je .write_char_tty_cga320
    cmp byte [ds:BDA_ACTIVE_VIDEOMODE], 6
    je .write_char_tty_cga640

    jmp	.write_char_tty_text

  .write_char_tty_cga320:
    call put_cga320_char
    jmp	.write_char_tty_upd_cursor

  .write_char_tty_cga640:
    call put_cga640_char 
    jmp	.write_char_tty_upd_cursor

  .write_char_tty_text:
    mov	[es:bp+di], al
  .write_char_tty_upd_cursor:
    inc	byte [ds:si+BDA_CURSOR_X]
    mov	ah, [ds:BDA_NUM_COLUMNS]
    cmp	byte [ds:si+BDA_CURSOR_X], ah
    jge	.write_char_tty_nl

  .write_char_tty_set_crtc_cursor:
    cmp	bh, [ds:BDA_ACTIVE_PAGE_SIZE]
    jne	.write_char_tty_done

    mov	ax, [ds:si+BDA_CURSOR_X]
    mov	[ds:BDA_CURSOR_X], ax
    call set_crtc_cursor_pos

  .write_char_tty_done:
    pop	bp
    pop	di
    pop	si
    pop	cx
    pop	bx
    pop	ax
    pop	es
    pop	ds

    iret

int10_write_char_attrib:
    ; AL = character to display
    ; BH = page number
    ; BL = attribute (text mode) or foreground colour (gfx mode)
    ; CX = number of times to write character
 
    push si
    push di
    push ds
    push es
    push ax
    push bx
    push cx
    push bp

    push ax
    mov	ax, 0x40
    mov	ds, ax
    pop ax

    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 0x0
    je .write_char_attrib_t40
    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 0x1
    je .write_char_attrib_t40
    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 0x2
    je .write_char_attrib_t80
    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 0x3
    je .write_char_attrib_t80

	  ; Not a text mode, so only page 1 is valid.
	  cmp	bh, 0
    jne	.write_char_attrib_done
    jmp	.write_char_attrib_page_ok

  .write_char_attrib_t40:
	  cmp	bh, 0x07
	  jg .write_char_attrib_done
	  jmp	.write_char_attrib_page_ok

  .write_char_attrib_t80:
	  cmp	bh, 0x07
	  jg .write_char_attrib_done

  .write_char_attrib_page_ok:

    ; Get bios data cursor position offset for this page into SI
    push bx
    mov	bl, bh
    mov	bh, 0
    shl	bx, 1
    mov	si, bx
    pop	bx

	  push ax

    ; Get video page offset into BP
    mov	al, [ds:BDA_ACTIVE_PAGE_SIZE+1]
    mul	byte bh
    mov	ah, al
    mov	al, 0
    mov	bp, ax

    ; Get offset for the cursor position within the page into DI
    mov	al, [ds:si+BDA_CURSOR_Y]
    mul	byte [ds:BDA_NUM_COLUMNS]
    add	al, [ds:si+BDA_CURSOR_X]
    adc	ah, 0
    shl	ax, 1
    mov	di, ax

	  ; Get video RAM segment into ES
	  mov	ax, 0xb800
	  mov	es, ax

	  pop	ax

	  mov	ah, bl	; ax = char + attr to write

	  mov	bx, 0xb800
	  mov	es, bx

  .write_char_attrib_repeat:
	  cmp cx, 0
	  je .write_char_attrib_done        

    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 4
    je .write_char_attrib_cga320
    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 5
    je .write_char_attrib_cga320
    cmp	byte [ds:BDA_ACTIVE_VIDEOMODE], 6
    je .write_char_attrib_cga640

	  jmp	.write_char_attrib_text

  .write_char_attrib_cga320:
	  call put_cga320_char
	  jmp	.write_char_attrib_next

  .write_char_attrib_cga640:
	  call put_cga640_char 
	  jmp	.write_char_attrib_next

  .write_char_attrib_text:
	  mov	[es:bp+di], ax
 
  .write_char_attrib_next:
	  dec	cx
	  add	di, 2
	  jmp	.write_char_attrib_repeat    

  .write_char_attrib_done:
    pop	bp
    pop	cx
    pop	bx
    pop	ax
    pop	es
    pop	ds
    pop	di
    pop	si

    iret

scroll_up_window:
set_crtc_cursor_pos:

put_cga320_char:
put_cga320_char_double:
put_cga640_char:
    ret

init_video:
    push ax
    push ds

    mov ax,0x40
    mov ds,ax               ; Point DS to the BDA

    mov al,1
    out 0x3B8,al            ; Reset modectrl register

    mov ax,[ds:BDA_MACHINE_WORD]
    and ax,0xFFCF
    or ax,0x20                        ; Set CGA support for now
    mov [ds:BDA_MACHINE_WORD],ax

    mov byte [ds:BDA_ACTIVE_VIDEOMODE], 3
    mov word [ds:BDA_NUM_COLUMNS], 80
    mov word [ds:BDA_ACTIVE_PAGE_SIZE], 0x1000
    mov byte [ds:BDA_CURSOR_X], 0
    mov byte [ds:BDA_CURSOR_Y], 0
    mov byte [ds:BDA_NUM_ROWS], 24

    pop ds
    pop ax
    ret

install_handler:
    push ds
    push ax
    push es

    mov ax,0
    mov ds,ax   ; Point DS to the IVT
    mov ax,0x40
    mov es,ax   ; Point ES to the BDA
  
    ; Back-up the original vector set up by the BIOS!
    ;mov ax,[ds:INT10_OFFSET]          ; BIOS INT10 offset
    ;mov [es:BDA_INT10_OFFSET],ax      ; Save to our save position!
    ;mov ax,[ds:INT10_SEGMENT]         ; BIOS INT10 segment
    ;mov [es:BDA_INT10_SEGMENT],ax     ; Save to our save position!

    ; Set up our own vector!
    mov word [ds:INT10_OFFSET], int10_handler   ; Hook our
    mov [ds:INT10_SEGMENT],cs                   ; INT10 interrupt vector!

    pop es
    pop ax
    pop ds
    ret

;;;;;;;;;;;;;;;;;; Data ;;;;;;;;;;;;;;;;;;

INT10_OFFSET equ 0x40
INT10_SEGMENT equ 0x42
BDA_INT10_OFFSET equ 0xAC
BDA_INT10_SEGMENT equ 0xAE

BDA_MACHINE_WORD equ 0x10
BDA_ACTIVE_VIDEOMODE equ 0x49
BDA_NUM_COLUMNS equ 0x4A
BDA_ACTIVE_PAGE_SIZE equ 0x4C
BDA_CURSOR_X equ 0x50
BDA_CURSOR_Y equ 0x51
BDA_CURSOR_SHAPE_END equ 0x60
BDA_CURSOR_SHAPE_START equ 0x61
BDA_NUM_ROWS equ 0x84

times 0x1FFF-($-$$) db 0
checksum db 0