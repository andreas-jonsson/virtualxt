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

package debug

import (
	"bufio"
	"bytes"
	"encoding/hex"
	"errors"
	"flag"
	"fmt"
	"log"
	"net"
	"os"
	"os/signal"
	"runtime"
	"strings"
	"sync"
	"time"

	"github.com/andreas-jonsson/i8088-core/emulator/memory"
	"github.com/andreas-jonsson/i8088-core/emulator/peripheral"
	"github.com/andreas-jonsson/i8088-core/emulator/processor"
)

var (
	EnableDebug, noHistory, debugBreak bool
	tcpDebug                           net.Conn
)

var ErrQuit = errors.New("QUIT!")

var (
	internalLogger = &Logger{}
	Log            = log.New(internalLogger, "", 0)
)

type Logger struct {
	sync.RWMutex
}

func (l *Logger) Write(p []byte) (n int, err error) {
	l.Lock()
	if tcpDebug != nil {
		p = bytes.ReplaceAll(p, []byte("<<<!\n"), []byte{})
		p = bytes.ReplaceAll(p, []byte{0xA}, []byte{0xA, 0xD})
		n, err = tcpDebug.Write(p)
		if err != nil {
			tcpDebug = nil
		}
	}
	l.Unlock()
	return
}

func init() {
	flag.BoolVar(&noHistory, "nohistory", false, "do not build histogram")
	flag.BoolVar(&EnableDebug, "debug", false, "enable telnet debugger")
	flag.BoolVar(&debugBreak, "break", false, "break on startup")

	log.SetOutput(internalLogger)
}

func readLine() string {
	internalLogger.RLock()
	defer internalLogger.RUnlock()

	for tcpDebug == nil {
		internalLogger.RUnlock()
		runtime.Gosched()
		internalLogger.RLock()
	}

	scanner := bufio.NewScanner(tcpDebug)
	for scanner.Scan() {
		return scanner.Text()
	}
	if err := scanner.Err(); err != nil {
		tcpDebug = nil
	}
	return ""
}

type Device struct {
	signChan            chan os.Signal
	historyChan         chan string
	numInstructionsLost uint64
	lastInstruction     memory.Pointer
	breakOnIRET         bool

	mips        float64
	stats       processor.Stats
	updateStats time.Time
	breakpoints []uint16
	codeOffset  uint16

	memPeripherals [0x100000]memory.Memory

	r *processor.Registers
	p processor.Processor
}

func (m *Device) Install(p processor.Processor) error {
	m.historyChan = make(chan string, 128)
	m.signChan = make(chan os.Signal, 1)
	signal.Notify(m.signChan, os.Interrupt)

	for i := 0; i < 0x100000; i++ {
		m.memPeripherals[i] = p.GetMappedMemoryDevice(memory.Pointer(i))
	}
	if err := p.InstallMemoryDevice(m, 0x0, 0xFFFFF); err != nil {
		return err
	}

	go func() {
		ln, _ := net.Listen("tcp", ":23")
		for {
			conn, _ := ln.Accept()
			internalLogger.Lock()
			tcpDebug = conn
			internalLogger.Unlock()

			name, _ := os.Hostname()
			log.Print("Connected to: ", name)
		}
	}()

	m.p = p
	m.r = p.GetRegisters()
	m.updateStats = time.Now()
	return nil
}

func (m *Device) getFlags() string {
	s := [9]rune{'-', '-', '-', '-', '-', '-', '-', '-', '-'}
	if m.r.CF {
		s[0] = 'C'
	}
	if m.r.PF {
		s[1] = 'P'
	}
	if m.r.AF {
		s[2] = 'A'
	}
	if m.r.ZF {
		s[3] = 'Z'
	}
	if m.r.SF {
		s[4] = 'S'
	}
	if m.r.TF {
		s[5] = 'T'
	}
	if m.r.IF {
		s[6] = 'I'
	}
	if m.r.DF {
		s[7] = 'D'
	}
	if m.r.OF {
		s[8] = 'O'
	}
	return "\n" + string(s[:]) + "\n"
}

