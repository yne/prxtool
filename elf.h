#define ELF_SECT_MAX_NAME 128

/* Structure defining a single elf section */
typedef struct{
	uint32_t iName;
	uint32_t iType;
	uint32_t iFlags;
	uint32_t iAddr;
	uint32_t iOffset;
	uint32_t iSize;
	uint32_t iLink;
	uint32_t iInfo;
	uint32_t iAddralign;
	uint32_t iEntsize;

	uint8_t *pData;
	char szName[ELF_SECT_MAX_NAME];
	struct ElfReloc *pRelocs;
	uint32_t iRelocCount;
}ElfSection;

typedef struct{
	uint32_t iType;
	uint32_t iOffset;
	uint32_t iVaddr;
	uint32_t iPaddr;
	uint32_t iFilesz;
	uint32_t iMemsz;
	uint32_t iFlags;
	uint32_t iAlign;

	/* Aliased pointer to the data (in the original Elf)*/
	uint8_t  *pData;
}ElfProgram;

/* Structure to hold elf header data, in native format */
typedef struct{
	uint32_t iMagic;
	uint32_t iClass;
	uint32_t iData;
	uint32_t iIdver;
	uint32_t iType; 
	uint32_t iMachine; 
	uint32_t iVersion; 
	uint32_t iEntry; 
	uint32_t iPhoff; 
	uint32_t iShoff; 
	uint32_t iFlags; 
	uint32_t iEhsize;
	uint32_t iPhentsize; 
	uint32_t iPhnum; 
	uint32_t iShentsize; 
	uint32_t iShnum; 
	uint32_t iShstrndx; 
}ElfHeader;

typedef struct{
	const char* secname;
	uint32_t base;
	uint32_t type;
	uint32_t symbol;
	uint32_t offset;
	uint32_t info;
	uint32_t addr;
}ElfReloc;

typedef struct{
	const char *symname;
	uint32_t name;
	uint32_t value;
	uint32_t size;
	uint32_t info;
	uint32_t other;
	uint32_t shndx;
}ElfSymbol;

/* Define ELF types */
typedef uint32_t Elf32_Addr; 
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Off;
typedef int32_t Elf32_Sword;
typedef uint32_t Elf32_Word;

#define ELF_MAGIC	0x464C457F

#define ELF_MIPS_TYPE 0x0002
#define ELF_PRX_TYPE  0xFFA0

#define SHT_NULL 0 
#define SHT_PROGBITS 1 
#define SHT_SYMTAB 2 
#define SHT_STRTAB 3 
#define SHT_RELA 4 
#define SHT_HASH 5 
#define SHT_DYNAMIC 6 
#define SHT_NOTE 7 
#define SHT_NOBITS 8 
#define SHT_REL 9 
#define SHT_SHLIB 10 
#define SHT_DYNSYM 11 
#define SHT_LOPROC 0x70000000 
#define SHT_HIPROC 0x7fffffff 
#define SHT_LOUSER 0x80000000 
#define SHT_HIUSER 0xffffffff

#define SHT_PRXRELOC (SHT_LOPROC | 0xA0)

// MIPS Reloc Entry Types
#define R_MIPS_NONE     0
#define R_MIPS_16       1
#define R_MIPS_32       2
#define R_MIPS_26       4
#define R_MIPS_HI16     5
#define R_MIPS_LO16     6

/* Unsupported for PRXes (loadcore.prx ignores them) */
#define R_MIPS_REL32    3
#define R_MIPS_GPREL16  7
#define R_MIPS_LITERAL  8
#define R_MIPS_GOT16    9
#define R_MIPS_PC16     10
#define R_MIPS_CALL16   11
#define R_MIPS_GPREL32  12

/* For the new relocation type */
#define R_MIPS_X_HI16   13
#define R_MIPS_X_J26    14
#define R_MIPS_X_JAL26  15


#define SHF_WRITE 		1
#define SHF_ALLOC 		2
#define SHF_EXECINSTR 	4

#define PT_NULL 		0
#define PT_LOAD 		1
#define PT_DYNAMIC 		2
#define PT_INTERP 		3
#define PT_NOTE 		4
#define PT_SHLIB 		5
#define PT_PHDR 		6
#define PT_LOPROC 		0x70000000
#define PT_HIPROC 		0x7fffffff

#define PT_PRXRELOC             0x700000A0
#define PT_PRXRELOC2            0x700000A1

/* ELF file header */
typedef struct { 
	Elf32_Word e_magic;
	uint8_t e_class;
	uint8_t e_data;
	uint8_t e_idver;
	uint8_t e_pad[9];
	Elf32_Half e_type; 
	Elf32_Half e_machine; 
	Elf32_Word e_version; 
	Elf32_Addr e_entry; 
	Elf32_Off e_phoff; 
	Elf32_Off e_shoff; 
	Elf32_Word e_flags; 
	Elf32_Half e_ehsize; 
	Elf32_Half e_phentsize; 
	Elf32_Half e_phnum; 
	Elf32_Half e_shentsize; 
	Elf32_Half e_shnum; 
	Elf32_Half e_shstrndx; 
} __attribute__((packed)) Elf32_Ehdr;

/* ELF section header */
typedef struct { 
	Elf32_Word sh_name; 
	Elf32_Word sh_type; 
	Elf32_Word sh_flags; 
	Elf32_Addr sh_addr; 
	Elf32_Off sh_offset; 
	Elf32_Word sh_size; 
	Elf32_Word sh_link; 
	Elf32_Word sh_info; 
	Elf32_Word sh_addralign; 
	Elf32_Word sh_entsize; 
} __attribute__((packed)) Elf32_Shdr;

typedef struct { 
	Elf32_Word p_type; 
	Elf32_Off p_offset; 
	Elf32_Addr p_vaddr; 
	Elf32_Addr p_paddr; 
	Elf32_Word p_filesz; 
	Elf32_Word p_memsz; 
	Elf32_Word p_flags; 
	Elf32_Word p_align; 
} Elf32_Phdr;

#define ELF32_R_SYM(i) ((i)>>8) 
#define ELF32_R_TYPE(i) ((uint8_t)(i&0xFF))

typedef struct { 
	Elf32_Addr r_offset; 
	Elf32_Word r_info; 
} Elf32_Rel;

#define ELF32_ST_BIND(i) ((i)>>4)
#define ELF32_ST_TYPE(i) ((i)&0xf)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))

#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_LOPROC 13
#define STT_HIPROC 15

typedef struct { 
	Elf32_Word st_name; 
	Elf32_Addr st_value; 
	Elf32_Word st_size; 
	unsigned char st_info; 
	unsigned char st_other; 
	Elf32_Half st_shndx; 
} __attribute__((packed)) Elf32_Sym;

