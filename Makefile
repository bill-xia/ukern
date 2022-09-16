AS := /usr/bin/nasm
CC := /usr/bin/gcc
CFLAGS := -I include

include boot/Makefrag
include kernel/Makefrag

all: image

pre-qemu: image

image: boot/boot kernel/kernel
	objdump -D -b binary -mi386 boot/boot > boot/boot_disas.S
	cp boot/boot image
	dd oflag=append conv=notrunc if=kernel/kernel of=image

qemu: pre-qemu
	qemu-system-x86_64 -drive file=image,format=raw

qemu-gdb: pre-qemu
	qemu-system-x86_64 -drive file=image,format=raw -s -S

gdb: pre-qemu
	gdb -n -x .gdbinit

clean:
	rm -f boot/boot_asm boot/boot_c boot/boot_disas.S image kernel/*.o kernel/kernel kernel/entry