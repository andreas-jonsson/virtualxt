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
	"errors"
	"time"

	"github.com/gdamore/tcell"
)

func (m *Device) SendKeyEvent(ev interface{}) error {
	switch t := ev.(type) {
	case *tcell.EventKey:
		deviceEvent := createEventFromTCELL(t)
		if deviceEvent == ScanInvalid {
			return errors.New("unknown key")
		}

		if err := m.pushEvent(deviceEvent); err != nil {
			return err
		}

		go func() {
			deviceEvent |= KeyUpMask
			time.Sleep(10 * time.Millisecond)
			for m.pushEvent(deviceEvent) != nil {
			}
		}()
	default:
		return errors.New("unknown event type")
	}
	return nil
}

func createEventFromTCELL(ev *tcell.EventKey) Scancode {
	switch ev.Key() {
	case tcell.KeyEscape:
		return ScanEscape
	case tcell.KeyEnter:
		return ScanEnter
	case tcell.KeyBackspace:
		return ScanBackspace
	case tcell.KeyTab:
		return ScanTab
	case tcell.KeyCtrlRightSq: //tcell.KeyCtrlLeftSq
		return ScanControl
	case tcell.KeyDown:
		return ScanKPDown
	case tcell.KeyLeft:
		return ScanKPLeft
	case tcell.KeyRight:
		return ScanKPRight
	case tcell.KeyUp:
		return ScanKPUp

		/*
			case SDLK_LALT:
				key.scancode |= VXT_KEY_ALT
				return key
			case SDLK_NUMLOCKCLEAR:
				key.scancode |= VXT_KEY_NUMLOCK
				return key
			case SDLK_SCROLLLOCK:
				key.scancode |= VXT_KEY_SCROLLOCK
				return key
			case SDLK_CAPSLOCK:
				key.scancode |= VXT_KEY_CAPSLOCK
				return key
			case SDLK_LSHIFT:
				key.scancode |= VXT_KEY_LSHIFT
				return key
			case SDLK_RSHIFT:
				key.scancode |= VXT_KEY_RSHIFT
				return key
			case SDLK_PRINTSCREEN:
				key.scancode |= VXT_KEY_PRINT
				return key

			case SDLK_DELETE:
				key.scancode |= VXT_KEY_KP_DELETE_PERIOD
				return key
			case SDLK_INSERT:
				key.scancode |= VXT_KEY_KP_INSERT_0
				return key
			case SDLK_END:
				key.scancode |= VXT_KEY_KP_END_1
				return key
			case SDLK_PAGEUP:
				key.scancode |= VXT_KEY_KP_PAGEUP_9
				return key
			case SDLK_PAGEDOWN:
			key.scancode |= VXT_KEY_KP_PAGEDOWN_3
			return key
				case SDLK_HOME:
			key.scancode |= VXT_KEY_KP_HOME_7
			return key

			case SDLK_F1:
				key.scancode |= VXT_KEY_F1
				return key
			case SDLK_F2:
				key.scancode |= VXT_KEY_F2
				return key
			case SDLK_F3:
				key.scancode |= VXT_KEY_F3
				return key
			case SDLK_F4:
				key.scancode |= VXT_KEY_F4
				return key
			case SDLK_F5:
				key.scancode |= VXT_KEY_F5
				return key
			case SDLK_F6:
				key.scancode |= VXT_KEY_F6
				return key
			case SDLK_F7:
				key.scancode |= VXT_KEY_F7
				return key
			case SDLK_F8:
				key.scancode |= VXT_KEY_F8
				return key
			case SDLK_F9:
				key.scancode |= VXT_KEY_F9
				return key
			case SDLK_F10:
				key.scancode |= VXT_KEY_F10
				return key
		*/
	case tcell.KeyRune:
		if c := byte(ev.Rune()); c > 0x1F && c < 0x7F {
			return asciiToScancode[c-0x20]
		}

	}
	return ScanInvalid
}

var asciiToScancode = [96]Scancode{
	ScanSpace,
	Scan1,
	ScanQuote,
	Scan3,
	Scan4,
	Scan5,
	Scan7,
	ScanQuote,
	Scan9,
	Scan0,
	Scan8,
	ScanEqual,
	ScanComma,
	ScanMinus,
	ScanPeriod,
	ScanSlash,
	Scan0,
	Scan1,
	Scan2,
	Scan3,
	Scan4,
	Scan5,
	Scan6,
	Scan7,
	Scan8,
	Scan9,
	ScanSemicolon,
	ScanSemicolon,
	ScanComma,
	ScanEqual,
	ScanPeriod,
	ScanSlash,
	Scan2,
	ScanA,
	ScanB,
	ScanC,
	ScanD,
	ScanE,
	ScanF,
	ScanG,
	ScanH,
	ScanI,
	ScanJ,
	ScanK,
	ScanL,
	ScanM,
	ScanN,
	ScanO,
	ScanP,
	ScanQ,
	ScanR,
	ScanS,
	ScanT,
	ScanU,
	ScanV,
	ScanW,
	ScanX,
	ScanY,
	ScanZ,
	ScanLBracket,
	ScanBackslash,
	ScanRBracket,
	Scan6,
	ScanMinus,
	ScanBackquote,
	ScanA,
	ScanB,
	ScanC,
	ScanD,
	ScanE,
	ScanF,
	ScanG,
	ScanH,
	ScanI,
	ScanJ,
	ScanK,
	ScanL,
	ScanM,
	ScanN,
	ScanO,
	ScanP,
	ScanQ,
	ScanR,
	ScanS,
	ScanT,
	ScanU,
	ScanV,
	ScanW,
	ScanX,
	ScanY,
	ScanZ,
	ScanLBracket,
	ScanBackslash,
	ScanRBracket,
	ScanBackquote,
}
