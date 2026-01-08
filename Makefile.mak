# Makefile.mak

CC = i686-elf-gcc
LD = i686-elf-ld
CFLAGS = -ffreestanding -O2 -Wall -Wextra

all: os.iso

# Compile bootloader (GCC-compatible .s or .S)
boot.o: boot.s
	$(CC) -c boot.s -o boot.o

# Compile kernel
kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

# Link bootloader + kernel
kernel.bin: boot.o kernel.o
	$(LD) -T linker.ld -o kernel.bin boot.o kernel.o

# Prepare ISO folder and create ISO
os.iso: kernel.bin
	rm -rf iso
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/
	cp grub.cfg iso/boot/grub/
	grub-mkrescue -o os.iso iso

# Run in QEMU
run: os.iso
	qemu-system-i386 -cdrom os.iso

# Clean build files
clean:
	rm -rf *.o *.bin iso os.iso
