#include <efi.h>
#include <efilib.h>
#include <elf.h>

struct FRAME_BUFFER {
	UINT32	*Buffer;
	UINT64	BufferSize;
	UINT32	Width,
		Height,
		PixelsPerScanLine;
} FrameBuffer;

EFI_FILE* OpenFile(EFI_FILE* Directory, CHAR16* Path, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable){
	EFI_FILE* LoadedFile;

	EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
	SystemTable->BootServices->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage);

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem;
	SystemTable->BootServices->HandleProtocol(LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&FileSystem);

	if (Directory == NULL){
		FileSystem->OpenVolume(FileSystem, &Directory);
	}

	EFI_STATUS s = Directory->Open(Directory, &LoadedFile, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
	if (s != EFI_SUCCESS){
		return NULL;
	}
	return LoadedFile;
}

EFI_STATUS
InitializeGOP() {
	EFI_GUID GOPGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *GOP;
	EFI_STATUS s;
	UINT32 Mode;

	s = uefi_call_wrapper(BS->LocateProtocol, 3, &GOPGuid, NULL, (void**)&GOP);
	if (EFI_ERROR(s)) {
		Print(L"Unable to locate GOP\n\r");
	} else {
		Print(L"GOP located.\n\r");
	}

	for (Mode = 0; Mode < GOP->Mode->MaxMode; ++Mode) {
		UINTN SizeOfInfo;
		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
		GOP->QueryMode(GOP, Mode, &SizeOfInfo, &Info);
		if (Info->HorizontalResolution == 1920 && Info->VerticalResolution == 1080) {
			GOP->SetMode(GOP, Mode);
			break;
		}
	}
	
	if (Mode == GOP->Mode->MaxMode) {
		Print(L"Initialize GOP: no 1920x1080 mode.\n");
		return EFI_SUCCESS;
	}

	FrameBuffer.Buffer = (UINT32 *)GOP->Mode->FrameBufferBase;
	FrameBuffer.BufferSize = GOP->Mode->FrameBufferSize;
	FrameBuffer.Width = GOP->Mode->Info->HorizontalResolution;
	FrameBuffer.Height = GOP->Mode->Info->VerticalResolution;
	FrameBuffer.PixelsPerScanLine = GOP->Mode->Info->PixelsPerScanLine;

	// Print(L"BltBuffer base %lx, size %ld, width %d, height %d, pps %d\r\n",
	// 	FrameBuffer.Buffer,
	// 	FrameBuffer.BufferSize,
	// 	FrameBuffer.Width,
	// 	FrameBuffer.Height,
	// 	FrameBuffer.PixelsPerScanLine
	// );

	// Clear the screen now
	UINTN Row, Col;
	for (Row = 0; Row < 1080; ++Row) {
		for (Col = 0; Col < 1920; ++Col) {
			FrameBuffer.Buffer[Row * 1920 + Col] = 0;
		}
	}

	return s;
}

typedef unsigned int size_t;

int
memcmp(void *s1, void *s2, size_t siz)
{
	char *a = s1, *b = s2;
	size_t i = 0;
	for (i = 0; i < siz; ++i) {
		if (a[i] != b[i])
			return 1;
	}
	return 0;
}

EFI_STATUS
LoadELF64Image(EFI_FILE *File)
{
	Elf64_Ehdr *ElfHdr;
	uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, sizeof(Elf64_Ehdr), (void **)&ElfHdr);
	UINTN Size = sizeof(Elf64_Ehdr);
	File->Read(File, &Size, ElfHdr);
	if (
		memcmp(&ElfHdr->e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0 ||
		ElfHdr->e_ident[EI_CLASS] != ELFCLASS64 ||
		ElfHdr->e_ident[EI_DATA] != ELFDATA2LSB ||
		ElfHdr->e_type != ET_EXEC ||
		ElfHdr->e_machine != EM_X86_64 ||
		ElfHdr->e_version != EV_CURRENT
	) {
		Print(L"Kernel format is bad %x\n.", File);
		while (1)
			;
	}

	// Program Header
	UINTN NProgHdr;

	Elf64_Phdr *ProgHdr;
	uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, sizeof(Elf64_Phdr), (void **)&ProgHdr);
	for (NProgHdr = 0; NProgHdr < ElfHdr->e_phnum; NProgHdr++) {
		File->SetPosition(File, ElfHdr->e_phoff + NProgHdr * ElfHdr->e_phentsize);
		Size = sizeof(Elf64_Phdr);
		File->Read(File, &Size, ProgHdr);

		if (!(ProgHdr->p_flags & PT_LOAD)) {
			continue;
		}

		// Print(L"Load one segment, offset: %lx, paddr, %lx, filesz %lx, memsz %lx.\n",
		// 	ProgHdr->p_offset,
		// 	ProgHdr->p_paddr,
		// 	ProgHdr->p_filesz,
		// 	ProgHdr->p_memsz
		// );
		File->SetPosition(File, ProgHdr->p_offset);

		UINT64 Addr;
		for (Addr = ProgHdr->p_paddr; Addr < ProgHdr->p_paddr + ProgHdr->p_filesz; Addr += EFI_PAGE_SIZE) {
			uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress, EfiLoaderData, 1, &Addr);
			if (ProgHdr->p_paddr + ProgHdr->p_filesz - Addr >= EFI_PAGE_SIZE) {
				Size = EFI_PAGE_SIZE;
				File->Read(File, &Size, (void *)Addr);
			} else {
				Size = ProgHdr->p_paddr + ProgHdr->p_filesz - Addr;
				File->Read(File, &Size, (void *)Addr);
				UINT8 *BSS;
				for (BSS = (void *)(Addr + Size); (UINT64)BSS < (Addr + EFI_PAGE_SIZE); ++BSS) {
					*BSS = 0;
				}
			}
		}
		for (; Addr < ProgHdr->p_paddr + ProgHdr->p_memsz; Addr += EFI_PAGE_SIZE) {
			uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress, EfiLoaderData, 1, &Addr);
			UINT64 *BSS;
			for (BSS = (void *)Addr; (UINT64)BSS < (Addr + EFI_PAGE_SIZE); ++BSS) {
				*BSS = 0;
			}
		}
	}

	int (*KernelEntry)() = (int (*)())(ElfHdr->e_entry);
	Print(L"KernelEntry: %lx.\n\r", KernelEntry);
	while (1);
	// KernelEntry();

	return EFI_SUCCESS;
}

EFI_STATUS
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	InitializeLib(ImageHandle, SystemTable);

	EFI_FILE* KernelFile = OpenFile(NULL, L"KERNEL", ImageHandle, SystemTable);
	if (KernelFile == NULL){
		Print(L"Could not load kernel \n\r");
	} else{
		Print(L"Kernel Loaded Successfully \n\r");
	}

	// InitializeGOP();
	LoadELF64Image(KernelFile);


	return EFI_SUCCESS; // Exit the UEFI application
}
