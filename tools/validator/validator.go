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

package main

import (
	"compress/gzip"
	"flag"
	"log"
	"os"
	"io"
	"fmt"

	"github.com/andreas-jonsson/virtualxt/emulator/processor/validator"
	"github.com/andreas-jonsson/virtualxt/emulator/processor"
)

var (
	vxtInput = "virtualxt.bin"
	refInput = "validator.bin"
)

type MismatchType = int

const (
	Opcode MismatchType = iota

	IpBefore
	IpAfter

	FlagsBefore
	FlagsAfter

	RegsBefore
	RegsAfter

	MemRead
	MemWrite
)

const (
	BeforeOffset = 2
	AfterOffset  = 30
	ReadsOffset  = 58
	WritesOffset = 108
)

func init() {
	flag.StringVar(&vxtInput, "virtualxt", vxtInput, "VirtualXT CPU")
	flag.StringVar(&refInput, "validation", refInput, "Reference CPU")
}

func main() {
	flag.Parse()
	log.SetFlags(0)

	vxtFp, err := os.Open(vxtInput)
	if err != nil {
		log.Panic(err)
	}
	defer vxtFp.Close()

	refFp, err := os.Open(refInput)
	if err != nil {
		log.Panic(err)
	}
	defer refFp.Close()

	vxtDec, err := gzip.NewReader(vxtFp)
	if err != nil {
		log.Panic(err)
	}
	defer vxtDec.Close()

	refDec, err := gzip.NewReader(refFp)
	if err != nil {
		log.Panic(err)
	}
	defer refDec.Close()

	var vxt_events []validator.Event
	var ref_events []validator.Event

	fmt.Println("Decoding events...")

	for {
		vbuf := make([]byte, 158)
		rbuf := make([]byte, 158)

		_, err1 := io.ReadFull(vxtDec, vbuf)
		_, err2 := io.ReadFull(refDec, rbuf)

		vxt_events = append(vxt_events, decodeEvent(vbuf))
		ref_events = append(ref_events, decodeEvent(rbuf))
	
		if (err1 == io.EOF && err2 != io.EOF) || (err1 != io.EOF && err2 == io.EOF) {
			fmt.Println("Warning: The number of events is not similar for the two files, the smallest one will be taken")
			break
		} else if err1 == io.EOF && err2 == io.EOF {
			break
		}

		if err1 != nil {
			log.Panic("Error reading", vxtInput, err.Error())
		} else if err2 != nil {
			log.Panic("Error reading", refInput, err.Error())
		}
	}

	fmt.Print("Decoded", len(vxt_events), "event(s).\n\n")

	for i := 0; i < len(vxt_events); i++ {
		vxt := vxt_events[i]
		ref := ref_events[i]

		n_differences := 0
		fmt.Printf("Checking for differences at address %s (opcode %02X)...\n\n", addrToString(vxt.Before), vxt.Opcode);

		if vxt.Opcode != ref.Opcode {
			printDiff(vxt, ref, Opcode)
			n_differences++
		}

		if vxt.Before.IP != ref.Before.IP {
			printDiff(vxt, ref, IpBefore)
			n_differences++
		}
		if vxt.After.IP != ref.After.IP {
			printDiff(vxt, ref, IpAfter)
			n_differences++
		}

		if vxt.Before.Flags != ref.Before.Flags {
			printDiff(vxt, ref, FlagsBefore)
			n_differences++
		}
		if vxt.After.Flags != ref.After.Flags {
			printDiff(vxt, ref, FlagsAfter)
			n_differences++
		}

		if vxt.Before.Regs != ref.Before.Regs {
			printDiff(vxt, ref, RegsBefore)
			n_differences++
		}
		if vxt.After.Regs != ref.After.Regs {
			printDiff(vxt, ref, RegsAfter)
			n_differences++
		}

		fmt.Print("\nFound", n_differences, "difference(s).\n\n")

		if n_differences > 0 {
			fmt.Println("Memory writes/reads (virtualxt):")
			printMemOps(vxt)

			fmt.Println("Memory writes/reads (reference):")
			printMemOps(ref)
		}

		fmt.Println("------------------")
	}
}

