AS := /usr/bin/nasm
CC := /usr/bin/gcc
CFLAGS := -I include -O1

all: image

pre-qemu: image

image: boot/boot.o
	ld -T boot/boot.ld --oformat=binary boot/boot.o -o image
	# cp image image.mid
	# dd oflag=append conv=notrunc if=kernel/main.o of=image

boot/boot.o: boot/boot.S
	$(AS) $< -f elf64 -o $@
	# objdump -b binary -D $@ -m i8086 > boot/boot.asm

boot/loader.o: boot/loader.c
	$(CC) $(CFLAGS) $< -c -o $@

kernel/main.o: kernel/main.c
	$(CC) $(CFLAGS) $< -c -o $@

qemu: pre-qemu
	qemu-system-x86_64 -drive file=image,format=raw

qemu-gdb: pre-qemu
	qemu-system-x86_64 -drive file=image,format=raw -s -S

gdb: pre-qemu
	gdb -n -x .gdbinit

kill:
	sudo killall -9 qemu-system-x86*

clean:
	rm -f boot/boot.o boot/boot.asm image boot/loader.o