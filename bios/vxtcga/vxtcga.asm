; VXTCGA - CGA/HGA Video BIOS for VirtualXT.
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
db 0x20             ; We're a 16k ROM(32*512 bytes)
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
    je int10_set_vm
    cmp	ah, 0x01 ; Set cursor shape
    je int10_set_cshape
    cmp	ah, 0x02 ; Set cursor position
    je int10_set_cursor
    cmp	ah, 0x03 ; Get cursor position
    je int10_get_cursor

    ; 0x04 Get Light Pen Position

    cmp	ah, 0x05 ; Set active display page
    je int10_set_disp_page
    cmp	ah, 0x06 ; Scroll up window
    je int10_scrollup
    cmp	ah, 0x07 ; Scroll down window
    je int10_scrolldown
    cmp	ah, 0x08 ; Get character at cursor
    je int10_charatcur
    cmp	ah, 0x09 ; Write char and attribute
    je int10_write_char_attrib
    cmp	ah, 0x0a ; Write char only
    je int10_write_char
    cmp	ah, 0x0b ; set colour
    je int10_set_colour

    ; 0x0C Write Pixel Dot
    ; 0x0D Read Pixel Dot

    cmp	ah, 0x0e ; Write character at cursor position, tty mode
    je int10_write_char_tty
    cmp	ah, 0x0f ; Get video mode
    je int10_get_vm

    iret

int10_get_vm:
    ; int 10, 0f
    ; On exit:
    ;  AH = number of screen columns
    ;  AL = mode currently active
    ;  BH = current display page

    push ds
    mov	ax, 0x40
    mov	ds, ax

    mov	ah, [BDA_NUM_COLUMNS]
    mov	al, [BDA_ACTIVE_VIDEOMODE]
    mov	bh, [BDA_ACTIVE_VIDEO_PAGE]

    pop	ds
    iret

int10_set_vm:
    ; AL contains the requested mode.
    ; CGA supports modes 0 to 6 only

    push ds
    push dx
    push cx
    push bx
    push ax

    push ax
    ; Get the bios data segment into ds
    mov	ax, 0x40
    mov	ds, ax

    ; Disable video out when changing modes
    mov	dx, 0x3d8
    mov	al, 0
    out	dx, al
    pop	ax

    mov	ah, al
    and	ax, 0x807f
    push ax

    ; Store the video mode. This if fixed later for invalid modes.
    mov	byte [BDA_ACTIVE_VIDEOMODE], al

    push	ax

    ; Set sequencer registers
    mov	ax, 0x03c4
    mov	dx, ax
    mov	al, 0x01
    out	dx, al
    mov	ax, 0x03c5
    mov	dx, ax
    mov	al, 0x01
    out	dx, al

    mov	ax, 0x03c4
    mov	dx, ax
    mov	al, 0x03
    out	dx, al
    mov	ax, 0x03c5
    mov	dx, ax
    mov	al, 0x00
    out	dx, al

    mov	ax, 0x03c4
    mov	dx, ax
    mov	al, 0x04
    out	dx, al
    mov	ax, 0x03c5
    mov	dx, ax
    mov	al, 0x04
    out	dx, al

    ; Set the Graphics mode register 3 to 0x00.
    mov	ax, 0x03ce
    mov	dx, ax
    mov	al, 0x03
    out	dx, al
    mov	ax, 0x03cf
    mov	dx, ax
    mov	al, 0x00
    out	dx, al

    ; Set the Graphics mode register 5 to 0x10.
    mov	ax, 0x03ce
    mov	dx, ax
    mov	al, 0x05
    out	dx, al
    mov	ax, 0x03cf
    mov	dx, ax
    mov	al, 0x10
    out	dx, al

    ; Set the Graphics mode register 8 to 0xff.
    mov	ax, 0x03ce
    mov	dx, ax
    mov	al, 0x08
    out	dx, al
    mov	ax, 0x03cf
    mov	dx, ax
    mov	al, 0xff
    out	dx, al

    pop	ax

    ; Check CGA video modes
    cmp	al, 0
    je .set_vm_0
    cmp	al, 1
    je .set_vm_1
    cmp	al, 2
    je .set_vm_2
    cmp	al, 3
    je .set_vm_3
    cmp	al, 4
    je .set_vm_4
    cmp	al, 5
    je .set_vm_5
    cmp	al, 6
    je .set_vm_6

    ; All other video modes set mode 3
    mov	al, 3
    mov	byte [BDA_ACTIVE_VIDEOMODE], al
    jmp	.set_vm_3

  .set_vm_0:
    ; BW40
    mov	word [BDA_ACTIVE_PAGE_SIZE], 0x0800
    mov	ax, 0x000c
    mov	bx, 40		; BX = video columns
    mov	cx, 0x0818	; CL = video rows, CH = scan lines per character
    jmp	.set_vm_write

  .set_vm_1:
    ; CO40
    mov	word [BDA_ACTIVE_PAGE_SIZE], 0x0800
    mov	ax, 0x0008
    mov	bx, 40		; BX = video columns
    mov	cx, 0x0818	; CL = video rows, CH = scan lines per character
    jmp	.set_vm_write

  .set_vm_2:
    ; BW80
    mov	word [BDA_ACTIVE_PAGE_SIZE], 0x1000
    mov	ax, 0x000d
    mov	bx, 80		; BX = video columns
    mov	cx, 0x0818	; CL = video rows, CH = scan lines per character
    jmp	.set_vm_write

  .set_vm_3:
    ; CO80
    mov	word [BDA_ACTIVE_PAGE_SIZE], 0x1000
    mov	ax, 0x0009
    mov	bx, 80		; BX = video columns
    mov	cx, 0x0818	; CL = video rows, CH = scan lines per character
    jmp	.set_vm_write

  .set_vm_4:
    ; 320x200, 4 colour
    mov	word [BDA_ACTIVE_PAGE_SIZE], 0x4000
    mov	ax, 0x000a
    mov	bx, 40		; BX = video columns
    mov	cx, 0x0818	; CL = video rows, CH = scan lines per character
    jmp	.set_vm_write

  .set_vm_5:
    ; 320x200 grey
    mov	word [BDA_ACTIVE_PAGE_SIZE], 0x4000
    mov	ax, 0x000e
    mov	bx, 40		; BX = video columns
    mov	cx, 0x0818	; CL = video rows, CH = scan lines per character
    jmp	.set_vm_write

  .set_vm_6:
    ; 640x200 mono

    ; Set int 43 to the 8x8 single dot char table
    push	es
    mov	ax, 0
    mov	es, ax
    mov	cx, cga_glyphs
    mov	word [es:4*0x43], cx
    mov	cx, 0xf000
    mov	word [es:4*0x43 + 2], cx
    pop	es

    mov	word [BDA_ACTIVE_PAGE_SIZE], 0x4000
    mov	ax, 0x071a
    mov	bx, 80		; BX = video columns
    mov	cx, 0x0818	; CL = video rows, CH = scan lines per character

  .set_vm_write:
    ; CGA video modes
    ; write the video data to the video mode registers and bios data area
    ; al = port 0x3d8 value, ah = port 0x3d9 value
    ; bx = video text columns

    xchg ah, al
    mov	dx, 0x3d9
    out	dx, al
    mov	byte [BDA_COLOR_PALETTE], al

    xchg	ah, al
    mov	dx, 0x3d8
    out	dx, al
    mov	byte [BDA_ADAPTER_INTERNAL_MODE], al

  .set_vm_upd:
    mov	word [BDA_NUM_COLUMNS], bx
    mov	word [BDA_NUM_ROWS], cx
    mov	byte [BDA_ACTIVE_VIDEO_PAGE], 0x00
    mov	word [BDA_ACTIVE_PAGE_OFFSET], 0x0000

    ; Set all page cursors back to 0, 0
    mov	word [BDA_CURSOR_X], 0
    mov	word [BDA_CURSOR_X+2], 0
    mov	word [BDA_CURSOR_X+4], 0
    mov	word [BDA_CURSOR_X+6], 0
    mov	word [BDA_CURSOR_X+8], 0
    mov	word [BDA_CURSOR_X+10], 0
    mov	word [BDA_CURSOR_X+12], 0
    mov	word [BDA_CURSOR_X+14], 0

  .set_vm_cls:
    pop	ax
    cmp	ah, 0x80
    je .set_vm_nocls

    mov	bh, 7		        ; Black background, white foreground
    call clear_screen

  .set_vm_nocls:
    pop	ax
    pop	bx
    pop	cx
    pop	dx
    pop	ds
    iret

