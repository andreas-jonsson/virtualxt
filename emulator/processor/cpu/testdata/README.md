**License:** Unknown (possible PD)

**Source:** https://forum.osdev.org/viewtopic.php?f=13&t=23739&start=15

```
Artlav
Post subject: Re: an 8086 PC emulator
-------------------------------------

Nice work.

If you're interested, here is my collection of testsuites for EPC's 80186:
http://orbides.1gb.ru/80186_tests.zip
res_*.bin contains the results expected at the offset 0x0 when you run *.bin test placed as a BIOS.

Although, if you have win 3.0 booting these are a little late.

About the timing - do you want to get precise real-time experience or something?
You won't get away from CPU overuse without many tricks. There is nothing better then scheduling to free the CPU (like sleep(1);), but it's hard to predict the duration.
Best results will be with rdtsc and active waiting, but you're still subjected to task switching.
Why bother?
```
