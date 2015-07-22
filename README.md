x86-OS
======

x86 operating system written in NASM. At the moment, this OS is basically a bootloader that sets up<br/>
the machine to run a C kernel.<br/>
Eventually I'll actually write the kernel.
<br/><br/>
<h4>Currently implemented features:</h4>
* 2-stage bootloader
* FAT12 driver
* 1.44MB floppy disk I/O
* Enable A20 gate
* Protected mode
* Global Descriptor Table
* Kernel stub
* Protected mode for VGA text output
<br/>
<h4>Todo:</h4>
* Write the kernel
