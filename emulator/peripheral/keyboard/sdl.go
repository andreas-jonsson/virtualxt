// +build sdl

/*
Copyright (C) 2019-2020 Andreas T Jonsson

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

package keyboard

import (
	"log"
	"os"
	"time"

	"github.com/veandco/go-sdl2/sdl"
)

func (m *Device) startSDLEventLoop() {
	go func() {
		for range time.Tick(time.Second / 30) {
			sdl.Do(func() {
				for event := sdl.PollEvent(); event != nil; event = sdl.PollEvent() {
					switch ev := event.(type) {
					case *sdl.QuitEvent:
						os.Exit(0)
					case *sdl.KeyboardEvent:
						m.sdlProcessKey(ev)
					}
				}
			})
		}
	}()
}

func (m *Device) sdlProcessKey(ev *sdl.KeyboardEvent) {
	if scan := sdlKeyToScan(ev.Keysym.Sym); scan != ScanInvalid {
		if ev.Type == sdl.KEYUP {
			scan |= KeyUpMask
		}
		m.pushEvent(scan)
	} else {
		log.Printf("Invalid key \"%s\"", sdl.GetKeyName(ev.Keysym.Sym))
	}
}

func sdlKeyToScan(key sdl.Keycode) Scancode {
	switch key {
	case sdl.K_ESCAPE:
		return ScanEscape
	case sdl.K_1:
		return Scan1
	case sdl.K_2:
		return Scan2
	case sdl.K_3:
		return Scan3
	case sdl.K_4:
		return Scan4
	case sdl.K_5:
		return Scan5
	case sdl.K_6:
		return Scan6
	case sdl.K_7:
		return Scan7
	case sdl.K_8:
		return Scan8
	case sdl.K_9:
		return Scan9
	case sdl.K_0:
		return Scan0
	case sdl.K_MINUS:
		return ScanMinus
	case sdl.K_EQUALS:
		return ScanEqual
	case sdl.K_BACKSPACE:
		return ScanBackspace
	case sdl.K_TAB:
		return ScanTab
	case sdl.K_q:
		return ScanQ
	case sdl.K_w:
		return ScanW
	case sdl.K_e:
		return ScanE
	case sdl.K_r:
		return ScanR
	case sdl.K_t:
		return ScanT
	case sdl.K_y:
		return ScanY
	case sdl.K_u:
		return ScanU
	case sdl.K_i:
		return ScanI
	case sdl.K_o:
		return ScanO
	case sdl.K_p:
		return ScanP
	case sdl.K_LEFTBRACKET:
		return ScanLBracket
	case sdl.K_RIGHTBRACKET:
		return ScanRBracket
	case sdl.K_RETURN, sdl.K_KP_ENTER:
		return ScanEnter
	case sdl.K_LCTRL, sdl.K_RCTRL:
		return ScanControl
	case sdl.K_a:
		return ScanA
	case sdl.K_s:
		return ScanS
	case sdl.K_d:
		return ScanD
	case sdl.K_f:
		return ScanF
	case sdl.K_g:
		return ScanG
	case sdl.K_h:
		return ScanH
	case sdl.K_j:
		return ScanJ
	case sdl.K_k:
		return ScanK
	case sdl.K_l:
		return ScanL
	case sdl.K_SEMICOLON:
		return ScanSemicolon
	case sdl.K_QUOTE:
		return ScanQuote
	case sdl.K_BACKQUOTE:
		return ScanBackquote
	case sdl.K_LSHIFT:
		return ScanLShift
	case sdl.K_BACKSLASH:
		return ScanBackslash
	case sdl.K_z:
		return ScanZ
	case sdl.K_x:
		return ScanX
	case sdl.K_c:
		return ScanC
	case sdl.K_v:
		return ScanV
	case sdl.K_b:
		return ScanB
	case sdl.K_n:
		return ScanN
	case sdl.K_m:
		return ScanM
	case sdl.K_COMMA:
		return ScanComma
	case sdl.K_PERIOD:
		return ScanPeriod
	case sdl.K_SLASH:
		return ScanSlash
	case sdl.K_RSHIFT:
		return ScanRShift
	case sdl.K_PRINTSCREEN:
		return ScanPrint
	case sdl.K_LALT, sdl.K_RALT:
		return ScanAlt
	case sdl.K_SPACE:
		return ScanSpace
	case sdl.K_CAPSLOCK:
		return ScanCapslock
	case sdl.K_F1:
		return ScanF1
	case sdl.K_F2:
		return ScanF2
	case sdl.K_F3:
		return ScanF3
	case sdl.K_F4:
		return ScanF4
	case sdl.K_F5:
		return ScanF5
	case sdl.K_F6:
		return ScanF6
	case sdl.K_F7:
		return ScanF7
	case sdl.K_F8:
		return ScanF8
	case sdl.K_F9:
		return ScanF9
	case sdl.K_F10:
		return ScanF10
	case sdl.K_NUMLOCKCLEAR:
		return ScanNumlock
	case sdl.K_SCROLLLOCK:
		return ScanScrlock
	case sdl.K_KP_7:
		return ScanKPHome
	case sdl.K_KP_8, sdl.K_UP:
		return ScanKPUp
	case sdl.K_KP_9:
		return ScanKPPageup
	case sdl.K_KP_MINUS:
		return ScanKPMinus
	case sdl.K_KP_4, sdl.K_LEFT:
		return ScanKPLeft
	case sdl.K_KP_5:
		return ScanKP5
	case sdl.K_KP_6, sdl.K_RIGHT:
		return ScanKPRight
	case sdl.K_KP_PLUS:
		return ScanKPPlus
	case sdl.K_KP_1:
		return ScanKPEnd
	case sdl.K_KP_2, sdl.K_DOWN:
		return ScanKPDown
	case sdl.K_KP_3:
		return ScanKPPagedown
	case sdl.K_KP_0:
		return ScanKPInsert
	case sdl.K_KP_COMMA:
		return ScanKPDelete
	default:
		return ScanInvalid
	}
}
