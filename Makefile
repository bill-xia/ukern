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

image: pre-build $(OBJ_DIR)/boot/boot $(OBJ_DIR)/kernel/kernel
	cp $(OBJ_DIR)/boot/boot image
	dd oflag=append conv=notrunc if=$(OBJ_DIR)/kernel/kernel of=image

qemu: pre-qemu
	qemu-system-x86_64 $(QEMU_FLAGS) -drive file=image,format=raw

qemu-gdb: pre-qemu
	qemu-system-x86_64 $(QEMU_FLAGS) -drive file=image,format=raw -s -S

gdb: pre-qemu
	gdb -n -x .gdbinit

clean:
	rm -rf obj/ image