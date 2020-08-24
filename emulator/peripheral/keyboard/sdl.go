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
	"time"

	"github.com/andreas-jonsson/virtualxt/emulator/dialog"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/smouse"
	"github.com/veandco/go-sdl2/sdl"
)

func (m *Device) startEventLoop() {
	sdl.Do(func() {
		if err := sdl.InitSubSystem(sdl.INIT_EVENTS); err != nil {
			log.Print(err)
			return
		}
		sdl.EventState(sdl.DROPFILE, sdl.ENABLE)
	})

	go func() {
		var mouse smouse.SerialMouse

		initMouse := func() bool {
			if mouse == nil {
				// TODO: Fix this!
				var (
					ok   bool
					base uint16 = 0x3F8
				)
				if mouse, ok = m.cpu.GetMappedIODevice(base).(smouse.SerialMouse); !ok {
					log.Printf("Could not find serial mouse at base address: 0x%X", base)
					return false
				}
			}
			return sdl.GetRelativeMouseMode()
		}

		ticker := time.NewTicker(time.Second / 30)
		defer ticker.Stop()

		for {
			select {
			case <-m.quitChan:
				sdl.Do(func() {
					sdl.QuitSubSystem(sdl.INIT_EVENTS)
				})
				close(m.quitChan)
				return
			case <-ticker.C:
				sdl.Do(func() {
					for event := sdl.PollEvent(); event != nil; event = sdl.PollEvent() {
						switch ev := event.(type) {
						case *sdl.QuitEvent:
							sdl.SetRelativeMouseMode(false)
							dialog.AskToQuit()
						case *sdl.KeyboardEvent:
							m.sdlProcessKey(ev)
						/*
							case *sdl.WindowEvent:
								if ev.Event == sdl.WINDOWEVENT_MAXIMIZED {
									w, err := sdl.GetWindowFromID(ev.WindowID)
									if err != nil {
										continue
									}
									w.SetFullscreen(sdl.WINDOW_FULLSCREEN_DESKTOP)
									sdl.SetRelativeMouseMode(true)
								}
						*/
						case *sdl.MouseMotionEvent:
							if initMouse() {
								_, _, state := sdl.GetMouseState()
								var buttons byte
								if state&sdl.ButtonLMask() != 0 {
									buttons = 2
								}
								if state&sdl.ButtonRMask() != 0 {
									buttons |= 1
								}
								mouse.PushEvent(buttons, int8(ev.XRel), int8(ev.YRel))
							}
						case *sdl.MouseButtonEvent:
							if ev.Type == sdl.MOUSEBUTTONDOWN {
								state := uint32(ev.Button)
								if state&sdl.ButtonMMask() != 0 {
									sdl.SetRelativeMouseMode(false)
									continue
								}
								sdl.SetRelativeMouseMode(true)

								if initMouse() {
									var buttons byte
									if state&sdl.ButtonLMask() != 0 {
										buttons = 2
									}
									if state&sdl.ButtonRMask() != 0 {
										buttons |= 1
									}
									mouse.PushEvent(buttons, 0, 0)
								}
							}
						case *sdl.DropEvent:
							if ev.Type == sdl.DROPFILE {
								dialog.MountFloppyImage(ev.File)
							}
						}
					}
				})
			}
		}
	}()
}

func (m *Device) sdlProcessKey(ev *sdl.KeyboardEvent) {
	if !sdl.GetRelativeMouseMode() {
		return
	}

	w, err := sdl.GetWindowFromID(ev.WindowID)
	if err != nil {
		log.Printf("Could not find window: 0x%X", ev.WindowID)
		return
	}

	keyUp := ev.Type == sdl.KEYUP
	if ev.Keysym.Scancode == sdl.SCANCODE_F11 {
		if keyUp {
			if (w.GetFlags() & sdl.WINDOW_FULLSCREEN) != 0 {
				w.SetFullscreen(0)
				sdl.SetRelativeMouseMode(false)
			} else {
				w.SetFullscreen(sdl.WINDOW_FULLSCREEN_DESKTOP)
				sdl.SetRelativeMouseMode(true)
			}
		}
	} else if ev.Keysym.Scancode == sdl.SCANCODE_F12 {
		if keyUp {
			w.SetFullscreen(0)
			sdl.SetRelativeMouseMode(false)
			dialog.MainMenu()
		}
	} else if scan := sdlScanToXTScan(ev.Keysym.Scancode); scan != ScanInvalid {
		if keyUp {
			scan |= KeyUpMask
		}
		m.pushEvent(scan)
	} else {
		log.Printf("Invalid key \"%s\"", sdl.GetKeyName(ev.Keysym.Sym))
	}
}

