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
	"bytes"
	"flag"
	"log"
	"sync"

	"github.com/gdamore/tcell"
)

type tcellPlatform struct {
	sync.Mutex

	buffer bytes.Buffer
	screen tcell.Screen

	mouseHandler    func(byte, int8, int8)
	keyboardHandler func(Scancode)
}

var tcellPlatformInstance tcellPlatform

func tcellStart(mainLoop func(Platform), configs ...Config) {
	// Default to max speed in CLI mode.
	flag.Set("mips", "0")
	flag.Parse()

	for _, cfg := range configs {
		if err := cfg(&tcellPlatformInstance); err != nil {
			log.Fatal(err)
		}
	}

	tcell.SetEncodingFallback(tcell.EncodingFallbackASCII)

	var err error
	if tcellPlatformInstance.screen, err = tcell.NewScreen(); err != nil {
		log.Fatal(err)
	}

	Instance = &tcellPlatformInstance
	s := tcellPlatformInstance.screen

	if err = s.Init(); err != nil {
		log.Fatal(err)
	}
	defer s.Fini()

	s.ShowCursor(-1, -1)
	s.EnableMouse()
	s.Clear()

	if err := tcellPlatformInstance.initializeTcellEvents(); err != nil {
		log.Fatal(err)
	}
	mainLoop(Instance)
}

func (p *tcellPlatform) HasAudio() bool {
	return false
}

func (p *tcellPlatform) RenderGraphics([]byte, byte, byte, byte) {
	// Not supported for the Tcell platform.
}

func (p *tcellPlatform) RenderText(mem []byte, blink bool, bg, cx, cy int) {
	p.Lock()
	p.buffer.Reset()
	p.buffer.Write(mem)
	p.Unlock()
	p.screen.PostEvent(tcell.NewEventInterrupt(drawEvent{&p.buffer, blink, bg, cx, cy}))
}

func (p *tcellPlatform) SetTitle(title string) {
}

func (p *tcellPlatform) QueueAudio(soundBuffer []byte) {
}

func (p *tcellPlatform) AudioSpec() AudioSpec {
	return AudioSpec{}
}

func (p *tcellPlatform) EnableAudio(b bool) {
}

func (p *tcellPlatform) SetKeyboardHandler(h func(Scancode)) {
	p.Lock()
	p.keyboardHandler = h
	p.Unlock()
}

func (p *tcellPlatform) SetMouseHandler(h func(byte, int8, int8)) {
	p.Lock()
	p.mouseHandler = h
	p.Unlock()
}
