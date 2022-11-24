AS := /usr/bin/nasm
CC := /usr/bin/gcc
CFLAGS := -nostdinc -fno-builtin -g -fno-stack-protector
QEMU_FLAGS := #-d cpu_reset
OBJ_DIR := obj

all: image

include boot/Makefrag
include kernel/Makefrag
include user/Makefrag

pre-qemu: image

pre-build:
	mkdir -p obj/kernel/lib
	mkdir -p obj/user/lib
	mkdir -p obj/boot

fsimg: $(KERNEL_BINS)
	dd if=/dev/zero of=fsimg bs=512 count=4096
	mkfs.exfat fsimg -b 512 -c 512
	mkdir fsdir
	sudo mount fsimg fsdir/ -o loop
	echo "hel" | sudo tee -a fsdir/hel
	echo "helloworld" | sudo tee -a fsdir/helloworld
	for bin in $(KERNEL_BINS) ; do \
		sudo cp $$bin fsdir/ ; \
	done
	sudo umount fsdir
	rm -rd fsdir

image: pre-build $(OBJ_DIR)/boot/boot $(OBJ_DIR)/kernel/kernel fsimg
	cp $(OBJ_DIR)/boot/boot image
	dd oflag=append conv=notrunc if=$(OBJ_DIR)/kernel/kernel of=image

qemu: pre-qemu
	qemu-system-x86_64 $(QEMU_FLAGS) -device qemu-xhci -drive file=image,format=raw -drive file=fsimg,format=raw

qemu-gdb: pre-qemu
	qemu-system-x86_64 $(QEMU_FLAGS) -drive file=image,format=raw -drive file=fsimg,format=raw -s -S

gdb: pre-qemu
	gdb -n -x .gdbinit

clean:
	rm -rf obj/ image fsdir/ fsimg