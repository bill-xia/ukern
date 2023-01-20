%define CR0_PE 0x1
%define CR0_PG 0x80000000
%define CR0_WP 0x10000

%define CR4_PAE 0x20

%define IA32_EFER_LME 0x100

%define KERN_CODE 0x08
%define KERN_DATA 0x10

%define PDE_P 0x1
%define PDE_W 0x2
%define PTE_P 0x1
%define PTE_W 0x2
%define PML4E_P 0x1
%define PML4E_W 0x2
%define PDPTE_P 0x1
%define PDPTE_W 0x2

%define Disk_SecPerTrk 18
%define Disk_DrvNum 0

%macro SEG_DESC 3
; %1 base, %2 lim, %3 type
    dw ((%2 >> 12) & 0xFFFF),(%1 & 0xFFFF)
    db ((%1 >> 16) & (0xFF)),(0x90 | %3),(0xC0 | ((%2 >> 28) & 0xf)),(%1 >> 24)
%endmacro

; extern c_entry

[BITS 16]

section .text

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax

; disable cursor
    mov ax, 0100h
    mov cx, 3F00h
    int 10h

; get memory info
    mov bx, 0
    mov di, bx
    mov bx, 0x900
    mov es, bx ; ES:DI - 0x9000
    mov ebx, 0
    mov edx, 0x534D4150
beg_get_meminfo:
    mov eax, 0xe820 ; Query System Address Map
    mov ecx, 24
    int 15h
    add di, 24
    cmp ebx, 0
    jne beg_get_meminfo

    add di, 16
    mov dword [es:di], 0

; setting fs for accessing memory above 1MB
    in al, 92h
    or al, 2h
    out 92h, al
    
    cli
    db 0x66
    lgdt [gdt_desc]
    mov eax, cr0
    or  eax, CR0_PE
    mov cr0, eax

    mov ax, 0x10 ; kernel data
    mov fs, ax
    mov eax, cr0

    mov eax, cr0
    and ax, ~CR0_PE
    mov cr0, eax

    sti

; read ELF64 header
    mov ax, 0x0
    mov es, ax
    mov bx, 0x7e00 ; es:bx - 0x7e00
    mov ax, 1 ; kernel image at sector 1
    mov cx, 1 ; 0x7e00 - 0x8000
    call Func_ReadOneSector

    mov eax, [0x7e20]
    add eax, 0x7e00
    mov [phoff], eax 
    mov ax, [0x7e36]
    mov [phentsize], ax
    mov ax, [0x7e38]
    push ax ; phnum
Label_ReadProgramHeader:
    mov eax, [phoff]
    mov edx, [eax + 24] ; paddr / load address
    mov [paddr], edx
    mov [paddrrem], edx
    mov edx, [eax + 8] ; file offset
    shr edx, 9 ; calc LBA
    add edx, 1 ; skip MBR
    mov [lba], edx
    mov edx, [eax + 40] ; size in memory
    mov [memsz], edx
    mov ecx, [eax + 32] ; size in file
    mov [filesz], ecx
    add ecx, 511 ; also read the tail
    shr ecx, 9 ; calc num of sectors to read
    mov ebx, [eax] ; type
    cmp ebx, 1
    jne Label_ReadNextProgramHeader
Label_ReadProgramSector:
    mov ax, 0
    mov es, ax
    mov bx, 0x8000
    push ecx
    mov cx, 1
    mov eax, [lba]
    call Func_ReadOneSector

    mov eax, [paddr]
    mov di, 0x8000
    call Func_MoveOneSector
    add dword [paddr], 0x200
    inc dword [lba]

    pop ecx
    dec ecx
    jnz Label_ReadProgramSector

    push ecx

    mov eax, [paddrrem]
    mov ecx, eax
    add eax, [filesz]
    add ecx, [memsz]

Label_MemClear:
    cmp eax, ecx
    jge Label_MemClearEnd
    mov byte [eax], 0
    inc eax
    jmp Label_MemClear

Label_MemClearEnd:
    pop ecx

Label_ReadNextProgramHeader:
    ; read one program header done
    pop ax ; phnum
    dec ax
    jz prot_mode

    push ax ; phnum
    mov ax, [phentsize]
    add [phoff], ax
    jmp Label_ReadProgramHeader

prot_mode:
; getting into protected mode
    cli
    db 0x66
    lgdt [gdt_desc]
    mov eax, cr0
    or  eax, CR0_PE
    mov cr0, eax

    jmp dword KERN_CODE:prot_start

; ============== Functions ==============
; read kernel image
    
; ax - LBA
; cl - num of sectors to read
; es:bx - logical address

; https://en.wikipedia.org/wiki/INT_13H#INT_13h_AH=42h:_Extended_Read_Sectors_From_Drive
Func_ReadOneSector:
    push dword 0000h
    push dword eax
    push word es
    push word bx
    push word cx
    push word 10h
    mov ah, 42h
    mov dl, 80h
    mov si, sp
    int 13h
    add sp, 16
    ret

; move one sector
; from es:di to fs:eax
Func_MoveOneSector:
    mov ecx, eax
    add ecx, 0x200
Label_CopyByte:
    mov bl, [es:di]
    mov [fs:eax], bl
    inc di
    inc eax
    cmp eax, ecx
    jne Label_CopyByte
    ret

[BITS 32]
prot_start:
    mov eax, KERN_DATA
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov eax, 0x7c00
    mov esp, eax

    call 0x100000
    ; jmp $

section .data
; boot_message: db "hello from ukern"
phoff: dd 0
phentsize: dd 0
paddr: dd 0
paddrrem: dd 0
lba: dd 0
filesz: dd 0
memsz: dd 0

gdt_protect:
dq 0 ; first null segment
SEG_DESC 0,0xFFFFFFFF,0x0A ; kernel code
SEG_DESC 0,0xFFFFFFFF,0x02 ; kernel data

gdt_desc:
dw 0x17
dd gdt_protect

section .mbrsign
dw 0xAA55
