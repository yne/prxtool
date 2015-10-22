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

typedef struct{
	ElfHeader header;
	char      filename[PATH_MAX];
	int       blElfLoaded;
	uint32_t  baseAddr;
	uint8_t   *pElf    ;size_t iElfSize;
	uint8_t   *pElfBin ;size_t iBinSize;
	ElfSection*sections;size_t iSHCount;
	ElfProgram*programs;size_t iPHCount;
	ElfSection*strtab  ;size_t strCount;
	ElfSymbol *symbols ;size_t symbolsCount;
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


#include "endianness.c"
#include "elf.dump.c"
#include "elf.load.c"
