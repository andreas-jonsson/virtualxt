// +build ignore

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

package joystick

import (
	"log"
	"math"
	"time"

	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/veandco/go-sdl2/sdl"
)

type Device struct {
	sticks    [2]*sdl.Joystick
	timeStamp time.Time
}

func (m *Device) Install(p processor.Processor) error {
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
			stick := sdl.JoystickOpen(i)
			log.Printf("Joystick %d: %s", i, stick.Name())
			m.sticks[i] = stick
		}
	})
	if err != nil {
		return err
	}
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
	sdl.Do(func() {
		for _, stick := range m.sticks {
			if stick != nil && stick.Attached() {
				stick.Close()
			}
		}
		sdl.QuitSubSystem(sdl.INIT_JOYSTICK)
	})
	return nil
}

func (m *Device) highResistance(stick *sdl.Joystick, axis int) bool {
	d := time.Since(m.timeStamp) / time.Microsecond
	r := (float64(int32(stick.Axis(axis))+32768) / float64(math.MaxUint16)) * 100000.0
	t := 24.2 + 0.011*r
	return time.Duration(t) > d
}

func (m *Device) In(port uint16) byte {
	var data byte
	// TODO: Perhaps this should be done in another thread?
	sdl.Do(func() {
		for idx, stick := range m.sticks {
			if stick != nil && stick.Attached() {
				shift := byte(idx * 2)
				if m.highResistance(stick, 0) { // X-Axis
					data |= 1 << shift
				}
				if m.highResistance(stick, 1) { // Y-Axis
					data |= 2 << shift
				}

				data |= stick.Button(0) << (4 + shift)
				data |= stick.Button(1) << (5 + shift)
			}
		}
	})
	return data
}

func (m *Device) Out(port uint16, data byte) {
	m.timeStamp = time.Now()
}
