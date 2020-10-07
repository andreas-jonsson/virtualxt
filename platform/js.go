// +build js

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
	"log"
	"net/http"
	"os"
	"path/filepath"
	"strings"
	"syscall/js"

	"github.com/spf13/afero"
)

type jsPlatform struct {
	canvas, context js.Value
	fileSystem      afero.Fs

	mouseHandler    func(byte, int8, int8)
	keyboardHandler func(Scancode)
}

var jsPlatformInstance jsPlatform

func ConfigWithWindowSize(w, h int) Config {
	return func(internalPlatform) error {
		return nil
	}
}

func ConfigWithAudio(p internalPlatform) error {
	return nil
}

func ConfigWithFullscreen(p internalPlatform) error {
	return nil
}

func Start(mainLoop func(Platform), configs ...Config) {
	jsPlatformInstance.fileSystem = afero.NewMemMapFs()

	for _, cfg := range configs {
		if err := cfg(&jsPlatformInstance); err != nil {
			log.Fatal(err)
		}
	}

	document := js.Global().Get("document")
	canvas := document.Call("getElementById", "virtualxt-canvas")
	if canvas.IsNull() {
		log.Fatal("Could not find \"virtualxt-canvas\"")
	}
	jsPlatformInstance.canvas = canvas

	canvas.Call("setAttribute", "width", "640")
	canvas.Call("setAttribute", "height", "200")
	canvas.Set("imageSmoothingEnabled", false)
	canvas.Set("oncontextmenu", js.FuncOf(func(js.Value, []js.Value) interface{} { return nil }))

	style := canvas.Get("style")
	style.Set("width", "640px")
	style.Set("height", "480px")
	style.Set("cursor", "none")

	jsPlatformInstance.context = canvas.Call("getContext", "2d")

	document.Set("onkeydown", js.FuncOf(func(_ js.Value, e []js.Value) interface{} {
		a := e[0]
		if h := jsPlatformInstance.keyboardHandler; h != nil {
			if s := toScancode(a.Get("key").String()); s != ScanInvalid {
				a.Call("preventDefault")
				h(s)
			}
		}
		return nil
	}))

	document.Set("onkeyup", js.FuncOf(func(_ js.Value, e []js.Value) interface{} {
		a := e[0]
		if h := jsPlatformInstance.keyboardHandler; h != nil {
			if s := toScancode(a.Get("key").String()); s != ScanInvalid {
				a.Call("preventDefault")
				h(s | KeyUpMask)
			}
		}
		return nil
	}))

	jsMouseHandler := js.FuncOf(func(_ js.Value, e []js.Value) interface{} {
		a := e[0]
		if h := jsPlatformInstance.mouseHandler; h != nil {
			x, y := a.Get("movementX").Int(), a.Get("movementY").Int()
			state := byte(a.Get("buttons").Int())

			var buttons byte
			if state&1 != 0 {
				buttons = 2
			}
			if state&2 != 0 {
				buttons |= 1
			}
			h(buttons, int8(x), int8(y))
		}
		return nil
	})

	canvas.Set("onmousemove", jsMouseHandler)
	canvas.Set("onmouseup", jsMouseHandler)
	canvas.Set("onmousedown", jsMouseHandler)

	Instance = &jsPlatformInstance
	setDialogFileSystem(Instance)
	mainLoop(Instance)
}

func (p *jsPlatform) downloadFile(name string) error {
	resp, err := http.Get(name)
	if err != nil {
		return err
	}
	defer resp.Body.Close()

	if err := p.fileSystem.MkdirAll(filepath.Dir(name), 0755); err != nil {
		return err
	}

	fp, err := p.fileSystem.Create(name)
	if err != nil {
		return err
	}
	defer fp.Close()

	if _, err = io.Copy(fp, resp.Body); err != nil {
		return err
	}
	return nil
}

func (p *jsPlatform) Open(name string) (File, error) {
	if fp, err := p.fileSystem.Open(name); err == nil {
		return fp, nil
	}
	if err := p.downloadFile(name); err != nil {
		return nil, err
	}
	return p.fileSystem.Open(name)
}

func (p *jsPlatform) Create(name string) (File, error) {
	return p.fileSystem.Create(name)
}

func (p *jsPlatform) OpenFile(name string, flag int, perm os.FileMode) (File, error) {
	if fp, err := p.fileSystem.OpenFile(name, flag, perm); err == nil {
		return fp, nil
	}
	if err := p.downloadFile(name); err != nil {
		return nil, err
	}
	return p.fileSystem.OpenFile(name, flag, perm)
}

