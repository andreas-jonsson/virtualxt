// +build sdl

/*
Copyright (c) 2019-2020 Andreas T Jonsson

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

package platform

import (
	"log"
	"time"

	"github.com/andreas-jonsson/virtualxt/platform/dialog"
	"github.com/veandco/go-sdl2/sdl"
)

func (p *sdlPlatform) initializeSDLEvents() error {
	var err error
	sdl.Do(func() {
		if err = sdl.InitSubSystem(sdl.INIT_EVENTS); err != nil {
			return
		}
		sdl.EventState(sdl.DROPFILE, sdl.ENABLE)
	})
	if err != nil {
		return err
	}

	p.quitChan = make(chan struct{})
	registerCleanup(p, shutdownSDLEvents)

	go func() {
		ticker := time.NewTicker(time.Second / 30)
		defer ticker.Stop()

		for {
			select {
			case <-p.quitChan:
				close(p.quitChan)
				return
			case <-ticker.C:
				sdl.Do(func() {
					for event := sdl.PollEvent(); event != nil; event = sdl.PollEvent() {
						switch ev := event.(type) {
						case *sdl.QuitEvent:
							sdl.SetRelativeMouseMode(false)
							dialog.AskToQuit()
						case *sdl.KeyboardEvent:
							p.sdlProcessKey(ev)
						/*
							case *sdl.WindowEvent:
								if ev.Event == sdl.WINDOWEVENT_MAXIMIZED {
									p.window.SetFullscreen(sdl.WINDOW_FULLSCREEN_DESKTOP)
									sdl.SetRelativeMouseMode(true)
								}
						*/
						case *sdl.MouseMotionEvent:
							if p.mouseHandler != nil && sdl.GetRelativeMouseMode() {
								_, _, state := sdl.GetMouseState()
								var buttons byte
								if state&sdl.ButtonLMask() != 0 {
									buttons = 2
								}
								if state&sdl.ButtonRMask() != 0 {
									buttons |= 1
								}
								p.mouseHandler(buttons, int8(ev.XRel), int8(ev.YRel))
							}
						case *sdl.MouseButtonEvent:
							if ev.Type == sdl.MOUSEBUTTONDOWN {
								state := uint32(ev.Button)
								if state == sdl.BUTTON_MIDDLE {
									sdl.SetRelativeMouseMode(false)
									continue
								}
								sdl.SetRelativeMouseMode(true)

								if p.mouseHandler != nil {
									var buttons byte
									if state == sdl.BUTTON_LEFT {
										buttons = 2
									}
									if state == sdl.BUTTON_RIGHT {
										buttons |= 1
									}
									p.mouseHandler(buttons, 0, 0)
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
	return nil
}

func shutdownSDLEvents(p *sdlPlatform) {
	p.quitChan <- struct{}{}
	<-p.quitChan
	sdl.Do(func() {
		sdl.QuitSubSystem(sdl.INIT_EVENTS)
	})
}

func (p *sdlPlatform) sdlProcessKey(ev *sdl.KeyboardEvent) {
	keyUp := ev.Type == sdl.KEYUP
	if ev.Keysym.Scancode == sdl.SCANCODE_F11 {
		if keyUp {
			if (p.window.GetFlags() & sdl.WINDOW_FULLSCREEN) != 0 {
				p.window.SetFullscreen(0)
				sdl.SetRelativeMouseMode(false)
			} else {
				p.window.SetFullscreen(sdl.WINDOW_FULLSCREEN_DESKTOP)
				sdl.SetRelativeMouseMode(true)
			}
		}
	} else if ev.Keysym.Scancode == sdl.SCANCODE_F12 {
		if keyUp {
			p.window.SetFullscreen(0)
			sdl.SetRelativeMouseMode(false)
			dialog.MainMenu()
		}
	} else if scan := sdlScanToXTScan(ev.Keysym.Scancode); scan != ScanInvalid && p.keyboardHandler != nil {
		if keyUp {
			scan |= KeyUpMask
		}
		p.keyboardHandler(scan)
	} else {
		log.Printf("Invalid key \"%s\"", sdl.GetKeyName(ev.Keysym.Sym))
	}
}

func (p *sdlPlatform) SetKeyboardHandler(h func(Scancode)) {
	sdl.Do(func() {
		p.keyboardHandler = h
	})
}

func (p *sdlPlatform) SetMouseHandler(h func(byte, int8, int8)) {
	sdl.Do(func() {
		p.mouseHandler = h
	})
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
