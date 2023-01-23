AS := /usr/bin/nasm
CC := /usr/bin/gcc
CFLAGS := -nostdinc -fno-builtin -g -fno-stack-protector
# QEMU_FLAGS := -d cpu_reset
OBJ_DIR := obj

all: image

include boot/Makefrag
include kernel/Makefrag
include user/Makefrag

pre-qemu: image

pre-build:
	mkdir -p obj/kernel/lib
	mkdir -p obj/kernel/pcie
	mkdir -p obj/kernel/fs
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
	truncate -s515585 image # to work on q35, weird... see https://stackoverflow.com/questions/68746570/how-to-make-simple-bootloader-for-q35-machine

qemu: pre-qemu
	qemu-system-x86_64 $(QEMU_FLAGS) -machine q35 -drive file=image,format=raw -drive file=gptimg,format=raw #-device qemu-xhci
	# -drive file=fsimg,if=none,id=nvm,format=raw -device nvme,serial=deadbeef,drive=nvm 

qemu-gdb: pre-qemu
	qemu-system-x86_64 $(QEMU_FLAGS) -machine q35 -drive file=image,format=raw -drive file=fsimg,format=raw -s -S

gdb: pre-qemu
	gdb -n -x .gdbinit

clean:
	rm -rf obj/ image fsdir/ fsimg