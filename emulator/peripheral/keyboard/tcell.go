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
	case tcell.KeyPrint:
		return ScanPrint
	case tcell.KeyDelete:
		return ScanKPDelete
	case tcell.KeyInsert:
		return ScanKPInsert
	case tcell.KeyEnd:
		return ScanKPEnd
	case tcell.KeyPgUp:
		return ScanKPUp
	case tcell.KeyPgDn:
		return ScanKPDown
	case tcell.KeyHome:
		return ScanKPHome
	case tcell.KeyF1:
		return ScanF1
	case tcell.KeyF2:
		return ScanF2
	case tcell.KeyF3:
		return ScanF3
	case tcell.KeyF4:
		return ScanF4
	case tcell.KeyF5:
		return ScanF5
	case tcell.KeyF6:
		return ScanF6
	case tcell.KeyF7:
		return ScanF7
	case tcell.KeyF8:
		return ScanF8
	case tcell.KeyF9:
		return ScanF9
	case tcell.KeyF10:
		return ScanF10

	// Not implemented!
	// ScanAlt, ScanNumlock, ScanScrlock, ScanCapslock, ScanLShift, ScanRShift

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
