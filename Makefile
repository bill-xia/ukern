NASM := nasm
GCCPREFIX := $(shell touch .gccprefix && cat .gccprefix)
CC := $(GCCPREFIX)gcc
LD := $(GCCPREFIX)ld
AS := $(GCCPREFIX)as
CFLAGS := -Wall -nostdinc -ffreestanding -g -mcmodel=large -fno-pic
QEMU_FLAGS := -d cpu_reset
OBJ_DIR := obj
PREBUILD_DIRS := \
	$(OBJ_DIR)/kernel/lib \
	$(OBJ_DIR)/kernel/pcie \
	$(OBJ_DIR)/kernel/fs \
	$(OBJ_DIR)/user/lib \
	$(OBJ_DIR)/boot \
	$(OBJ_DIR)/efi
EFI_FILES := \
	OVMF_CODE.fd \
	OVMF_VARS.fd

.PHONY: all qemu qemu-bios clean

all: image

bios: image fsimg

$(PREBUILD_DIRS):
	mkdir -p $@

$(EFI_FILES):
	cp /usr/share/OVMF/$@ $@

include boot/Makefrag
include kernel/Makefrag
include user/Makefrag

fsimg: $(KERNEL_BINS)
	dd if=/dev/zero of=fsimg bs=512 count=4096
	mkfs.exfat fsimg -b 512 -c 512
	mkdir fsdir
	mount fsimg fsdir/ -o loop || { echo "Mount failed"; exit 1; }
	echo "hel" | tee -a fsdir/hel
	echo "helloworld" | tee -a fsdir/helloworld
	for bin in $(KERNEL_BINS) ; do \
		cp $$bin fsdir/ ; \
	done
	umount fsdir
	[ -d fsdir ] && rm -rf fsdir || true

image: pre-build $(OBJ_DIR)/boot/boot $(OBJ_DIR)/kernel/kernel_bios
	cp $(OBJ_DIR)/boot/boot image
	dd oflag=append conv=notrunc if=$(OBJ_DIR)/kernel/kernel of=image
	truncate -s515585 image # to work on q35, weird... see https://stackoverflow.com/questions/68746570/how-to-make-simple-bootloader-for-q35-machine

gnu-efi/x86_64/apps/ukernbl.efi: gnu-efi/apps/ukernbl.c
	make -C gnu-efi -f Makefile
	cp gnu-efi/x86_64/apps/ukernbl.efi obj/efi

diskimg: $(PREBUILD_DIRS) $(EFI_FILES) $(OBJ_DIR)/kernel/kernel gnu-efi/x86_64/apps/ukernbl.efi
	@echo "[*] Building disk image with EFI files ..."
	dd if=/dev/zero of=diskimg bs=1M count=200
	(echo g; echo n; echo; echo; echo +100M; echo t; echo EFI System; echo n; echo; echo; echo; echo w) | fdisk diskimg
	sync

	kpartx -l diskimg | cut -d " " -f 1 | head -n 1 > .disk_dev_efi
	kpartx -l diskimg | cut -d " " -f 1 | tail -n 1 > .disk_dev_main
	kpartx -a diskimg

	mkfs.fat /dev/mapper/$$(cat .disk_dev_efi) -F 32
	mkfs.exfat /dev/mapper/$$(cat .disk_dev_main) -b 512 -c 512
	sync

	mkdir efifs
	mount /dev/mapper/$$(cat .disk_dev_efi) efifs
	rm -rf efifs/*
	mkdir -p efifs/EFI/boot
	mkdir -p efifs/EFI/ukern
	cp gnu-efi/x86_64/apps/ukernbl.efi efifs/EFI/boot/bootx64.EFI
	cp obj/kernel/kernel efifs/EFI/ukern/kernel
	cp tamsyn.psf efifs/EFI/ukern/tamsyn.psf
	sync
	umount efifs
	[ -d efifs ] && rm -rf efifs || true

	mkdir mainfs
	mount /dev/mapper/$$(cat .disk_dev_main) mainfs
	rm -rf mainfs/*
	echo "hel" | tee -a mainfs/hel
	echo "helloworld" | tee -a mainfs/helloworld
	mkdir -p mainfs/a/b
	mkdir -p mainfs/b
	echo "foo" | tee -a mainfs/a/foo
	echo "foo" | tee -a mainfs/a/b/foo
	echo "bar" | tee -a mainfs/b/bar
	for bin in $(KERNEL_BINS) ; do \
		cp $$bin mainfs/ ; \
	done
	sync
	umount mainfs
	[ -d mainfs ] && rm -rf mainfs || true
	kpartx -d diskimg

qemu: diskimg
	qemu-system-x86_64 \
		$(QEMU_FLAGS) \
		-machine q35 \
		-drive file=diskimg,format=raw \
		-drive if=pflash,format=raw,unit=0,readonly=on,file=OVMF_CODE.fd \
		-drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd

qemu-bios: bios
	qemu-system-x86_64 $(QEMU_FLAGS) -machine q35 -drive file=image,format=raw -drive file=gptimg,format=raw #-device qemu-xhci
	# -drive file=fsimg,if=none,id=nvm,format=raw -device nvme,serial=deadbeef,drive=nvm 

qemu-gdb: diskimg
	qemu-system-x86_64 $(QEMU_FLAGS) \
		-machine q35 \
		-drive file=diskimg,format=raw \
		-drive if=pflash,format=raw,unit=0,readonly=on,file=OVMF_CODE.fd \
		-drive if=pflash,format=raw,unit=1,file=OVMF_VARS.fd \
		-s \
		-S

qemu-bios-gdb: bios
	qemu-system-x86_64 $(QEMU_FLAGS) -machine q35 -drive file=image,format=raw -drive file=gptimg,format=raw -s -S

gdb:
	gdb -n -x .gdbinit

clean:
	rm -rf obj/ image efifs/ mainfs/ fsimg
	dmsetup remove_all
	losetup -D