int10_set_disp_page:
    ; On entry:
    ;   AL = page number

    push ds
    push si
    push dx
    push bx	
    push ax

    ; SI = display page cursor pos offset
    mov	bl, al
    mov	bh, 0
    shl	bx, 1
    mov	si, bx

    mov	bx, 0x40
    mov	ds, bx

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x00
    je .set_disp_page_t40

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x01
    je .set_disp_page_t40

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x02
    je .set_disp_page_t80

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x03
    je .set_disp_page_t80

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x04
    je .set_disp_page_done

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x05
    je .set_disp_page_done

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x06
    je .set_disp_page_done

    jmp	.set_disp_page_done

  .set_disp_page_t40:
    cmp	al, 8
    jge	.set_disp_page_done

    mov	[BDA_ACTIVE_VIDEO_PAGE], al
    mov	bl, 0x08
    mul	byte bl
    mov	bh, al
    mov	bl, 0

    jmp	.set_disp_page_upd

  .set_disp_page_t80:
    cmp	al, 8
    jge	.set_disp_page_done	

    mov	[BDA_ACTIVE_VIDEO_PAGE], al
    mov	bl, 0x10
    mul	byte bl
    mov	bh, al
    mov	bl, 0

  .set_disp_page_upd:
    ; bx contains page offset, so store it in the BIOS data area
    mov	[BDA_ACTIVE_PAGE_OFFSET], bx

    ; update CRTC page offset
    mov	dx, 0x03d4
    mov	al, 0x0c
    out	dx, al

    mov	dx, 0x03d5
    mov	al, bh
    out	dx, al

    mov	dx, 0x03d4
    mov	al, 0x0d
    out	dx, al

    mov	dx, 0x03d5
    mov	al, bl
    out	dx, al
    
    ; update CRTC cursor pos
    mov	dl, [si+BDA_CURSOR_X]
    mov	[BDA_CRT_CURSOR_X], dl
    mov	dl, [si+BDA_CURSOR_Y]
    mov	[BDA_CRT_CURSOR_Y], dl
    call set_crtc_cursor_pos

  .set_disp_page_done:
    pop	ax
    pop	bx
    pop	dx
    pop	si
    pop	ds
    iret

int10_scrollup:
    ; AL = number of lines by which to scroll up (00h = clear entire window)
    ; BH = attribute used to write blank lines at bottom of window
    ; CH,CL = row,column of window's upper left corner
    ; DH,DL = row,column of window's lower right corner

    push	bp

    push ax
    push ds
    mov	ax, 0x40
    mov	ds, ax
    mov	bp, [BDA_ACTIVE_PAGE_OFFSET]
    pop	ds
    pop	ax

    cmp	al, 0
    jne	.scroll_up_window

    call clear_window
    jmp	.scrollup_done

  .scroll_up_window:
    call scroll_up_window

  .scrollup_done:
    pop	bp
    iret
 
int10_scrolldown:
    ; AL = number of lines by which to scroll down (00h = clear entire window)
    ; BH = attribute used to write blank lines at bottom of window
    ; CH,CL = row,column of window's upper left corner
    ; DH,DL = row,column of window's lower right corner

	  push bp

    push ax
    push ds
    mov	ax, 0x40
    mov	ds, ax
    mov	bp, [BDA_ACTIVE_PAGE_OFFSET]
    pop	ds
    pop	ax

    cmp	al, 0
    jne	.scroll_down_window

    call clear_window
    jmp	.scrolldown_done

  .scroll_down_window:
	  call scroll_down_window

  .scrolldown_done:
    pop	bp
    iret

int10_set_cshape:
    ; CH = cursor start line (bits 0-4) and options (bits 5-7).
    ; CL = bottom cursor line (bits 0-4).

    push ds
    push ax
    push cx
    push dx

    mov	ax, 0x40
    mov	ds, ax

    mov	ax, cx
    and	ch, 01100000b
    cmp	ch, 00100000b
    jne	.cur_visible

  .cur_not_visible:
    mov	cx, ax
    jmp	.cur_vischk_done

  .cur_visible:
    ; Only store cursor shape if visible.
    and	ax, 0x1f1f
    mov	[BDA_CURSOR_SHAPE_END], ax
    or	ax, 0x6000
    mov	cx, ax
      
  .cur_vischk_done:
    ; Set CRTC registers for cursor shape
    mov	al, 0x0a
    mov	dx, 0x3d4
    out	dx, al

    mov	al, ch
    mov	dx, 0x3d5
    out	dx, al

    mov	al, 0x0b
    mov	dx, 0x3d4
    out	dx, al

    mov	al, cl
    mov	dx, 0x3d5
    out	dx, al

  .set_cshape_done:
    pop	dx
    pop	cx
    pop	ax
    pop	ds
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

    mov	[si+BDA_CURSOR_Y], dh
    mov	[si+BDA_CURSOR_X], dl

    cmp	bh, [BDA_ACTIVE_VIDEO_PAGE]
    jne	.set_cursor_done

    mov	[BDA_CRT_CURSOR_Y], dh
    mov	[BDA_CRT_CURSOR_X], dl
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

	  mov	cx, [BDA_CURSOR_SHAPE_END]

	  cmp	bh, 0x07
	  jg .get_cursor_done

    push bx
    mov	bl, bh
    mov	bh, 0
    shl	bx, 1
    mov	si, bx
    pop	bx

    mov	dh, [si+BDA_CURSOR_Y]
    mov	dl, [si+BDA_CURSOR_X]

  .get_cursor_done:
    pop	ax
    pop	si
    pop	ds
    iret

int10_charatcur:
    ; This returns the character at the cursor.
    ;   BH = display page
    ; On exit:
    ;   AH, AL = attribute code, ASCII character code
    
    push ds
    push es
    push si
    push di
    push bx

    mov	ax, 0x40
    mov	ds, ax

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x00
    je .charatcur_t40
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x01
    je .charatcur_t40
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x02
    je .charatcur_t80
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x03
    je .charatcur_t80

    ; Can't do this in graphics mode
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x04
    jge	.charatcur_done

  .charatcur_t40:
    cmp	bh, 0x07
    jg .charatcur_done
    jmp	.charatcur_page_ok

  .charatcur_t80:
    cmp	bh, 0x07
    jg .charatcur_done

  .charatcur_page_ok:
    ; Get bios data cursor position offset for this page into SI
    push bx
    mov	bl, bh
    mov	bh, 0
    shl	bx, 1
    mov	si, bx
    pop	bx

    ; Get video page offset into DI
    mov	al, [BDA_ACTIVE_PAGE_SIZE+1]
    mul	byte bh
    mov	ah, al
    mov	al, 0
    mov	di, ax

    ; Get video RAM offset for the cursor position into BX
    mov	al, [si+BDA_CURSOR_Y]
    mul	byte [BDA_NUM_COLUMNS]
    add	al, [si+BDA_CURSOR_X]
    adc	ah, 0
    shl	ax, 1
    mov	bx, ax

    ; Get video RAM segment into ES
    mov	ax, 0xb800
    mov	es, ax

    mov	ax, [es:bx+di]

  .charatcur_done:
    pop	bx
    pop	di
    pop	si
    pop	es
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

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x00
    je .write_char_t40
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x01
    je .write_char_t40
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x02
    je .write_char_t80
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x03
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
    mov	al, [BDA_ACTIVE_PAGE_SIZE+1]
    mul	byte bh
    mov	ah, al
    mov	al, 0
    mov	bp, ax

    ; Get offset for the cursor position within the page into DI
    mov	al, [si+BDA_CURSOR_Y]
    mul	byte [BDA_NUM_COLUMNS]
    add	al, [si+BDA_CURSOR_X]
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

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 4
    je .write_char_cga320
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 5
    je .write_char_cga320
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 6
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

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x0
    je .write_char_tty_t40
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x1
    je .write_char_tty_t40
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x2
    je .write_char_tty_t80
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x3
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
    mov	al, [BDA_ACTIVE_PAGE_SIZE+1]
    mul	byte bh
    mov	ah, al
    mov	al, 0
    mov	bp, ax

    ; Get offset for the cursor position within the page into DI
    mov	al, [si+BDA_CURSOR_Y]
    mul	byte [BDA_NUM_COLUMNS]
    add	al, [si+BDA_CURSOR_X]
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
    dec	byte [si+BDA_CURSOR_X]
    cmp	byte [si+BDA_CURSOR_X], 0
    jg .write_char_tty_set_crtc_cursor

    mov	byte [si+BDA_CURSOR_X], 0    
    jmp	.write_char_tty_set_crtc_cursor

  .write_char_tty_nl:
    mov	byte [si+BDA_CURSOR_X], 0
    inc	byte [si+BDA_CURSOR_Y]

    mov	cl, [BDA_NUM_ROWS]
    cmp	byte [si+BDA_CURSOR_Y], cl
    jbe	.write_char_tty_done
    mov	byte [si+BDA_CURSOR_Y], cl

    push ax
    push cx
    push dx

    mov al, 1
    mov	bx, 0x0700
    mov	cx, 0
    mov	dh, [BDA_NUM_ROWS]
    mov	dl, [BDA_NUM_COLUMNS]
    dec	dl

    call scroll_up_window

    pop	dx
    pop	cx
    pop	ax

    jmp	.write_char_tty_set_crtc_cursor

  .write_char_tty_cr:
    mov	byte [si+BDA_CURSOR_X],0
    jmp	.write_char_tty_set_crtc_cursor

  .write_char_tty_printable:
    cmp byte [BDA_ACTIVE_VIDEOMODE], 4
    je .write_char_tty_cga320
    cmp byte [BDA_ACTIVE_VIDEOMODE], 5
    je .write_char_tty_cga320
    cmp byte [BDA_ACTIVE_VIDEOMODE], 6
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
    inc	byte [si+BDA_CURSOR_X]
    mov	ah, [BDA_NUM_COLUMNS]
    cmp	byte [si+BDA_CURSOR_X], ah
    jge	.write_char_tty_nl

  .write_char_tty_set_crtc_cursor:
    cmp	bh, [BDA_ACTIVE_VIDEO_PAGE]
    jne	.write_char_tty_done

    mov	ax, [si+BDA_CURSOR_X]
    mov	[BDA_CRT_CURSOR_X], ax
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

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x0
    je .write_char_attrib_t40
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x1
    je .write_char_attrib_t40
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x2
    je .write_char_attrib_t80
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x3
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
    mov	al, [BDA_ACTIVE_PAGE_SIZE+1]
    mul	byte bh
    mov	ah, al
    mov	al, 0
    mov	bp, ax

    ; Get offset for the cursor position within the page into DI
    mov	al, [si+BDA_CURSOR_Y]
    mul	byte [BDA_NUM_COLUMNS]
    add	al, [si+BDA_CURSOR_X]
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

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 4
    je .write_char_attrib_cga320
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 5
    je .write_char_attrib_cga320
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 6
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

