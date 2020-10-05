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

package platform

import (
	"flag"
	"os"

	"github.com/andreas-jonsson/virtualxt/platform/dialog"
	"github.com/veandco/go-sdl2/sdl"
)

type sdlPlatform struct {
	postInitConfigs []func(*sdlPlatform) error
	cleanCallBacks  []func(*sdlPlatform)

	quitChan        chan struct{}
	mouseHandler    func(byte, int8, int8)
	keyboardHandler func(Scancode)

	sdlFlags, sdlWindowFlags uint32
	windowSizeX, windowSizeY int32

	audioSpec     *sdl.AudioSpec
	audioDeviceID sdl.AudioDeviceID

	cgaBGColor [3]byte

	window   *sdl.Window
	renderer *sdl.Renderer
	texture  *sdl.Texture
}

var sdlPlatformInstance sdlPlatform

func registerPostConfig(p internalPlatform, cfg func(*sdlPlatform) error) {
	sp := p.(*sdlPlatform)
	sp.postInitConfigs = append(sp.postInitConfigs, cfg)
}

func registerCleanup(p internalPlatform, cb func(p *sdlPlatform)) {
	sp := p.(*sdlPlatform)
	sp.cleanCallBacks = append(sp.cleanCallBacks, cb)
}

func ConfigWithWindowSize(w, h int) Config {
	return func(p internalPlatform) error {
		sp := p.(*sdlPlatform)
		sp.windowSizeX = int32(w)
		sp.windowSizeY = int32(h)
		return nil
	}
}

func ConfigWithAudio(p internalPlatform) error {
	const (
		frequency  = 48000
		latency    = 10
		toneVolume = 32
	)

	nextPow := func(v uint16) uint16 {
		v--
		v |= v >> 1
		v |= v >> 2
		v |= v >> 4
		v |= v >> 8
		v++
		return v
	}

	registerPostConfig(p, func(sp *sdlPlatform) error {
		var err error
		sdl.Do(func() {
			if err = sdl.InitSubSystem(sdl.INIT_AUDIO); err != nil {
				return
			}

			sp.audioSpec = &sdl.AudioSpec{
				Freq:     frequency,
				Format:   sdl.AUDIO_U8,
				Channels: 1,
				Samples:  nextPow(uint16((frequency / 1000) * latency)),
			}

			var have sdl.AudioSpec
			if sp.audioDeviceID, err = sdl.OpenAudioDevice("", false, sp.audioSpec, &have, sdl.AUDIO_ALLOW_FREQUENCY_CHANGE|sdl.AUDIO_ALLOW_CHANNELS_CHANGE); err == nil {
				sp.audioSpec = &have
				sdl.PauseAudioDevice(sp.audioDeviceID, true)

				registerCleanup(p, func(sp *sdlPlatform) {
					sdl.Do(func() {
						sdl.CloseAudioDevice(sp.audioDeviceID)
						sdl.QuitSubSystem(sdl.INIT_AUDIO)
					})
				})
			}
		})
		return err
	})
	return nil
}

func ConfigWithFullscreen(p internalPlatform) error {
	p.(*sdlPlatform).sdlWindowFlags |= sdl.WINDOW_FULLSCREEN_DESKTOP
	return nil
}

func Start(mainLoop func(Platform), configs ...Config) {
	if f := flag.Lookup("text"); f != nil && f.Value.(flag.Getter).Get().(bool) {
		tcellStart(mainLoop, []Config{}...)
		return
	}

	errHandle := func(err error) {
		dialog.ShowErrorMessage(err.Error())
		os.Exit(-1)
	}

	p := &sdlPlatformInstance
	p.windowSizeX = 640
	p.windowSizeY = 480
	p.sdlWindowFlags = sdl.WINDOW_RESIZABLE

	sdl.Main(func() {
		for _, cfg := range configs {
			if err := cfg(p); err != nil {
				errHandle(err)
			}
		}

		if err := sdl.Init(p.sdlFlags); err != nil {
			errHandle(err)
		}
		defer sdl.Quit()

		for _, cfg := range p.postInitConfigs {
			if err := cfg(p); err != nil {
				errHandle(err)
			}
		}

		defer func() {
			for _, cb := range p.cleanCallBacks {
				cb(p)
			}
		}()

		Instance = p
		if err := p.initializeVideo(); err != nil {
			errHandle(err)
		}
		if err := p.initializeSDLEvents(); err != nil {
			errHandle(err)
		}
		mainLoop(p)
	})
	os.Exit(0) // Calling Exit is required!
}

func (p *sdlPlatform) HasAudio() bool {
	return p.audioSpec != nil
}

func (p *sdlPlatform) QueueAudio(soundBuffer []byte) {
	if p.HasAudio() {
		sdl.Do(func() {
			sdl.QueueAudio(p.audioDeviceID, soundBuffer)
		})
	}
}

func (p *sdlPlatform) AudioSpec() AudioSpec {
	return AudioSpec{
		Freq:     int(p.audioSpec.Freq),
		Channels: int(p.audioSpec.Channels),
		Samples:  int(p.audioSpec.Samples),
	}
}

func (p *sdlPlatform) EnableAudio(b bool) {
	if p.HasAudio() {
		sdl.Do(func() {
			sdl.ClearQueuedAudio(p.audioDeviceID)
			sdl.PauseAudioDevice(p.audioDeviceID, !b)
		})
	}
}
