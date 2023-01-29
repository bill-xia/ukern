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
	mov rax, 0x00000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x00], rax
	mov rax, 0x40000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x08], rax
	mov rax, 0x80000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x10], rax
	mov rax, 0xC0000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x18], rax
	mov rax, 0x100000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x20], rax
	mov rax, 0x140000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x28], rax
	mov rax, 0x180000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x30], rax
	mov rax, 0x1C0000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x38], rax
	mov rax, 0x200000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x40], rax
	mov rax, 0x240000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x48], rax
	mov rax, 0x280000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x50], rax
	mov rax, 0x2C0000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x58], rax
	mov rax, 0x300000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x60], rax
	mov rax, 0x340000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x68], rax
	mov rax, 0x380000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x70], rax
	mov rax, 0x3C0000000
	or rax, PDPTE_P | PDPTE_W | PDPTE_PS
	mov qword [pdpt0 + 0x78], rax
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
pdpt0:		times 512 dq 0
pdpt255:	times 512 dq 0
pd:		times 512 dq 0
pt:		times 512 dq 0