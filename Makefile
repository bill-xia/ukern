AS := /usr/bin/nasm
CC := /usr/bin/gcc
CFLAGS := -I include

all: image

include boot/Makefrag
include kernel/Makefrag

pre-qemu: image

image: boot/boot kernel/kernel
	cp boot/boot image
	dd oflag=append conv=notrunc if=kernel/kernel of=image

qemu: pre-qemu
	qemu-system-x86_64 -drive file=image,format=raw

qemu-gdb: pre-qemu
	qemu-system-x86_64 -drive file=image,format=raw -s -S

gdb: pre-qemu
	gdb -n -x .gdbinit

clean:
	rm -f boot/boot.out boot/boot image kernel/*.o kernel/kernel kernel/entry