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
	"log"

	"github.com/veandco/go-sdl2/sdl"
)

func (p *sdlPlatform) initializeVideo() error {
	var err error
	sdl.Do(func() {
		if err = sdl.InitSubSystem(sdl.INIT_VIDEO); err != nil {
			return
		}

		sdl.SetHint(sdl.HINT_RENDER_SCALE_QUALITY, "0")
		sdl.SetHint(sdl.HINT_WINDOWS_NO_CLOSE_ON_ALT_F4, "1")
		if p.window, p.renderer, err = sdl.CreateWindowAndRenderer(p.windowSizeX, p.windowSizeY, p.sdlWindowFlags); err != nil {
			return
		}
		p.window.SetTitle("VirtualXT")
		if p.texture, err = p.renderer.CreateTexture(sdl.PIXELFORMAT_ABGR8888, sdl.TEXTUREACCESS_STREAMING, 640, 200); err != nil {
			return
		}
		err = p.renderer.SetLogicalSize(640, 480)
	})
	if err != nil {
		return err
	}

	registerCleanup(p, shutdownVideo)
	return nil
}

func shutdownVideo(p *sdlPlatform) {
	sdl.Do(func() {
		p.texture.Destroy()
		p.renderer.Destroy()
		p.window.Destroy()
		sdl.QuitSubSystem(sdl.INIT_VIDEO)
	})
}

func (p *sdlPlatform) RenderGraphics(_ VideoMode, backBuffer []byte) {
	if len(backBuffer) != 640*200*4 {
		log.Panic("invalid back buffer size")
	}

	sdl.Do(func() {
		//p.renderer.SetDrawColor(p.cgaBGColor[0], p.cgaBGColor[1], p.cgaBGColor[2], 0xFF)
		p.renderer.Clear()

		p.texture.Update(nil, backBuffer, 640*4)
		p.renderer.Copy(p.texture, nil, nil)

		p.renderer.Present()
	})
}

func (p *sdlPlatform) RenderText(mem []byte) {
	// Not supported for the SDL platform.
}

func (p *sdlPlatform) SetTitle(title string) {
	sdl.Do(func() {
		p.window.SetTitle(title)
	})
}
