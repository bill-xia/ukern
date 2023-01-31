#include <efi.h>
#include <efilib.h>
#include <elf.h>

struct SCREEN {
	UINT64	Buffer;
	UINT64	Glyph;
	UINT64	BufferSize;
	UINT32	Width,
		Height,
		PixelsPerScanLine,
		HPixel,
		VPixel,
		BytesPerGlyph;
} Screen;

struct MEM_MAP {
	UINT64	List,
		MapSize,
		MapKey,
		DescSize,
		DescVer;
} MemMap;

struct BOOT_ARGS {
        struct SCREEN *screen;
        struct MEM_MAP *mem_map;
	void *RSDP;
} BootArgs;

#define PSF_FONT_MAGIC 0x864ab572
 
typedef struct {
	uint32_t magic;         /* magic bytes to identify PSF */
	uint32_t version;       /* zero */
	uint32_t headersize;    /* offset of bitmaps in file, 32 */
	uint32_t flags;         /* 0 if there's no unicode table */
	uint32_t numglyph;      /* number of glyphs */
	uint32_t bytesperglyph; /* size of each glyph */
	uint32_t height;        /* height in pixels */
	uint32_t width;         /* width in pixels */
} PSF_HEADER;

EFI_FILE* OpenFile(EFI_FILE* Directory, CHAR16* Path, EFI_HANDLE ImageHandle){
	EFI_FILE* LoadedFile;

	EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
	uefi_call_wrapper(BS->HandleProtocol, 3, ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage);

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem;
	uefi_call_wrapper(BS->HandleProtocol, 3, LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&FileSystem);

	if (Directory == NULL){
		uefi_call_wrapper(FileSystem->OpenVolume, 2, FileSystem, &Directory);
	}

	EFI_STATUS s = uefi_call_wrapper(Directory->Open, 5, Directory, &LoadedFile, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
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
		uefi_call_wrapper(GOP->QueryMode, 4, GOP, Mode, &SizeOfInfo, &Info);
		if (Info->HorizontalResolution == 1920 && Info->VerticalResolution == 1080) {
			// Print(L"GOP Mode %d\n\r", Mode);
			uefi_call_wrapper(GOP->SetMode, 2, GOP, Mode);
			break;
		}
	}

	if (Mode == GOP->Mode->MaxMode) {
		Print(L"Initialize GOP: no 1920x1080 mode.\n");
		return EFI_SUCCESS;
	}

	for (Mode = 0; Mode < GOP->Mode->MaxMode; ++Mode) {
		UINTN SizeOfInfo;
		EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *Info;
		uefi_call_wrapper(GOP->QueryMode, 4, GOP, Mode, &SizeOfInfo, &Info);
		if (Info->HorizontalResolution == 1920 && Info->VerticalResolution == 1080) {
			Print(L"GOP Mode %d\n\r", Mode);
			// GOP->SetMode(GOP, Mode);
			// break;
		}
	}
	

	Screen.Buffer = GOP->Mode->FrameBufferBase;
	Screen.BufferSize = GOP->Mode->FrameBufferSize;
	Screen.Width = GOP->Mode->Info->HorizontalResolution;
	Screen.Height = GOP->Mode->Info->VerticalResolution;
	Screen.PixelsPerScanLine = GOP->Mode->Info->PixelsPerScanLine;

	Print(L"BltBuffer base %lx, size %ld, width %d, height %d, pps %d, format %d\r\n",
		Screen.Buffer,
		Screen.BufferSize,
		Screen.Width,
		Screen.Height,
		Screen.PixelsPerScanLine,
		GOP->Mode->Info->PixelFormat
	);

	// while (1);

	// Clear the screen now
	// UINTN Row, Col;
	// for (Row = 0; Row < 1080; ++Row) {
	// 	for (Col = 0; Col < 1920; ++Col) {
	// 		Screen.Buffer[Row * 1920 + Col] = 0;
	// 	}
	// }

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
GetMemMap()
{
	// Print(L"==========\n");
	UINTN	MMSize = 0,
		MapKey,
		DescSize;
	UINT32	DescVer;

	EFI_STATUS s;
	EFI_MEMORY_DESCRIPTOR *MM = NULL;
	s = uefi_call_wrapper(BS->GetMemoryMap, 5, &MMSize, MM, &MapKey, &DescSize, &DescVer);
	// Print(L"MM first call: %d, MMSize %d    ", s, MMSize);
	MMSize += 10 * DescSize;
	s = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, MMSize, (void **)&MM);
	if (s != EFI_SUCCESS) {
		Print(L"Allocate MM failed.\n");
	}
	s = uefi_call_wrapper(BS->GetMemoryMap, 5, &MMSize, MM, &MapKey, &DescSize, &DescVer);

	MemMap.List = (UINT64)MM;
	MemMap.DescSize = DescSize;
	MemMap.DescVer = DescVer;
	MemMap.MapKey = MapKey;
	MemMap.MapSize = MMSize;

	Print(L"MemMap List: %lx\n", MM);
	// while (1);

	UINTN	NEntry = (MMSize / DescSize);

	Print(L"MM ptr %lx, MM: %d, Nentry %d\n", MM, s, NEntry);
	if (NEntry > 16)
		NEntry = 16;
	
	UINT64 Addr = (UINT64)MM;
	UINT32 n = 0, i;
	for (i = 0; i < NEntry; ++i) {
		EFI_MEMORY_DESCRIPTOR *Map = (EFI_MEMORY_DESCRIPTOR *)(Addr + i * DescSize);
		// if (Map->PhysicalStart <= 0x100000 && Map->PhysicalStart + Map->NumberOfPages * EFI_PAGE_SIZE >= 0x101000 && Map->Type != EfiConventionalMemory) {
		// 	Print(L"0x1000000 Type: %d [%lx, %lx]", Map->Type, Map->PhysicalStart, Map->PhysicalStart + Map->NumberOfPages * EFI_PAGE_SIZE);
		// 	while (1);
		// }
		// if (Map->Type != EfiLoaderData)
		// 	continue;
		Print(L"[%010lx,%010lx] %02d   ",
			Map->PhysicalStart,
			Map->PhysicalStart + Map->NumberOfPages * EFI_PAGE_SIZE,
			Map->Type
		);
		if (n++ % 8 == 7) {
			Print(L"\n");
		}
	}
	// while (1);
	// Print(L"Print MemoryMap Done.\n==========\n");

	return EFI_SUCCESS;
}

