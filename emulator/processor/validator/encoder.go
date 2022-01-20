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

package validator

import (
	"bytes"
	"compress/gzip"
	"log"
)

type Encoder struct {
	writer *gzip.Writer
}

func NewEncoder(buffer *bytes.Buffer) *Encoder {
	return &Encoder{ writer: gzip.NewWriter(buffer) }
}

func (enc *Encoder) Encode(event Event) {
	enc.AppendByte(event.Opcode)
	enc.AppendByte(event.OpcodeExt)
	
	infos := [2]RegsInfo{event.Before, event.After}
	for _, info := range infos {
		enc.AppendByte(uint8(info.IP >> 0x8))
		enc.AppendByte(uint8(info.IP & 0xff))

		enc.AppendByte(uint8(info.Flags >> 0x8))
		enc.AppendByte(uint8(info.Flags & 0xff))

		for _, reg := range info.Regs {
			enc.AppendByte(uint8(reg >> 0x8))
			enc.AppendByte(uint8(reg & 0xff))
		}
	}

	events := [2][10]MemOp{event.Reads, event.Writes}
	for _, ops := range events {
		for _, op := range ops {
			enc.AppendByte(uint8(op.Addr >> 0x18))
			enc.AppendByte(uint8(op.Addr >> 0x10 & 0xff))
			enc.AppendByte(uint8(op.Addr >> 0x8 & 0xff))
			enc.AppendByte(uint8(op.Addr & 0xff))
			
			enc.AppendByte(op.Data)
		}
	}
}

func (enc *Encoder) AppendByte(value byte) {
	enc.writer.Write([]byte{value})
}

func (enc *Encoder) Close() {
	if err := enc.writer.Close(); err != nil {
        log.Print(err)
    }
}
