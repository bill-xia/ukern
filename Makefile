AS := /usr/bin/nasm
CC := /usr/bin/gcc
CFLAGS := -Wall -nostdinc -fno-builtin -g -fno-stack-protector -O2
# QEMU_FLAGS := -d cpu_reset
OBJ_DIR := obj

all: image

include boot/Makefrag
include kernel/Makefrag
include user/Makefrag

bios: image fsimg

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

image: pre-build $(OBJ_DIR)/boot/boot $(OBJ_DIR)/kernel/kernel_bios
	cp $(OBJ_DIR)/boot/boot image
	dd oflag=append conv=notrunc if=$(OBJ_DIR)/kernel/kernel of=image
	truncate -s515585 image # to work on q35, weird... see https://stackoverflow.com/questions/68746570/how-to-make-simple-bootloader-for-q35-machine

gnu-efi/x86_64/apps/ukernbl.efi: gnu-efi/apps/ukernbl.c
	make -C gnu-efi -f Makefile
	cp gnu-efi/x86_64/apps/ukernbl.efi obj/efi

diskimg:
	dd if=/dev/zero of=diskimg bs=1024 count=200000
	(echo g; echo n; echo; echo; echo +100M; echo t; echo EFI System; echo n; echo; echo; echo; echo w) | fdisk diskimg
	sync
	sudo kpartx -l diskimg | cut -d " " -f 1 | head -n 1 > .disk_dev_efi
	sudo kpartx -l diskimg | cut -d " " -f 1 | tail -n 1 > .disk_dev_main
	sudo kpartx -a diskimg
	sudo mkfs.fat /dev/mapper/$$(cat .disk_dev_efi) -F 32
	sudo mkfs.exfat /dev/mapper/$$(cat .disk_dev_main) -b 512 -c 512
	sudo kpartx -d diskimg

efi: pre-build $(OBJ_DIR)/kernel/kernel gnu-efi/x86_64/apps/ukernbl.efi diskimg
	sudo kpartx -l diskimg | cut -d " " -f 1 | head -n 1 > .disk_dev_efi
	sudo kpartx -l diskimg | cut -d " " -f 1 | tail -n 1 > .disk_dev_main
	sudo kpartx -a diskimg

	mkdir efifs
	sudo mount /dev/mapper/$$(cat .disk_dev_efi) efifs
	sudo rm -rf efifs/*
	sudo mkdir -p efifs/EFI/boot
	sudo mkdir -p efifs/EFI/ukern
	sudo cp gnu-efi/x86_64/apps/ukernbl.efi efifs/EFI/boot/bootx64.EFI
	sudo cp obj/kernel/kernel efifs/EFI/ukern/kernel
	sudo cp tamsyn.psf efifs/EFI/ukern/tamsyn.psf
	sync
	sudo umount efifs
	rm -rd efifs

	mkdir mainfs
	sudo mount /dev/mapper/$$(cat .disk_dev_main) mainfs
	sudo rm -rf mainfs/*
	echo "hel" | sudo tee -a mainfs/hel
	echo "helloworld" | sudo tee -a mainfs/helloworld
	sudo mkdir -p mainfs/a/b
	sudo mkdir -p mainfs/b
	echo "foo" | sudo tee -a mainfs/a/foo
	echo "foo" | sudo tee -a mainfs/a/b/foo
	echo "bar" | sudo tee -a mainfs/b/bar
	for bin in $(KERNEL_BINS) ; do \
		sudo cp $$bin mainfs/ ; \
	done
	sync
	sudo umount mainfs
	rm -rd mainfs
	sudo kpartx -d diskimg

qemu: efi
	qemu-system-x86_64 $(QEMU_FLAGS) -machine q35 -drive file=diskimg,format=raw -drive if=pflash,format=raw,unit=0,readonly=on,file=/usr/share/OVMF/OVMF_CODE.fd -drive if=pflash,format=raw,unit=1,file=./OVMF_VARS.fd

qemu-bios: bios
	qemu-system-x86_64 $(QEMU_FLAGS) -machine q35 -drive file=image,format=raw -drive file=gptimg,format=raw #-device qemu-xhci
	# -drive file=fsimg,if=none,id=nvm,format=raw -device nvme,serial=deadbeef,drive=nvm 

qemu-gdb: efi
	qemu-system-x86_64 $(QEMU_FLAGS) -machine q35 -drive file=diskimg,format=raw -drive if=pflash,format=raw,unit=0,readonly=on,file=/usr/share/OVMF/OVMF_CODE.fd -drive if=pflash,format=raw,unit=1,file=./OVMF_VARS.fd -s -S

qemu-bios-gdb: bios
	qemu-system-x86_64 $(QEMU_FLAGS) -machine q35 -drive file=image,format=raw -drive file=gptimg,format=raw -s -S

gdb:
	gdb -n -x .gdbinit

clean:
	rm -rf obj/ image efifs/ mainfs/ fsimg