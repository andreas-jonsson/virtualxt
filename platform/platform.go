/*
Copyright (c) 2019-2021 Andreas T Jonsson

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
