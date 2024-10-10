# V20 V1
This is a set of NEC V20 (μPD70108) CPU tests produced by Daniel Balsom using the [Arduino8088](https://github.com/dbalsom/arduino_8088) interface and the [MartyPC](https://github.com/dbalsom/martypc) emulator.

 - The ```v1_native``` directory contains the V20's native instruction set.
 - The ```v1_emulation``` directory will (eventually) contain the V20's 8080 emulation mode instruction set.

### Current Version: 1.0.2

### Changes from 8088 Test Suite V1

If you have worked with my previous test suite for the Intel 8088, you will find a few changes to the test format and conventions.
 - The address column in the 'cycles' array is now represents the state of the address/data bus per-cycle. It is up to you to latch the address on ALE if you wish. Having the cycle-accurate output of the bus lines enables more accurate emulation verification.
 - The values for 'ram' are no longer sorted by address; but should appear in the order in which they were accessed.
 - The final state only includes memory and register values that have changed. The entire flags register is included if any flag has changed.
 - Half of the instructions execute from a full instruction prefetch queue. See "Using The Tests" for more information.

### About the Tests

These tests are produced with an NEC V20 CPU running in Maximum mode.

10,000 tests are generally provided per opcode, with the following exceptions:
- String instructions are limited to 5,000 tests due to their large size, even when masking CX to 7 bits.
- Shift and rotate instructions that use a CL count (D2, D3) or an immediate count (C0, C1) are limited to 5,000 tests.
- ENTER is limited to 2000 tests due to the large size of resulting stack frames.
- INC and DEC instructions with fixed register operands are limited to 1,000 tests as they are trivial.
- Flag instructions (F5,F8-FD) are limited to 1,000 tests as they are trivial.

All tests assume a full 1MB of RAM is mapped to the processor and writable.

No wait states are incurred during any of the tests. The interrupt and trap flags are not exercised.

This test set exercises the V20's processor instruction queue. Every other instruction will execute from a full instruction queue.

### Using the Tests

The simplest way to run each test is to override your emulated CPU's normal reset vector (FFFF:0000) to the CS:IP of the test's initial state, then reset the CPU. 

#### Non-Prefetched Tests

- If the initial queue state is empty, the instruction is not prefetched. Once the reset procedure completes, the instruction should begin normally by being fetched after the reset routine has completed.

#### Prefetched Tests

- If the test specifies an initial queue state, the provided initial queue contents should be set after the CPU reset routine flushes the instruction queue. This should trigger your emulator to suspend prefetching since the queue is full. In my emulator, I provide the CPU with a vector of bytes to install in the queue before I call ```cpu.reset()```. You could also pass your reset method a vector of bytes to install.

- You will need to add the length of the queue contents to your PC/PFP register (or adjust the reset vector IP) so that fetching begins at the correct address. 

- Once you read the first instruction byte out of the queue to begin test execution, prefetching should be resumed since there is now room in the queue. It takes two cycles to begin a fetch after reading from a full queue, therefore tests that specify an initial queue state will start with two 'Ti' cycle states.

### Why Are Instructions Longer Than Expected?

Instruction cycles begin from the cycle in which the CPU's queue status lines indicate that an instruction "First Byte" has been fetched - this may be an optional instruction prefix, in which case there will be multiple First Byte statuses until the first byte that is a non-prefixed opcode byte is read. If the test is starting from an empty prefetch queue, the final opcode byte and any modrm or displacement must be read from the bus before instruction execution can begin.

Instructions cycles end when the queue status lines indicate that the first byte of the next instruction has been read from the queue. If the instruction started fully prefetched and the next instruction byte was present in the queue at the end of the instruction, the instruction cycles should reflect the documented "best case" instruction timings. If the queue was empty at the end of execution, the extra cycles spent fetching the next instruction byte may lengthen the instruction from documented timings. There is no indication from the CPU when an instruction ends - only when a new one begins.

### Segment Override Prefixes
Random segment override prefixes have been prepended to a percentage of instructions, even if they may not do anything. This isn't completely useless - a few bugs have been found where segment overrides had an effect when they should not have.

### String Prefixes

String instructions may be randomly prepended by a REP, REPE, REPNE, REPC or REPNC instruction prefix. In this event, CX is masked to 7 bits to produce reasonably sized tests (A string instruction with CX==65535 would be over a million cycles in execution). 
- If you are not specifically writing a V20 emulator, you will want to filter out tests that utilize REPC and REPNC prefixes.

### Instruction Prefetching

All bytes fetched after the initial instruction bytes are set to 0x90 (144) (NOP). Therefore, the queue contents at the end of all tests will contain only NOPs, with a maximum of 3 (since one has been read out).

### Sample test:

```json
{
    "name": "add byte [ss:bp+di-64h], cl",
    "bytes": [0, 75, 156],
    "initial": {
        "regs": {
            "ax": 21153,
            "bx": 59172,
            "cx": 33224,
            "dx": 61687,
            "cs": 12781,
            "ss": 7427,
            "ds": 600,
            "es": 52419,
            "sp": 49014,
            "bp": 9736,
            "si": 52001,
            "di": 10025,
            "ip": 694,
            "flags": 62546
        },
        "ram": [
            [205190, 0],
            [205191, 75],
            [205192, 156],
            [205193, 144],
            [138493, 20]
        ],
        "queue": [0, 75, 156, 144]
    },
    "final": {
        "regs": {
            "ip": 697,
            "flags": 62594
        },
        "ram": [
            [138493, 220]
        ],
        "queue": [144, 144]
    },
    "cycles": [
        [0, 139664, "--", "---", "---", 0, 0, "PASV", "Ti", "F", 0],
        [0, 139664, "--", "---", "---", 0, 0, "PASV", "Ti", "S", 75],
        [1, 205194, "--", "---", "---", 0, 0, "CODE", "T1", "-", 0],
        [0, 139658, "CS", "R--", "---", 0, 0, "CODE", "T2", "S", 156],
        [0, 139664, "CS", "R--", "---", 0, 144, "PASV", "T3", "-", 0],
        [0, 139664, "CS", "---", "---", 0, 0, "PASV", "T4", "-", 0],
        [1, 138493, "--", "---", "---", 0, 0, "MEMR", "T1", "-", 0],
        [0, 72957, "SS", "R--", "---", 0, 0, "MEMR", "T2", "-", 0],
        [0, 72724, "SS", "R--", "---", 0, 20, "PASV", "T3", "-", 0],
        [0, 72724, "SS", "---", "---", 0, 0, "PASV", "T4", "-", 0],
        [1, 205195, "--", "---", "---", 0, 0, "CODE", "T1", "-", 0],
        [0, 139659, "CS", "R--", "---", 0, 0, "CODE", "T2", "-", 0],
        [0, 139664, "CS", "R--", "---", 0, 144, "PASV", "T3", "-", 0],
        [0, 139664, "CS", "---", "---", 0, 0, "PASV", "T4", "-", 0],
        [1, 138493, "--", "---", "---", 0, 0, "MEMW", "T1", "-", 0],
        [0, 72924, "SS", "-A-", "---", 0, 0, "MEMW", "T2", "-", 0],
        [0, 72924, "SS", "-AW", "---", 0, 220, "PASV", "T3", "-", 0],
        [0, 72924, "SS", "---", "---", 0, 0, "PASV", "T4", "-", 0]
    ],
    "hash": "0a5e080b128ccba34c8786753c6c56a98efef942",
    "idx": 0
}
```
The 'name' field is a user-readable disassembly of the instruction.

The 'bytes' list contains the instruction bytes that make up the full instruction. You do not need to do anything with these, as the instruction bytes are also set up in the initial memory state.

The 'initial' and 'final' keys contain the register, memory and instruction queue states before and after instruction execution.

The 'hash' key is a SHA1 hash of the test json. It should uniquely identify any test in the suite.

The 'idx' key is the numerical index of the test in the JSON array.

The 'cycles' key is a list of lists, each sublist corresponding to a single CPU cycle, storing several defined fields. From left to right, the cycle fields are:  
 
 - Pin status bitfield
 - Bus value
 - Segment status
 - Memory status
 - IO status
 - Data bus
 - Bus status
 - T-state
 - Queue operation status
 - Queue byte read

The first column is a bitfield representing certain chip pin states. 

 - Bit #0 of this field represents the ALE (Address Latch Enable) pin output, which in Maximum Mode is output by the i8288. This signal is asserted on T1 to instruct the motherboard's address latches to store the current address. This is necessary since the address and data lines of the 8088 are multiplexed and a full, valid address is only on the bus while ALE is asserted. 
 - Bit #1 of this field represents the INTR pin input. This is not currently exercised, but may be in future test releases.
 - Bit #2 of this field represents the NMI pin input. This is not currently exercised, but may be in future test releases.

The second column, bus value, represents the state of the multiplexed address/data/status bus. It contains a valid address only when ALE is asserted. You will need to save the address latch yourself on ALE if required.

The segment status indicates which segment is in use to calculate addresses by the CPU, using segment-offset addressing. This field represents the S3 and S4 status lines of the V20.

The memory status field is representive of the outputs of an i8288 Bus Controller. From left to right, this field will contain `RAW` or `---`.  `R` represents the MRDC status line, `A` represents the AMWC status line, and `W` represents the MWTC status line. These status lines are active-low. A memory read will occur on T3 or the last Tw t-state when MRDC is active. A memory write will occur on T3 or the last Tw t-state when AMWC is active. At this point, the value of the data bus field will be valid and will represent the byte read or written.

The IO status field is representive of the outputs of an i8288 Bus Controller. From left to right, this field will contain `RAW` or `---`.  `R` represents the IORC status line. `A` represents the AIOWC status line. `W` represents the IOWC status line. These status lines are active-low. An IO read will occur on T3 or the last Tw t-state when IORC is active. An IO write will occur on T3 or the last Tw t-state when AIOWC is active. At this point, the value of the data bus field will be valid and will represent the byte read or written.

The bus status lines indicate the type of bus m-cycle currently in operation. Either `INTA`, `IOR`, `IOW`, `MEMR`, `MEMW`, `HALT`, `CODE`, or `PASV`.  These states represent the octal value of the S0-S2 status lines.

The T-state is the current T-state of the CPU, which is either `T1`, `T2`, `T3`, `Tw` or `T4` during a bus cycle, or `Ti` if no bus cycle is occurring. T-states are not exposed directly by the CPU, but can be derived from bus activity and so are calculated for you.

The queue operation status will contain either `F`, `S`, `E` or `-`. `F` indicates a "First Byte" of an instruction or instruction prefix has been read.  `S` indicates a "Subsequent" byte of an instruction has been read - either a modrm, displacement, or operand. `E` indicates that the instruction queue has been Emptied/Flushed. All queue operation statuses reflect an operation that actually occurred on the previous cycle.  

When the queue operation status is not `-`, then the value of the queue byte read field is valid and represents the byte read from the queue. This field represents the QS0 and QS1 status lines.

### General Notes

For more information on the V20 and 8288 status lines, see their respective white papers.

If you are not interested in writing a cycle-accurate emulator, you may only be interested in the final register and ram state.

Note that these tests include many undocumented/undefined opcodes. The V20 has no concept of an invalid instruction, and will perform some task for any provided sequence of instruction bytes. Additionally, flags may be changed by documented instructions in ways that are officially undefined.

The V20 often has very different behavior than the 8088 when presented with undefined instruction forms.

### Per-Instruction Notes
 - **62**: BOUND with a second register operand is invalid. The V20 will halt if given this invalid form*, therefore only valid forms are provided.
 - **63**: An undefined instruction that does nothing, but spends some time doing it.
 - **6C,6D**: REPC/REPNC prefixes on INS and OUTS act as regular REP prefixes.
 - **8C,8E**: These instructions are only defined for a reg value of 0-3. However, only the first two bits are checked, so forms with all possible reg values are included (except as explained below).
 - **8E**: The form of this instruction with CS as a destination is undefined. Although the V20 will perform this as expected, the operation is not useful due to failure to flush the prefetch queue, which also causes issues for the Aduino8088 CPU server. Therefore, this form is omitted.
 - **8F**: The behavior of 8F with reg != 0 is undefined and not included.
 - **9B**: WAIT is not included in this test set.
 - **8D,C4,C5**: 'r, r' forms of LEA, LES and LDS are undefined. On a real CPU they may leak the value of the last calculated EA. The last calculated EA is unknown in a single-instruction context, so these forms are omitted. They may be included in the future paired with an initializing LEA instruction.
 - **A4-A7,AA-AF**: CX is masked to 7 bits. This provides a reasonable test length, as the full 65535 value in CX with a REP prefix could result in over one million cycles.
 - **A6,A7**: CMPS reverses the order in which it accesses its operands when prefixed with a REPE/REPNE prefix. I mention this here in the hope it will help you avoid the lengthy confusion it caused me.
 - **C0,C1**: The immediate operand count is masked to 6 bits. This shortens the possible test length, while still hopefully catching the case where the immediate is improperly masked to 5 bits (186+ behavior).
 - **C6,C7**: Although the reg != 0 forms of these instructions are officially undefined, this field is ignored. Therefore, the test set contains random values for reg.
 - **C8**: Intel documentation defines the nesting level to be modulo 32 of the immediate byte operand; the V20 apparently does not take the modulus. This leads to very long instructions, so the immediate is masked modulo 32.
 - **D2,D3**: The CL count register is masked to 6 bits. This shortens the possible test length, while still hopefully catching the case where CL is improperly masked to 5 bits (186+ behavior).
 - **6C,6D,E4,E5,EC,ED**: All forms of the IN/INS instruction should return 0xFF on IO read.
 - **F0, F1**: The LOCK prefix is not exercised in this test set.
 - **F4**: HALT is not included in this test set.
 - **F6.6, F6.7, F7.6, F7.7** - These instructions can generate division exceptions on overflow or division by 0. When this occurs, cycle traces continue until the first byte of the exception handler is fetched and read from the queue. The IVT entry for INT0 is set up to point to 1024 (0400h).
   - Both the 8088 and V20 do not have proper exceptions as we may read about in Intel manuals. Instead, they generate a "Type-0" interrupt when there is a division overflow. Like any other interrupt, the address pushed to the stack is the address of the *next* instruction after IDIV. This differs from future Intel processors which introduced proper exceptions with different return conventions.
 - **F6.7, F7.7** - On the 8088, presence of a REP prefix preceding IDIV will invert the sign of the quotient. On V20, REP prefixes on IDIV have no effect. REP prefixes have been prepended to 10% of IDIV tests so you can check this behavior.
 - **FE**: FE.2-7 are invalid instructions, but will attempt to execute their FF counterpart microcode with the width bit set to 0. They may be included at a later date once I have figured out their basic operation. FE.3 and FE.5 register forms cause the V20 to halt.
 - **FF**: FF.3 and FF.5 register forms are invalid. They cause the V20 to halt and are excluded.

#### Extended Opcodes
 - **0F20, 0F22, 0F26** - Providing a length of 0 or 255 to the string BCD instructions will cause underflow of the internal loop counter, and cause the instructions to execute 65535 iterations for over 1 million cycles. These values of CL are filtered out as a result. Note that the test data is random - meaning the strings provided to these instructions are almost always not valid BCD. Their behavior is deterministic regardless. If you add/subtract them as normal and apply DAA/DAS to each byte, they should validate.
 - **0F31, 0F33, 0F39, 0F3B** - These instructions are only valid with a register operand (mod==3). Providing a memory operand will cause them to run for over 130,000 cycles. These invalid forms are omitted.
 - **0F33, 0F3B** - Using AL or AH as the first operand to EXT is invalid. These forms are omitted.

*When the V20 halts due to an illegal instruction, there is no HALT bus state indicated.

### Tricky Bits and Advice

If you are validating memory operations for both addresses and values, you might struggle a bit with IDIV exceptions. During the call to the Type-0 interrupt "exception" handler, the flags register will be written to the stack - containing several undefined flags. You will need some strategy to mark when flags are on the stack so you can mask them, or simply disable validation of memory operation values when you detect exceptions occur.  One good way to detect an exception is if a test performs four successive reads from addresses 0000-0003.

Challenge: Actually implement the undefined flags for division!

### Masking Flags: metadata.json

If you are not interested in emulating the undefined behavior of the V20, you can use the included ```metadata.json``` file which lists which instructions are undocumented or undefined and provides values that can be used to mask undefined flags.

In ```metadata.json```, opcodes are listed as object keys, each a key being the opcode as a 2 or 4 character hex string. Each opcode entry has a `status` field which may be `extension`, `normal`, `prefix`, `alias`, `undocumented`, `undefined`, `invalid`, or `fpu`

An opcode marked `prefix` is an instruction prefix.
An opcode marked `normal` is a defined instruction with documented behavior and should be implemented by all emulators of this architecture.
An opcode marked `alias` is simply an alias for another instruction. These exist because the mask that determines which microcode maps to which opcode is not always perfectly specific. 
An opcode marked `undocumented` has well-defined and potentially useful behavior, such as SETMO and SETMOC. (Not relevant on V20)
An opcode marked `undefined` may or may not have predictable behavior, but its behavior is likely unique to the V20.
An opcode marked `invalid` will cause the V20 to halt.
An opcode marked `fpu` is an FPU instruction (ESC opcode).

Each opcode entry also has an `arch` field which represents the architecture in which this instruction was introduced. Values may be `86`, `186`, or `v30` (I use the "full size" version of each CPU type, so for 8088 use `86`, for V20 use `v30`, etc.).

If present, the `flags` field indicates which flags are undefined after the instruction has executed. A flag is either a letter from the pattern `odiszapc` indicating it is undefined, or a period, indicating it is defined. The `flags-mask` field is a 16 bit value that can be applied with an AND to the flags register to clear undefined flags. To run the tests while ignoring undefined flags, apply this mask to the test's final `flags` value and your emulator's resulting Flags register before comparing them.

An opcode may have a `reg` field which will be an object of opcode extensions/register specifiers represented by single digit string keys - this is the 'reg' field of the modrm byte.  Certain opcodes may be defined or undefined depending on their register specifier or opcode extension. Therefore, each entry within this `reg` object will have the same fields as a top-level opcode object. 

An opcode may be an extended opcode where the first opcode byte is 0F. These opcodes will be be stored under four-character keys, such as `0F10`.