typedef __attribute__((sysv_abi)) int (*KERNEL_ENTRY)(struct BOOT_ARGS *);

EFI_STATUS
LoadELF64Image(EFI_FILE *File, KERNEL_ENTRY *KernelEntry)
{
	Elf64_Ehdr *ElfHdr;
	uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, sizeof(Elf64_Ehdr), (void **)&ElfHdr);
	UINTN Size = sizeof(Elf64_Ehdr);
	uefi_call_wrapper(File->Read, 3, File, &Size, ElfHdr);
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
		uefi_call_wrapper(File->SetPosition, 2, File, ElfHdr->e_phoff + NProgHdr * ElfHdr->e_phentsize);
		Size = sizeof(Elf64_Phdr);
		uefi_call_wrapper(File->Read, 3, File, &Size, ProgHdr);

		if (ProgHdr->p_type != PT_LOAD) {
			continue;
		}

		Print(L"Load one segment, offset: %lx, paddr, %lx, filesz %lx, memsz %lx, type %x.\n",
			ProgHdr->p_offset,
			ProgHdr->p_paddr,
			ProgHdr->p_filesz,
			ProgHdr->p_memsz,
			ProgHdr->p_type
		);

		uefi_call_wrapper(File->SetPosition, 2, File, ProgHdr->p_offset);

		UINT64 Addr;
		for (Addr = ProgHdr->p_paddr; Addr < ProgHdr->p_paddr + ProgHdr->p_filesz; Addr += EFI_PAGE_SIZE) {
			EFI_STATUS s = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress, EfiLoaderData, 1, &Addr);
			if (s != EFI_SUCCESS) {
				Print(L"Loading Kernel: Alloc Page %lx failed.\n", Addr);
				while (1);
			}
			if (ProgHdr->p_paddr + ProgHdr->p_filesz - Addr >= EFI_PAGE_SIZE) {
				Size = EFI_PAGE_SIZE;
				s = uefi_call_wrapper(File->Read, 3, File, &Size, (void *)Addr);
			} else {
				Size = ProgHdr->p_paddr + ProgHdr->p_filesz - Addr;
				s = uefi_call_wrapper(File->Read, 3, File, &Size, (void *)Addr);
				UINT8 *BSS;
				for (BSS = (UINT8 *)(Addr + Size); (UINT64)BSS < (Addr + EFI_PAGE_SIZE); ++BSS) {
					*BSS = 0;
				}
			}
			if (s != EFI_SUCCESS) {
				Print(L"Read File failed, Addr %lx.\n", Addr);
				while (1);
			}
		}
		for (; Addr < ProgHdr->p_paddr + ProgHdr->p_memsz; Addr += EFI_PAGE_SIZE) {
			EFI_STATUS s = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress, EfiLoaderData, 1, &Addr);
			if (s != EFI_SUCCESS) {
				Print(L"Loading Kernel: Alloc Page %lx failed.\n", Addr);
				while (1);
			}
			UINT64 *BSS;
			for (BSS = (UINT64 *)Addr; (UINT64)BSS < (Addr + EFI_PAGE_SIZE); ++BSS) {
				*BSS = 0;
			}
		}
	}

	// UINT32 *Buffer = (UINT32 *)Screen.Buffer;
	// int i;
	// for (i = 0; i < 1920 * 1080; ++i) {
	// 	Buffer[i] = 0xFFFF00;
	// }

	*KernelEntry = (KERNEL_ENTRY)(ElfHdr->e_entry);
	Print(L"KernelEntry: %lx.\n\r", *KernelEntry);

	uefi_call_wrapper(BS->FreePool, 1, (void *)ElfHdr);

	return EFI_SUCCESS;
}