func decodeEvent(buffer []byte) validator.Event {
	opcode := buffer[0]
	opcodeExt := buffer[1]

	before := decodeRegsInfo(buffer, BeforeOffset)
	after := decodeRegsInfo(buffer, AfterOffset)

	before.Flags = maskUndefFlags(opcode, opcodeExt, before.Flags)
	after.Flags = maskUndefFlags(opcode, opcodeExt, before.Flags)

	reads := decodeMemOp(buffer, ReadsOffset)
	writes := decodeMemOp(buffer, WritesOffset)
	
	return validator.Event{opcode, opcodeExt, before, after, reads, writes}
}

func decodeRegsInfo(buffer []byte, offset int) validator.RegsInfo {
	info := validator.RegsInfo{}
	info.IP = uint16(buffer[offset]) << 0x8 | uint16(buffer[offset + 1])
	info.Flags = uint16(buffer[offset + 2]) << 0x8 | uint16(buffer[offset + 3])

	for i := 0; i < 12; i++ {
		offset := offset + 4 + 2 * i
		info.Regs[i] = uint16(buffer[offset]) << 0x8 | uint16(buffer[offset + 1])
	}
	return info
}

func decodeMemOp(buffer []byte, offset int) [10]validator.MemOp {
	op := [10]validator.MemOp{}
	for i := 0; i < 10; i++ {
		offset := offset + 5 * i
		op[i].Addr = uint32(buffer[offset]) << 0x18 | uint32(buffer[offset + 1]) << 0x10 | uint32(buffer[offset + 2]) << 0x8 | uint32(buffer[offset + 3])
		op[i].Data = buffer[offset + 4]
	}
	return op
}

func printDiff(a, b validator.Event, mtype MismatchType) {
	switch mtype {
	case Opcode:
		fmt.Printf("* Opcodes mismatch: got %02X, expected %02X\n", a.Opcode, b.Opcode)
	case IpBefore:
		fmt.Printf("* IPs mismatch: got %04X, expected %04X (before execution)\n", a.Before.IP, b.Before.IP)
	case IpAfter:
		fmt.Printf("* IPs mismatch: got %04X, expected %04X (after execution)\n", a.After.IP, b.After.IP)
	case FlagsBefore:
		fmt.Printf("* Flags mismatch: got %04X, expected %04X (before execution)\n", a.Before.Flags, b.Before.Flags)
		fmt.Printf("\tGot:      %s\n", flagsToString(a.Before.Flags))
		fmt.Printf("\tExpected: %s\n", flagsToString(b.Before.Flags))
	case FlagsAfter:
		fmt.Printf("* Flags mismatch: got %04X, expected %04X (after execution)\n", a.After.Flags, b.After.Flags)
		fmt.Printf("\tGot:      %s\n", flagsToString(a.After.Flags))
		fmt.Printf("\tExpected: %s\n", flagsToString(b.After.Flags))
	case RegsBefore:
		fmt.Println("* Registers mismatch (before execution)")
		fmt.Printf("\tGot:      %s\n", regsToString(a.Before.Regs))
		fmt.Printf("\tExpected: %s\n", regsToString(b.Before.Regs))		
	case RegsAfter:
		fmt.Println("* Registers mismatch (after execution)")
		fmt.Printf("\tGot:      %s\n", regsToString(a.After.Regs))
		fmt.Printf("\tExpected: %s\n", regsToString(b.After.Regs))
	}
}

