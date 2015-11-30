void elf_dumpHeader(ElfCtx*elf, FILE*stream){
	ElfHeader*h = &elf->header;
	fprintf(stream,"\n[ELF]\n");
	fprintf(stream,"Type:%04X (%s), Start:0x%X Flags:%08X\n",h->type,h->type==ELF_MIPS_TYPE?"ELF":"PRX",h->entry,h->flags);
	fprintf(stream,"Programs: %2d*%dB at %04X\n",      h->PHnum,h->PHentSize,h->PHoff);
	fprintf(stream,"Sections: %2d*%dB at %04X (.strtab at %d)\n",h->SHnum,h->SHentSize,h->SHoff,h->SHstrIndex);
}

void elf_dumpPrograms(ElfCtx*elf,FILE*stream){
	char*type2str[]={"NULL","LOAD","DYNAMIC","INTERP","NOTE","SHLIB","PHDR"};

	if(!elf->PH_count)
		return;
	fprintf(stream,"\n[Programs]\nType    Offset VirtAddr PhysAddr FileSz MemSz  Flg Al\n");
	for(ElfProgram*p = elf->program; p < elf->program+elf->PH_count; p++)
		fprintf(stream,"%-7s %06X %08X %08X %06X %06X %03X %2X\n",
			p->type<=6?type2str[p->type]:p->type==PT_PRXRELOC?"PRXRELOC":p->type==PT_PRXRELOC2?"PRXRELOC2":"?",
			p->iOffset,p->iVaddr,p->iPaddr,p->iFilesz,p->iMemsz,p->flags,p->iAlign);
}

void elf_dumpSections(ElfCtx*elf,FILE*stream){
	char*type2str[]={"NULL","PROGBIT","SYMTAB","STRTAB","RELA","HASH","DYNAMIC","NOTE","NOBITS","REL","SHLIB","DYNSYM"};

	if(!elf->SH_count)
		return;
	fprintf(stream,"\n[Sections]\nType     Addr     Off    Size  ES Flg Lk Inf Al Name\n");
	for(ElfSection*s = elf->section; s < elf->section + elf->SH_count;s++)
		fprintf(stream,"%-7s %08X %06X %06X %02X %3X %2X %3X %2d %s\n",
			s->type<=11?type2str[s->type]:s->type==SHT_PRXRELOC?"PRXREL1":"?",
			s->iAddr,s->iOffset,s->iSize,s->iEntsize,s->flags,s->iLink,s->iInfo,s->iAddralign,s->szName);
}

void elf_dumpSymbols(ElfCtx*elf,FILE*stream){
	if(!elf->symbol_count)
		return;
	fprintf(stream,"\n[Symbols]\nName Value Size Info Other Shndx\n");
	for(ElfSymbol*s=elf->symbol;s < elf->symbol+elf->symbol_count; s++)
		fprintf(stream," %d '%s' %08X %08X %02X %02X %04X\n",
			s->name, s->symname,s->value,s->size,s->info,s->other,s->shndx);
}

void elf_dumpRelocs(ElfCtx* elf,FILE*stream){
	char* g_szRelTypes[16] = {
		"NONE","16","32","REL32","26","HI16","LO16","GPREL16","LITERAL",
		"GOT16","PC16","CALL16","GPREL32","HI16","J26","JAL26"
	};
	if(!elf->reloc_count)
		return;
	fprintf(stream,"\n[Relocations]\nType      Symbol Offset Info Sections\n", elf->reloc_count);
	for(ElfReloc*rel = elf->reloc; rel < elf->reloc+elf->reloc_count; rel++)
		fprintf(stream,"%d:%-7s %6d %06X %04X %s\n", rel->type,
			rel->type<16?g_szRelTypes[rel->type]:"?", rel->symbol,rel->offset, rel->info, rel->secname);
}

void elf_dump(ElfCtx* elf,FILE*stream){
	elf_dumpHeader(elf,stream);
	elf_dumpPrograms(elf,stream);
	elf_dumpSections(elf,stream);
	elf_dumpRelocs(elf,stream);
	elf_dumpSymbols(elf,stream);
}
