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

package cpu

import (
	"bytes"
	"fmt"
	"io/ioutil"
	"testing"

	"github.com/andreas-jonsson/virtualxt/emulator/memory"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/pic"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/ram"
	"github.com/andreas-jonsson/virtualxt/emulator/peripheral/rom"
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
)

func loadBin(t *testing.T, name string) []byte {
	bin, err := ioutil.ReadFile(name)
	if err != nil {
		t.Fatal(err)
	}
	return bin
}

func runTest(t *testing.T, progName string) *CPU {
	p := NewCPU([]peripheral.Peripheral{
		&ram.Device{},
		&rom.Device{
			RomName: fmt.Sprintf("TEST: %s.bin", progName),
			Base:    memory.NewPointer(0xF000, 0),
			Reader:  bytes.NewReader(loadBin(t, fmt.Sprintf("testdata/%s.bin", progName))),
		},
		&pic.Device{},
		//&debug.Device{},
	})
	defer p.Close()

	// Tests are written for 80186+ machines.
	p.SetV20Support(true)

	p.Reset()
	p.IP = 0xFFF0
	p.CS = 0xF000

	for {
		if _, err := p.Step(); err != nil {
			if err != processor.ErrCPUHalt {
				t.Fatal(err)
			}
			break
		}
		if p.Registers.Debug {
			t.Fatal("CPU hit breakpoint!")
		}
	}
	return p
}

func runTestAndVerify(t *testing.T, progName string, nerr int) {
	p := runTest(t, progName)
	res := loadBin(t, fmt.Sprintf("testdata/res_%s.bin", progName))
	cerr := 0
	for i, v := range res {
		if r := p.ReadByte(memory.NewPointer(0, uint16(i))); r != v {
			t.Logf("Invalid result at offset 0x%X. (Got 0x%X but expected 0x%X)", i, r, v)
			cerr++
		}
	}
	if cerr != nerr {
		t.Fatalf("%d bytes diff", cerr)
	}
}

func TestAdd(t *testing.T) {
	runTestAndVerify(t, "add", 0)
}

func TestBcdcnv(t *testing.T) {
	runTestAndVerify(t, "bcdcnv", 2)
}

func TestBitwise(t *testing.T) {
	runTestAndVerify(t, "bitwise", 0)
}

func TestCmpneg(t *testing.T) {
	runTestAndVerify(t, "cmpneg", 0)
}

func TestControl(t *testing.T) {
	runTestAndVerify(t, "control", 0)
}

func TestDatatrnf(t *testing.T) {
	runTestAndVerify(t, "datatrnf", 0)
}

func TestDiv(t *testing.T) {
	runTestAndVerify(t, "div", 3)
}

func TestInterrupt(t *testing.T) {
	runTestAndVerify(t, "interrupt", 0)
}

func TestJmpmov(t *testing.T) {
	p := runTest(t, "jmpmov")
	if r := p.ReadWord(memory.Pointer(0)); r != 0x4001 {
		t.Errorf("Invalid result! (Got 0x%X but expected 0x4001)", r)
	}
}

func TestJump1(t *testing.T) {
	runTestAndVerify(t, "jump1", 0)
}

func TestJump2(t *testing.T) {
	runTestAndVerify(t, "jump2", 0)
}

func TestMul(t *testing.T) {
	runTestAndVerify(t, "mul", 8)
}

func TestRep(t *testing.T) {
	runTestAndVerify(t, "rep", 0)
}

func TestRotate(t *testing.T) {
	runTestAndVerify(t, "rotate", 0)
}

func TestSegpr(t *testing.T) {
	runTestAndVerify(t, "segpr", 0)
}

func TestShifts(t *testing.T) {
	runTestAndVerify(t, "shifts", 0)
}

func TestStrings(t *testing.T) {
	runTestAndVerify(t, "strings", 0)
}

func TestSub(t *testing.T) {
	runTestAndVerify(t, "sub", 1)
}