func (p *jsPlatform) HasAudio() bool {
	return false
}

func (p *jsPlatform) RenderGraphics(backBuffer []byte, r, g, b byte) {
	img := p.context.Call("getImageData", 0, 0, 640, 200)
	data := img.Get("data")

	js.CopyBytesToJS(data, backBuffer)
	p.context.Call("putImageData", img, 0, 0)
}

func (p *jsPlatform) RenderText([]byte, bool, int, int, int) {
	panic("not implemented")
}

func (p *jsPlatform) SetTitle(title string) {
	js.Global().Get("document").Set("title", title)
}

func (p *jsPlatform) QueueAudio(soundBuffer []byte) {
}

func (p *jsPlatform) AudioSpec() AudioSpec {
	return AudioSpec{}
}

func (p *jsPlatform) EnableAudio(b bool) {
}

func (p *jsPlatform) SetKeyboardHandler(h func(Scancode)) {
	p.keyboardHandler = h
}

func (p *jsPlatform) SetMouseHandler(h func(byte, int8, int8)) {
	p.mouseHandler = h
}

func toScancode(key string) Scancode {
	switch strings.ToLower(key) {
	case "escape":
		return ScanEscape
	case "1":
		return Scan1
	case "2":
		return Scan2
	case "3":
		return Scan3
	case "4":
		return Scan4
	case "5":
		return Scan5
	case "6":
		return Scan6
	case "7":
		return Scan7
	case "8":
		return Scan8
	case "9":
		return Scan9
	case "0":
		return Scan0
	//case "-":
	//	return ScanMinus
	//case "=":
	//	return ScanEqual
	case "backspace":
		return ScanBackspace
	case "tab":
		return ScanTab
	case "q":
		return ScanQ
	case "w":
		return ScanW
	case "e":
		return ScanE
	case "r":
		return ScanR
	case "t":
		return ScanT
	case "y":
		return ScanY
	case "u":
		return ScanU
	case "i":
		return ScanI
	case "o":
		return ScanO
	case "p":
		return ScanP
		/*
			case sdl.SCANCODE_LEFTBRACKET:
				return ScanLBracket
			case sdl.SCANCODE_RIGHTBRACKET:
				return ScanRBracket
		*/
	case "enter":
		return ScanEnter
	case "control":
		return ScanControl
	case "a":
		return ScanA
	case "s":
		return ScanS
	case "d":
		return ScanD
	case "f":
		return ScanF
	case "g":
		return ScanG
	case "h":
		return ScanH
	case "j":
		return ScanJ
	case "k":
		return ScanK
	case "l":
		return ScanL
	//case ";":
	//	return ScanSemicolon
	/*
		case sdl.SCANCODE_APOSTROPHE:
			return ScanQuote
		case sdl.SCANCODE_GRAVE:
			return ScanBackquote
		case sdl.SCANCODE_LSHIFT:
			return ScanLShift
	*/
	//case "/":
	//	return ScanBackslash
	case "z":
		return ScanZ
	case "x":
		return ScanX
	case "c":
		return ScanC
	case "v":
		return ScanV
	case "b":
		return ScanB
	case "n":
		return ScanN
	case "m":
		return ScanM
	case ",":
		return ScanComma
	case ".":
		return ScanPeriod
	//case "\\":
	//	return ScanSlash
	case "shift":
		return ScanRShift
	//case sdl.SCANCODE_PRINTSCREEN:
	//	return ScanPrint
	case "alt", "altgraph":
		return ScanAlt
	case " ":
		return ScanSpace
	/*
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
	*/
	case "home":
		return ScanKPHome
	case "arrowup":
		return ScanKPUp
	case "pageup":
		return ScanKPPageup
	//case sdl.SCANCODE_KP_MINUS:
	//	return ScanKPMinus
	case "arrowleft":
		return ScanKPLeft
	case "clear":
		return ScanKP5
	case "arrowright":
		return ScanKPRight
	//case "+":
	//	return ScanKPPlus
	case "end":
		return ScanKPEnd
	case "arrowdown":
		return ScanKPDown
	case "pagedown":
		return ScanKPPagedown
	case "insert":
		return ScanKPInsert
	case "delete":
		return ScanKPDelete
	default:
		log.Print("Invalid key: ", key)
		return ScanInvalid
	}
}
