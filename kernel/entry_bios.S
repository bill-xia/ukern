;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; This file doesn't work yet
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%define CR0_PE 0x1
%define CR0_PG 0x80000000
%define CR0_WP 0x10000

%define CR4_PAE 0x20

%define IA32_EFER_LME 0x100

%define KERN_CODE 0x08
%define KERN_DATA 0x10

%define PML4E_P 0x1
%define PML4E_W 0x2
%define PDPTE_P 0x1
%define PDPTE_W 0x2
%define PDPTE_PS 0x80
%define PDE_P 0x1
%define PDE_W 0x2
%define PTE_P 0x1
%define PTE_W 0x2

%macro SEG_DESC_64 5
; %1 base, %2 lim, %3 type, %4 L, %5 dpl
	dw ((%2 >> 12) & 0xFFFF),(%1 & 0xFFFF)
	db ((%1 >> 16) & (0xFF)),(0x90 | %3 | (%5 << 5)),(%4 | ((%2 >> 28) & 0xf)),(%1 >> 24)
%endmacro

section .text

[BITS 32]

global prot_to_ia32e
prot_to_ia32e:
	cli
	mov eax, cr0
	and eax, ~CR0_PG
	mov cr0, eax

; getting into IA32e mode

; preparing 4-level page tables
	mov dword [pml4], pdpt0 + (PML4E_P | PML4E_W)
	mov dword [pml4 + 0x800], pdpt0 + (PML4E_P | PML4E_W) ; map memory above KERNBASE
	mov dword [pdpt0], 0x0 | PDPTE_P | PDPTE_W | PDPTE_PS
	mov dword [pdpt0 + 0x08], 0x40000000 | PDPTE_P | PDPTE_W | PDPTE_PS
	mov dword [pdpt0 + 0x10], 0x80000000 | PDPTE_P | PDPTE_W | PDPTE_PS
	mov dword [pdpt0 + 0x18], 0xC0000000 | PDPTE_P | PDPTE_W | PDPTE_PS
; kernel stack
	mov dword [pml4 + 0xFF8], pdpt255 + (PML4E_P | PML4E_W)
	mov dword [pdpt255 + 0xFF8], pd + (PDPTE_P | PDPTE_W)
	mov dword [pd + 0xFF8], pt + (PDE_P | PDE_W)
	mov dword [pt + 0xFF0], kstack + (PTE_P | PTE_W)

	mov eax, cr4
	or eax, CR4_PAE
	mov cr4, eax

	mov eax, pml4
	mov cr3, eax

	mov ecx, 0xC0000080
	rdmsr
	or eax, IA32_EFER_LME
	wrmsr

	mov eax, cr0
	or eax, CR0_PG | CR0_PE
	mov cr0, eax

here:
	db 0x66
	lgdt [gdt_64_desc]
	jmp KERN_CODE:ia32e

section .data

gdt_64:
dq 0
SEG_DESC_64 0,0,0x08,0x20,0 ; KERN_CODE 0x08

gdt_64_desc:
dw 0x0F
dq gdt_64

align 16
kargs: dq 0

align 4096
pml4:		times 512 dq 0
pdpt0:		times 512 dq 0
pdpt255:	times 512 dq 0
pd:		times 512 dq 0
pt:		times 512 dq 0
kstack:		times 512 dq 0

[BITS 64]

extern main
section .text64
ia32e:
	; need to be canonical address, the high 4 bytes
	; cannot be 0x0000, see "The Manual" vol.1 3.3.7.1
	mov rax, 0xFFFFFFFFFFFFF000 ; kernel stack
	mov rsp, rax
	mov rax, main
	call rax