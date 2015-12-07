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
uint32_t elf_translate(ElfCtx*elf, uint32_t vaddr){
	for (ElfProgram*p=elf->program;p<elf->program+elf->PH_count;p++) {
		if (p->type != PT_LOAD) continue;
		if ((vaddr >= p->iVaddr) && (vaddr < p->iVaddr+p->iMemsz) && (vaddr < p->iVaddr+p->iFilesz))
			return vaddr - p->iVaddr + p->iOffset;
	}
	return 0;
}
uint32_t*elf_at(ElfCtx*elf, uint32_t vaddr){
	for (ElfProgram*p=elf->program;p<elf->program+elf->PH_count;p++)
		if ((p->type == PT_LOAD) && (vaddr >= p->iVaddr) && (vaddr < p->iVaddr+p->iMemsz) && (vaddr < p->iVaddr+p->iFilesz))
			return (uint32_t*)(elf->elf+vaddr - p->iVaddr + p->iOffset);
	fprintf(stderr,"Invalid Virtual Address %08X\n",vaddr);
	exit(1);
	return NULL;
}

//#define elf_at(ELF,ADDR) (&(ELF).elf+elf_translate(&(ELF),ADDR))

#include "endianness.c"
#include "elf.load.c"
