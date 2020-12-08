// +build !js

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
	"bytes"
	"flag"
	"log"
	"os"
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
	setDialogFileSystem(Instance)
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

func (*tcellPlatform) Open(name string) (File, error) {
	return os.Open(name)
}

func (*tcellPlatform) Create(name string) (File, error) {
	return os.Create(name)
}

func (*tcellPlatform) OpenFile(name string, flag int, perm os.FileMode) (File, error) {
	return os.OpenFile(name, flag, perm)
}

func (p *tcellPlatform) HasAudio() bool {
	return false
}

func (p *tcellPlatform) RenderGraphics([]byte, byte, byte, byte) {
	panic("not implemented")
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