func printMemOps(event validator.Event) {
	for i := 0; i < len(event.Reads); i++ {
		read := event.Reads[i]
		write := event.Writes[i]

		if read.Addr != 0xFFFFFFFF {
			fmt.Printf("\t[R:%08X/%02X]\n", read.Addr, read.Data)
		}
		if write.Addr != 0xFFFFFFFF {
			fmt.Printf("\t[W:%08X/%02X]\n", write.Addr, write.Data)
		}
	}
}

func addrToString(info validator.RegsInfo) string {
	return fmt.Sprintf("%04X:%04X", info.Regs[validator.CS], info.IP)
}

func flagsToString(value uint16) string {
	tmp := processor.Flags(value)
	flags := &tmp

	return fmt.Sprintf("C=%t P=%t A=%t Z=%t S=%t T=%t I=%t D=%t O=%t", 
		flags.GetBool(processor.Carry), 
		flags.GetBool(processor.Parity), 
		flags.GetBool(processor.Adjust), 
		flags.GetBool(processor.Zero),
		flags.GetBool(processor.Sign),
		flags.GetBool(processor.Trap),
		flags.GetBool(processor.InterruptEnable),
		flags.GetBool(processor.Direction),
		flags.GetBool(processor.Overflow),
	)
}

func regsToString(values [12]uint16) string {
	return fmt.Sprintf("AX=%04X CX=%04X DX=%04X BX=%04X SP=%04X BP=%04X SI=%04X DI=%04X ES=%04X CS=%04X SS=%04X DS=%04X", 
		values[validator.AX], values[validator.CX], values[validator.DX], values[validator.BX],
		values[validator.SP], values[validator.BP], values[validator.SI], values[validator.DI],
		values[validator.ES], values[validator.CS], values[validator.SS], values[validator.DS],
	)
}

// Set undefined flags to 0 so that the comparison isn't biaised by undefined behaviors
func maskUndefFlags(opcode byte, opcodeExt byte, flags uint16) uint16 {
	switch opcode {
	case 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x84, 0x85, 0xA8, 0xA9:
		return flags ^ uint16(processor.Adjust)
	case 0x27, 0x2F:
		return flags ^ uint16(processor.Overflow)
	case 0x37, 0x3F:
		return flags ^ uint16(processor.Overflow | processor.Sign | processor.Zero | processor.Parity)
	case 0x69, 0x6B:
		return flags ^ uint16(processor.Sign | processor.Zero | processor.Adjust | processor.Parity)
	case 0x80, 0x81, 0x82, 0x83:
		switch opcodeExt {
		case 1, 4, 6:
			return flags ^ uint16(processor.Adjust)
		}
	case 0xC0, 0xC1, 0xD2:
		switch opcodeExt {
		case 4, 5, 6:
			return flags ^ uint16(processor.Overflow | processor.Adjust | processor.Carry)
		case 7:
			return flags ^ uint16(processor.Overflow | processor.Adjust)
		default:
			return flags ^ uint16(processor.Overflow)
		}
	case 0xD0, 0xD1:
		switch opcodeExt {
		case 4, 5, 6, 7:
			return flags ^ uint16(processor.Adjust)
		}
	case 0xD3:
		switch opcodeExt {
		case 4, 5, 6:
			return flags ^ uint16(processor.Overflow | processor.Adjust | processor.Carry)
		case 7:
			return flags ^ uint16(processor.Adjust)
		default:
			return flags ^ uint16(processor.Overflow)
		}
	case 0xD4, 0xD5:
		return flags ^ uint16(processor.Overflow | processor.Adjust | processor.Carry)
	case 0xF6, 0xF7:
		switch opcodeExt {
		case 0, 1:
			return flags ^ uint16(processor.Adjust)
		case 4, 5:
			return flags ^ uint16(processor.Sign | processor.Zero | processor.Adjust | processor.Parity)
		case 6, 7:
			return flags ^ uint16(processor.Overflow | processor.Sign | processor.Zero | processor.Adjust | processor.Parity | processor.Carry)
		}		
	}
	return flags
}
