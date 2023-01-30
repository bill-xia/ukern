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


[BITS 64]

extern main
extern kstack
section .text64

global ia32e
ia32e:
	cli
	mov qword [pml4], pdpt0 + (PML4E_P | PML4E_W)
	mov qword [pml4 + 0x800], pdpt0 + (PML4E_P | PML4E_W) ; map memory above KERNBASE
; kernel stack
	mov rax, pdpt255 + (PML4E_P | PML4E_W)
	mov qword [pml4 + 0xFF8], rax
	mov rax, pd + (PDPTE_P | PDPTE_W)
	mov qword [pdpt255 + 0xFF8], rax
	mov rax, pt + (PDE_P | PDE_W)
	mov qword [pd + 0xFF8], rax
	mov rax, kstack
	mov rbx, 0x00007FFFFFFFFFFF
	and rax, rbx
	or rax, (PTE_P | PTE_W)
	mov qword [pt + 0xFF0], rax

	mov rax, pml4
	mov cr3, rax

	;mov rax, 0xC0000000
	;mov dword [rax], 0xFFFF00
	;jmp $
	; need to be canonical address, the high 4 bytes
	; cannot be 0x0000, see "The Manual" vol.1 3.3.7.1
	mov rax, 0xFFFFFFFFFFFFF000 ; kernel stack
	mov rsp, rax
	mov rax, main
	call rax

section .data64

align 4096
pml4:		times 512 dq 0
pdpt0:	; map first 32G, should be enough
	dq 0x00000083
	dq 0x40000083
	dq 0x80000083
	dq 0xC0000083
	dq 0x100000083
	dq 0x140000083
	dq 0x180000083
	dq 0x1C0000083
	dq 0x200000083
	dq 0x240000083
	dq 0x280000083
	dq 0x2C0000083
	dq 0x300000083
	dq 0x340000083
	dq 0x380000083
	dq 0x3C0000083
	dq 0x400000083
	dq 0x440000083
	dq 0x480000083
	dq 0x4C0000083
	dq 0x500000083
	dq 0x540000083
	dq 0x580000083
	dq 0x5C0000083
	dq 0x600000083
	dq 0x640000083
	dq 0x680000083
	dq 0x6C0000083
	dq 0x700000083
	dq 0x740000083
	dq 0x780000083
	dq 0x7C0000083
times (512 - 32) dq 0
pdpt255:	times 512 dq 0
pd:		times 512 dq 0
pt:		times 512 dq 0