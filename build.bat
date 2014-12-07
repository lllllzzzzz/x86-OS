@echo off
C:\Users\Luke\AppData\Local\nasm\nasm.exe -f bin "C:\Users\Luke\Programming\My OS\Stage1\Boot.asm" -o "C:\Users\Luke\Programming\My OS\Release\Boot.bin"
C:\Users\Luke\AppData\Local\nasm\nasm.exe -f bin "C:\Users\Luke\Programming\My OS\Stage2\Stage2.asm" -o "C:\Users\Luke\Programming\My OS\Release\Stage2.bin"
C:\Users\Luke\AppData\Local\nasm\nasm.exe -f bin "C:\Users\Luke\Programming\My OS\Kernel\Kernel.asm" -o "C:\Users\Luke\Programming\My OS\Release\Kernel.bin"

copy "C:\Users\Luke\Programming\My OS\Release\Stage2.bin" "A:\Stage2.bin"
copy "C:\Users\Luke\Programming\My OS\Release\Kernel.bin" "A:\Kernel.bin"

pause