set architecture i8086
set disassembly-flavor intel
set pagination off
layout asm
layout reg
focus cmd

target remote localhost:1234

echo \nGDB's $pc and $sp is 20bit, effectively $pc = $cs * 16 + IP.\n
echo $fs and $gs is mapped to the 16bit virtual IP and SP registers.\n\n
