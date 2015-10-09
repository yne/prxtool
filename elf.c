/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * ProcessElf.C - Implementation of a class to manipulate a ELF
 ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "elf.h"
#include "endianness.c"

typedef struct{
	/* Pointers to the original elf and a binary image of the elf */
	uint8_t *pElf;
	uint32_t iElfSize;
	uint8_t *pElfBin;
	uint32_t iBinSize;
	int blElfLoaded;

	char szFilename[PATH_MAX];

	ElfSection pElfSections[64];
	int iSHCount;
	ElfProgram pElfPrograms[16];
	int iPHCount;
	ElfSection pElfStrtab[64];
	ElfHeader elfHeader;
	ElfSymbol*pElfSymbols;
	int iSymCount;

	/* The base address of the ELF */
	uint32_t iBaseAddr;
}ElfCProcessElf;

uint8_t* ElfLoadFileToMem(ElfCProcessElf*elf, const char *szFilename, uint32_t *lSize){
	FILE *fp = fopen(szFilename, "rb");
	if(!fp)
		return fprintf(stderr, "Could not open file %s\n", szFilename),NULL;
	fseek(fp, 0, SEEK_END);
	*lSize = ftell(fp);
	rewind(fp);
	
	if(*lSize >= sizeof(Elf32_Ehdr)){
		uint8_t pData[*lSize];
		if(fread(pData, 1, *lSize, fp) == *lSize){
			fprintf(stdout, "ELF Loaded (%d bytes)",*lSize);
		}else fprintf(stderr, "Could not read in file data");
	}else fprintf(stderr, "File not large enough to contain an ELF");
	fclose(fp);
	return NULL;//pData;
}

void ElfDumpHeader(ElfCProcessElf*elf){
	fprintf(stdout,"Magic %08X\n",    elf->elfHeader.iMagic);
	fprintf(stdout,"Class %d\n",      elf->elfHeader.iClass);
	fprintf(stdout,"Data %d\n",       elf->elfHeader.iData);
	fprintf(stdout,"Idver %d\n",      elf->elfHeader.iIdver);
	fprintf(stdout,"Type %04X\n",     elf->elfHeader.iType);
	fprintf(stdout,"Start %08X\n",    elf->elfHeader.iEntry);
	fprintf(stdout,"PH Offs %08X\n",  elf->elfHeader.iPhoff);
	fprintf(stdout,"SH Offs %08X\n",  elf->elfHeader.iShoff);
	fprintf(stdout,"Flags %08X\n",    elf->elfHeader.iFlags);
	fprintf(stdout,"EH Size %d\n",    elf->elfHeader.iEhsize);
	fprintf(stdout,"PHEntSize %d\n",  elf->elfHeader.iPhentsize);
	fprintf(stdout,"PHNum %d\n",      elf->elfHeader.iPhnum);
	fprintf(stdout,"SHEntSize %d\n",  elf->elfHeader.iShentsize);
	fprintf(stdout,"SHNum %d\n",      elf->elfHeader.iShnum);
	fprintf(stdout,"SHStrndx %d\n\n", elf->elfHeader.iShstrndx);
}

void ElfLoadHeader(ElfCProcessElf*elf, const Elf32_Ehdr* pHeader){
	elf->elfHeader.iMagic     = LW(pHeader->e_magic);
	elf->elfHeader.iClass     = pHeader->e_class;
	elf->elfHeader.iData      = pHeader->e_data;
	elf->elfHeader.iIdver     = pHeader->e_idver;
	elf->elfHeader.iType      = LH(pHeader->e_type);
	elf->elfHeader.iMachine   = LH(pHeader->e_machine);
	elf->elfHeader.iVersion   = LW(pHeader->e_version);
	elf->elfHeader.iEntry     = LW(pHeader->e_entry);
	elf->elfHeader.iPhoff     = LW(pHeader->e_phoff);
	elf->elfHeader.iShoff     = LW(pHeader->e_shoff);
	elf->elfHeader.iFlags     = LW(pHeader->e_flags);
	elf->elfHeader.iEhsize    = LH(pHeader->e_ehsize);
	elf->elfHeader.iPhentsize = LH(pHeader->e_phentsize);
	elf->elfHeader.iPhnum     = LH(pHeader->e_phnum);
	elf->elfHeader.iShentsize = LH(pHeader->e_shentsize);
	elf->elfHeader.iShnum     = LH(pHeader->e_shnum);
	elf->elfHeader.iShstrndx  = LH(pHeader->e_shstrndx);
}