func sdlScanToXTScan(scan sdl.Scancode) Scancode {
	switch scan {
	case sdl.SCANCODE_ESCAPE:
		return ScanEscape
	case sdl.SCANCODE_1:
		return Scan1
	case sdl.SCANCODE_2:
		return Scan2
	case sdl.SCANCODE_3:
		return Scan3
	case sdl.SCANCODE_4:
		return Scan4
	case sdl.SCANCODE_5:
		return Scan5
	case sdl.SCANCODE_6:
		return Scan6
	case sdl.SCANCODE_7:
		return Scan7
	case sdl.SCANCODE_8:
		return Scan8
	case sdl.SCANCODE_9:
		return Scan9
	case sdl.SCANCODE_0:
		return Scan0
	case sdl.SCANCODE_MINUS:
		return ScanMinus
	case sdl.SCANCODE_EQUALS:
		return ScanEqual
	case sdl.SCANCODE_BACKSPACE:
		return ScanBackspace
	case sdl.SCANCODE_TAB:
		return ScanTab
	case sdl.SCANCODE_Q:
		return ScanQ
	case sdl.SCANCODE_W:
		return ScanW
	case sdl.SCANCODE_E:
		return ScanE
	case sdl.SCANCODE_R:
		return ScanR
	case sdl.SCANCODE_T:
		return ScanT
	case sdl.SCANCODE_Y:
		return ScanY
	case sdl.SCANCODE_U:
		return ScanU
	case sdl.SCANCODE_I:
		return ScanI
	case sdl.SCANCODE_O:
		return ScanO
	case sdl.SCANCODE_P:
		return ScanP
	case sdl.SCANCODE_LEFTBRACKET:
		return ScanLBracket
	case sdl.SCANCODE_RIGHTBRACKET:
		return ScanRBracket
	case sdl.SCANCODE_RETURN, sdl.SCANCODE_KP_ENTER:
		return ScanEnter
	case sdl.SCANCODE_LCTRL, sdl.SCANCODE_RCTRL:
		return ScanControl
	case sdl.SCANCODE_A:
		return ScanA
	case sdl.SCANCODE_S:
		return ScanS
	case sdl.SCANCODE_D:
		return ScanD
	case sdl.SCANCODE_F:
		return ScanF
	case sdl.SCANCODE_G:
		return ScanG
	case sdl.SCANCODE_H:
		return ScanH
	case sdl.SCANCODE_J:
		return ScanJ
	case sdl.SCANCODE_K:
		return ScanK
	case sdl.SCANCODE_L:
		return ScanL
	case sdl.SCANCODE_SEMICOLON:
		return ScanSemicolon
	case sdl.SCANCODE_APOSTROPHE:
		return ScanQuote
	case sdl.SCANCODE_GRAVE:
		return ScanBackquote
	case sdl.SCANCODE_LSHIFT:
		return ScanLShift
	case sdl.SCANCODE_BACKSLASH:
		return ScanBackslash
	case sdl.SCANCODE_Z:
		return ScanZ
	case sdl.SCANCODE_X:
		return ScanX
	case sdl.SCANCODE_C:
		return ScanC
	case sdl.SCANCODE_V:
		return ScanV
	case sdl.SCANCODE_B:
		return ScanB
	case sdl.SCANCODE_N:
		return ScanN
	case sdl.SCANCODE_M:
		return ScanM
	case sdl.SCANCODE_COMMA:
		return ScanComma
	case sdl.SCANCODE_PERIOD:
		return ScanPeriod
	case sdl.SCANCODE_SLASH:
		return ScanSlash
	case sdl.SCANCODE_RSHIFT:
		return ScanRShift
	case sdl.SCANCODE_PRINTSCREEN:
		return ScanPrint
	case sdl.SCANCODE_LALT, sdl.SCANCODE_RALT:
		return ScanAlt
	case sdl.SCANCODE_SPACE:
		return ScanSpace
	case sdl.SCANCODE_CAPSLOCK:
		return ScanCapslock
	case sdl.SCANCODE_F1:
		return ScanF1
	case sdl.SCANCODE_F2:
		return ScanF2
	case sdl.SCANCODE_F3:
		return ScanF3
	case sdl.SCANCODE_F4:
		return ScanF4
	case sdl.SCANCODE_F5:
		return ScanF5
	case sdl.SCANCODE_F6:
		return ScanF6
	case sdl.SCANCODE_F7:
		return ScanF7
	case sdl.SCANCODE_F8:
		return ScanF8
	case sdl.SCANCODE_F9:
		return ScanF9
	case sdl.SCANCODE_F10:
		return ScanF10
	case sdl.SCANCODE_NUMLOCKCLEAR:
		return ScanNumlock
	case sdl.SCANCODE_SCROLLLOCK:
		return ScanScrlock
	case sdl.SCANCODE_KP_7:
		return ScanKPHome
	case sdl.SCANCODE_KP_8, sdl.SCANCODE_UP:
		return ScanKPUp
	case sdl.SCANCODE_KP_9:
		return ScanKPPageup
	case sdl.SCANCODE_KP_MINUS:
		return ScanKPMinus
	case sdl.SCANCODE_KP_4, sdl.SCANCODE_LEFT:
		return ScanKPLeft
	case sdl.SCANCODE_KP_5:
		return ScanKP5
	case sdl.SCANCODE_KP_6, sdl.SCANCODE_RIGHT:
		return ScanKPRight
	case sdl.SCANCODE_KP_PLUS:
		return ScanKPPlus
	case sdl.SCANCODE_KP_1:
		return ScanKPEnd
	case sdl.SCANCODE_KP_2, sdl.SCANCODE_DOWN:
		return ScanKPDown
	case sdl.SCANCODE_KP_3:
		return ScanKPPagedown
	case sdl.SCANCODE_KP_0:
		return ScanKPInsert
	case sdl.SCANCODE_KP_COMMA:
		return ScanKPDelete
	default:
		return ScanInvalid
	}
}
