# PI8088 Hardware Validator

This is a hardware validator for testing instruction accuracy in 8088/V20 PC emulators. It plugs in the top/back of an RaspberryPi 4/400 computer and was built for testing the VirtualXT IBM PC/XT emulator. But it can easily be adopted for use with other emulators as well.

## Hardware
The hardware is very simple for this project. It is just a passive PCB that attaches to the RaspberryPi and connects any CMOS 8088 compatible CPU, like the 80C88 or NEC V20 to the RaspberryPi's GPIO pins. Something that should be noted is that the system is running on 3.3V and not 5V. Although that is out of spec for the CPU's in question it seems to run completely stable in the frequencies used here. 

## Software
On a software level the system works by first executing a setup programs that loads a CPU state that match the emulators state before the instruction in question was executed. It then lets the instruction execute on the attached CPU. Then proceeds by running a serialization program that retrieves the correct CPU state and compares it with the emulator's state after instruction execution. The user will then be presented with the diffs if any are found.

## Limitations
Although the cycle counts are reported, there are (at least in theory) situations where the exact count can not be trusted. The CPU is running in minimum mode which makes it hard to determine the exact status. For example, if a prefetch is performed during execution it is very complicated to determine how long the actual execution of the instruction took. It might be possible to resolve if the CPU is executing in maximum mode but that requires hardware changes.