
void elf_dumpHeader(ElfCtx*elf,FILE*stream){
	fprintf(stream,"Magic %08X\n",    elf->header.magic);
	fprintf(stream,"Class %d\n",      elf->header.iClass);
	fprintf(stream,"Data %d\n",       elf->header.iData);
	fprintf(stream,"Idver %d\n",      elf->header.iIdver);
	fprintf(stream,"Type %04X\n",     elf->header.type);
	fprintf(stream,"Start %08X\n",    elf->header.entry);
	fprintf(stream,"PH Offs %08X\n",  elf->header.PHoff);
	fprintf(stream,"SH Offs %08X\n",  elf->header.SHoff);
	fprintf(stream,"Flags %08X\n",    elf->header.flags);
	fprintf(stream,"EH Size %d\n",    elf->header.EHsize);
	fprintf(stream,"PHEntSize %d\n",  elf->header.PHentSize);
	fprintf(stream,"PHNum %d\n",      elf->header.PHnum);
	fprintf(stream,"SHEntSize %d\n",  elf->header.SHentSize);
	fprintf(stream,"SHNum %d\n",      elf->header.SHnum);
	fprintf(stream,"SHStrndx %d\n\n", elf->header.SHstrIndex);
}

void elf_dumpSymbols(ElfCtx*elf,FILE*stream){
	for(int i = 0; i < elf->symbolsCount; i++){
		fprintf(stream,"Symbol %d\n",   i);
		fprintf(stream,"Name %d, '%s'\n", elf->symbols[i].name, elf->symbols[i].symname);
		fprintf(stream,"Value %08X\n",    elf->symbols[i].value);
		fprintf(stream,"Size  %08X\n",    elf->symbols[i].size);
		fprintf(stream,"Info  %02X\n",    elf->symbols[i].info);
		fprintf(stream,"Other %02X\n",    elf->symbols[i].other);
		fprintf(stream,"Shndx %04X\n\n",  elf->symbols[i].shndx);
	}
}

void elf_dumpPrograms(ElfCtx*elf,FILE*stream){
	for(uint32_t i = 0; i < elf->iPHCount; i++){
		fprintf(stream,"Program Header %d:\n", i);
		fprintf(stream,"Type: %08X\n",    elf->programs[i].type);
		fprintf(stream,"Offset: %08X\n",  elf->programs[i].iOffset);
		fprintf(stream,"VAddr: %08X\n",   elf->programs[i].iVaddr);
		fprintf(stream,"PAddr: %08X\n",   elf->programs[i].iPaddr);
		fprintf(stream,"FileSz: %d\n",    elf->programs[i].iFilesz);
		fprintf(stream,"MemSz: %d\n",     elf->programs[i].iMemsz);
		fprintf(stream,"Flags: %08X\n",   elf->programs[i].flags);
		fprintf(stream,"Align: %08X\n\n", elf->programs[i].iAlign);
	}
}

void elf_dumpSections(ElfCtx*elf,FILE*stream){
	if(!elf->sections)return;

	for(int i = 0; i < elf->iSHCount; i++){
		fprintf(stream,"Section %d\n"     , i);
		fprintf(stream,"Name: %d %s\n"    , elf->sections[i].iName, elf->sections[i].szName);
		fprintf(stream,"Type: %08X\n"     , elf->sections[i].type);
		fprintf(stream,"Flags: %08X\n"    , elf->sections[i].flags);
		fprintf(stream,"Addr: %08X\n"     , elf->sections[i].iAddr);
		fprintf(stream,"Offset: %08X\n"   , elf->sections[i].iOffset);
		fprintf(stream,"Size: %08X\n"     , elf->sections[i].iSize);
		fprintf(stream,"Link: %08X\n"     , elf->sections[i].iLink);
		fprintf(stream,"Info: %08X\n"     , elf->sections[i].iInfo);
		fprintf(stream,"Addralign: %08X\n", elf->sections[i].iAddralign);
		fprintf(stream,"Entsize: %08X\n"  , elf->sections[i].iEntsize);
		fprintf(stream,"Data %p\n\n"      , elf->sections[i].pData);
	}
}
