;-------------------------------------------------------------------
;
; PROGRAM   Use!UMBs.sys - UMB manager for PC/XT/ATs
;
; PURPOSE   This program creates a device driver that will handle
;           calls for Upper Memory Blocks. In doing so, it gives
;           you the possibility to save conventional memory by
;           storing device drivers and resident programs in the
;           UMBs and hence save conventional memory.
;
; REQUIRES  It will work on any PC/XT/AT or higher, either with or
;           without extended memory. What you obviously *must* have
;           are UMBs, and MS-DOS Version 5.0.
;
; AUTHOR    Marco van Zwetselaar    Phone: **-31-30-313128
;           Oorsprongpark 5         FidoNet: Marco van.Zwetselaar
;           3581 ES Utrecht                  NetMail 2:281/701
;           The Netherlands         Utrecht, 7 Nov 1991
;
;-------------------------------------------------------------------
;
; IMPORTANT NOTE to the programmer
;
; I am only a beginner in assembly language, and haven't found out
; as yet how to create a .com file. When you recompile this program,
; take care you do that as a com file, and then rename it to a .sys
; file.
;
; I used an alternative method that you may use as well: I compiled
; it to .exe format and then removed the first (half?) sector. In
; fact, the first bytes in the final file should be:
;
; FF FF FF FF 00 E0 ...
;
; which are the bytes that start at nextDev. (Almost any device
; driver file should start with the first two words being FFFF (-1)).
;
;====================================================================
;
; THE ACTUAL ASSEMBLY CODE
;
;---------------------------------------------
; This is the structure of the request header; not stored in file
;
rh              segment at 0
                org     02h
rh_cmd          db      ?               ;command-code
rh_status       dw      ?               ;return-code
                org     0Eh
rh0_brk_ofs     dw      ?               ;end of driver offset
rh0_brk_seg     dw      ?               ;end of driver segment
rh              ends
;
;--------------------------------------------
; The driverheader

                org     0h
zwetsUMB        segment public
                assume  cs:zwetsumb, es:rh, ds:nothing

; any device driver will start with the following info:

next_dev        dd      -1
attribute       dw      0E000h
strategy        dw      strat
interrupt       dw      intr
dev_name        db      'I_LOVE_M'

;--------------------------------------------
; Storage space for pointer to Request Header

hdrptr  label   word
        dw      ?,?

;--------------------------------------------
; Strategy: store pointer to Request Header:

strat   proc    far
        mov     [hdrptr],bx
        mov     [hdrptr]+2,es
        retf
strat   endp

;--------------------------------------------
; Interrupt: The routine that fires the installrequest

intr    proc    far

        push    ax
        push    bx
        push    es
        pushf

        les     bx, dword ptr [hdrptr]  ;point es:bx to ReqHeader
        mov     [bx].rh0_brk_seg, cs    ;and already return the seg

; Remember that the device will only answer the init-request
; which is called by dos as it installs the driver.
; Any later calls will be ignored - they are not needed anyway.

        mov     al, es:[bx].rh_cmd      ;commandcode into al
        or      al, al                  ;is it 0 (=init)?
        jnz     other                   ;   no => get off
        jmp     inst                    ;   yes => install

other:  cmp     al, 10h                 ;is it legal call?
        jbe     short dos               ;   yes => "Sorry"
        mov     ax, 8003h               ;   no => "Unknown command"
        jmp     short exit

dos:    xor     ax, ax                  ;"Sorry - not implememented"

exit:   or      ax, 0100h               ;Set done bit
        mov     es:[bx].rh_status, ax   ;set return code

        popf
        pop     es
        pop     bx
        pop     ax

        retf                            ; back to dos

intr    endp

;------------------------------------------------------
; Storage space for the addresses of free umb blocks

umb     struc
start   dw      ?
len     dw      ?
umb     ends

;you can add to the following list, as long as the last one is 0,0

free    umb     <0D000h, 1FFFh>   ;my only block (at d000, length 1fff)
        umb     <0, 0>            ;space for extra block
        umb     <0, 0>            ;and another block
        umb     <0, 0>            ;The end of the blocks is marked
                                  ; with 0,0 DON'T REMOVE THIS!!!!!

;------------------------------------------------------
; This is the code for the actual UMB memory provider

umbreq  proc    near

        push    ds
        push    si
        push    cx
        push    bx

        push    cs
        pop     ds

        mov     si, offset free
        xor     cx, cx                  ;clear dx and use it to compare
        xchg    dx, cx                  ;to largest found as yet - cx=size

