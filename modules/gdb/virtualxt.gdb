set architecture i8086
set disassembly-flavor intel
set pagination off
layout asm
layout reg
focus cmd

target remote localhost:1234

echo \nGDB's $pc and $sp is 20bit, effectively $pc = $cs * 16 + IP.\n
echo The other registers are 16bit despite the 'e' prefix.\n\n
