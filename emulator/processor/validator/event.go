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
	"math"
)

const (
	DefulatQueueSize  = 1024            // 1KB
	DefaultBufferSize = 0x100000 * 1024 // 1GB
)

const (
	AL byte = iota
	AH
	AX
	CL
	CH
	CX
	DL
	DH
	DX
	BL
	BH
	BX
	SP
	BP
	SI
	DI
	ES
	CS
	SS
	DS
)

const (
	WideReg byte = 1 << iota
	ReadReg
	WriteReg
)

type RegOp struct {
	Flags, Name byte
	Data        uint16
}

type Event struct {
	Opcode        byte
	Regs          [16]RegOp
	Reads, Writes [10]MemOp
}

type MemOp struct {
	Addr uint32
	Data byte
}

var emptyMemOp = MemOp{math.MaxUint32, 0}

var EmptyEvent = Event{
	Reads: [10]MemOp{
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
	},
	Writes: [10]MemOp{
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
		emptyMemOp,
	},
}