search: lodsw                           ;read startsegment into ax
        or      ax, ax                  ;is it 0
        jz      fini                    ;   then buzz off
        mov     bx, ax                  ;   otherwise store into bx
        lodsw                           ;read length in ax
        cmp     bx, 0FFFFh              ;is startsegm=ffff=given away
        je      search                  ;   yes: then find next block
        cmp     dx, ax                  ;   no: larger than largest upto now?
        jae     szchk                   ;       no: check size
        mov     dx, ax                  ;       yes: mark as largest

szchk:  cmp     cx, ax                  ;will it fit?
        ja      search                  ;   no: search next

        ; YEAH! Found a block

        mov     [si-4], 0FFFFh          ; mark as used
        mov     dx, ax                  ; put blocksize in dx
        mov     ax, 1                   ;say Succes
        pop     cx                      ;still had bx on stack
        jmp     short return

fini:   xor     ax, ax                  ;no fitting UMB
        or      dx, dx                  ;is dx=0?, so largest found=0
        jnz     sizerr                  ;   if not: problem with size
        pop     bx                      ;   if yes: BH should stay intact
        mov     bl, 0B1h                ;       and BL signals: out of UMBs
        jmp     short return

sizerr: pop     bx                      ;BH remains intact
        mov     bl, 0B0h                ;BL signals: there are smaller ones

return: pop     cx
        pop     si
        pop     ds
        popf
        retf

umbreq  endp