int ElfValidateHeader(ElfCProcessElf*elf){
	//assert(elf->pElf != NULL);
	//assert(elf->iElfSize > 0);

	ElfLoadHeader(elf, (Elf32_Ehdr*) elf->pElf);

	if(elf->elfHeader.iMagic != ELF_MAGIC)
		return fprintf(stderr, "Magic value incorrect (not an ELF?)"),0;

	uint32_t iPhend = (elf->elfHeader.iPhnum > 0)?elf->elfHeader.iPhoff + (elf->elfHeader.iPhentsize * elf->elfHeader.iPhnum):0;
	uint32_t iShend = (elf->elfHeader.iShnum > 0)?elf->elfHeader.iShoff + (elf->elfHeader.iShentsize * elf->elfHeader.iShnum):0;

	if(iPhend > elf->iElfSize)
		return fprintf(stderr, "Program header information invalid"),0;
	if(iShend > elf->iElfSize)
		return fprintf(stderr, "Sections header information invalid"),0;
	//ElfDumpHeader(elf);
	return 0;
}

ElfSection* ElfFindSection(ElfCProcessElf*elf, const char *szName){
	if((!elf->pElfSections) || (elf->iSHCount <= 0) || (!elf->pElfStrtab))
		return NULL;
	if(!szName)// Return the default entry, kinda pointless :P
		return &elf->pElfSections[0];
	for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++)
		if(!strcmp(elf->pElfSections[iLoop].szName, szName))
			return &elf->pElfSections[iLoop];
	return NULL;
}

ElfSection *ElfFindSectionByAddr(ElfCProcessElf*elf, unsigned int dwAddr){
	if((!elf->pElfSections) || (elf->iSHCount <= 0) || (!elf->pElfStrtab))
		return NULL;
	for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++){
		if(elf->pElfSections[iLoop].iFlags & SHF_ALLOC){
			uint32_t sectaddr = elf->pElfSections[iLoop].iAddr;
			uint32_t sectsize = elf->pElfSections[iLoop].iSize;
			if((dwAddr >= sectaddr) && (dwAddr < (sectaddr + sectsize)))
				return &elf->pElfSections[iLoop];
		}
	}
	return NULL;
}

int ElfAddrIsText(ElfCProcessElf*elf, unsigned int dwAddr){
	ElfSection *sect = ElfFindSectionByAddr(elf, dwAddr);
	if(sect && sect->iFlags & SHF_EXECINSTR)
		return 1;
	return 0;
}

const char *ElfGetSymbolName(ElfCProcessElf*elf, uint32_t name, uint32_t shndx){
	if((shndx > 0) && (shndx < (uint32_t) elf->iSHCount)){
		if((elf->pElfSections[shndx].iType == SHT_STRTAB) && (name < elf->pElfSections[shndx].iSize)){
			return (char *) (elf->pElfSections[shndx].pData + name);
		}
	}

	return "";
}

int ElfLoadPrograms(ElfCProcessElf*elf){
	if((elf->elfHeader.iPhoff > 0) && (elf->elfHeader.iPhnum > 0) && (elf->elfHeader.iPhentsize > 0)){
		uint8_t *pData = elf->pElf + elf->elfHeader.iPhoff;
		elf->pElfPrograms[elf->elfHeader.iPhnum];
		if(!elf->pElfPrograms)
			return 0;
		elf->iPHCount = elf->elfHeader.iPhnum;
		fprintf(stdout, "Program Headers:");
		for(uint32_t iLoop = 0; iLoop < (uint32_t) elf->iPHCount; iLoop++){
			Elf32_Phdr *pHeader = (Elf32_Phdr *) pData;
			elf->pElfPrograms[iLoop].iType = LW(pHeader->p_type);
			elf->pElfPrograms[iLoop].iOffset = LW(pHeader->p_offset);
			elf->pElfPrograms[iLoop].iVaddr = LW(pHeader->p_vaddr);
			elf->pElfPrograms[iLoop].iPaddr = LW(pHeader->p_paddr);
			elf->pElfPrograms[iLoop].iFilesz = LW(pHeader->p_filesz);
			elf->pElfPrograms[iLoop].iMemsz = LW(pHeader->p_memsz);
			elf->pElfPrograms[iLoop].iFlags = LW(pHeader->p_flags);
			elf->pElfPrograms[iLoop].iAlign = LW(pHeader->p_align);
			elf->pElfPrograms[iLoop].pData = elf->pElf + elf->pElfPrograms[iLoop].iOffset;
			pData += elf->elfHeader.iPhentsize;
		}
		for(uint32_t iLoop = 0; iLoop < (uint32_t) elf->iPHCount; iLoop++){
			fprintf(stdout,"Program Header %d:\n", iLoop);
			fprintf(stdout,"Type: %08X\n", elf->pElfPrograms[iLoop].iType);
			fprintf(stdout,"Offset: %08X\n", elf->pElfPrograms[iLoop].iOffset);
			fprintf(stdout,"VAddr: %08X\n", elf->pElfPrograms[iLoop].iVaddr);
			fprintf(stdout,"PAddr: %08X\n", elf->pElfPrograms[iLoop].iPaddr);
			fprintf(stdout,"FileSz: %d\n", elf->pElfPrograms[iLoop].iFilesz);
			fprintf(stdout,"MemSz: %d\n", elf->pElfPrograms[iLoop].iMemsz);
			fprintf(stdout,"Flags: %08X\n", elf->pElfPrograms[iLoop].iFlags);
			fprintf(stdout,"Align: %08X\n\n", elf->pElfPrograms[iLoop].iAlign);
		}
	}
	return 1;
}

