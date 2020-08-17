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

// #include <stdlib.h>
// typedef unsigned char Uint8;
// void AudioCallback(void *userdata, Uint8 *stream, int len);
import "C"
import (
	"errors"
	"reflect"
	"time"
	"unsafe"

	"github.com/andreas-jonsson/virtualxt/emulator/peripheral"
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
	"github.com/veandco/go-sdl2/sdl"
)

type pitInterface interface {
	GetFrequency(channel int) float64
}

type spkrDevice struct {
	cpu     processor.Processor
	pit     pitInterface
	ticker  *time.Ticker
	step    uint64
	enabled bool
	buffer  chan int8
}

//export AudioCallback
func AudioCallback(userdata unsafe.Pointer, stream *C.Uint8, length C.int) {
	m := (*spkrDevice)(userdata)
	n := int(length)
	hdr := reflect.SliceHeader{Data: uintptr(unsafe.Pointer(stream)), Len: n, Cap: n}
	buf := *(*[]C.Uint8)(unsafe.Pointer(&hdr))

	for i := 0; i < n; i++ {
		select {
		case s := <-m.buffer:
			buf[i] = C.Uint8(s)
		default:
			buf[i] = 0 // Silence?
		}
	}
}

func NewDevice() peripheral.Peripheral {
	return (*spkrDevice)(C.calloc(1, C.size_t(unsafe.Sizeof(spkrDevice{}))))
}

func (m *spkrDevice) Install(p processor.Processor) error {
	const freq = 48000
	const numSamples = 128 // ???

	m.cpu = p
	m.buffer = make(chan int8, numSamples)
	m.ticker = time.NewTicker(time.Second / freq)

	var err error
	sdl.Do(func() {
		spec := &sdl.AudioSpec{
			Freq:     freq,
			Format:   sdl.AUDIO_U8,
			Channels: 1,
			Samples:  numSamples,
			Callback: sdl.AudioCallback(C.AudioCallback),
			UserData: unsafe.Pointer(m),
		}
		if err = sdl.OpenAudio(spec, nil); err == nil {
			sdl.PauseAudio(!m.enabled)
		}
	})
	if err != nil {
		return err
	}
	return p.InstallIODeviceAt(m, 0x61)
}

func (m *spkrDevice) Name() string {
	return "PC Speaker"
}

func (m *spkrDevice) Reset() {
}

func (m *spkrDevice) Step(cycles int) error {
	if m.pit == nil {
		var ok bool
		if m.pit, ok = m.cpu.GetMappedIODevice(0x40).(pitInterface); !ok {
			return errors.New("could not find PIT")
		}
	}

	if !m.enabled {
		return nil
	}

	select {
	case <-m.ticker.C:
	default:
		return nil
	}

	const sampleRate = 48000 / 128

	fullStep := uint64(sampleRate / m.pit.GetFrequency(2))
	if fullStep < 2 {
		fullStep = 2
	}

	var sample int8 = -32
	if m.step < fullStep>>1 {
		sample = 32
	}

	select {
	case m.buffer <- sample:
	default:
	}

	m.step = (m.step + 1) % fullStep
	return nil
}

func (m *spkrDevice) In(port uint16) byte {
	return 0
}

func (m *spkrDevice) Out(_ uint16, data byte) {
	if b := data&3 == 3; b != m.enabled {
		m.enabled = b
		sdl.Do(func() {
			sdl.PauseAudio(!b)
		})
	}
}

func (m *spkrDevice) Close() error {
	sdl.Do(sdl.CloseAudio)
	C.free(unsafe.Pointer(m))
	return nil
}