;------------------------------------------------------
; Storage space for old XMS-handler. The first byte stores WHETHER there
; was an XMS handler already (that didn't provide UMB services).
; The next two words specify its location.

isoldx  db      00h

oldxms  label word
        dw ?,?

;------------------------------------------------------
; This is the small XMS manager that we install. It will only react
; to requests for UMBs (so when AH=10h). If not, it *should* pass
; control to the existing XMS manager - provided there is one. Or
; otherwise inform the caller that there is no eXt memory.

        assume es:nothing, ds:nothing

umbctrlr:

        jmp     short hookch            ;MUST be a *short* jmp
        nop                             ; The nops are there because
        nop                             ; a himem handler always start
        nop                             ; with 5 special bytes.

hookch: pushf
        cmp     ah, 10h                 ;is it a UMB request?
        je      umbreq                  ;  yes: execute it
        cmp     [isoldx], 0h            ;  no: is there an XMS-handler?
        jz      Xreq1                   ;     no: give response like it
        popf                            ;     yes: jump to old handler
        jmp     dword ptr cs:[oldxms]

; Below is the code that will respond to non-UMB (and thus XMS-)
; requests when no XMS-manager is present. To nearly all requests
; (there are 11h) it will answer with AX=0000, meaning failure
; and BL=80, which gives the errormessage: not implemented.
; Only fns 00h and 08h are treated specially (as well as fn 10h, but
; that has been filtered out above - it's the UMB-request).

Xreq1:  cmp     ah, 0h                  ;req 00h = get XMS version
        jnz     Xreq2
        mov     bx, 0h                  ; ->internal revision nr=0
        mov     dx, 0h                  ; ->HMA not present
        jmp     short byebye

Xreq2:  cmp     ah, 08h                 ;req 08h = query extended mem
        jnz     Xreq3
        mov     dx, 0h                  ; ->0k extended memory

Xreq3:  mov     bl, 80h                 ; ->error: not implemented
        
byebye: mov     ax, 00h                 ; ->failure (always the case)
        popf
        retf

;------------------------------------------------------
; Storage space for old interrupt vector 2f

old2f   label   word
        dw      ?,?

;-------------------------------------------------------
; This is the code that is chained to the front of interrupt 2f.
; It will handle requests nrs. 4300 and 4310, and leave all others
;    to the existing code.

handl:  pushf

        cmp     ah, 43h                 ;is request for HIMEM?
        jne     not4me                  ; no: pass on

        cmp     al, 10h                 ; is request: where is handler?
        jne     mustbe                  ;    if not, then must be al=00

        mov     bx, offset umbctrlr     ; put handler's ofs in bx
        push    cs                      ; and its segment
        pop     es                      ; in es
        jmp     short getout

mustbe: mov     al, 80h                 ;al <>10 so must be 00, so request
                                        ;is: is there handler? Yes => 80h
getout: popf
        iret

not4me: popf                            ;request wasn't for me
        jmp     dword ptr cs:[old2f]    ;so give back to old handl

;=================================================================
;
; NON-RESIDENT PART
;
; The part that follows now will not remain resident. It will be
; executed only once: at the time the driver is installed by
; DOS' bootprocedures.
;
; This code takes care of (1) hooking our intercepting code to
; int 2f, (2) hooking our small XMS-manager to the already installed
; one - provided there is one, (3) and if not, installing the
; the response-robot that will inform callers that there's no eXt
; memory.
;

;-------------------------------------------------------
; Installation routine. Called once.

        assume  ds:zwetsUMB, es:rh


inst:   push    cx                      ;save em
        push    dx
        push    ds

        push    cs                      ;and set ds to cs
        pop     ds

        mov     ah, 09h
        mov     dx, offset hello1$      ;say welcome
        int     21h
        mov     dx, offset empty$
        int     21h
        mov     dx, offset hello2$
        int     21h
        mov     dx, offset empty$
        int     21h
        mov     dx, offset hello3$
        int     21h
        mov     dx, offset hello4$
        int     21h
        mov     dx, offset empty$
        int     21h
        mov     dx, offset autho1$
        int     21h
        mov     dx, offset autho2$
        int     21h
        mov     dx, offset empty$
        int     21h

        mov     ax, 4300h               ;check if there is an XMShandler
        int     2Fh                     ;already
        cmp     al, 080h                ;is it there?
        jne     SetVec                  ;  no: go further

        mov     [isoldx],01h            ;signal yourself there is one
        mov     ax, 4310h               ;get its address
        int     2Fh
        mov     [oldxms], bx            ;and store it
        mov     [oldxms+2], es

        mov     ah, 09h                 ;tell user
        mov     dx, offset found$
        int     21h
        mov     dx, offset empty$
        int     21h

        mov     ah, 10h                 ;check if it services UMBs by
        mov     dx, 0FFFFh              ;trying to allocate FAR TOO much
        call    dword ptr [oldxms]
        cmp     bl, 80h                 ;error: not implemented?
        jz      SetVec                  ; then good - continue

        mov     ah, 09h                 ;otherwise: get off,
        mov     dx, offset oops$        ;you're not needed.
        int     21h
        les     bx, dword ptr [hdrptr]
        xor     ax,ax
        mov     es:[bx].rh0_brk_ofs, ax
        jmp     short dexit

SetVec: mov     ax, 352Fh               ; get vector 2f
        int     21h
        mov     [old2f], bx             ;and store it
        mov     [old2f]+2, es

        mov     ax, 252Fh               ;set it to our own
        mov     dx, offset handl
        int     21h

        mov     ah, 09h
        mov     dx, offset instok$      ;say we did it
        int     21h

        mov     ax, offset inst         ;upto inst it should stay
        les     bx, dword ptr [hdrptr]  ;  resident, which we tell
        mov     es:[bx].rh0_brk_ofs, ax ;  dos via the rqhdr.
        xor     ax,ax                   ;0 no errors

dexit:  pop     ds
        pop     dx
        pop     cx
        jmp     exit

hello1$ db  0Dh,0Ah,'ีออออออออออออออต   Use!UMBs   ฦอออออออออออออธ',0Dh,0Ah,'$'
empty$  db          'ณ                                           ณ',0Dh,0Ah,'$'
hello2$ db          'ณ        Upper Memory Block - Manager       ณ',0Dh,0Ah,'$'
hello3$ db          'ณ   Works on any PC/XT/AT, either with or   ณ',0Dh,0Ah,'$'
hello4$ db          'ณ    without Extended or Expanded Memory!   ณ',0Dh,0Ah,'$'
found$  db          'ณ     * found extended memory manager *     ณ',0Dh,0Ah,'$'
autho1$ db          'ณ        Author: Marco van Zwetselaar       ณ',0Dh,0Ah,'$'
autho2$ db          "ณ   dedicated it to the Public Domain 1991  ณ",0Dh,0Ah,'$'
instok$ db          'ิออต Version 2.0 ฦออออออออออต Installed! ฦออพ',0Dh,0Ah,0Ah,'$'
oops$   db          'ิอต Not Installed!  UMBs Managed Already! ฦอพ',0Dh,0Ah,0Ah,'$'

zwetsUMB   ends
           end

