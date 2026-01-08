.set MAGIC, 0x1BAD002
.set FLAGS, 0x0
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .text
.global _start
_start:
call kernel_main
cli
hang:
hlt
jmp hang