clear_window:
    ; BH = attribute used to write blank lines at bottom of window
    ; CH,CL = row,column of window's upper left corner
    ; DH,DL = row,column of window's lower right corner
    ; BP = video page offset

    push ax
    push bx
    push cx
    push dx

    push ds
    push es
    push di

    mov	ax, 0x40
    mov	ds, ax

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x04
    je .clear_window_gfx320
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x05
    je .clear_window_gfx320
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x06
    je .clear_window_gfx640

    push bx

    mov	bx, 0xb800
    mov	es, bx

	  pop	bx

    ; Get the byte offset of the top left corner of the window into DI	

    mov	ah, 0
    mov	al, [BDA_NUM_COLUMNS]	; ax = characters per row
    mul	ch	; ax = top row screen offset
    add	al, cl	
    adc	ah, 0	; ax = top left screen offset
    shl	ax, 1	; convert to byte address
    add	ax, bp	; add display page offset
    
    mov	di, ax

    ; bl = number of rows in the window to clear
    mov	bl, dh
    sub	bl, ch
    inc	bl

    ; cx = number of words to write
    mov	ax, 0
    mov	al, dl
    sub	al, cl
    inc	al
    mov	cx, ax

    ; ax = clear screen value
    mov	al, 0
    mov	ah, bh

    cld

  .clear_window_1_row:
    cmp	bl, 0
    je .clear_window_done

    push cx
    push di

	  rep	stosw

    pop	di
    add	di, [BDA_NUM_COLUMNS]
    add	di, [BDA_NUM_COLUMNS]

	  pop	cx

	  dec	bl
	  jmp	.clear_window_1_row

  .clear_window_gfx320:
    ; Get the address offset in video ram for the top left character into DI
    mov	al, 80 ; bytes per row
    mul	byte ch
    shl	ax, 1
    shl	ax, 1
    add	al, cl
    adc	ah, 0
    add	al, cl
    adc	ah, 0
    mov	di, ax

    ; bl = number of rows in the window to clear
    mov	bl, dh
    sub	bl, ch
    shl	bl, 1
    shl	bl, 1

    ; cx = number of words to clear
    push	ax
    mov	ax, 0
    mov	al, dl
    sub	al, cl
    inc	al
    mov	cx, ax
    pop	ax

    cld

  .clear_window_gfx320_loop:
    push cx
    push di
    mov	ax, 0xb800
    mov	es, ax
    mov	ax, 0
    rep	stosw
    pop	di
    pop	cx

    push cx
    push di
    mov	ax, 0xba00
    mov	es, ax
    mov	ax, 0
    rep	stosw
    pop	di
    pop	cx

	  add	di, 80

    dec	bl
    jnz	.clear_window_gfx320_loop

	  jmp	.clear_window_done

  .clear_window_gfx640:
    ; Get the address offset in video ram for the top left character into DI
    mov	al, 80 ; bytes per row
    mul	byte ch
    shl	ax, 1
    shl	ax, 1
    add	al, cl
    adc	ah, 0
    mov	di, ax

    ; bl = number of rows in the window to clear
    mov	bl, dh
    sub	bl, ch
    shl	bl, 1
    shl	bl, 1

    ; cx = number of bytes to clear
    mov	ax, 0
    mov	al, dl
    sub	al, cl
    inc	al
    mov	cx, ax

    cld

  .clear_window_gfx640_loop:
    push cx
    push di
    mov	ax, 0xb800
    mov	es, ax
    mov	ax, 0
    rep	stosb
    pop	di
    pop	cx

    push cx
    push di
    mov	ax, 0xba00
    mov	es, ax
    mov	ax, 0
    rep	stosb
    pop	di
    pop	cx

    add	di, 80

    dec	bl
    jnz	.clear_window_gfx640_loop

  .clear_window_done:
    pop	di
    pop	es
    pop	ds

    pop	dx
    pop	cx
    pop	bx
    pop	ax
    ret

scroll_up_window:
    ; AL = number of lines by which to scroll up
    ; BH = attribute used to write blank lines at bottom of window
    ; CH,CL = row,column of window's upper left corner
    ; DH,DL = row,column of window's lower right corner 
    ; BP = video page offset

    push ax
    push bx
    push cx
    push dx

    push ds
    push es
    push di
    push si

    push ax
    mov	ax, 0x40
    mov	ds, ax
    pop	ax

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x04
    je .scroll_up_gfx320
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x05
    je .scroll_up_gfx320
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x06
    je .scroll_up_gfx640

    ; Get screen RAM address into es and ds
    push ax

    mov	ax, 0xb800
    mov	es, ax
    mov	ds, ax

    pop	ax

	  ; Get the first and second window row offsets into DI and SI
  	push ax
	  push ds

    mov	ax, 0x40
    mov	ds, ax

    mov	ah, 0
    mov	al, [BDA_NUM_COLUMNS]	; ax = characters per row
    mul	ch	; ax = window top row screen word offset
    add	al, cl	
    adc	ah, 0	; ax = window top left screen word offset
    shl	ax, 1	; convert to byte address
   	add	ax, bp  ; Add video page offset

    mov	di, ax	
    add	al, [BDA_NUM_COLUMNS]
    adc	ah, 0
    add	al, [BDA_NUM_COLUMNS]
    adc	ah, 0
    mov	si, ax

    pop	ds
    pop	ax

    ; bl = number of rows in the window to scroll
    mov bl, dh
    sub bl, ch

    ; cx = number of words to move
    push ax
    mov	ax, 0
    mov	al, dl
    sub	al, cl
    inc	al
    mov	cx, ax
    pop	ax

    cld

  .scroll_up_1_row:
    cmp	al, 0
    je .scroll_up_done

    push di
    push si
    push bx

  .scroll_up_loop:
    cmp bl, 0
    je .scroll_up_1_row_done

    push cx
    push di
    push si

	  rep	movsw

    pop	si
    pop	di
    pop	cx

    push ax
    push ds

    mov	ax, 0x40
    mov	ds, ax

    mov	ax, si
    add	al, [BDA_NUM_COLUMNS]
    adc	ah, 0
    add	al, [BDA_NUM_COLUMNS]
    adc	ah, 0
    mov	si, ax

    mov	ax, di
    add	al, [BDA_NUM_COLUMNS]
    adc	ah, 0
    add	al, [BDA_NUM_COLUMNS]
    adc	ah, 0
    mov	di, ax

    pop	ds
    pop	ax

	  dec	bl
	  jmp	.scroll_up_loop

  .scroll_up_1_row_done:

	  ; fill the last row with the bh attribute, null character
    push ax
    push cx
    mov	al, 0
    mov	ah, bh
    rep	stosw
    pop	cx
    pop	ax

    pop	bx
    pop	si
    pop	di

    dec	al
    jmp	.scroll_up_1_row

  .scroll_up_gfx320:

	  ; Get the first and second window row offsets into DI and SI
  	push ax

    ; Get the address offset in video ram for the top left character into DI
    mov	al, 80 ; bytes per row
    mul	byte ch
    shl	ax, 1
    shl	ax, 1
    add	al, cl
    adc	ah, 0
    add	al, cl
    adc	ah, 0
    mov	di, ax
    
    add	ax, 320
    mov	si, ax

    pop	ax

    ; bl = number of rows in the window to scroll
    mov bl, dh
    sub bl, ch
    shl	bl, 1
    shl	bl, 1

    ; cx = number of words to move
    push ax
    mov	ax, 0
    mov	al, dl
    sub	al, cl
    inc	al
    mov	cx, ax
    pop	ax

    cld

  .scroll_up_gfx320_1_row:
    cmp	al, 0
    je .scroll_up_done

    push di
    push si
    push bx

  .scroll_up_gfx320_loop:
	  cmp bl, 0
	  je .scroll_up_gfx320_1_row_done

    push ax

    push cx
    push di
    push si

    mov	ax, 0xb800
    mov	ds, ax
    mov	es, ax
    rep	movsw

    pop	si
    pop	di
    pop	cx

    push cx
    push di
    push si

    mov	ax, 0xba00
    mov	ds, ax
    mov	es, ax
    rep	movsw

    pop	si
    pop	di
    pop	cx

    add	si, 80
    add	di, 80

	  pop	ax

	  dec	bl
	  jmp	.scroll_up_gfx320_loop

  .scroll_up_gfx320_1_row_done:

    ; fill the last row with zeros
    push ax
    push bx
    mov	bl, 4

  .scroll_up_gfx320_last_row_loop:
    push cx
    push di
    mov	ax, 0xb800
    mov	es, ax
    mov	ax, 0
    rep	stosw
    pop	di
    pop	cx

    push cx
    push di
    mov	ax, 0xba00
    mov	es, ax
    mov	ax, 0
    rep	stosw
    pop	di
    pop	cx

	  add	di, 80

    dec	bl
    jnz	.scroll_up_gfx320_last_row_loop

    pop	bx
    pop	ax

    pop	bx
    pop	si
    pop	di

    dec	al
    jmp	.scroll_up_gfx320_1_row

  .scroll_up_gfx640:

	  ; Get the first and second window row offsets into DI and SI
  	push ax

    ; Get the address offset in video ram for the top left character into DI
    mov	al, 80 ; bytes per row
    mul	byte ch
    shl	ax, 1
    shl	ax, 1
    add	al, cl
    adc	ah, 0
    mov	di, ax
    
    add	ax, 320
    mov	si, ax

    pop	ax

    ; bl = number of rows in the window to scroll
    mov bl, dh
    sub bl, ch
    shl	bl, 1
    shl	bl, 1

    ; cx = number of bytes to move
    push ax
    mov	ax, 0
    mov	al, dl
    sub	al, cl
    inc	al
    mov	cx, ax
    pop	ax

    cld

  .scroll_up_gfx640_1_row:
    cmp	al, 0
    je .scroll_up_done

    push di
    push si
    push bx

  .scroll_up_gfx640_loop:
    cmp bl, 0
    je .scroll_up_gfx640_1_row_done

    push	ax

    push 	cx
    push 	di
    push	si

    mov	ax, 0xb800
    mov	ds, ax
    mov	es, ax
    rep	movsb

    pop	si
    pop	di
    pop	cx

    push 	cx
    push 	di
    push	si

    mov	ax, 0xba00
    mov	ds, ax
    mov	es, ax
    rep	movsb

    pop	si
    pop	di
    pop	cx

    add	si, 80
    add	di, 80

    pop	ax

    dec	bl
    jmp	.scroll_up_gfx640_loop

  .scroll_up_gfx640_1_row_done:

    ; fill the last row with zeros
    push ax
    push bx
    mov	bl, 4

  .scroll_up_gfx640_last_row_loop:
    push cx
    push di
    mov	ax, 0xb800
    mov	es, ax
    mov	ax, 0
    rep	stosb
    pop	di
    pop	cx

    push cx
    push di
    mov	ax, 0xba00
    mov	es, ax
    mov	ax, 0
    rep	stosb
    pop	di
    pop	cx

    add	di, 80

    dec	bl
    jnz	.scroll_up_gfx640_last_row_loop

    pop	bx
    pop	ax

    pop	bx
    pop	si
    pop	di

	  dec	al
	  jmp	.scroll_up_gfx640_1_row

  .scroll_up_done:
    pop	si
    pop	di
    pop	es
    pop	ds

    pop	dx
    pop	cx
    pop	bx
    pop	ax
    ret

