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

package validator

import (
	"math"

	"github.com/andreas-jonsson/virtualxt/emulator/processor"
)

const (
	DefulatQueueSize  = 1024            // 1KB
	DefaultBufferSize = 0x100000 * 1024 // 1GB
)

type Event struct {
	Opcode        byte
	Regs          [2]processor.Registers
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