int ElfLoadSymbols(ElfCProcessElf*elf){
	fprintf(stdout,"Size %d\n", sizeof(Elf32_Sym));
	ElfSection *pSymtab = ElfFindSection(elf, ".symtab");
	if((!pSymtab) || (pSymtab->iType != SHT_SYMTAB) || (!pSymtab->pData))
		return 0;
	uint32_t symidx = pSymtab->iLink;
	elf->iSymCount = pSymtab->iSize / sizeof(Elf32_Sym);
	elf->pElfSymbols[elf->iSymCount];
	
	Elf32_Sym *pSym = (Elf32_Sym*) pSymtab->pData;
	for(int iLoop = 0; iLoop < elf->iSymCount; iLoop++){
		elf->pElfSymbols[iLoop].name    = LW(pSym->st_name);
		elf->pElfSymbols[iLoop].symname = ElfGetSymbolName(elf, elf->pElfSymbols[iLoop].name, symidx);
		elf->pElfSymbols[iLoop].value   = LW(pSym->st_value);
		elf->pElfSymbols[iLoop].size    = LW(pSym->st_size);
		elf->pElfSymbols[iLoop].info    = pSym->st_info;
		elf->pElfSymbols[iLoop].other   = pSym->st_other;
		elf->pElfSymbols[iLoop].shndx   = LH(pSym->st_shndx);
		fprintf(stdout,"Symbol %d\n",   iLoop);
		fprintf(stdout,"Name %d, '%s'\n", elf->pElfSymbols[iLoop].name, elf->pElfSymbols[iLoop].symname);
		fprintf(stdout,"Value %08X\n",    elf->pElfSymbols[iLoop].value);
		fprintf(stdout,"Size  %08X\n",    elf->pElfSymbols[iLoop].size);
		fprintf(stdout,"Info  %02X\n",    elf->pElfSymbols[iLoop].info);
		fprintf(stdout,"Other %02X\n",    elf->pElfSymbols[iLoop].other);
		fprintf(stdout,"Shndx %04X\n\n",  elf->pElfSymbols[iLoop].shndx);
		pSym++;
	}
	return 0;
}

int ElfFillSection(ElfCProcessElf*elf, ElfSection* elfSect, const Elf32_Shdr *pSection){
	//assert(pSection != NULL);
	elfSect->iName = LW(pSection->sh_name);
	elfSect->iType = LW(pSection->sh_type);
	elfSect->iFlags = LW(pSection->sh_flags);
	elfSect->iAddr = LW(pSection->sh_addr);
	elfSect->iOffset = LW(pSection->sh_offset);
	elfSect->iSize = LW(pSection->sh_size);
	elfSect->iLink = LW(pSection->sh_link);
	elfSect->iInfo = LW(pSection->sh_info);
	elfSect->iAddralign = LW(pSection->sh_addralign);
	elfSect->iEntsize = LW(pSection->sh_entsize);
	elfSect->pData = elf->pElf + elfSect->iOffset;
	elfSect->pRelocs = NULL;
	elfSect->iRelocCount = 0;

	if(((elfSect->pData + elfSect->iSize) > (elf->pElf + elf->iElfSize)) && (elfSect->iType != SHT_NOBITS))
		return fprintf(stderr, "Section too big for file"),elfSect->pData = NULL,0;

	return 1;
}