scroll_down_window:
    ; AL = number of lines by which to scroll down (00h = clear entire window)
    ; BH = attribute used to write blank lines at top of window
    ; CH,CL = row,column of window's upper left corner
    ; DH,DL = row,column of window's lower right corner
    ; BP = video page offset

    push ax
    push bx
    push cx
    push dx

    push ds
    push es
    push di
    push si

    push ax
    mov	ax, 0x40
    mov	ds, ax
    pop	ax

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x04
    je .scroll_dn_gfx320
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x05
    je .scroll_dn_gfx320
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 0x06
    je .scroll_dn_gfx640

    ; Get screen RAM address into es and ds
    push ax

    mov	ax, 0xb800
    mov	es, ax
    mov	ds, ax

    pop	ax

    ; Get the last and second last window row offsets into DI and SI
    push ax
    push ds

    mov	ax, 0x40
    mov	ds, ax

    mov	ah, 0
    mov	al, [BDA_NUM_COLUMNS]	; ax = bytes per row
    mul	dh	; ax = window bottom row screen word offset
    add	al, dl	
    adc	ah, 0	; ax = window bottom right screen word offset
    shl	ax, 1	; convert to byte address
    add	ax, bp  ; add video page offset

    mov	di, ax	
    sub	al, [BDA_NUM_COLUMNS]
    sbb	ah, 0
    sub	al, [BDA_NUM_COLUMNS]
    sbb	ah, 0
    mov	si, ax

    pop	ds
    pop	ax

    ; bl = number of rows in the window to scroll
    mov	bl, dh
    sub	bl, ch

    ; cx = number of words to move
    push ax
    mov	ax, 0
    mov	al, dl
    sub	al, cl
    inc	al
    mov	cx, ax
    pop	ax

    std

  .scroll_down_1_row:
    cmp	al, 0
    je .scroll_down_done

    push di
    push si
    push bx

  .scroll_down_loop:
    cmp bl, 0
    je .scroll_down_1_row_done

    push cx
    push di
    push si

    rep movsw

    pop	si
    pop	di
    pop	cx

    push ax
    push ds

    mov	ax, 0x40
    mov	ds, ax

    mov	ax, si
    sub	al, [BDA_NUM_COLUMNS]
    sbb	ah, 0
    sub	al, [BDA_NUM_COLUMNS]
    sbb	ah, 0
    mov	si, ax

    mov	ax, di
    sub	al, [BDA_NUM_COLUMNS]
    sbb	ah, 0
    sub	al, [BDA_NUM_COLUMNS]
    sbb	ah, 0
    mov	di, ax

    pop	ds
    pop	ax

    dec	bl
    jmp	.scroll_down_loop

  .scroll_down_1_row_done:

    ; fill the last row with the bh attribute, null character
    push ax
    push cx
    mov	al, 0
    mov	ah, bh
    rep	stosw
    pop	cx
    pop	ax

    pop	bx
    pop	si
    pop	di

    dec	al
    jmp	.scroll_down_1_row

  .scroll_dn_gfx320:

    ; Get the last and second last window row offsets into DI and SI
    push ax

    ; Get the address offset in video ram for the top left character into DI
    mov	al, 80 ; bytes per row
    mul	byte dh
    shl	ax, 1
    shl	ax, 1
    add	al, dl
    adc	ah, 0
    add	al, dl
    adc	ah, 0
    add	ax, 240
    mov	di, ax
    
    sub	ax, 320
    mov	si, ax

    pop	ax

    ; bl = number of rows in the window to scroll
    mov	bl, dh
    sub	bl, ch
    shl	bl, 1
    shl	bl, 1

    ; cx = number of words to move
    push ax
    mov	ax, 0
    mov	al, dl
    sub	al, cl
    inc	al
    mov	cx, ax
    pop	ax

    std

  .scroll_dn_gfx320_1_row:
    cmp	al, 0
    je .scroll_down_done

    push	di
    push	si
    push	bx

  .scroll_dn_gfx320_loop:
    cmp bl, 0
    je .scroll_dn_gfx320_1_row_done

    push ax

    push cx
    push di
    push si

    mov	ax, 0xb800
    mov	ds, ax
    mov	es, ax
    rep	movsw

    pop	si
    pop	di
    pop	cx

    push cx
    push di
    push si

    mov	ax, 0xba00
    mov	ds, ax
    mov	es, ax
    rep	movsw

    pop	si
    pop	di
    pop	cx

    sub	si, 80
    sub	di, 80

    pop	ax

    dec	bl
    jmp	.scroll_dn_gfx320_loop

  .scroll_dn_gfx320_1_row_done:

    ; fill the last row with zeros
    push ax
    push bx
    mov	bl, 4

  .scroll_dn_gfx320_last_row_loop:
    push cx
    push di
    mov	ax, 0xb800
    mov	es, ax
    mov	ax, 0
    rep	stosw
    pop	di
    pop	cx

    push cx
    push di
    mov	ax, 0xba00
    mov	es, ax
    mov	ax, 0
    rep	stosw
    pop	di
    pop	cx

    sub	di, 80

    dec	bl
    jnz	.scroll_dn_gfx320_last_row_loop

    pop	bx
    pop	ax

    pop	bx
    pop	si
    pop	di

    dec	al
    jmp	.scroll_dn_gfx320_1_row

  .scroll_dn_gfx640:

    ; Get the first and second window row offsets into DI and SI
    push ax

    ; Get the address offset in video ram for the bottom right character into DI
    mov	al, 80 ; bytes per row
    mul	byte dh
    shl	ax, 1
    shl	ax, 1
    add	al, dl
    adc	ah, 0
    add	ax, 240
    mov	di, ax
    
    sub	ax, 320
    mov	si, ax

    pop	ax

    ; bl = number of rows in the window to scroll
    mov	bl, dh
    sub	bl, ch
    shl	bl, 1
    shl	bl, 1

    ; cx = number of bytes to move
    push ax
    mov	ax, 0
    mov	al, dl
    sub	al, cl
    inc	al
    mov	cx, ax
    pop	ax

    std

  .scroll_dn_gfx640_1_row:
    cmp	al, 0
    je .scroll_down_done

    push di
    push si
    push bx

  .scroll_dn_gfx640_loop:
    cmp bl, 0
    je .scroll_dn_gfx640_1_row_done

    push ax

    push cx
    push di
    push si

    mov	ax, 0xb800
    mov	ds, ax
    mov	es, ax
    rep	movsb

    pop	si
    pop	di
    pop	cx

    push cx
    push di
    push si

    mov	ax, 0xba00
    mov	ds, ax
    mov	es, ax
    rep	movsb

    pop	si
    pop	di
    pop	cx

    sub	si, 80
    sub	di, 80

    pop	ax

    dec	bl
    jmp	.scroll_dn_gfx640_loop

  .scroll_dn_gfx640_1_row_done:

    ; fill the last row with zeros
    push ax
    push bx
    mov	bl, 4

  .scroll_dn_gfx640_last_row_loop:
    push cx
    push di
    mov	ax, 0xb800
    mov	es, ax
    mov	ax, 0
    rep	stosb
    pop	di
    pop	cx

    push cx
    push di
    mov	ax, 0xba00
    mov	es, ax
    mov	ax, 0
    rep	stosb
    pop	di
    pop	cx

    sub	di, 80

    dec	bl
    jnz	.scroll_dn_gfx640_last_row_loop

    pop	bx
    pop	ax

    pop	bx
    pop	si
    pop	di

    dec	al
    jmp	.scroll_dn_gfx640_1_row
      
  .scroll_down_done:
    pop	si
    pop	di
    pop	es
    pop	ds

    pop	dx
    pop	cx
    pop	bx
    pop	ax
    ret

