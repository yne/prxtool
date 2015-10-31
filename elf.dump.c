
void elf_dumpHeader(ElfCtx*elf, FILE*stream){
	ElfHeader*h = &elf->header;
	fprintf(stream,"\nELF\n");
	fprintf(stream,"Type: %04X, Start 0x%X,Flags %08X\n",h->type,h->entry,h->flags);
	fprintf(stream,"Programs: %2d*%dB at %X\n",      h->PHnum,h->PHentSize,h->PHoff);
	fprintf(stream,"Sections: %2d*%dB at %X (str at %d)\n",h->SHnum,h->SHentSize,h->SHoff,h->SHstrIndex);
}

void elf_dumpSymbols(ElfCtx*elf,FILE*stream){
	fprintf(stream,"\nSymbols:\nName Value Size Info Other Shndx");
	if(!elf->symbol_count)
		return (void)fprintf(stream,"There is no symbol in this file.");
	for(ElfSymbol*s=elf->symbol;s < elf->symbol+elf->symbol_count; s++)
		fprintf(stream," %d '%s' %08X %08X %02X %02X %04X\n",
			s->name, s->symname,s->value,s->size,s->info,s->other,s->shndx);
}

void elf_dumpPrograms(ElfCtx*elf,FILE*stream){
	char*type2str[]={"NULL","LOAD","DYNAMIC","INTERP","NOTE","SHLIB","PHDR"};

	if(!elf->PH_count)
		return (void)fprintf(stream,"There is no program in this file.");
	fprintf(stream,"\nPrograms:\nType    Offset VirtAddr PhysAddr FileSz MemSz  Flg Al\n");
	for(ElfProgram*p = elf->program; p < elf->program+elf->PH_count; p++)
		fprintf(stream,"%-7s %06X %08X %08X %06X %06X %03X %2X\n",
			p->type<=6?type2str[p->type]:p->type==PT_PRXRELOC?"PRXRELOC":p->type==PT_PRXRELOC2?"PRXRELOC2":"?",
			p->iOffset,p->iVaddr,p->iPaddr,p->iFilesz,p->iMemsz,p->flags,p->iAlign);
}

void elf_dumpSections(ElfCtx*elf,FILE*stream){
	char*type2str[]={"NULL","PROGBIT","SYMTAB","STRTAB","RELA","HASH","DYNAMIC","NOTE","NOBITS","REL","SHLIB","DYNSYM"};

	if(!elf->SH_count)
		return (void)fprintf(stream,"There is no section in this file.");
	fprintf(stream,"\nSections:\nType     Addr     Off    Size  ES Flg Lk Inf Al Name\n");
	for(ElfSection*s = elf->section; s < elf->section + elf->SH_count;s++)
		fprintf(stream,"%-7s %08X %06X %06X %02X %3X %2X %3X %2d %s\n",
			s->type<=11?type2str[s->type]:s->type==SHT_PRXRELOC?"PRXREL1":"?",
			s->iAddr,s->iOffset,s->iSize,s->iEntsize,s->flags,s->iLink,s->iInfo,s->iAddralign,s->szName);
}

int elf_dumpRelocs(ElfCtx* elf){
	char* g_szRelTypes[16] = {
		"R_NONE","R_16","R_32","R_REL32","R_26","R_HI16","R_LO16","R_GPREL16","R_LITERAL",
		"R_GOT16","R_PC16","R_CALL16","R_GPREL32","X_HI16","X_J26","X_JAL26"
	};
	fprintf(stdout,"Dumping reloc %zu\n", elf->reloc_count);
	for(ElfReloc*rel = elf->reloc; rel < elf->reloc+elf->reloc_count; rel++)
		fprintf(stdout,"Reloc %s Type:%d(%s) Symbol:%d Offset %08X Info:%08X\n", rel->secname, rel->type,
			rel->type<16?g_szRelTypes[rel->type]:"?", rel->symbol, rel->offset, rel->info);
	return 0;
}
