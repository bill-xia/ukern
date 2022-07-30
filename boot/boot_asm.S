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

%macro SEG_DESC 3
; %1 base, %2 lim, %3 type
    dw ((%2 >> 12) & 0xFFFF),(%1 & 0xFFFF)
    db ((%1 >> 16) & (0xFF)),(0x90 | %3),(0xC0 | ((%2 >> 28) & 0xf)),(%1 >> 24)
%endmacro

extern c_entry

[BITS 16]

section .text

start:
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov fs, ax

; clear screen
    mov ax, 0600h
    mov cx, 0000h
    mov dx, 184fh
    int 10h

    in al, 92h
    or al, 2h
    out 92h, al
    
    cli

; getting into protected mode
    db 0x66
    lgdt [gdt_desc]
    mov eax, cr0
    or  eax, CR0_PE
    mov cr0, eax
    jmp dword KERN_CODE:prot_start
    
[BITS 32]
prot_start:
    mov eax, KERN_DATA
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov eax, 0x7c00
    mov esp, eax

    call c_entry
    jmp $

section .data
boot_message: db "hello from ukern"

gdt_protect:
dq 0 ; first null segment
SEG_DESC 0,0xFFFFFFFF,0x0A ; kernel code
SEG_DESC 0,0xFFFFFFFF,0x02 ; kernel data

gdt_desc:
dw 0x17
dd gdt_protect

section .mbrsign
dw 0xAA55
