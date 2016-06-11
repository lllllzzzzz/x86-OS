x86-OS
======

x86 operating system written in NASM. At the moment, this OS is basically a bootloader that sets up<br/>
the machine to run a C kernel.
<br/><br/>
##Currently implemented features:
* 2-stage bootloader
* FAT12 driver
* 1.44MB floppy disk I/O
* Enable A20 gate
* Protected mode
* Global Descriptor Table
* Functions for Protected Mode VGA text output
* Kernel stub
<br/>

##TODO:
* Write kernel
