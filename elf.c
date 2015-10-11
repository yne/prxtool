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
	char filename[PATH_MAX];
	ElfSection sections[64];
	size_t iSHCount;
	ElfProgram programs[16];
	size_t iPHCount;
	ElfSection strtab[64];
	ElfHeader header;
	ElfSymbol*symbols;
	size_t symbolsCount;
	uint32_t baseAddr;
}ElfCtx;


ElfSection* elf_findSection(ElfCtx*elf, const char *szName){
	if((!elf->sections) || (!elf->iSHCount) || (!elf->strtab))
		return NULL;
	if(!szName)// Return the default entry, kinda pointless :P
		return &elf->sections[0];
	for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++)
		if(!strcmp(elf->sections[iLoop].szName, szName))
			return &elf->sections[iLoop];
	return NULL;
}

ElfSection *elf_findSectionByAddr(ElfCtx*elf, unsigned int dwAddr){
	if((!elf->sections) || (!elf->iSHCount) || (!elf->strtab))
		return NULL;
	for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++){
		if(elf->sections[iLoop].flags & SHF_ALLOC){
			uint32_t sectaddr = elf->sections[iLoop].iAddr;
			uint32_t sectsize = elf->sections[iLoop].iSize;
			if((dwAddr >= sectaddr) && (dwAddr < (sectaddr + sectsize)))
				return &elf->sections[iLoop];
		}
	}
	return NULL;
}

int elf_addrIsText(ElfCtx*elf, unsigned int dwAddr){
	ElfSection *sect = elf_findSectionByAddr(elf, dwAddr);
	return sect && (sect->flags & SHF_EXECINSTR);
}

const char *elf_getSymbolName(ElfCtx*elf, uint32_t name, uint32_t shndx){
	if((shndx) && (shndx < (uint32_t) elf->iSHCount))
		if((elf->sections[shndx].type == SHT_STRTAB) && (name < elf->sections[shndx].iSize))
			return (char *) (elf->sections[shndx].pData + name);
	return "";
}

uint32_t elf_getBaseAddr(ElfCtx*elf){
	return elf->blElfLoaded?elf->baseAddr:0;
}

uint32_t elf_getTopAddr(ElfCtx*elf){
	return elf->blElfLoaded?elf->baseAddr + elf->iBinSize:0;
}

uint32_t elf_getLoadSize(ElfCtx*elf){
	return elf->blElfLoaded?elf->iBinSize:0;
}

ElfSection* elf_getSections(ElfCtx*elf, uint32_t *iSHCount){
	if(elf->blElfLoaded){
		*iSHCount = elf->iSHCount;
		return elf->sections;
	}
	return NULL;
}

const char *elf_getElfName(ElfCtx*elf){
	return elf->filename;
}

#include "elf.dump.c"
#include "elf.load.c"