; CRTC cursor position helper
set_crtc_cursor_pos:
    push es
    push ax
    push bx
    push dx

    mov	ax, 0x40
    mov	es, ax

    mov	bh, [es:BDA_CRT_CURSOR_Y]
    mov	bl, [es:BDA_CRT_CURSOR_X]

    ; Set CRTC cursor address
    mov	al, [es:BDA_NUM_COLUMNS]
    mul	bh
    add	al, bl
    adc	ah, 0
    mov	bx, ax

    mov	al, 0x0e
    mov	dx, 0x3d4
    out	dx, al

    mov	al, bh
    mov	dx, 0x3d5
    out	dx, al

    mov	al, 0x0f
    mov	dx, 0x3d4
    out	dx, al

    mov	al, bl
    mov	dx, 0x3d5
    out	dx, al

    pop	dx
    pop	bx
    pop	ax
    pop	es

    ret

clear_screen:
    ; Clear video memory with attribute in BH

    push ax
    push bx
    push cx
    push ds
    push es
    push di
    push si

    mov	ax, 0x40
    mov	ds, ax

    mov	al, [BDA_ACTIVE_VIDEO_PAGE]
    mov	ah, 0
    shl	ax, 1
    mov	si, ax

    mov	byte [si+BDA_CURSOR_X], 0
    mov	byte [BDA_CRT_CURSOR_X], 0
    mov	byte [si+BDA_CURSOR_Y], 0
    mov	byte [BDA_CRT_CURSOR_Y], 0

    cmp	byte [BDA_ACTIVE_VIDEOMODE], 4
    je .clear_gfx
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 5
    je .clear_gfx
    cmp	byte [BDA_ACTIVE_VIDEOMODE], 6
    je .clear_gfx

    cld
    mov	al, [BDA_ACTIVE_PAGE_SIZE+1]
    mul	byte [BDA_ACTIVE_VIDEO_PAGE]
    mov	ah, al
    mov	al, 0
    shr	ax, 1
    shr	ax, 1
    shr	ax, 1
    shr	ax, 1
    add	ax, 0xb800
    mov	es, ax
    mov	di, 0
    mov	al, 0
    mov	ah, bh
    mov	cx, [BDA_ACTIVE_PAGE_SIZE]
    rep	stosw
    jmp	.clear_done

  .clear_gfx:
    cld
    mov	ax, 0xb800
    mov	es, ax
    mov	di, 0
    mov	ax, 0
    mov	cx, 4000
    rep	stosw

    mov	ax, 0xba00
    mov	es, ax
    mov	di, 0
    mov	ax, 0
    mov	cx, 4000
    rep	stosw

  .clear_done:
    call set_crtc_cursor_pos

    pop	si
    pop	di
    pop	es
    pop	ds
    pop	cx
    pop	bx
    pop	ax

    ret

put_cga320_char:
    ; Character is in AL
    ; Colour is in AH
    
    push ax
    push bx
    push cx
    push ds	
    push es
    push di
    push bp

    ; Get the colour mask into BH
    cmp	ah, 1
    jne	.put_cga320_char_c2
    mov	bh, 0x55
    jmp	.put_cga320_char_cdone
    
  .put_cga320_char_c2:
    cmp	ah, 2
    jne	.put_cga320_char_c3
    mov	bh, 0xAA
    jmp	.put_cga320_char_cdone
  
  .put_cga320_char_c3:
    mov	bh, 0xFF

  .put_cga320_char_cdone:
    ; Get glyph character top offset into bp and character segment into cx
    test al, 0x80
    jne	.put_cga320_char_high

    ; Characters 0 .. 127 are always in ROM
    mov	ah, 0
    shl	ax, 1
    shl	ax, 1
    shl	ax, 1
    add	ax, cga_glyphs
    mov	bp, ax

    mov	cx, cs

    jmp	.put_cga320_char_vidoffset

  .put_cga320_char_high:
    ; Characters 128 .. 255 get their address from interrupt vector 1F
    and	al, 0x7F
    mov	ah, 0
    shl	ax, 1
    shl	ax, 1
    shl	ax, 1
    mov	bp, ax

    mov	ax, 0
    mov	ds, ax
    mov	ax, [ds:0x7c]
    add	bp, ax 

    mov	cx, [ds:0x7e]

  .put_cga320_char_vidoffset:
    mov	ax, 0x40
    mov	ds, ax

    ; Get the address offset in video ram for the top of the character into DI
    mov	al, 80 ; bytes per row
    mul	byte [ds:BDA_CURSOR_Y]
    shl	ax, 1
    shl	ax, 1
    add	al, [ds:BDA_CURSOR_X]
    adc	ah, 0
    add	al, [ds:BDA_CURSOR_X]
    adc	ah, 0
    mov	di, ax

    ; get segment for character data into ds
    mov	ds, cx

    ; get video RAM address for even lines into es
    mov	ax, 0xb800
    mov	es, ax

    push di

    mov	bl, byte [ds:bp]
    ; Translate character glyph into CGA 320 format
    call put_cga320_char_double
    stosw
    add	di, 78

    mov	bl, byte [ds:bp+2]
    ; Translate character glyph into CGA 320 format
    call put_cga320_char_double
    stosw
    add	di, 78

    mov	bl, byte [ds:bp+4]
    ; Translate character glyph into CGA 320 format
    call put_cga320_char_double
    stosw
    add	di, 78

    mov	bl, byte [ds:bp+6]
    ; Translate character glyph into CGA 320 format
    call put_cga320_char_double
    stosw

    ; get video RAM address for odd lines into es
    mov ax, 0xba00
    mov es, ax

    pop	di

    mov	bl, byte [ds:bp+1]
    ; Translate character glyph into CGA 320 format
    call put_cga320_char_double
    stosw
    add	di, 78

    mov	bl, byte [ds:bp+3]
    ; Translate character glyph into CGA 320 format
    call put_cga320_char_double
    stosw
    add	di, 78

    mov	bl, byte [ds:bp+5]
    ; Translate character glyph into CGA 320 format
    call put_cga320_char_double
    stosw
    add	di, 78

    mov	bl, byte [ds:bp+7]
    ; Translate character glyph into CGA 320 format
    call put_cga320_char_double
    stosw	

  .put_cga320_char_done:
    pop	bp
    pop	di
    pop	es
    pop	ds
    pop	cx
    pop	bx
    pop	ax
    ret

