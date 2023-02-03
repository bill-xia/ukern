# ukern

这是一个简单的宏内核操作系统，运行在 x86_64 体系结构上。

## Features

它处在开发的初级阶段，目前还很简单。

### 用户视角

- 1920*1080 分辨率的命令行界面
- 使用键盘输入，屏幕输出，禁用鼠标
- 支持读取 SATA 硬盘上的 exFAT 文件系统
- 内核自带 shell：支持 cd, ls, cat 三个命令，可以读取文件系统；支持 up/down 键回溯历史

### 开发者视角

- 自制 UEFI bootloader
- 虚拟内存，用户进程内存隔离
- 用户态/内核态切换，多进程并发执行
- 多个 syscall(虽然还不够多)：`exit, fork, exec, wait, open, read, write, close, opendir, readdir, chdir, pipe`
- LAPIC 时钟中断，抢占式调度
- 中断式的 PS/2 键盘驱动
- 轮询式的 SATA 驱动
- 识别 MBR 和 GPT 分区表，exFAT 文件系统
- stdin/stdout/stderr, pipe, PWD

## Usage

### 环境配置

我在 Ubuntu 系统上开发，下面的命令适用于 Ubuntu。其他 Linux 发行版应该可以轻松找到对应的包名称。

```
sudo apt install gcc binutils build-essential qemu ovmf nasm kpartx fdisk # to be tested and completed
cp /usr/share/OVMF/OVMF_VARS.fd ./
```

目前的硬盘镜像中 bootloader 在 EFI 分区的 `/EFI/boot/bootx64.efi`，会被自动加载起来，不需要配置 UEFI 启动项。

#### 交叉编译器（推荐）

如果您在 x86_64 架构的 Linux 机器上编译，可能使用系统自带的编译器也是可以的。然而构建一个交叉编译器总是最推荐的选项。

