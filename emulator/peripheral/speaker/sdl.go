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

package speaker

import (
	"log"
	"sync"
	"time"

	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/veandco/go-sdl2/sdl"
)

const (
	frequency  = 48000
	latency    = 10
	toneVolume = 32
)

type pitInterface interface {
	GetFrequency(channel int) float64
}

type Device struct {
	cpu processor.Processor
	pit pitInterface

	deviceID sdl.AudioDeviceID
	spec     *sdl.AudioSpec

	sampleIndex          uint64
	toneHz, toneHzBuffer float64

	enabled, turbo bool
	port           byte

	lock     sync.Mutex
	quitChan chan struct{}
}

func nextPow(v uint16) uint16 {
	v--
	v |= v >> 1
	v |= v >> 2
	v |= v >> 4
	v |= v >> 8
	v++
	return v
}

func (m *Device) Install(p processor.Processor) error {
	var err error
	sdl.Do(func() {
		if err = sdl.InitSubSystem(sdl.INIT_AUDIO); err != nil {
			return
		}

		m.spec = &sdl.AudioSpec{
			Freq:     frequency,
			Format:   sdl.AUDIO_U8,
			Channels: 1,
			Samples:  nextPow(uint16((frequency / 1000) * latency)),
		}

		var have sdl.AudioSpec
		if m.deviceID, err = sdl.OpenAudioDevice("", false, m.spec, &have, sdl.AUDIO_ALLOW_FREQUENCY_CHANGE|sdl.AUDIO_ALLOW_CHANNELS_CHANGE); err == nil {
			m.spec = &have
			sdl.PauseAudioDevice(m.deviceID, true)
		}
	})
	if err != nil {
		return err
	}

	m.cpu = p
	m.startUpdateLoop()
	return p.InstallIODeviceAt(m, 0x61)
}

func (m *Device) TurboSwitch() bool {
	return m.turbo
}

func (m *Device) Name() string {
	return "PC Speaker"
}

func (m *Device) Reset() {
	m.lock.Lock()
	defer m.lock.Unlock()

	m.sampleIndex = 0
	m.toneHz = 0
	m.toneHzBuffer = 0
	m.port = 4
	m.turbo = true
	m.enabled = false

	sdl.Do(func() {
		sdl.PauseAudioDevice(m.deviceID, true)
		sdl.ClearQueuedAudio(m.deviceID)
	})
}

func (m *Device) startUpdateLoop() {
	m.quitChan = make(chan struct{})

	if m.pit == nil {
		var ok bool
		if m.pit, ok = m.cpu.GetMappedIODevice(0x40).(pitInterface); !ok {
			log.Print("could not find PIT")
			return
		}
	}

	go func() {
		numSamples := int(m.spec.Samples)
		bytesPerSample := int(m.spec.Channels)
		bytesToWrite := numSamples * bytesPerSample

		soundBuffer := make([]byte, bytesToWrite)
		sampleCount := bytesToWrite / bytesPerSample

		ticker := time.NewTicker(time.Second / time.Duration(int(m.spec.Freq)/numSamples))
		defer ticker.Stop()

	loop:
		for {
			select {
			case <-m.quitChan:
				close(m.quitChan)
				return
			case <-ticker.C:
				m.lock.Lock()

				if !m.enabled || m.toneHz == 0 {
					m.lock.Unlock()
					continue loop
				}

				squareWavePeriod := uint64(float64(m.spec.Freq) / m.toneHz)
				halfSquareWavePeriod := squareWavePeriod / 2
				if halfSquareWavePeriod == 0 {
					m.lock.Unlock()
					continue loop
				}

				var ptr int
				for i := 0; i < sampleCount; i++ {
					var sampleValue int8 = -toneVolume
					if m.sampleIndex++; (m.sampleIndex/halfSquareWavePeriod)%2 != 0 {
						sampleValue = toneVolume
					}

					for j := 0; j < int(m.spec.Channels); j++ {
						soundBuffer[ptr] = byte(sampleValue)
						ptr++
					}
				}

				m.lock.Unlock()

				sdl.Do(func() {
					sdl.QueueAudio(m.deviceID, soundBuffer)
				})
			}
		}
	}()
}

func (m *Device) Step(int) error {
	if toneHz := m.pit.GetFrequency(2); toneHz != m.toneHzBuffer {
		m.lock.Lock()
		m.toneHzBuffer = toneHz
		m.toneHz = toneHz
		m.lock.Unlock()
	}
	return nil
}

func (m *Device) In(port uint16) byte {
	return m.port
}

func (m *Device) Out(_ uint16, data byte) {
	m.lock.Lock()
	defer m.lock.Unlock()

	m.port = data
	turbo := data&4 != 0

	if m.turbo != turbo {
		m.turbo = turbo
		log.Print("Turbo switch: ", turbo)
	}

	if b := data&3 == 3; b != m.enabled {
		m.enabled = b
		sdl.Do(func() {
			sdl.ClearQueuedAudio(m.deviceID)
			sdl.PauseAudioDevice(m.deviceID, !b)
			m.sampleIndex = 0
		})
	}
}

func (m *Device) Close() error {
	m.quitChan <- struct{}{}
	<-m.quitChan

	sdl.Do(func() {
		sdl.CloseAudioDevice(m.deviceID)
		sdl.QuitSubSystem(sdl.INIT_AUDIO)
	})
	return nil
}
