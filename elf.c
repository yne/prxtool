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
	uint32_t  baseAddr;
	uint8_t   *elf    ;size_t elf_count;
	uint8_t   *bin    ;size_t bin_count;
	ElfSection*section;size_t SH_count;
	ElfProgram*program;size_t PH_count;
	ElfSection*strtab ;size_t strCount;
	ElfSymbol *symbol ;size_t symbol_count;
	ElfReloc  *reloc  ;size_t reloc_count;
}ElfCtx;


ElfSection* elf_findSection(ElfCtx*elf, const char *szName){
	if((!elf->section) || (!elf->SH_count) || (!elf->strtab))
		return NULL;
	if(!szName)// Return the default entry, kinda pointless :P
		return &elf->section[0];
	for(size_t i = 0; i < elf->SH_count; i++)
		if(!strcmp(elf->section[i].szName, szName))
			return &elf->section[i];
	return NULL;
}


#include "endianness.c"
#include "elf.dump.c"
#include "elf.load.c"