func (m *Device) printRegisters() {
	r := m.r
	regs := fmt.Sprintf(
		"AL 0x%X (%d)\tCL 0x%X (%d)\tDL 0x%X (%d)\tBL 0x%X (%d)\nAH 0x%X (%d)\tCH 0x%X (%d)\tDH 0x%X (%d)\tBH 0x%X (%d)\nAX 0x%X (%d)\tCX 0x%X (%d)\tDX 0x%X (%d)\tBX 0x%X (%d)\n\n",
		r.AL(), r.AL(), r.CL(), r.CL(), r.DL(), r.DL(), r.BL(), r.BL(),
		r.AH(), r.AH(), r.CH(), r.CH(), r.DH(), r.DH(), r.BH(), r.BH(),
		r.AX, r.AX, r.CX, r.CX, r.DX, r.DX, r.BX, r.BX,
	) + fmt.Sprintf(
		"SP 0x%X (%d)\tBP 0x%X (%d)\nSI 0x%X (%d)\tDI 0x%X (%d)\n\n",
		r.SP, r.SP, r.BP, r.BP, r.SI, r.SI, r.DI, r.DI,
	) + fmt.Sprintf(
		"ES 0x%X (%d)\tCS 0x%X (%d)\nSS 0x%X (%d)\tDS 0x%X (%d)",
		r.ES, r.ES, r.CS, r.CS, r.SS, r.SS, r.DS, r.DS,
	)
	log.Println(regs)
	log.Println(m.getFlags())
}

func instructionToString(op byte) string {
	return fmt.Sprintf("%s (0x%X)", OpcodeName(op), op)
}

func (m *Device) showMemory(rng string) {
	var from, to int
	switch n, _ := fmt.Sscanf(rng, "%x,%x", &from, &to); n {
	case 1:
		d := m.p.ReadByte(memory.Pointer(from))
		log.Printf("0x%X: 0x%X (%d)\n", from, d, d)
	case 2:
		if num := (to + 1) - from; num > 0 {
			buffer := make([]byte, num)
			for i := range buffer {
				buffer[i] = m.p.ReadByte(memory.Pointer(from + i))
			}
			log.Print(hex.Dump(buffer))
		}
	default:
		log.Println("invalid memory range")
	}
}

func toASCII(b byte) string {
	if b == 0 {
		return "."
	} else if b < 0x20 {
		return fmt.Sprint("?")
	} else if b > 0x7E {
		return fmt.Sprint("#")
	}
	return fmt.Sprintf("%c", b)
}

func (m *Device) renderVideo() {
	p := memory.Pointer(0xB0000)
	for i := 0; i < 25*2; i += 2 {
		log.Print("| <<<!")
		for j := 0; j < 80*2; j += 2 {
			log.Print(toASCII(m.p.ReadByte(p)) + "<<<!")
			p += 2
		}
		log.Print("")
	}
}

func (m *Device) setCodeOffset(of string) {
	var o uint16
	if n, _ := fmt.Sscanf(of, "%x", &o); n == 1 {
		log.Printf("Code offset at: 0x%X\n", o)
		m.codeOffset = uint16(o)
	}
}

func (m *Device) showBreakpoints() {
	for i, br := range m.breakpoints {
		log.Printf("%d:\t0x%X\n", i, br)
	}
}

func (m *Device) setBreakpoint(br string) {
	var b uint16
	if n, _ := fmt.Sscanf(br, "%x", &b); n == 1 {
		log.Printf("Breakpoint set at: CS:0x%X\n", b)
		m.breakpoints = append(m.breakpoints, b)
	}
}

func (m *Device) removeBreakpoint(br string) {
	var i int
	if n, _ := fmt.Sscanf(br, "%d", &i); n == 1 && i < len(m.breakpoints) {
		log.Printf("Removed breakpoint %d at: CS:0x%X\n", i, m.breakpoints[i])
		m.breakpoints = append(m.breakpoints[:i], m.breakpoints[i+1:]...)
	}
}

func (m *Device) showHistoryWithLength(hl string) {
	var num int
	if n, _ := fmt.Sscanf(hl, "%d", &num); n == 1 {
		if num <= 0 {
			num = 0xFFFFF
		}
		m.showHistory(num)
		return
	}
	log.Println("invalid history range")
}

func (m *Device) showHistory(num int) {
	log.Println("| Lost instructions:", m.numInstructionsLost)
	for i := 0; i < len(m.historyChan) && i < num; i++ {
		select {
		case inst := <-m.historyChan:
			log.Println(inst)
			m.historyChan <- inst
		}
	}
}

func (m *Device) pushHistory(inst string) {
	select {
	case m.historyChan <- inst:
	default:
		<-m.historyChan
		m.numInstructionsLost++
		m.historyChan <- inst
	}
}

func (m *Device) csToString() string {
	switch m.r.CS {
	case 0xF000:
		return "BIOS"
	case 0x7C00:
		return "BOOT"
	default:
		return fmt.Sprintf("0x%X", m.r.CS)
	}
}

