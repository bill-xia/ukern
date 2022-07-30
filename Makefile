AS := /usr/bin/nasm
CC := /usr/bin/gcc
CFLAGS := -I include -m32

all: image

pre-qemu: image

image: boot/boot_asm boot/boot_c kernel/main.o
	ld -T boot/boot.ld --oformat=binary boot/boot_asm boot/boot_c -o image
	# cp image image.mid
	ld -T kernel/kernel.ld -N kernel/main.o -o kernel/main
	dd oflag=append conv=notrunc if=kernel/main of=image

boot/boot_asm: boot/boot_asm.S
	$(AS) $< -f elf32 -o $@
	# objdump -b binary -D $@ -m i8086 > boot/boot.asm

boot/boot_c: boot/boot_c.c
	$(CC) $(CFLAGS) -O1 $< -c -o $@

kernel/main.o: kernel/main.c
	$(CC) $(CFLAGS) $^ -c -o $@

kernel/ukernio.o: kernel/ukernio.c
	$(CC) $(CFLAGS) $^ -c -o $@

qemu: pre-qemu
	qemu-system-x86_64 -drive file=image,format=raw

qemu-gdb: pre-qemu
	qemu-system-x86_64 -drive file=image,format=raw -s -S

gdb: pre-qemu
	gdb -n -x .gdbinit

kill:
	sudo killall -9 qemu-system-x86*

clean:
	rm -f boot/boot_asm boot/boot_c image kernel/*.o kernel/main