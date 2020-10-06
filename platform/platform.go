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

package platform

import (
	"io"
	"os"

	"github.com/andreas-jonsson/virtualxt/platform/dialog"
)

type internalPlatform interface{}

type Config func(internalPlatform) error

type AudioSpec struct {
	Freq,
	Channels,
	Samples int
}

type File interface {
	io.ReadWriteSeeker
	io.ReaderAt
	io.Closer
}

type FileSystem interface {
	Create(name string) (File, error)
	Open(name string) (File, error)
	OpenFile(name string, flag int, perm os.FileMode) (File, error)
}

type Platform interface {
	FileSystem

	HasAudio() bool
	RenderGraphics(backBuffer []byte, r, g, b byte)
	RenderText(mem []byte, blink bool, bg, cx, cy int)
	SetTitle(title string)
	QueueAudio(soundBuffer []byte)
	AudioSpec() AudioSpec
	EnableAudio(b bool)
	SetKeyboardHandler(h func(Scancode))
	SetMouseHandler(h func(byte, int8, int8))
}

var Instance Platform

func setDialogFileSystem(fs FileSystem) {
	dialog.OpenFileFunc = func(name string, flag int, perm os.FileMode) (dialog.File, error) {
		fp, err := fs.OpenFile(name, flag, perm)
		return fp.(dialog.File), err
	}
}

type Scancode byte

const KeyUpMask Scancode = 0x80

const (
	ScanInvalid Scancode = iota
	ScanEscape
	Scan1
	Scan2
	Scan3
	Scan4
	Scan5
	Scan6
	Scan7
	Scan8
	Scan9
	Scan0
	ScanMinus
	ScanEqual
	ScanBackspace
	ScanTab
	ScanQ
	ScanW
	ScanE
	ScanR
	ScanT
	ScanY
	ScanU
	ScanI
	ScanO
	ScanP
	ScanLBracket
	ScanRBracket
	ScanEnter
	ScanControl
	ScanA
	ScanS
	ScanD
	ScanF
	ScanG
	ScanH
	ScanJ
	ScanK
	ScanL
	ScanSemicolon
	ScanQuote
	ScanBackquote
	ScanLShift
	ScanBackslash
	ScanZ
	ScanX
	ScanC
	ScanV
	ScanB
	ScanN
	ScanM
	ScanComma
	ScanPeriod
	ScanSlash
	ScanRShift
	ScanPrint
	ScanAlt
	ScanSpace
	ScanCapslock
	ScanF1
	ScanF2
	ScanF3
	ScanF4
	ScanF5
	ScanF6
	ScanF7
	ScanF8
	ScanF9
	ScanF10
	ScanNumlock
	ScanScrlock
	ScanKPHome
	ScanKPUp
	ScanKPPageup
	ScanKPMinus
	ScanKPLeft
	ScanKP5
	ScanKPRight
	ScanKPPlus
	ScanKPEnd
	ScanKPDown
	ScanKPPagedown
	ScanKPInsert
	ScanKPDelete
)
