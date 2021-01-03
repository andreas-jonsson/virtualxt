// +build sdl

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

package joystick

import (
	"flag"
	"log"
	"math"
	"sync"
	"time"

	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/veandco/go-sdl2/sdl"
)

type joystick struct {
	stick    *sdl.Joystick
	axis     [2]int16
	timeouts [2]float64
	buttons  byte
	attached bool
}

type Device struct {
	lock      sync.RWMutex
	sticks    [2]joystick
	timeStamp time.Time
	quitChan  chan struct{}
}

func (m *Device) Install(p processor.Processor) error {
	if !enabled {
		return nil
	}

	var err error
	sdl.Do(func() {
		if err = sdl.InitSubSystem(sdl.INIT_JOYSTICK); err != nil {
			return
		}

		numSticks := sdl.NumJoysticks()
		if numSticks == 0 {
			log.Print("No joystick's found!")
		}

		for i := 0; i < numSticks && i < 2; i++ {
			s := &m.sticks[i]
			s.stick = sdl.JoystickOpen(i)
			s.buttons = 3
			s.attached = true
			log.Printf("Joystick %d: %s", i, s.stick.Name())
		}
	})
	if err != nil {
		return err
	}

	m.startUpdateLoop()
	return p.InstallIODeviceAt(m, 0x201)
}

func (m *Device) Name() string {
	return "Game Port Joysticks"
}

func (m *Device) Reset() {
}

func (m *Device) Step(int) error {
	return nil
}

func (m *Device) Close() error {
	if !enabled {
		return nil
	}
	m.quitChan <- struct{}{}
	<-m.quitChan
	return nil
}

func (m *Device) startUpdateLoop() {
	m.quitChan = make(chan struct{})
	go func() {
		ticker := time.NewTicker(time.Second / 30)
		defer ticker.Stop()

		for {
			select {
			case <-m.quitChan:
				sdl.Do(func() {
					for _, stick := range m.sticks {
						if s := stick.stick; s != nil && s.Attached() {
							s.Close()
						}
					}
					sdl.QuitSubSystem(sdl.INIT_JOYSTICK)
				})
				close(m.quitChan)
				return
			case <-ticker.C:
				sdl.Do(func() {
					m.lock.Lock()
					for idx := range m.sticks {
						stick := &m.sticks[idx]
						s := stick.stick
						stick.attached = s != nil && s.Attached()

						if stick.attached {
							stick.axis[0] = s.Axis(0)
							stick.axis[1] = s.Axis(1)
							stick.buttons = s.Button(0) | (s.Button(1) << 1)
						}
					}
					m.lock.Unlock()
				})
			}
		}
	}()
}

func (m *Device) In(port uint16) byte {
	var data byte = 0xF0
	d := float64(time.Since(m.timeStamp) / time.Microsecond)

	m.lock.RLock()
	for i := range m.sticks {
		if stick := &m.sticks[i]; stick.attached {
			shift := byte(i * 2)

			xt := &stick.timeouts[0] // X-Axis
			if *xt -= d; *xt > 0 {
				data |= 1 << shift
			} else {
				*xt = 0
			}

			yt := &stick.timeouts[1] // Y-Axis
			if *yt -= d; *yt > 0 {
				data |= 2 << shift
			} else {
				*yt = 0
			}

			data ^= stick.buttons << (4 + shift)
		}
	}
	m.lock.RUnlock()
	return data
}

func posToOhm(axis int16) float64 {
	return (float64(axis+1) / float64(math.MaxUint16)) * 60000
}

func axisTimeout(axis int16) float64 {
	return (24.2 + 0.011*posToOhm(axis)) * 1000.0
}

func (m *Device) Out(port uint16, data byte) {
	m.timeStamp = time.Now()
	m.lock.RLock()
	for i := range m.sticks {
		if stick := &m.sticks[i]; stick.attached {
			stick.timeouts[0] = axisTimeout(stick.axis[0])
			stick.timeouts[1] = axisTimeout(stick.axis[1])
		}
	}
	m.lock.RUnlock()
}

var enabled bool

func init() {
	flag.BoolVar(&enabled, "joystick", enabled, "Enable joystick support")
}
