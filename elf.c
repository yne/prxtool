/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * ProcessElf.C - Implementation of a class to manipulate a ELF
 ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>



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
	/* Pointer to the section name */
	const char* secname;
	/* Base address */
	uint32_t base;
	/* Type */
	uint32_t type;
	/* Symbol (if known) */
	uint32_t symbol;
	/* Offset into the file */
	uint32_t offset;
	/* New Address for the relocation (to do with what you will) */
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

typedef struct{
	/* Pointers to the original elf and a binary image of the elf */
	uint8_t *m_pElf;
	uint32_t m_iElfSize;
	uint8_t *m_pElfBin;
	uint32_t m_iBinSize;
	int m_blElfLoaded;

	char m_szFilename[PATH_MAX];

	ElfSection m_pElfSections[64];
	int m_iSHCount;
	ElfProgram m_pElfPrograms[16];
	int m_iPHCount;
	ElfSection m_pElfStrtab[64];
	ElfHeader m_elfHeader;
	ElfSymbol m_pElfSymbols;
	int m_iSymCount;

	/* The base address of the ELF */
	uint32_t m_iBaseAddr;
}ElfCProcessElf;

/*
uint8_t* ElfLoadFileToMem_(uint32_t lSize){
	uint8_t pData[lSize];
	if(pData != NULL){
		if(fread(pData, 1, *lSize, fp) == *lSize)
			fprintf(stdout, "ELF Loaded (%d bytes)",*lSize);
		else fprintf(stderr, "Could not read in file data");
	}else fprintf(stderr, "Could not allocate memory");
}
uint8_t* ElfLoadFileToMem(const char *szFilename, uint32_t *lSize){
	FILE *fp = fopen(szFilename, "rb");
	if(fp){
		fseek(fp, 0, SEEK_END);
		*lSize = ftell(fp);
		rewind(fp);
		
		if(*lSize >= sizeof(Elf32_Ehdr)){
			ElfLoadFileToMem_();
		}else fprintf(stderr, "File not large enough to contain an ELF");
		fclose(fp);
	}else fprintf(stderr, "Could not open file %s\n", szFilename);

	return NULL;//pData;
}

void ElfElfDumpHeader(){
	fprintf(stdout,"Magic %08X\n", m_elfHeader.iMagic);
	fprintf(stdout,"Class %d\n", m_elfHeader.iClass);
	fprintf(stdout,"Data %d\n", m_elfHeader.iData);
	fprintf(stdout,"Idver %d\n", m_elfHeader.iIdver);
	fprintf(stdout,"Type %04X\n", m_elfHeader.iType);
	fprintf(stdout,"Start %08X\n", m_elfHeader.iEntry);
	fprintf(stdout,"PH Offs %08X\n", m_elfHeader.iPhoff);
	fprintf(stdout,"SH Offs %08X\n", m_elfHeader.iShoff);
	fprintf(stdout,"Flags %08X\n", m_elfHeader.iFlags);
	fprintf(stdout,"EH Size %d\n", m_elfHeader.iEhsize);
	fprintf(stdout,"PHEntSize %d\n", m_elfHeader.iPhentsize);
	fprintf(stdout,"PHNum %d\n", m_elfHeader.iPhnum);
	fprintf(stdout,"SHEntSize %d\n", m_elfHeader.iShentsize);
	fprintf(stdout,"SHNum %d\n", m_elfHeader.iShnum);
	fprintf(stdout,"SHStrndx %d\n\n", m_elfHeader.iShstrndx);
}

void ElfElfLoadHeader(const Elf32_Ehdr* pHeader){
	m_elfHeader.iMagic 		= LW(pHeader->e_magic);
	m_elfHeader.iClass 		= pHeader->e_class;
	m_elfHeader.iData 		= pHeader->e_data;
	m_elfHeader.iIdver 		= pHeader->e_idver;
	m_elfHeader.iType 		= LH(pHeader->e_type);
	m_elfHeader.iMachine 	= LH(pHeader->e_machine);
	m_elfHeader.iVersion 	= LW(pHeader->e_version);
	m_elfHeader.iEntry 		= LW(pHeader->e_entry);
	m_elfHeader.iPhoff 		= LW(pHeader->e_phoff);
	m_elfHeader.iShoff 		= LW(pHeader->e_shoff);
	m_elfHeader.iFlags 		= LW(pHeader->e_flags);
	m_elfHeader.iEhsize		= LH(pHeader->e_ehsize);
	m_elfHeader.iPhentsize 	= LH(pHeader->e_phentsize);
	m_elfHeader.iPhnum 		= LH(pHeader->e_phnum);
	m_elfHeader.iShentsize 	= LH(pHeader->e_shentsize);
	m_elfHeader.iShnum 		= LH(pHeader->e_shnum);
	m_elfHeader.iShstrndx 	= LH(pHeader->e_shstrndx);
}

int ElfElfValidateHeader(){
	Elf32_Ehdr* pHeader;
	int blRet = 0;

	assert(m_pElf != NULL);
	assert(m_iElfSize > 0);

	pHeader = (Elf32_Ehdr*) m_pElf;

	ElfLoadHeader(pHeader);

	if(m_elfHeader.iMagic == ELF_MAGIC){
		uint32_t iPhend = 0;
		uint32_t iShend = 0;

		// Check that if we have program and section headers they are valid
		if(m_elfHeader.iPhnum > 0){
			iPhend = m_elfHeader.iPhoff + (m_elfHeader.iPhentsize * m_elfHeader.iPhnum);
		}

		if(m_elfHeader.iShnum > 0){
			iShend = m_elfHeader.iShoff + (m_elfHeader.iShentsize * m_elfHeader.iShnum);
		}

		fprintf(stdout,"%08X, %08X, %08X\n", iPhend, iShend, m_iElfSize);

		if((iPhend <= m_iElfSize) && (iShend <= m_iElfSize)){
			blRet = 1;
		}else{
			fprintf(stderr, "Program or sections header information invalid");
		}
	}else{
		fprintf(stderr, "Magic value incorrect (not an ELF?)");
	}

	if(COutput::GetDebug()){
		ElfDumpHeader();
	}

	return blRet;
}

ElfSection* ElfElfFindSection(const char *szName){
	ElfSection* pSection = NULL;

	if((m_pElfSections != NULL) && (m_iSHCount > 0) && (m_pElfStrtab != NULL)){
		int iLoop;

		if(szName == NULL){
			// Return the default entry, kinda pointless :P
			pSection = &m_pElfSections[0];
		}else{
			for(iLoop = 0; iLoop < m_iSHCount; iLoop++){
				if(strcmp(m_pElfSections[iLoop].szName, szName) == 0){
					pSection = &m_pElfSections[iLoop];
				}
			}
		}
	}

	return pSection;
}

ElfSection *ElfElfFindSectionByAddr(unsigned int dwAddr){
	ElfSection* pSection = NULL;

	if((m_pElfSections != NULL) && (m_iSHCount > 0) && (m_pElfStrtab != NULL)){
		int iLoop;

		for(iLoop = 0; iLoop < m_iSHCount; iLoop++){
			if(m_pElfSections[iLoop].iFlags & SHF_ALLOC){
				uint32_t sectaddr = m_pElfSections[iLoop].iAddr;
				uint32_t sectsize = m_pElfSections[iLoop].iSize;

				if((dwAddr >= sectaddr) && (dwAddr < (sectaddr + sectsize))){
					pSection = &m_pElfSections[iLoop];
				}
			}
		}
	}

	return pSection;
}

int ElfElfAddrIsText(unsigned int dwAddr){
	int blRet = 0;
	ElfSection *sect;

	sect = ElfFindSectionByAddr(dwAddr);
	if(sect){
		if(sect->iFlags & SHF_EXECINSTR){
			blRet = 1;
		}
	}

	return blRet;
}

const char *ElfGetSymbolName(uint32_t name, uint32_t shndx){
	if((shndx > 0) && (shndx < (uint32_t) m_iSHCount)){
		if((m_pElfSections[shndx].iType == SHT_STRTAB) && (name < m_pElfSections[shndx].iSize)){
			return (char *) (m_pElfSections[shndx].pData + name);
		}
	}

	return "";
}

int ElfLoadPrograms(){
	int blRet = 1;

	if((m_elfHeader.iPhoff > 0) && (m_elfHeader.iPhnum > 0) && (m_elfHeader.iPhentsize > 0)){
		Elf32_Phdr *pHeader;
		uint8_t *pData;
		uint32_t iLoop;

		pData = m_pElf + m_elfHeader.iPhoff;

		SAFE_ALLOC(m_pElfPrograms, ElfProgram[m_elfHeader.iPhnum]);

		if(m_pElfPrograms != NULL){
			m_iPHCount = m_elfHeader.iPhnum;
			COutput::Puts(LEVEL_DEBUG, "Program Headers:");

			for(iLoop = 0; iLoop < (uint32_t) m_iPHCount; iLoop++){
				pHeader = (Elf32_Phdr *) pData;
				m_pElfPrograms[iLoop].iType = LW(pHeader->p_type);
				m_pElfPrograms[iLoop].iOffset = LW(pHeader->p_offset);
				m_pElfPrograms[iLoop].iVaddr = LW(pHeader->p_vaddr);
				m_pElfPrograms[iLoop].iPaddr = LW(pHeader->p_paddr);
				m_pElfPrograms[iLoop].iFilesz = LW(pHeader->p_filesz);
				m_pElfPrograms[iLoop].iMemsz = LW(pHeader->p_memsz);
				m_pElfPrograms[iLoop].iFlags = LW(pHeader->p_flags);
				m_pElfPrograms[iLoop].iAlign = LW(pHeader->p_align);
				m_pElfPrograms[iLoop].pData = m_pElf + m_pElfPrograms[iLoop].iOffset;

				pData += m_elfHeader.iPhentsize;
			}

			if(COutput::GetDebug()){
				for(iLoop = 0; iLoop < (uint32_t) m_iPHCount; iLoop++){
					fprintf(stdout,"Program Header %d:\n", iLoop);
					fprintf(stdout,"Type: %08X\n", m_pElfPrograms[iLoop].iType);
					fprintf(stdout,"Offset: %08X\n", m_pElfPrograms[iLoop].iOffset);
					fprintf(stdout,"VAddr: %08X\n", m_pElfPrograms[iLoop].iVaddr);
					fprintf(stdout,"PAddr: %08X\n", m_pElfPrograms[iLoop].iPaddr);
					fprintf(stdout,"FileSz: %d\n", m_pElfPrograms[iLoop].iFilesz);
					fprintf(stdout,"MemSz: %d\n", m_pElfPrograms[iLoop].iMemsz);
					fprintf(stdout,"Flags: %08X\n", m_pElfPrograms[iLoop].iFlags);
					fprintf(stdout,"Align: %08X\n\n", m_pElfPrograms[iLoop].iAlign);
				}
			}
		}else{
			blRet = 0;
		}
	}

	return blRet;
}

int ElfLoadSymbols(){
	ElfSection *pSymtab;
	int blRet = 1;

	fprintf(stdout,"Size %d\n", sizeof(Elf32_Sym));

	pSymtab = ElfFindSection(".symtab");
	if((pSymtab != NULL) && (pSymtab->iType == SHT_SYMTAB) && (pSymtab->pData != NULL)){
		Elf32_Sym *pSym;
		int iLoop, iSymcount;
		uint32_t symidx;

		symidx = pSymtab->iLink;
		iSymcount = pSymtab->iSize / sizeof(Elf32_Sym);
		SAFE_ALLOC(m_pElfSymbols, ElfSymbol[iSymcount]);
		if(m_pElfSymbols != NULL){
			m_iSymCount = iSymcount;
			pSym = (Elf32_Sym*) pSymtab->pData;
			for(iLoop = 0; iLoop < iSymcount; iLoop++){
				m_pElfSymbols[iLoop].name = LW(pSym->st_name);
				m_pElfSymbols[iLoop].symname = GetSymbolName(m_pElfSymbols[iLoop].name, symidx);
				m_pElfSymbols[iLoop].value = LW(pSym->st_value);
				m_pElfSymbols[iLoop].size = LW(pSym->st_size);
				m_pElfSymbols[iLoop].info = pSym->st_info;
				m_pElfSymbols[iLoop].other = pSym->st_other;
				m_pElfSymbols[iLoop].shndx = LH(pSym->st_shndx);
				fprintf(stdout,"Symbol %d\n", iLoop);
				fprintf(stdout,"Name %d, '%s'\n", m_pElfSymbols[iLoop].name, m_pElfSymbols[iLoop].symname);
				fprintf(stdout,"Value %08X\n",m_pElfSymbols[iLoop].value);
				fprintf(stdout,"Size  %08X\n", m_pElfSymbols[iLoop].size);
				fprintf(stdout,"Info  %02X\n", m_pElfSymbols[iLoop].info);
				fprintf(stdout,"Other %02X\n", m_pElfSymbols[iLoop].other);
				fprintf(stdout,"Shndx %04X\n\n", m_pElfSymbols[iLoop].shndx);
				pSym++;
			}
		}else{
			fprintf(stderr, "Could not allocate memory for symbols\n");
			blRet = 0;
		}
	}

	return blRet;
}

int ElfFillSection(ElfSection& elfSect, const Elf32_Shdr *pSection){
	assert(pSection != NULL);

	elfSect.iName = LW(pSection->sh_name);
	elfSect.iType = LW(pSection->sh_type);
	elfSect.iFlags = LW(pSection->sh_flags);
	elfSect.iAddr = LW(pSection->sh_addr);
	elfSect.iOffset = LW(pSection->sh_offset);
	elfSect.iSize = LW(pSection->sh_size);
	elfSect.iLink = LW(pSection->sh_link);
	elfSect.iInfo = LW(pSection->sh_info);
	elfSect.iAddralign = LW(pSection->sh_addralign);
	elfSect.iEntsize = LW(pSection->sh_entsize);
	elfSect.pData = m_pElf + elfSect.iOffset;
	elfSect.pRelocs = NULL;
	elfSect.iRelocCount = 0;

	if(((elfSect.pData + elfSect.iSize) > (m_pElf + m_iElfSize)) && (elfSect.iType != SHT_NOBITS)){
		fprintf(stderr, "Section too big for file");
		elfSect.pData = NULL;
		return 0;
	}

	return 1;
}

void ElfElfDumpSections(){
	int iLoop;
	assert(m_pElfSections != NULL);

	for(iLoop = 0; iLoop < m_iSHCount; iLoop++){
		ElfSection* pSection;

		pSection = &m_pElfSections[iLoop];
		fprintf(stdout,"Section %d\n", iLoop);
		fprintf(stdout,"Name: %d %s\n", pSection->iName, pSection->szName);
		fprintf(stdout,"Type: %08X\n", pSection->iType);
		fprintf(stdout,"Flags: %08X\n", pSection->iFlags);
		fprintf(stdout,"Addr: %08X\n", pSection->iAddr);
		fprintf(stdout,"Offset: %08X\n", pSection->iOffset);
		fprintf(stdout,"Size: %08X\n", pSection->iSize);
		fprintf(stdout,"Link: %08X\n", pSection->iLink);
		fprintf(stdout,"Info: %08X\n", pSection->iInfo);
		fprintf(stdout,"Addralign: %08X\n", pSection->iAddralign);
		fprintf(stdout,"Entsize: %08X\n", pSection->iEntsize);
		fprintf(stdout,"Data %p\n\n", pSection->pData);
	}
}

// Build a binary image of the elf file in memory
// Really should build the binary image from program headers if no section headers
int ElfBuildBinaryImage(){
	int blRet = 0; 
	int iLoop;
	uint32_t iMinAddr = 0xFFFFFFFF;
	uint32_t iMaxAddr = 0;
	long iMaxSize = 0;

	assert(m_pElf != NULL);
	assert(m_iElfSize > 0);
	assert(m_pElfBin == NULL);
	assert(m_iBinSize == 0);

	// Find the maximum and minimum addresses
	if(m_elfHeader.iType == ELF_MIPS_TYPE){
		fprintf(stdout,"Using Section Headers for binary image\n");
		// If ELF type then use the sections
		for(iLoop = 0; iLoop < m_iSHCount; iLoop++){
			ElfSection* pSection;

			pSection = &m_pElfSections[iLoop];

			if(pSection->iFlags & SHF_ALLOC){
				if((pSection->iAddr + pSection->iSize) > (iMaxAddr + iMaxSize)){
					iMaxAddr = pSection->iAddr;
					iMaxSize = pSection->iSize;
				}

				if(pSection->iAddr < iMinAddr){
					iMinAddr = pSection->iAddr;
				}
			}
		}

		fprintf(stdout,"Min Address %08X, Max Address %08X, Max Size %d\n", 
									  iMinAddr, iMaxAddr, iMaxSize);

		if(iMinAddr != 0xFFFFFFFF){
			m_iBinSize = iMaxAddr - iMinAddr + iMaxSize;
			SAFE_ALLOC(m_pElfBin, uint8_t[m_iBinSize]);
			if(m_pElfBin != NULL){
				memset(m_pElfBin, 0, m_iBinSize);
				for(iLoop = 0; iLoop < m_iSHCount; iLoop++){
					ElfSection* pSection = &m_pElfSections[iLoop];

					if((pSection->iFlags & SHF_ALLOC) && (pSection->iType != SHT_NOBITS) && (pSection->pData != NULL)){
						memcpy(m_pElfBin + (pSection->iAddr - iMinAddr), pSection->pData, pSection->iSize);
					}
				}

				m_iBaseAddr = iMinAddr;
				blRet = 1;
			}
		}
	}else{
		// If PRX use the program headers
		fprintf(stdout,"Using Program Headers for binary image\n");
		for(iLoop = 0; iLoop < m_iPHCount; iLoop++){
			ElfProgram* pProgram;

			pProgram = &m_pElfPrograms[iLoop];

			if(pProgram->iType == PT_LOAD){
				if((pProgram->iVaddr + pProgram->iMemsz) > iMaxAddr){
					iMaxAddr = pProgram->iVaddr + pProgram->iMemsz;
				}

				if(pProgram->iVaddr < iMinAddr){
					iMinAddr = pProgram->iVaddr;
				}
			}
		}

		fprintf(stdout,"Min Address %08X, Max Address %08X\n", 
									  iMinAddr, iMaxAddr);

		if(iMinAddr != 0xFFFFFFFF){
			m_iBinSize = iMaxAddr - iMinAddr;
			SAFE_ALLOC(m_pElfBin, uint8_t[m_iBinSize]);
			if(m_pElfBin != NULL){
				memset(m_pElfBin, 0, m_iBinSize);
				for(iLoop = 0; iLoop < m_iPHCount; iLoop++){
					ElfProgram* pProgram = &m_pElfPrograms[iLoop];

					if((pProgram->iType == PT_LOAD) && (pProgram->pData != NULL)){
						fprintf(stdout,"Loading program %d 0x%08X\n", iLoop, pProgram->iType);
						memcpy(m_pElfBin + (pProgram->iVaddr - iMinAddr), pProgram->pData, pProgram->iFilesz);
					}
				}

				m_iBaseAddr = iMinAddr;
				blRet = 1;
			}
		}
	}

	return blRet;
}

int ElfLoadSections(){
	int blRet = 1;

	assert(m_pElf != NULL);

	if((m_elfHeader.iShoff != 0) && (m_elfHeader.iShnum > 0) && (m_elfHeader.iShentsize > 0)){
		SAFE_ALLOC(m_pElfSections, ElfSection[m_elfHeader.iShnum]);
		if(m_pElfSections != NULL){
			int iLoop;
			uint8_t *pData;
			Elf32_Shdr *pSection;

			m_iSHCount = m_elfHeader.iShnum;
			memset(m_pElfSections, 0, sizeof(ElfSection) * m_iSHCount);
			pData = m_pElf + m_elfHeader.iShoff;

			for(iLoop = 0; iLoop < m_iSHCount; iLoop++){
				pSection = (Elf32_Shdr*) pData;
				if(FillSection(m_pElfSections[iLoop], pSection) == 0){
					blRet = 0;
					break;
				}

				pData += m_elfHeader.iShentsize;
			}

			if((m_elfHeader.iShstrndx > 0) && (m_elfHeader.iShstrndx < (uint32_t) m_iSHCount)){
				if(m_pElfSections[m_elfHeader.iShstrndx].iType == SHT_STRTAB){
					m_pElfStrtab = &m_pElfSections[m_elfHeader.iShstrndx];
				}
			}

			if(blRet){
				// If we found a string table let's run through the sections fixing up names
				if(m_pElfStrtab != NULL){
					for(iLoop = 0; iLoop < m_iSHCount; iLoop++){
						strncpy(m_pElfSections[iLoop].szName, 
								(char *) (m_pElfStrtab->pData + m_pElfSections[iLoop].iName), ELF_SECT_MAX_NAME - 1);
						m_pElfSections[iLoop].szName[ELF_SECT_MAX_NAME-1] = 0;
					}
				}

				if(COutput::GetDebug()){
					ElfDumpSections();
				}
			}
		}else{
			fprintf(stderr, "Could not allocate memory for sections");
			blRet = 0;
		}
	}
	return blRet;
}

uint32_t ElfElfGetBaseAddr(){
	if(m_blElfLoaded){
		return m_iBaseAddr;
	}

	return 0;
}

uint32_t ElfElfGetTopAddr(){
	if(m_blElfLoaded){
		return m_iBaseAddr + m_iBinSize;
	}

	return 0;
}

uint32_t ElfElfGetLoadSize(){
	if(m_blElfLoaded){
		return m_iBinSize;
	}

	return 0;
}

int ElfLoadFromFile(const char *szFilename){
	int blRet = 0;

	// Return the object to a know state
	FreeMemory();

	m_pElf = LoadFileToMem(szFilename, m_iElfSize);
	if((m_pElf != NULL) && (ElfValidateHeader() == 1)){
		if((LoadPrograms() == 1) && (LoadSections() == 1) && (LoadSymbols() == 1) && (BuildBinaryImage() == 1)){
			strncpy(m_szFilename, szFilename, PATH_MAX-1);
			m_szFilename[PATH_MAX-1] = 0;
			blRet = 1;
			m_blElfLoaded = 1;
		}
	}

	if(blRet == 0){
		FreeMemory();
	}

	return blRet;
}

int ElfBuildFakeSections(unsigned int dwDataBase){
	int blRet = 0;

	if(dwDataBase >= m_iBinSize){
		// If invalid then set to 0
		fprintf(stdout, "Invalid data base address (%d), defaulting to 0\n", dwDataBase);
		dwDataBase = 0;
	}

	SAFE_ALLOC(m_pElfSections, ElfSection[3]);
	if(m_pElfSections){
		unsigned int textsize = m_iBinSize;
		unsigned int datasize = m_iBinSize;

		if(dwDataBase > 0){
			textsize = dwDataBase;
			datasize = m_iBinSize - dwDataBase;
		}

		m_iSHCount = 3;
		memset(m_pElfSections, 0, sizeof(ElfSection) * 3);
		m_pElfSections[1].iType = SHT_PROGBITS;
		m_pElfSections[1].iFlags = SHF_ALLOC | SHF_EXECINSTR;
		m_pElfSections[1].pData = m_pElfBin;
		m_pElfSections[1].iSize = textsize;
		strcpy(m_pElfSections[1].szName, ".text");
		m_pElfSections[2].iType = SHT_PROGBITS;
		m_pElfSections[2].iFlags = SHF_ALLOC;
		m_pElfSections[2].iAddr = dwDataBase;
		m_pElfSections[2].pData = m_pElfBin + dwDataBase;
		m_pElfSections[2].iSize = datasize;
		strcpy(m_pElfSections[2].szName, ".data");
		blRet = 1;
	}

	return blRet;
}

int ElfLoadFromBinFile(const char *szFilename, unsigned int dwDataBase){
	int blRet = 0;

	// Return the object to a know state
	FreeMemory();

	m_pElfBin = LoadFileToMem(szFilename, m_iBinSize);
	if((m_pElfBin != NULL) && (BuildFakeSections(dwDataBase))){
		strncpy(m_szFilename, szFilename, PATH_MAX-1);
		m_szFilename[PATH_MAX-1] = 0;
		blRet = 1;
		m_blElfLoaded = 1;
	}

	if(blRet == 0){
		FreeMemory();
	}

	return blRet;
}

ElfSection* ElfElfGetSections(uint32_t &iSHCount){
	if(m_blElfLoaded){
		iSHCount = m_iSHCount;
		return m_pElfSections;
	}

	return NULL;
}

const char *ElfGetElfName(){
	return m_szFilename;
}
*/