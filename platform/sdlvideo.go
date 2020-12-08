// +build sdl

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

func (p *sdlPlatform) RenderGraphics(backBuffer []byte, r, g, b byte) {
	if len(backBuffer) != 640*200*4 {
		log.Panic("invalid back buffer size")
	}

	sdl.Do(func() {
		p.renderer.SetDrawColor(r, g, b, 0xFF)
		p.renderer.Clear()

		p.texture.Update(nil, backBuffer, 640*4)
		p.renderer.Copy(p.texture, nil, nil)

		p.renderer.Present()
	})
}

func (p *sdlPlatform) RenderText([]byte, bool, int, int, int) {
	panic("not implemented")
}

func (p *sdlPlatform) SetTitle(title string) {
	sdl.Do(func() {
		p.window.SetTitle(title)
	})
}