void ElfDumpSections(ElfCProcessElf*elf){
	//assert(elf->pElfSections != NULL);

	for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++){
		ElfSection* pSection = &elf->pElfSections[iLoop];
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
int ElfBuildBinaryImage(ElfCProcessElf*elf){
	int blRet = 0; 
	uint32_t iMinAddr = 0xFFFFFFFF;
	uint32_t iMaxAddr = 0;
	long iMaxSize = 0;

	//assert(elf->pElf != NULL);
	//assert(elf->iElfSize > 0);
	//assert(elf->pElfBin == NULL);
	//assert(elf->iBinSize == 0);

	// Find the maximum and minimum addresses
	if(elf->elfHeader.iType == ELF_MIPS_TYPE){
		fprintf(stdout,"Using Section Headers for binary image\n");
		// If ELF type then use the sections
		for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++){
			ElfSection* pSection = &elf->pElfSections[iLoop];
			if(pSection->iFlags & SHF_ALLOC){
				if((pSection->iAddr + pSection->iSize) > (iMaxAddr + iMaxSize)){
					iMaxAddr = pSection->iAddr;
					iMaxSize = pSection->iSize;
				}
				if(pSection->iAddr < iMinAddr)
					iMinAddr = pSection->iAddr;
			}
		}
		fprintf(stdout,"Min Address %08X, Max Address %08X, Max Size %d\n", iMinAddr, iMaxAddr, iMaxSize);
		if(iMinAddr != 0xFFFFFFFF){
			elf->iBinSize = iMaxAddr - iMinAddr + iMaxSize;
			elf->pElfBin[elf->iBinSize];
			if(elf->pElfBin != NULL){
				memset(elf->pElfBin, 0, elf->iBinSize);
				for(iLoop = 0; iLoop < elf->iSHCount; iLoop++){
					ElfSection* pSection = &elf->pElfSections[iLoop];
					if((pSection->iFlags & SHF_ALLOC) && (pSection->iType != SHT_NOBITS) && (pSection->pData != NULL)){
						memcpy(elf->pElfBin + (pSection->iAddr - iMinAddr), pSection->pData, pSection->iSize);
					}
				}
				elf->iBaseAddr = iMinAddr;
				blRet = 1;
			}
		}
	}else{
		// If PRX use the program headers
		fprintf(stdout,"Using Program Headers for binary image\n");
		for(iLoop = 0; iLoop < elf->iPHCount; iLoop++){
			ElfProgram* pProgram = &elf->pElfPrograms[iLoop];
			if(pProgram->iType == PT_LOAD){
				if((pProgram->iVaddr + pProgram->iMemsz) > iMaxAddr)
					iMaxAddr = pProgram->iVaddr + pProgram->iMemsz;
				if(pProgram->iVaddr < iMinAddr)
					iMinAddr = pProgram->iVaddr;
			}
		}
		fprintf(stdout,"Min Address %08X, Max Address %08X\n", iMinAddr, iMaxAddr);
		if(iMinAddr != 0xFFFFFFFF){
			elf->iBinSize = iMaxAddr - iMinAddr;
			elf->pElfBin[elf->iBinSize];
			if(elf->pElfBin != NULL){
				memset(elf->pElfBin, 0, elf->iBinSize);
				for(iLoop = 0; iLoop < elf->iPHCount; iLoop++){
					ElfProgram* pProgram = &elf->pElfPrograms[iLoop];
					if((pProgram->iType == PT_LOAD) && (pProgram->pData != NULL)){
						fprintf(stdout,"Loading program %d 0x%08X\n", iLoop, pProgram->iType);
						memcpy(elf->pElfBin + (pProgram->iVaddr - iMinAddr), pProgram->pData, pProgram->iFilesz);
					}
				}
				elf->iBaseAddr = iMinAddr;
				blRet = 1;
			}
		}
	}

	return blRet;
}

