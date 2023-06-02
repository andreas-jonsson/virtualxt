target remote localhost:1234

set architecture i8086
set disassembly-flavor intel

echo \nGDB's $pc and $sp is 20bit, effectively $pc = $cs * 16 + IP.\n
echo $fs and $gs is mapped to the 16bit virtual IP and SP registers.\n\n