put_cga320_char_double:
    ; BL = character bit pattern
    ; BH = colour mask
    ; AX is set to double width character bit pattern

    mov	ax, 0
    test bl, 0x80
    je .put_chachar_bit6
    or al, 0xc0
  .put_chachar_bit6:
    test bl, 0x40
    je .put_chachar_bit5
    or al, 0x30
  .put_chachar_bit5:
    test bl, 0x20
    je .put_chachar_bit4
    or al, 0x0c
  .put_chachar_bit4:
    test bl, 0x10
    je .put_chachar_bit3
    or al, 0x03
  .put_chachar_bit3:
    test bl, 0x08
    je .put_chachar_bit2
    or ah, 0xc0
  .put_chachar_bit2:
    test bl, 0x04
    je .put_chachar_bit1
    or ah, 0x30
  .put_chachar_bit1:
    test bl, 0x02
    je .put_chachar_bit0
    or ah, 0x0c
  .put_chachar_bit0:
    test bl, 0x01
    je .put_chachar_done
    or ah, 0x03
  .put_chachar_done:
    and	al, bh
    and	ah, bh
    ret

put_cga640_char:
    ; Character is in AL

    push ax
    push bx
    push cx
    push ds	
    push es
    push di
    push bp

    ; Get glyph character top offset into bp and character segment into cx
    test al, 0x80
    jne	.put_cga640_char_high

    ; Characters 0 .. 127 are always in ROM
    mov	ah, 0
    shl	ax, 1
    shl	ax, 1
    shl	ax, 1
    add	ax, cga_glyphs
    mov	bp, ax

    mov	cx, cs

    jmp	.put_cga640_char_vidoffset

  .put_cga640_char_high:
    ; Characters 128 .. 255 get their address from interrupt vector 1F
    and	al, 0x7f
    mov	ah, 0
    shl	ax, 1
    shl	ax, 1
    shl	ax, 1
    mov	bp, ax

    mov	ax, 0
    mov	ds, ax
    mov	ax, [ds:0x7c]
    add	bp, ax 

    mov	cx, [ds:0x7e]

  .put_cga640_char_vidoffset:
    mov	ax, 0x40
    mov	ds, ax

    ; Get the address offset in video ram for the top of the character into DI
    mov	al, 80 ; bytes per row
    mul	byte [ds:BDA_CURSOR_Y]
    shl	ax, 1
    shl	ax, 1
    add	al, [ds:BDA_CURSOR_X]
    adc	ah, 0
    mov	di, ax

    ; get segment for character data into ds
    mov	ds, cx

    ; get video RAM address for even lines into ds
    mov	ax, 0xb800
    mov	es, ax

    push	di

    mov	al, byte [ds:bp]
    stosb
    add	di, 79

    mov	al, byte [ds:bp+2]
    stosb
    add	di, 79

    mov	al, byte [ds:bp+4]
    stosb
    add	di, 79

    mov	al, byte [ds:bp+6]
    stosb

    ; get video RAM address for odd lines into ds
    mov ax, 0xba00
    mov es, ax

    pop	di

    mov	al, byte [ds:bp+1]
    stosb
    add	di, 79

    mov	al, byte [ds:bp+3]
    stosb
    add	di, 79

    mov	al, byte [ds:bp+5]
    stosb
    add	di, 79

    mov	al, byte [ds:bp+7]
    stosb

    pop	bp
    pop	di
    pop	es
    pop	ds
    pop	cx
    pop	bx
    pop	ax
    ret

init_video:
    push ax
    push ds

    mov ax,0x40
    mov ds,ax               ; Point DS to the BDA

    mov ax,[BDA_MACHINE_WORD]
    and ax,0xFFCF
    or ax,0x20                        ; Set CGA support
    mov [BDA_MACHINE_WORD],ax

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
    ;mov ax,[INT10_OFFSET]             ; BIOS INT10 offset
    ;mov [es:BDA_INT10_OFFSET],ax      ; Save to our save position!
    ;mov ax,[INT10_SEGMENT]            ; BIOS INT10 segment
    ;mov [es:BDA_INT10_SEGMENT],ax     ; Save to our save position!

    ; Set up our own vector!
    mov word [INT10_OFFSET], int10_handler   ; Hook our
    mov [INT10_SEGMENT],cs                   ; INT10 interrupt vector!

    pop es
    pop ax
    pop ds
    ret

;;;;;;;;;;;;;;;;;; Data ;;;;;;;;;;;;;;;;;;

INT10_OFFSET equ 0x40
INT10_SEGMENT equ 0x42
BDA_INT10_OFFSET equ 0xAC
BDA_INT10_SEGMENT equ 0xAE

; Temp!
BDA_CRT_CURSOR_X equ 0xAC
BDA_CRT_CURSOR_Y equ 0xAE

BDA_MACHINE_WORD equ 0x10
BDA_ACTIVE_VIDEOMODE equ 0x49
BDA_NUM_COLUMNS equ 0x4A
BDA_ACTIVE_PAGE_SIZE equ 0x4C
BDA_ACTIVE_PAGE_OFFSET equ 4Eh
BDA_CURSOR_X equ 0x50
BDA_CURSOR_Y equ 0x51
BDA_CURSOR_SHAPE_END equ 0x60
BDA_CURSOR_SHAPE_START equ 0x61
BDA_ACTIVE_VIDEO_PAGE equ 0x62
BDA_ADAPTER_INTERNAL_MODE equ 0x65
BDA_COLOR_PALETTE equ 0x66
BDA_NUM_ROWS equ 0x84

