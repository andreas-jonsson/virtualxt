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
	"syscall/js"
	"time"

	"github.com/andreas-jonsson/virtualxt/platform/dialog"
	"github.com/spf13/afero"
)

func init() {
	go func() {
		for {
			log.Print("ping!")
			time.Sleep(time.Second * 3)
		}
	}()
}

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
	//body := document.Get("body")

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

	/*
		body.Call("appendChild", document.Call("createElement", "br"))

		fsButton := document.Call("createElement", "input")
		body.Call("appendChild", fsButton)

		fsButton.Set("type", "button")
		fsButton.Set("id", "fsButton")
		fsButton.Set("value", "Fullscreen")
		fsButton.Set("onclick", js.FuncOf(func(js.Value, []js.Value) interface{} {
			if _, err := s.ToggleFullscreen(); err != nil {
				log.Println(err)
			}
			return nil
		}))
	*/

	document.Set("onkeydown", js.FuncOf(func(_ js.Value, e []js.Value) interface{} {
		/*
			key := e[0].Get("key").String()
			select {
			case s.events <- &sys.KeyboardEvent{
				Key:  keyMapping[key],
				Type: sys.KeyboardDown,
				Name: key,
			}:
			default:
			}
		*/
		dialog.Quit()
		return nil
	}))

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
