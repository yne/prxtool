/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * ProcessElf.C - Implementation of a class to manipulate a ELF
 ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "elf.h"
#include "endianness.c"

typedef struct{
	uint8_t *pElf;//original elf from file
	size_t iElfSize;
	uint8_t *pElfBin;//binary image of the elf
	size_t iBinSize;
	int blElfLoaded;
	char szFilename[PATH_MAX];
	ElfSection pElfSections[64];
	size_t iSHCount;
	ElfProgram pElfPrograms[16];
	size_t iPHCount;
	ElfSection pElfStrtab[64];
	ElfHeader elfHeader;
	ElfSymbol*pElfSymbols;
	size_t iSymCount;
	uint32_t iBaseAddr;
}ElfCProcessElf;


ElfSection* ElfFindSection(ElfCProcessElf*elf, const char *szName){
	if((!elf->pElfSections) || (!elf->iSHCount) || (!elf->pElfStrtab))
		return NULL;
	if(!szName)// Return the default entry, kinda pointless :P
		return &elf->pElfSections[0];
	for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++)
		if(!strcmp(elf->pElfSections[iLoop].szName, szName))
			return &elf->pElfSections[iLoop];
	return NULL;
}

ElfSection *ElfFindSectionByAddr(ElfCProcessElf*elf, unsigned int dwAddr){
	if((!elf->pElfSections) || (!elf->iSHCount) || (!elf->pElfStrtab))
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
	return sect && (sect->iFlags & SHF_EXECINSTR);
}

const char *ElfGetSymbolName(ElfCProcessElf*elf, uint32_t name, uint32_t shndx){
	if((shndx) && (shndx < (uint32_t) elf->iSHCount))
		if((elf->pElfSections[shndx].iType == SHT_STRTAB) && (name < elf->pElfSections[shndx].iSize))
			return (char *) (elf->pElfSections[shndx].pData + name);
	return "";
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

#include "elf.dump.c"
#include "elf.load.c"
