AS := /usr/bin/nasm
CC := /usr/bin/gcc
CFLAGS := -nostdinc -fno-builtin -g -fno-stack-protector -Wno-error=address-of-packed-member
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
	mkdir -p obj/efi

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

ukernbl.efi: gnu-efi/apps/ukernbl.c
	make -C gnu-efi -f Makefile
	cp gnu-efi/x86_64/apps/ukernbl.efi obj/efi

efiimg: pre-build $(OBJ_DIR)/kernel/kernel fsimg ukernbl.efi
	dd if=/dev/zero of=efiimg bs=1024 count=200000
	(echo g; echo n; echo; echo; echo +100M; echo t; echo EFI System; echo n; echo; echo; echo; echo w) | fdisk efiimg
	sudo kpartx -l efiimg | cut -d " " -f 1 | head -n 1 > .efi_dev
	sudo kpartx -a efiimg
	sudo mkfs.fat /dev/mapper/$$(cat .efi_dev) -F 32
	sudo mount /dev/mapper/$$(cat .efi_dev) efifs
	sudo cp gnu-efi/x86_64/apps/ukernbl.efi efifs/BOOTX64.EFI
	sudo cp obj/kernel/kernel efifs/KERNEL
	sudo umount efifs
	sudo kpartx -d efiimg

qemu: pre-qemu efiimg
	qemu-system-x86_64 -machine q35 -drive file=efiimg,format=raw -bios /usr/share/ovmf/OVMF.fd -nodefaults -vga std

qemu-bios: pre-qemu
	qemu-system-x86_64 $(QEMU_FLAGS) -machine q35 -drive file=image,format=raw -drive file=gptimg,format=raw #-device qemu-xhci
	# -drive file=fsimg,if=none,id=nvm,format=raw -device nvme,serial=deadbeef,drive=nvm 

qemu-gdb: pre-qemu
	qemu-system-x86_64 $(QEMU_FLAGS) -machine q35 -drive file=image,format=raw -drive file=fsimg,format=raw -s -S

gdb: pre-qemu
	gdb -n -x .gdbinit

clean:
	rm -rf obj/ image fsdir/ fsimg efiimg