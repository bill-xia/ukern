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

%macro SEG_DESC_64 4
; %1 base, %2 lim, %3 type, %4 L
    dw ((%2 >> 12) & 0xFFFF),(%1 & 0xFFFF)
    db ((%1 >> 16) & (0xFF)),(0x90 | %3),(%4 | ((%2 >> 28) & 0xf)),(%1 >> 24)
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

; print
    mov ax, 1301h
    mov bx, 0007h
    mov cx, 16
    mov dx, 0
    mov bp, boot_message
    int 10h

; prepare page table
    ; page directory at 8000h, points to a page table at 9000h
    mov dword [8000h], 9000h | PDE_P | PDE_W
    ; page table at 9000h, points to pages at 0000h ~ 9000h
    mov eax, 9000h
    mov ebx, PTE_P | PTE_W
beg_prot_pgtbl:
    cmp eax, 9024h
    jg end_prot_pgtbl
    mov dword [eax], ebx
    add eax, 4
    add ebx, 1000h
    jmp beg_prot_pgtbl
end_prot_pgtbl:
    ; load page table
    mov eax, 8000h
    mov cr3, eax

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

; getting into IA32e mode
    mov eax, cr0
    and eax, ~CR0_PG
    mov cr0, eax

; preparing 4-level page tables
    mov dword [0x10000], 0x11000 | PML4E_P | PML4E_W
    mov dword [0x11000], 0x12000 | PDPTE_P | PDPTE_W
    mov dword [0x12000], 0x13000 | PDE_P | PDE_W

    mov eax, 0x13000
    mov ebx, PTE_P | PTE_W
beg_ia32e_pgtbl:
    cmp eax, 0x13098
    jg end_ia32e_pgtbl
    mov dword [eax], ebx
    add eax, 8
    add ebx, 0x1000
    jmp beg_ia32e_pgtbl
end_ia32e_pgtbl:

    mov eax, cr4
    or eax, CR4_PAE
    mov cr4, eax

    mov eax, 0x10000
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, IA32_EFER_LME
    wrmsr

    mov eax, cr0
    or eax, CR0_PG | CR0_PE
    mov cr0, eax

here:
    ; db 0x66
    lgdt [gdt_64_desc]
    jmp KERN_CODE:fin_mode

[BITS 64]
fin_mode:
    ; call c_entry
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

gdt_64:
dq 0
SEG_DESC_64 0,0,0x0A,0x20
SEG_DESC_64 0,0,0x02,0

gdt_64_desc:
dw 0x17
dq gdt_64

section .mbrsign
dw 0xAA55