func (m *Device) showMemMap() {
	var (
		startAddr      int
		lastDeviceName string
	)

	for i := 0; i < 0x100000; i++ {
		p, b := m.memPeripherals[i].(peripheral.Peripheral)
		name := "UNNAMED DEVICE"
		if b {
			name = p.Name()
		}

		isLast := i == 0xFFFFF
		if (lastDeviceName != name || isLast) && i > 0 {
			if end := i - 1; startAddr == end {
				log.Printf("0x%X: %s", startAddr, lastDeviceName)
			} else {
				if isLast {
					end++
				}
				log.Printf("0x%X-0x%X: %s", startAddr, end, lastDeviceName)
			}
			startAddr = i
		}
		lastDeviceName = name
	}
}

func (m *Device) ReadByte(addr memory.Pointer) byte {
	return m.memPeripherals[addr].ReadByte(addr)
}

func (m *Device) WriteByte(addr memory.Pointer, data byte) {
	m.memPeripherals[addr].WriteByte(addr, data)
	if data != 0 && addr == memory.NewPointer(0x40, 0x15) {
		log.Printf("BIOS Error: 0x%X", data)
		m.Break()
	}
	/*
		switch addr {
		case 0x70:
			log.Printf("Write: 0x%X @ %v", data, addr)
			m.Break()
		}
	*/
}

func (m *Device) Break() {
	debugBreak = true
	m.r.Debug = true
}

func (m *Device) Continue() {
	debugBreak = false
	m.r.Debug = false
}

func (m *Device) Step(cycles int) error {
	if time.Since(m.updateStats) >= time.Second {
		m.stats = m.p.GetStats()
		m.mips = float64(m.stats.NumInstructions) / 1000000.0
		m.updateStats = time.Now()
	}

	if m.r.Debug {
		debugBreak = true
	}

	select {
	case <-m.signChan:
		log.Println("BREAK!")
		m.Break()
	default:
	}

	ip := memory.NewPointer(m.r.CS, m.r.IP)
	op := m.p.ReadByte(ip)
	inst := instructionToString(op)

	if m.lastInstruction > 0 && m.lastInstruction != ip {
		m.Break()
		m.lastInstruction = 0
		log.Println(inst)
	}

	if m.breakOnIRET && op == 0xCF {
		m.Break()
		m.breakOnIRET = false
		log.Println(inst)
	}

	for i, br := range m.breakpoints {
		if m.r.IP == br {
			log.Println("BREAK:", i)
			m.Break()
		}
	}

	for debugBreak {

		log.Printf("[%s:0x%X] DEBUG><<<!", m.csToString(), m.r.IP-m.codeOffset)

		ln := readLine()
		switch {
		case ln == "q":
			return ErrQuit
		case ln == "c":
			m.Continue()
		case ln == "" || ln == "s":
			m.Continue()
			m.lastInstruction = ip
		case ln == "i":
			m.Continue()
			m.breakOnIRET = true
		case ln == "r":
			m.printRegisters()
		case ln == "v":
			m.renderVideo()
		case ln == "h":
			m.showHistory(16)
		case ln == "ch":
			log.Print("Clear history!")
		drainHistory:
			select {
			case <-m.historyChan:
				m.numInstructionsLost++
				goto drainHistory
			default:
			}
		case ln == "t":
			log.Printf("MIPS: %.2f\n", m.mips)
			log.Print(m.stats)
		case ln == "@":
			log.Printf("%v (%d)\n", ip-memory.Pointer(m.codeOffset), ip-memory.Pointer(m.codeOffset))
			log.Print(inst)
		case ln == "cb":
			log.Print("Clear breakpoints!")
			m.breakpoints = m.breakpoints[:0]
		case ln == "b":
			m.showBreakpoints()
		case ln == "p":
			m.showMemMap()
		case strings.HasPrefix(ln, "o "):
			m.setCodeOffset(ln[2:])
		case strings.HasPrefix(ln, "h "):
			m.showHistoryWithLength(ln[2:])
		case strings.HasPrefix(ln, "b "):
			m.setBreakpoint(ln[2:])
		case strings.HasPrefix(ln, "rb "):
			m.removeBreakpoint(ln[3:])
		case strings.HasPrefix(ln, "m "):
			m.showMemory(ln[2:])
		default:
			log.Print("unknown command: ", ln)
		}
	}

	if !noHistory {
		m.pushHistory(fmt.Sprintf("| [%s:0x%X] %s", m.csToString(), m.r.IP-m.codeOffset, inst))
	}

	return nil
}

func (m *Device) Name() string {
	return "Debug Device"
}

func (m *Device) Reset() {
}

func (m *Device) Close() error {
	if tcpDebug != nil {
		tcpDebug.Close()
		tcpDebug = nil
	}
	return nil
}
