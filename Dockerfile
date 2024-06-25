FROM ubuntu:22.04

RUN apt update && apt install -y sudo git gcc binutils build-essential qemu qemu-system-x86 ovmf nasm kpartx fdisk exfatprogs dosfstools
RUN apt update && apt install -y gdb
#https://github.com/moby/moby/issues/8710
#dmsetup remove_all
#losetup -D

RUN mkdir -p /root/ukern

CMD ["bash"]