构建交叉编译器的具体步骤请参考 osdev 上的[这篇文章](https://wiki.osdev.org/GCC_Cross-Compiler)，其中 `$TARGET` 应为 `x86_64-elf`。

构建好交叉编译器后，需要保证 gcc 和 binutils 可执行文件所在的路径在 `PATH` 环境变量中。然后运行 `echo "x86_64-elf-" > .gccprefix`（注意结尾的横杠）。这是可执行文件名的前缀，如果你使用了其他的前缀则更改为对应的命令即可。

> 暂时不支持在 MacOS 上开发，日后可能会用 brew 做一个交叉编译器的仓库，并添加其他支持。

### 虚拟机运行

```
make qemu
```

### 调试

```
make qemu-gdb
```

这时 qemu 会暂停，再打开另一个终端，运行

```
make gdb
```

gdb 会显示 `0x000000000000fff0 in ?? ()`，表示它当前暂停在了 0xfff0 这个地址。这时可以根据需要设置断点。输入 `c + Enter` 即可开始运行。`Ctrl+C` 会暂停虚拟机的运行，重新获得控制权。更多用法请参考 gdb 的手册。

另外，qemu 自带的控制台也能输出很有用的信息，选中 qemu 窗口后，按下 `Ctrl+Alt+2` 即可唤起。我最常用的两条是：`info mem` 查看内存映射，`info registers` 查看寄存器的值。这与 gdb 的 `info registers` 相比，多了 GDT, LDT, IDT 和 TR 寄存器，并且对 cs, ds 等段寄存器的隐藏部分也有显示。

### 物理机运行

ukern 目前只能在支持 UEFI 的 x86_64 机器上运行，机器需要有内置 PS/2 键盘，以及至少一个 SATA 硬盘。BIOS 需要启用 UEFI 模式，CSM 开启与否均可。屏幕需要支持 1920*1080分辨率。

关于硬件兼容性，主要是 PS/2 键盘和屏幕容易出问题。我在两台 ThinkPad 笔记本上测试都正常，然而没有测试过台式机，建议使用笔记本尝试运行。

#### 安装到 EFI 分区

在物理机上安装并运行 ukern 需要以下步骤：

1. 构建二进制文件，`make efi`。如果运行过 `make qemu` 也会保留需要的二进制文件。
2. 将 bootloader (`gnu-efi/x86_64/apps/ukernbl.efi`), 拷贝到 EFI 分区，拷贝后的名称随意。
3. 添加 UEFI 启动项，指向 2 中拷贝到 EFI 分区的文件
4. 将字体 (`./tamsyn.psf`) 和内核 (`obj/kernel/kernel`) 拷贝到 EFI 分区，它们必须分别处于 `$EFI_PART/EFI/ukern/tamsyn.psf` 和 `$EFI_PART/EFI/ukern/kernel` （假设 `$EFI_PART` 是你 EFI 分区的挂载点）。
5. 构建根文件系统。目前 ukern 会使用它找到的最后一个包含 exFAT 文件系统的分区作为根文件系统，理想情况是你的所有硬盘上只有一个 exFAT 分区。这个分区必须处在一块 SATA 硬盘上(ukern 目前无法读取 nvme 硬盘)，分区的类型 GUID 需要是 windows basic data 或 linux filesystem（一般在 Windows/Linux 上创建一个普通的数据分区即可）。将所有用户程序 (`obj/user/*`) 拷贝到这个文件系统的根目录中，以让内核自带的 shell 能够找到它们。你可以按照自己的需要添加其他文件/目录。

#### 使用 U 盘启动

这种方法更加便捷。需要如下步骤：

1. 构建二进制文件，`make efi`。如果运行过 `make qemu` 也会保留需要的二进制文件。
2. 将 diskimg 烧写进你的 U 盘，例如 `sudo dd if=diskimg of=/dev/sdc bs=4096`（假设你的 U 盘对应的设备文件是 `/dev/sdc`）。
3. 构建根文件系统。目前 ukern 会使用它找到的最后一个包含 exFAT 文件系统的分区作为根文件系统，理想情况是你的所有硬盘上只有一个 exFAT 分区。这个分区必须处在一块 SATA 硬盘上(ukern 目前无法读取 nvme 硬盘)，分区的类型 GUID 需要是 windows basic data 或 linux filesystem（一般在 Windows/Linux 上创建一个普通的数据分区即可）。将所有用户程序 (`obj/user/*`) 拷贝到这个文件系统的根目录中，以让内核自带的 shell 能够找到它们。你可以按照自己的需要添加其他文件/目录。

> 为什么还需要构建根文件系统？因为 ukern 也不能读取 U 盘中的数据。

## Misc

制作这个操作系统的初衷是自己设计这个内核，掌握设计系统的能力和 C 语言项目的各种工具的使用方法。我希望这个操作系统有如下特性：

1. 源码可读性高
2. 有良好的文档和易于理解的系统概念模型，便于自学
3. 尽可能地拥有高性能和简单的机制，简单优先，性能其次，功能多样性优先级最低
4. “正确地”使用 make, gcc, ld 等工具

它最终应当实现以下功能：

1. 实现用户态和内核态
2. 实现POSIX中最重要的系统调用，包括其所需的syscall和用户级库
3. 较完整地实现一个文件系统（如exFAT）
4. 实现IPC
5. 实现shell与一些常用的用户程序（如cd, ls, chown等）
6. 可以运行一些不需要标准库的OI代码！（需要有浮点数支持）
7. 细粒度的内核锁，不留下显然的低性能代码
8. 可以测量系统的性能

如下为进阶功能，视实现难度与意义再决定是否实现：

1. 操作系统内部的编辑器，甚至编译器！这样就可以在这个系统上工作了
2. 网络驱动
3. 图形界面
4. 多体系结构支持

## License

Copyright (c) bill-xia. All rights reserved.

Licensed under the [MIT](https://github.com/bill-xia/ukern/blob/main/LICENSE) license.