cga_glyphs:  
  db	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x81, 0xa5, 0x81, 0xbd, 0x99, 0x81, 0x7e
  db	0x7e, 0xff, 0xdb, 0xff, 0xc3, 0xe7, 0xff, 0x7e, 0x6c, 0xfe, 0xfe, 0xfe, 0x7c, 0x38, 0x10, 0x00
  db	0x10, 0x38, 0x7c, 0xfe, 0x7c, 0x38, 0x10, 0x00, 0x38, 0x7c, 0x38, 0xfe, 0xfe, 0xd6, 0x10, 0x38
  db	0x10, 0x10, 0x38, 0x7c, 0xfe, 0x7c, 0x10, 0x38, 0x00, 0x00, 0x18, 0x3c, 0x3c, 0x18, 0x00, 0x00
  db	0xff, 0xff, 0xe7, 0xc3, 0xc3, 0xe7, 0xff, 0xff, 0x00, 0x3c, 0x66, 0x42, 0x42, 0x66, 0x3c, 0x00
  db	0xff, 0xc3, 0x99, 0xbd, 0xbd, 0x99, 0xc3, 0xff, 0x0f, 0x03, 0x05, 0x7d, 0x84, 0x84, 0x84, 0x78
  db	0x3c, 0x42, 0x42, 0x42, 0x3c, 0x18, 0x7e, 0x18, 0x3f, 0x21, 0x3f, 0x20, 0x20, 0x60, 0xe0, 0xc0
  db	0x3f, 0x21, 0x3f, 0x21, 0x23, 0x67, 0xe6, 0xc0, 0x18, 0xdb, 0x3c, 0xe7, 0xe7, 0x3c, 0xdb, 0x18
  db	0x80, 0xe0, 0xf8, 0xfe, 0xf8, 0xe0, 0x80, 0x00, 0x02, 0x0e, 0x3e, 0xfe, 0x3e, 0x0e, 0x02, 0x00
  db	0x18, 0x3c, 0x7e, 0x18, 0x18, 0x7e, 0x3c, 0x18, 0x24, 0x24, 0x24, 0x24, 0x24, 0x00, 0x24, 0x00
  db	0x7f, 0x92, 0x92, 0x72, 0x12, 0x12, 0x12, 0x00, 0x3e, 0x63, 0x38, 0x44, 0x44, 0x38, 0xcc, 0x78
  db	0x00, 0x00, 0x00, 0x00, 0x7e, 0x7e, 0x7e, 0x00, 0x18, 0x3c, 0x7e, 0x18, 0x7e, 0x3c, 0x18, 0xff
  db	0x10, 0x38, 0x7c, 0x54, 0x10, 0x10, 0x10, 0x00, 0x10, 0x10, 0x10, 0x54, 0x7c, 0x38, 0x10, 0x00
  db	0x00, 0x18, 0x0c, 0xfe, 0x0c, 0x18, 0x00, 0x00, 0x00, 0x30, 0x60, 0xfe, 0x60, 0x30, 0x00, 0x00
  db	0x00, 0x00, 0x40, 0x40, 0x40, 0x7e, 0x00, 0x00, 0x00, 0x24, 0x66, 0xff, 0x66, 0x24, 0x00, 0x00
  db	0x00, 0x10, 0x38, 0x7c, 0xfe, 0xfe, 0x00, 0x00, 0x00, 0xfe, 0xfe, 0x7c, 0x38, 0x10, 0x00, 0x00
  db	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x38, 0x10, 0x10, 0x00, 0x10, 0x00
  db	0x24, 0x24, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24, 0x24, 0x7e, 0x24, 0x7e, 0x24, 0x24, 0x00
  db	0x18, 0x3e, 0x40, 0x3c, 0x02, 0x7c, 0x18, 0x00, 0x00, 0x62, 0x64, 0x08, 0x10, 0x26, 0x46, 0x00
  db	0x30, 0x48, 0x30, 0x56, 0x88, 0x88, 0x76, 0x00, 0x10, 0x10, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00
  db	0x10, 0x20, 0x40, 0x40, 0x40, 0x20, 0x10, 0x00, 0x20, 0x10, 0x08, 0x08, 0x08, 0x10, 0x20, 0x00
  db	0x00, 0x44, 0x38, 0xfe, 0x38, 0x44, 0x00, 0x00, 0x00, 0x10, 0x10, 0x7c, 0x10, 0x10, 0x00, 0x00
  db	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x20, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00
  db	0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x00
  db	0x3c, 0x42, 0x46, 0x4a, 0x52, 0x62, 0x3c, 0x00, 0x10, 0x30, 0x50, 0x10, 0x10, 0x10, 0x7c, 0x00
  db	0x3c, 0x42, 0x02, 0x0c, 0x30, 0x42, 0x7e, 0x00, 0x3c, 0x42, 0x02, 0x1c, 0x02, 0x42, 0x3c, 0x00
  db	0x08, 0x18, 0x28, 0x48, 0xfe, 0x08, 0x1c, 0x00, 0x7e, 0x40, 0x7c, 0x02, 0x02, 0x42, 0x3c, 0x00
  db	0x1c, 0x20, 0x40, 0x7c, 0x42, 0x42, 0x3c, 0x00, 0x7e, 0x42, 0x04, 0x08, 0x10, 0x10, 0x10, 0x00
  db	0x3c, 0x42, 0x42, 0x3c, 0x42, 0x42, 0x3c, 0x00, 0x3c, 0x42, 0x42, 0x3e, 0x02, 0x04, 0x38, 0x00
  db	0x00, 0x10, 0x10, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x10, 0x10, 0x00, 0x00, 0x10, 0x10, 0x20
  db	0x08, 0x10, 0x20, 0x40, 0x20, 0x10, 0x08, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x7e, 0x00, 0x00
  db	0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10, 0x00, 0x3c, 0x42, 0x02, 0x04, 0x08, 0x00, 0x08, 0x00
  db	0x3c, 0x42, 0x5e, 0x52, 0x5e, 0x40, 0x3c, 0x00, 0x18, 0x24, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x00
  db	0x7c, 0x22, 0x22, 0x3c, 0x22, 0x22, 0x7c, 0x00, 0x1c, 0x22, 0x40, 0x40, 0x40, 0x22, 0x1c, 0x00
  db	0x78, 0x24, 0x22, 0x22, 0x22, 0x24, 0x78, 0x00, 0x7e, 0x22, 0x28, 0x38, 0x28, 0x22, 0x7e, 0x00
  db	0x7e, 0x22, 0x28, 0x38, 0x28, 0x20, 0x70, 0x00, 0x1c, 0x22, 0x40, 0x40, 0x4e, 0x22, 0x1e, 0x00
  db	0x42, 0x42, 0x42, 0x7e, 0x42, 0x42, 0x42, 0x00, 0x38, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00
  db	0x0e, 0x04, 0x04, 0x04, 0x44, 0x44, 0x38, 0x00, 0x62, 0x24, 0x28, 0x30, 0x28, 0x24, 0x63, 0x00
  db	0x70, 0x20, 0x20, 0x20, 0x20, 0x22, 0x7e, 0x00, 0x63, 0x55, 0x49, 0x41, 0x41, 0x41, 0x41, 0x00
  db	0x62, 0x52, 0x4a, 0x46, 0x42, 0x42, 0x42, 0x00, 0x18, 0x24, 0x42, 0x42, 0x42, 0x24, 0x18, 0x00
  db	0x7c, 0x22, 0x22, 0x3c, 0x20, 0x20, 0x70, 0x00, 0x3c, 0x42, 0x42, 0x42, 0x4a, 0x3c, 0x03, 0x00
  db	0x7c, 0x22, 0x22, 0x3c, 0x28, 0x24, 0x72, 0x00, 0x3c, 0x42, 0x40, 0x3c, 0x02, 0x42, 0x3c, 0x00
  db	0x7f, 0x49, 0x08, 0x08, 0x08, 0x08, 0x1c, 0x00, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x00
  db	0x41, 0x41, 0x41, 0x41, 0x22, 0x14, 0x08, 0x00, 0x41, 0x41, 0x41, 0x49, 0x49, 0x49, 0x36, 0x00
  db	0x41, 0x22, 0x14, 0x08, 0x14, 0x22, 0x41, 0x00, 0x41, 0x22, 0x14, 0x08, 0x08, 0x08, 0x1c, 0x00
  db	0x7f, 0x42, 0x04, 0x08, 0x10, 0x21, 0x7f, 0x00, 0x78, 0x40, 0x40, 0x40, 0x40, 0x40, 0x78, 0x00
  db	0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x00, 0x78, 0x08, 0x08, 0x08, 0x08, 0x08, 0x78, 0x00
  db	0x10, 0x28, 0x44, 0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff
  db	0x10, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x02, 0x3e, 0x42, 0x3f, 0x00
  db	0x60, 0x20, 0x20, 0x2e, 0x31, 0x31, 0x2e, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x40, 0x42, 0x3c, 0x00
  db	0x06, 0x02, 0x02, 0x3a, 0x46, 0x46, 0x3b, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x7e, 0x40, 0x3c, 0x00
  db	0x0c, 0x12, 0x10, 0x38, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00, 0x3d, 0x42, 0x42, 0x3e, 0x02, 0x7c
  db	0x60, 0x20, 0x2c, 0x32, 0x22, 0x22, 0x62, 0x00, 0x10, 0x00, 0x30, 0x10, 0x10, 0x10, 0x38, 0x00
  db	0x02, 0x00, 0x06, 0x02, 0x02, 0x42, 0x42, 0x3c, 0x60, 0x20, 0x24, 0x28, 0x30, 0x28, 0x26, 0x00
  db	0x30, 0x10, 0x10, 0x10, 0x10, 0x10, 0x38, 0x00, 0x00, 0x00, 0x76, 0x49, 0x49, 0x49, 0x49, 0x00
  db	0x00, 0x00, 0x5c, 0x62, 0x42, 0x42, 0x42, 0x00, 0x00, 0x00, 0x3c, 0x42, 0x42, 0x42, 0x3c, 0x00
  db	0x00, 0x00, 0x6c, 0x32, 0x32, 0x2c, 0x20, 0x70, 0x00, 0x00, 0x36, 0x4c, 0x4c, 0x34, 0x04, 0x0e
  db	0x00, 0x00, 0x6c, 0x32, 0x22, 0x20, 0x70, 0x00, 0x00, 0x00, 0x3e, 0x40, 0x3c, 0x02, 0x7c, 0x00
  db	0x10, 0x10, 0x7c, 0x10, 0x10, 0x12, 0x0c, 0x00, 0x00, 0x00, 0x42, 0x42, 0x42, 0x46, 0x3a, 0x00
  db	0x00, 0x00, 0x41, 0x41, 0x22, 0x14, 0x08, 0x00, 0x00, 0x00, 0x41, 0x49, 0x49, 0x49, 0x36, 0x00
  db	0x00, 0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x42, 0x42, 0x42, 0x3e, 0x02, 0x7c
  db	0x00, 0x00, 0x7c, 0x08, 0x10, 0x20, 0x7c, 0x00, 0x0c, 0x10, 0x10, 0x60, 0x10, 0x10, 0x0c, 0x00
  db	0x10, 0x10, 0x10, 0x00, 0x10, 0x10, 0x10, 0x00, 0x30, 0x08, 0x08, 0x06, 0x08, 0x08, 0x30, 0x00
  db	0x32, 0x4c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x14, 0x22, 0x41, 0x41, 0x7f, 0x00
  db	0x3c, 0x42, 0x40, 0x42, 0x3c, 0x0c, 0x02, 0x3c, 0x00, 0x44, 0x00, 0x44, 0x44, 0x44, 0x3e, 0x00
  db	0x0c, 0x00, 0x3c, 0x42, 0x7e, 0x40, 0x3c, 0x00, 0x3c, 0x42, 0x38, 0x04, 0x3c, 0x44, 0x3e, 0x00
  db	0x42, 0x00, 0x38, 0x04, 0x3c, 0x44, 0x3e, 0x00, 0x30, 0x00, 0x38, 0x04, 0x3c, 0x44, 0x3e, 0x00
  db	0x10, 0x00, 0x38, 0x04, 0x3c, 0x44, 0x3e, 0x00, 0x00, 0x00, 0x3c, 0x40, 0x40, 0x3c, 0x06, 0x1c
  db	0x3c, 0x42, 0x3c, 0x42, 0x7e, 0x40, 0x3c, 0x00, 0x42, 0x00, 0x3c, 0x42, 0x7e, 0x40, 0x3c, 0x00
  db	0x30, 0x00, 0x3c, 0x42, 0x7e, 0x40, 0x3c, 0x00, 0x24, 0x00, 0x18, 0x08, 0x08, 0x08, 0x1c, 0x00
  db	0x7c, 0x82, 0x30, 0x10, 0x10, 0x10, 0x38, 0x00, 0x30, 0x00, 0x18, 0x08, 0x08, 0x08, 0x1c, 0x00
  db	0x42, 0x18, 0x24, 0x42, 0x7e, 0x42, 0x42, 0x00, 0x18, 0x18, 0x00, 0x3c, 0x42, 0x7e, 0x42, 0x00
  db	0x0c, 0x00, 0x7c, 0x20, 0x38, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x33, 0x0c, 0x3f, 0x44, 0x3b, 0x00
  db	0x1f, 0x24, 0x44, 0x7f, 0x44, 0x44, 0x47, 0x00, 0x18, 0x24, 0x00, 0x3c, 0x42, 0x42, 0x3c, 0x00
  db	0x00, 0x42, 0x00, 0x3c, 0x42, 0x42, 0x3c, 0x00, 0x20, 0x10, 0x00, 0x3c, 0x42, 0x42, 0x3c, 0x00
  db	0x18, 0x24, 0x00, 0x42, 0x42, 0x42, 0x3c, 0x00, 0x20, 0x10, 0x00, 0x42, 0x42, 0x42, 0x3c, 0x00
  db	0x00, 0x42, 0x00, 0x42, 0x42, 0x3e, 0x02, 0x3c, 0x42, 0x18, 0x24, 0x42, 0x42, 0x24, 0x18, 0x00
  db	0x42, 0x00, 0x42, 0x42, 0x42, 0x42, 0x3c, 0x00, 0x08, 0x08, 0x3e, 0x40, 0x40, 0x3e, 0x08, 0x08
  db	0x18, 0x24, 0x20, 0x70, 0x20, 0x42, 0x7c, 0x00, 0x44, 0x28, 0x7c, 0x10, 0x7c, 0x10, 0x10, 0x00
  db	0xf8, 0x4c, 0x78, 0x44, 0x4f, 0x44, 0x45, 0xe6, 0x1c, 0x12, 0x10, 0x7c, 0x10, 0x10, 0x90, 0x60
  db	0x0c, 0x00, 0x38, 0x04, 0x3c, 0x44, 0x3e, 0x00, 0x0c, 0x00, 0x18, 0x08, 0x08, 0x08, 0x1c, 0x00
  db	0x04, 0x08, 0x00, 0x3c, 0x42, 0x42, 0x3c, 0x00, 0x00, 0x04, 0x08, 0x42, 0x42, 0x42, 0x3c, 0x00
  db	0x32, 0x4c, 0x00, 0x7c, 0x42, 0x42, 0x42, 0x00, 0x34, 0x4c, 0x00, 0x62, 0x52, 0x4a, 0x46, 0x00
  db	0x3c, 0x44, 0x44, 0x3e, 0x00, 0x7e, 0x00, 0x00, 0x38, 0x44, 0x44, 0x38, 0x00, 0x7c, 0x00, 0x00
  db	0x10, 0x00, 0x10, 0x20, 0x40, 0x42, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x40, 0x40, 0x00, 0x00
  db	0x00, 0x00, 0x00, 0x7e, 0x02, 0x02, 0x00, 0x00, 0x42, 0xc4, 0x48, 0xf6, 0x29, 0x43, 0x8c, 0x1f
  db	0x42, 0xc4, 0x4a, 0xf6, 0x2a, 0x5f, 0x82, 0x02, 0x00, 0x10, 0x00, 0x10, 0x10, 0x10, 0x10, 0x00
  db	0x00, 0x12, 0x24, 0x48, 0x24, 0x12, 0x00, 0x00, 0x00, 0x48, 0x24, 0x12, 0x24, 0x48, 0x00, 0x00
  db	0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa
  db	0xdb, 0x77, 0xdb, 0xee, 0xdb, 0x77, 0xdb, 0xee, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10
  db	0x10, 0x10, 0x10, 0x10, 0xf0, 0x10, 0x10, 0x10, 0x10, 0x10, 0xf0, 0x10, 0xf0, 0x10, 0x10, 0x10
  db	0x14, 0x14, 0x14, 0x14, 0xf4, 0x14, 0x14, 0x14, 0x00, 0x00, 0x00, 0x00, 0xfc, 0x14, 0x14, 0x14
  db	0x00, 0x00, 0xf0, 0x10, 0xf0, 0x10, 0x10, 0x10, 0x14, 0x14, 0xf4, 0x04, 0xf4, 0x14, 0x14, 0x14
  db	0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x00, 0x00, 0xfc, 0x04, 0xf4, 0x14, 0x14, 0x14
  db	0x14, 0x14, 0xf4, 0x04, 0xfc, 0x00, 0x00, 0x00, 0x14, 0x14, 0x14, 0x14, 0xfc, 0x00, 0x00, 0x00
  db	0x10, 0x10, 0xf0, 0x10, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x10, 0x10, 0x10
  db	0x10, 0x10, 0x10, 0x10, 0x1f, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0xff, 0x00, 0x00, 0x00
  db	0x00, 0x00, 0x00, 0x00, 0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1f, 0x10, 0x10, 0x10
  db	0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x10, 0x10, 0x10, 0x10, 0xff, 0x10, 0x10, 0x10
  db	0x10, 0x10, 0x1f, 0x10, 0x1f, 0x10, 0x10, 0x10, 0x14, 0x14, 0x14, 0x14, 0x17, 0x14, 0x14, 0x14
  db	0x14, 0x14, 0x17, 0x10, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x10, 0x17, 0x14, 0x14, 0x14
  db	0x14, 0x14, 0xf7, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xf7, 0x14, 0x14, 0x14
  db	0x14, 0x14, 0x17, 0x10, 0x17, 0x14, 0x14, 0x14, 0x00, 0x00, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00
  db	0x14, 0x14, 0xf7, 0x00, 0xf7, 0x14, 0x14, 0x14, 0x10, 0x10, 0xff, 0x00, 0xff, 0x00, 0x00, 0x00
  db	0x14, 0x14, 0x14, 0x14, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0xff, 0x10, 0x10, 0x10
  db	0x00, 0x00, 0x00, 0x00, 0xff, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x1f, 0x00, 0x00, 0x00
  db	0x10, 0x10, 0x1f, 0x10, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x10, 0x1f, 0x10, 0x10, 0x10
  db	0x00, 0x00, 0x00, 0x00, 0x1f, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0x14, 0xff, 0x14, 0x14, 0x14
  db	0x10, 0x10, 0xff, 0x10, 0xff, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0xf0, 0x00, 0x00, 0x00
  db	0x00, 0x00, 0x00, 0x00, 0x1f, 0x10, 0x10, 0x10, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
  db	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0
  db	0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00
  db	0x00, 0x00, 0x31, 0x4a, 0x44, 0x4a, 0x31, 0x00, 0x00, 0x3c, 0x42, 0x7c, 0x42, 0x7c, 0x40, 0x40
  db	0x00, 0x7e, 0x42, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x3f, 0x54, 0x14, 0x14, 0x14, 0x14, 0x00
  db	0x7e, 0x42, 0x20, 0x18, 0x20, 0x42, 0x7e, 0x00, 0x00, 0x00, 0x3e, 0x48, 0x48, 0x48, 0x30, 0x00
  db	0x00, 0x44, 0x44, 0x44, 0x7a, 0x40, 0x40, 0x80, 0x00, 0x33, 0x4c, 0x08, 0x08, 0x08, 0x08, 0x00
  db	0x7c, 0x10, 0x38, 0x44, 0x44, 0x38, 0x10, 0x7c, 0x18, 0x24, 0x42, 0x7e, 0x42, 0x24, 0x18, 0x00
  db	0x18, 0x24, 0x42, 0x42, 0x24, 0x24, 0x66, 0x00, 0x1c, 0x20, 0x18, 0x3c, 0x42, 0x42, 0x3c, 0x00
  db	0x00, 0x62, 0x95, 0x89, 0x95, 0x62, 0x00, 0x00, 0x02, 0x04, 0x3c, 0x4a, 0x52, 0x3c, 0x40, 0x80
  db	0x0c, 0x10, 0x20, 0x3c, 0x20, 0x10, 0x0c, 0x00, 0x3c, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x00
  db	0x00, 0x7e, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x10, 0x10, 0x7c, 0x10, 0x10, 0x00, 0x7c, 0x00
  db	0x10, 0x08, 0x04, 0x08, 0x10, 0x00, 0x7e, 0x00, 0x08, 0x10, 0x20, 0x10, 0x08, 0x00, 0x7e, 0x00
  db	0x0c, 0x12, 0x12, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x90, 0x90, 0x60
  db	0x18, 0x18, 0x00, 0x7e, 0x00, 0x18, 0x18, 0x00, 0x00, 0x32, 0x4c, 0x00, 0x32, 0x4c, 0x00, 0x00
  db	0x30, 0x48, 0x48, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00
  db	0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x0f, 0x08, 0x08, 0x08, 0x08, 0xc8, 0x28, 0x18
  db	0x78, 0x44, 0x44, 0x44, 0x44, 0x00, 0x00, 0x00, 0x30, 0x48, 0x10, 0x20, 0x78, 0x00, 0x00, 0x00
  db	0x00, 0x00, 0x3c, 0x3c, 0x3c, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

times 0x3FFF-($-$$) db 0
checksum db 0