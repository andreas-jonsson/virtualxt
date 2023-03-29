# ch36x linux driver

## Description

​	This directory contains 3 parts, pci/pcie driver, application library and  example. This driver and application support pci bus interface chip ch365 and pcie bus interface chips ch367 and ch368.

1. Open "Terminal"
2. Switch to "driver" directory
3. Compile the driver using "make", you will see the module "ch36x.ko" if successful
4. Type "sudo make load" or "sudo insmod ch36x.ko" to load the driver dynamically
5. Type "sudo make unload" or "sudo rmmod ch36x.ko" to unload the driver
6. Type "sudo make install" to make the driver work permanently
7. Type "sudo make uninstall" to remove the driver

​	Before the driver works, you should make sure that the ch36x device has been plugged in and is working properly, you can use shell command "lspci" or "dmesg" to confirm that, ID of ch365 is [4348]:[5049], VID of ch367/ch368 is [1C00].

​	If ch36x device works well, the driver will created devices named "ch36xpcix" in /dev directory.

## Note

​	Any question, you can send feedback to mail: tech@wch.cn