EFI_STATUS
LoadFont(EFI_FILE *File)
{
	PSF_HEADER *PSFHdr;
	uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, sizeof(PSF_HEADER), (void **)&PSFHdr);
	UINTN Size = sizeof(PSF_HEADER);
	uefi_call_wrapper(File->Read, 3, File, &Size, PSFHdr);

	if (PSFHdr->magic != PSF_FONT_MAGIC) {
		Print(L"PSF format is bad.\n");
		while (1)
			;
	}

	// Program Header
	UINT8 *Glyph;
	uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, PSFHdr->numglyph * PSFHdr->bytesperglyph, (void **)&Glyph);
	uefi_call_wrapper(File->SetPosition, 2, File, PSFHdr->headersize);
	Size = PSFHdr->numglyph * PSFHdr->bytesperglyph;
	uefi_call_wrapper(File->Read, 3, File, &Size, Glyph);

	Screen.Glyph = (UINT64)Glyph;
	Screen.VPixel = PSFHdr->height;
	Screen.HPixel = PSFHdr->width;
	Screen.BytesPerGlyph = PSFHdr->bytesperglyph;

	uefi_call_wrapper(BS->FreePool, 1, (void *)PSFHdr);

	return EFI_SUCCESS;
}

EFI_STATUS
GetRSDP(EFI_SYSTEM_TABLE *SystemTable)
{
	EFI_GUID ACPI20Guid = ACPI_20_TABLE_GUID;
	UINT64	i;
	for (i = 0; i < SystemTable->NumberOfTableEntries; ++i) {
		if (CompareGuid(&SystemTable->ConfigurationTable[i].VendorGuid, &ACPI20Guid) == 0) {
			BootArgs.RSDP = SystemTable->ConfigurationTable[i].VendorTable;
			return EFI_SUCCESS;
		}
	}
	return EFI_UNSUPPORTED;
}

// 8868e871-e4f1-11d3-bc22-0080c73c8881.

EFI_STATUS
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
	InitializeLib(ImageHandle, SystemTable);

	EFI_FILE* KernelFile = OpenFile(NULL, L"\\EFI\\ukern\\kernel", ImageHandle);
	if (KernelFile == NULL){
		Print(L"Could not open kernel file.\n\r");
	} else{
		Print(L"Open kernel file Successfully.\n\r");
	}

	InitializeGOP();

	EFI_FILE* FontFile = OpenFile(NULL, L"\\EFI\\ukern\\tamsyn.psf", ImageHandle);
	if (FontFile == NULL) {
		Print(L"Could not open font file.\n\r");
	} else{
		Print(L"Open font file Successfully.\n\r");
	}
	// PrintMemMap();
	LoadFont(FontFile);

	KERNEL_ENTRY KernelEntry;
	LoadELF64Image(KernelFile, &KernelEntry);

	GetMemMap();

	GetRSDP(SystemTable);
	// while (1);
	BootArgs.screen = &Screen;
	BootArgs.mem_map = &MemMap;
	KernelEntry(&BootArgs);

	return EFI_SUCCESS; // Exit the UEFI application
}