int ElfLoadSections(ElfCProcessElf*elf){
	int found = 1;

	//assert(elf->pElf != NULL);

	if((!elf->elfHeader.iShoff) || (elf->elfHeader.iShnum <= 0) || (elf->elfHeader.iShentsize <= 0))
		return 1;
	elf->pElfSections[elf->elfHeader.iShnum];
	elf->iSHCount = elf->elfHeader.iShnum;
	memset(elf->pElfSections, 0, sizeof(ElfSection) * elf->iSHCount);
	uint8_t *pData = elf->pElf + elf->elfHeader.iShoff;

	for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++){
		if(ElfFillSection(elf, &elf->pElfSections[iLoop], (Elf32_Shdr*) pData) == 0){
			found = 0;
			break;
		}
		pData += elf->elfHeader.iShentsize;
	}

	if((elf->elfHeader.iShstrndx > 0) && (elf->elfHeader.iShstrndx < (uint32_t) elf->iSHCount)){
		if(elf->pElfSections[elf->elfHeader.iShstrndx].iType == SHT_STRTAB){
			//elf->pElfStrtab = &elf->pElfSections[elf->elfHeader.iShstrndx];
		}
	}

	if(found){
		// If we found a string table let's run through the sections fixing up names
		if(elf->pElfStrtab != NULL){
			for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++){
				strncpy(elf->pElfSections[iLoop].szName, 
						(char *) (elf->pElfStrtab->pData + elf->pElfSections[iLoop].iName), ELF_SECT_MAX_NAME - 1);
				elf->pElfSections[iLoop].szName[ELF_SECT_MAX_NAME-1] = 0;
			}
		}

		if(0){
			ElfDumpSections(elf);
		}
	}
	return 0;
}

uint32_t ElfGetBaseAddr(ElfCProcessElf*elf){
	return elf->blElfLoaded?elf->iBaseAddr:0;
}

uint32_t ElfGetTopAddr(ElfCProcessElf*elf){
	return elf->blElfLoaded?elf->iBaseAddr + elf->iBinSize:0;
}

uint32_t ElfGetLoadSize(ElfCProcessElf*elf){
	return elf->blElfLoaded?elf->iBinSize:0;
}

int ElfLoadFromFile(ElfCProcessElf*elf, const char *szFilename){
	elf->pElf = ElfLoadFileToMem(elf, szFilename, &elf->iElfSize);
	if(!elf->pElf || !ElfValidateHeader(elf) || !ElfLoadPrograms(elf) || !ElfLoadSections(elf) || !ElfLoadSymbols(elf) || !ElfBuildBinaryImage(elf))
		return 0;
	strncpy(elf->szFilename, szFilename, PATH_MAX-1);
	elf->szFilename[PATH_MAX-1] = 0;
	elf->blElfLoaded = 1;

	return 1;
}
int ElfBuildFakeSections(ElfCProcessElf*elf, unsigned int dwDataBase){
	int blRet = 0;

	if(dwDataBase >= elf->iBinSize){
		// If invalid then set to 0
		fprintf(stdout, "Invalid data base address (%d), defaulting to 0\n", dwDataBase);
		dwDataBase = 0;
	}

	elf->pElfSections[3];
	elf->iSHCount = 3;
	memset(elf->pElfSections, 0, sizeof(ElfSection) * 3);
	elf->pElfSections[1].iType = SHT_PROGBITS;
	elf->pElfSections[1].iFlags = SHF_ALLOC | SHF_EXECINSTR;
	elf->pElfSections[1].pData = elf->pElfBin;
	elf->pElfSections[1].iSize = dwDataBase>0?dwDataBase:elf->iBinSize;
	strcpy(elf->pElfSections[1].szName, ".text");
	elf->pElfSections[2].iType = SHT_PROGBITS;
	elf->pElfSections[2].iFlags = SHF_ALLOC;
	elf->pElfSections[2].iAddr = dwDataBase;
	elf->pElfSections[2].pData = elf->pElfBin + dwDataBase;
	elf->pElfSections[2].iSize = dwDataBase>0?elf->iBinSize - dwDataBase:elf->iBinSize;
	strcpy(elf->pElfSections[2].szName, ".data");
	return 1;
}
int ElfLoadFromBinFile(ElfCProcessElf*elf, const char *szFilename, unsigned int dwDataBase){
	if(!(elf->pElfBin = ElfLoadFileToMem(elf, szFilename, &elf->iBinSize)) || (!ElfBuildFakeSections(elf, dwDataBase)))
		return 0;
	strncpy(elf->szFilename, szFilename, PATH_MAX-1);
	elf->szFilename[PATH_MAX-1] = 0;
	elf->blElfLoaded = 1;
	return 1;
}

ElfSection* ElfGetSections(ElfCProcessElf*elf, uint32_t *iSHCount){
	if(elf->blElfLoaded){
		*iSHCount = elf->iSHCount;
		return elf->pElfSections;
	}
	return NULL;
}

const char *ElfGetElfName(ElfCProcessElf*elf){
	return elf->szFilename;
}