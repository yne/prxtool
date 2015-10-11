
void elf_dumpHeader(ElfCtx*elf){
	fprintf(stdout,"Magic %08X\n",    elf->header.magic);
	fprintf(stdout,"Class %d\n",      elf->header.iClass);
	fprintf(stdout,"Data %d\n",       elf->header.iData);
	fprintf(stdout,"Idver %d\n",      elf->header.iIdver);
	fprintf(stdout,"Type %04X\n",     elf->header.type);
	fprintf(stdout,"Start %08X\n",    elf->header.entry);
	fprintf(stdout,"PH Offs %08X\n",  elf->header.PHoff);
	fprintf(stdout,"SH Offs %08X\n",  elf->header.SHoff);
	fprintf(stdout,"Flags %08X\n",    elf->header.flags);
	fprintf(stdout,"EH Size %d\n",    elf->header.EHsize);
	fprintf(stdout,"PHEntSize %d\n",  elf->header.PHentSize);
	fprintf(stdout,"PHNum %d\n",      elf->header.PHnum);
	fprintf(stdout,"SHEntSize %d\n",  elf->header.SHentSize);
	fprintf(stdout,"SHNum %d\n",      elf->header.SHnum);
	fprintf(stdout,"SHStrndx %d\n\n", elf->header.SHstrIndex);
}

void elf_dumpSymbols(ElfCtx*elf){
	for(int i = 0; i < elf->symbolsCount; i++){
		fprintf(stdout,"Symbol %d\n",   i);
		fprintf(stdout,"Name %d, '%s'\n", elf->symbols[i].name, elf->symbols[i].symname);
		fprintf(stdout,"Value %08X\n",    elf->symbols[i].value);
		fprintf(stdout,"Size  %08X\n",    elf->symbols[i].size);
		fprintf(stdout,"Info  %02X\n",    elf->symbols[i].info);
		fprintf(stdout,"Other %02X\n",    elf->symbols[i].other);
		fprintf(stdout,"Shndx %04X\n\n",  elf->symbols[i].shndx);
	}
}

void elf_dumpPrograms(ElfCtx*elf){
	for(uint32_t i = 0; i < elf->iPHCount; i++){
		fprintf(stdout,"Program Header %d:\n", i);
		fprintf(stdout,"Type: %08X\n",    elf->programs[i].type);
		fprintf(stdout,"Offset: %08X\n",  elf->programs[i].iOffset);
		fprintf(stdout,"VAddr: %08X\n",   elf->programs[i].iVaddr);
		fprintf(stdout,"PAddr: %08X\n",   elf->programs[i].iPaddr);
		fprintf(stdout,"FileSz: %d\n",    elf->programs[i].iFilesz);
		fprintf(stdout,"MemSz: %d\n",     elf->programs[i].iMemsz);
		fprintf(stdout,"Flags: %08X\n",   elf->programs[i].flags);
		fprintf(stdout,"Align: %08X\n\n", elf->programs[i].iAlign);
	}
}

void elf_dumpSections(ElfCtx*elf){
	assert(elf->sections);

	for(int i = 0; i < elf->iSHCount; i++){
		fprintf(stdout,"Section %d\n"     , i);
		fprintf(stdout,"Name: %d %s\n"    , elf->sections[i].iName, elf->sections[i].szName);
		fprintf(stdout,"Type: %08X\n"     , elf->sections[i].type);
		fprintf(stdout,"Flags: %08X\n"    , elf->sections[i].flags);
		fprintf(stdout,"Addr: %08X\n"     , elf->sections[i].iAddr);
		fprintf(stdout,"Offset: %08X\n"   , elf->sections[i].iOffset);
		fprintf(stdout,"Size: %08X\n"     , elf->sections[i].iSize);
		fprintf(stdout,"Link: %08X\n"     , elf->sections[i].iLink);
		fprintf(stdout,"Info: %08X\n"     , elf->sections[i].iInfo);
		fprintf(stdout,"Addralign: %08X\n", elf->sections[i].iAddralign);
		fprintf(stdout,"Entsize: %08X\n"  , elf->sections[i].iEntsize);
		fprintf(stdout,"Data %p\n\n"      , elf->sections[i].pData);
	}
}
