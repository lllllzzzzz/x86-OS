x86-OS
======

x86 operating system written in NASM.
<br/><br/>
Currently implemented features:
* 2-stage bootloader
* FAT12 driver
* 1.44MB floppy disk I/O
* Enable A20 gate
* Protected mode
* Global Descriptor Table
* Kernel stub
* Protected mode for VGA text output
<br/><br/>
At the moment, this OS is basically a bootloader that sets up the machine to run a C kernel.<br/>
Eventually I'll actually write the kernel.
