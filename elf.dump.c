
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

void ElfDumpSymbols(ElfCProcessElf*elf){
	for(int i = 0; i < elf->iSymCount; i++){
		fprintf(stdout,"Symbol %d\n",   i);
		fprintf(stdout,"Name %d, '%s'\n", elf->pElfSymbols[i].name, elf->pElfSymbols[i].symname);
		fprintf(stdout,"Value %08X\n",    elf->pElfSymbols[i].value);
		fprintf(stdout,"Size  %08X\n",    elf->pElfSymbols[i].size);
		fprintf(stdout,"Info  %02X\n",    elf->pElfSymbols[i].info);
		fprintf(stdout,"Other %02X\n",    elf->pElfSymbols[i].other);
		fprintf(stdout,"Shndx %04X\n\n",  elf->pElfSymbols[i].shndx);
	}
}

void ElfDumpPrograms(ElfCProcessElf*elf){
	for(uint32_t i = 0; i < elf->iPHCount; i++){
		fprintf(stdout,"Program Header %d:\n", i);
		fprintf(stdout,"Type: %08X\n",    elf->pElfPrograms[i].iType);
		fprintf(stdout,"Offset: %08X\n",  elf->pElfPrograms[i].iOffset);
		fprintf(stdout,"VAddr: %08X\n",   elf->pElfPrograms[i].iVaddr);
		fprintf(stdout,"PAddr: %08X\n",   elf->pElfPrograms[i].iPaddr);
		fprintf(stdout,"FileSz: %d\n",    elf->pElfPrograms[i].iFilesz);
		fprintf(stdout,"MemSz: %d\n",     elf->pElfPrograms[i].iMemsz);
		fprintf(stdout,"Flags: %08X\n",   elf->pElfPrograms[i].iFlags);
		fprintf(stdout,"Align: %08X\n\n", elf->pElfPrograms[i].iAlign);
	}
}

void ElfDumpSections(ElfCProcessElf*elf){
	assert(elf->pElfSections);

	for(int i = 0; i < elf->iSHCount; i++){
		fprintf(stdout,"Section %d\n"     , i);
		fprintf(stdout,"Name: %d %s\n"    , elf->pElfSections[i].iName, elf->pElfSections[i].szName);
		fprintf(stdout,"Type: %08X\n"     , elf->pElfSections[i].iType);
		fprintf(stdout,"Flags: %08X\n"    , elf->pElfSections[i].iFlags);
		fprintf(stdout,"Addr: %08X\n"     , elf->pElfSections[i].iAddr);
		fprintf(stdout,"Offset: %08X\n"   , elf->pElfSections[i].iOffset);
		fprintf(stdout,"Size: %08X\n"     , elf->pElfSections[i].iSize);
		fprintf(stdout,"Link: %08X\n"     , elf->pElfSections[i].iLink);
		fprintf(stdout,"Info: %08X\n"     , elf->pElfSections[i].iInfo);
		fprintf(stdout,"Addralign: %08X\n", elf->pElfSections[i].iAddralign);
		fprintf(stdout,"Entsize: %08X\n"  , elf->pElfSections[i].iEntsize);
		fprintf(stdout,"Data %p\n\n"      , elf->pElfSections[i].pData);
	}
}
