// Build a binary image of the elf file in memory
// Really should build the binary image from program headers if no section headers

int elf_buildBinaryImageFromSection(ElfCtx*elf){
	uint32_t iMinAddr = ~0,iMaxAddr = 0;
	long iMaxSize = 0;
	
	for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++){
		ElfSection* pSection = &elf->sections[iLoop];
		if(pSection->flags & SHF_ALLOC){
			if((pSection->iAddr + pSection->iSize) > (iMaxAddr + iMaxSize)){
				iMaxAddr = pSection->iAddr;
				iMaxSize = pSection->iSize;
			}
			if(pSection->iAddr < iMinAddr)
				iMinAddr = pSection->iAddr;
		}
	}
	fprintf(stdout,"Min Address %08X, Max Address %08X, Max Size %ld\n", iMinAddr, iMaxAddr, iMaxSize);
	if(iMinAddr != ~0){
		elf->iBinSize = iMaxAddr - iMinAddr + iMaxSize;
		elf->pElfBin[elf->iBinSize];
		if(elf->pElfBin){
			memset(elf->pElfBin, 0, elf->iBinSize);
			for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++){
				ElfSection* pSection = &elf->sections[iLoop];
				if((pSection->flags & SHF_ALLOC) && (pSection->type != SHT_NOBITS) && (pSection->pData)){
					memcpy(elf->pElfBin + (pSection->iAddr - iMinAddr), pSection->pData, pSection->iSize);
				}
			}
			elf->baseAddr = iMinAddr;
			return 1;
		}
	}
	return 0;
}

int elf_buildBinaryImageFromProgram(ElfCtx*elf){
	uint32_t iMinAddr = ~0,iMaxAddr = 0;
	
	for(int iLoop = 0; iLoop < elf->iPHCount; iLoop++){
		ElfProgram* pProgram = &elf->programs[iLoop];
		if(pProgram->type == PT_LOAD){
			if((pProgram->iVaddr + pProgram->iMemsz) > iMaxAddr)
				iMaxAddr = pProgram->iVaddr + pProgram->iMemsz;
			if(pProgram->iVaddr < iMinAddr)
				iMinAddr = pProgram->iVaddr;
		}
	}
	fprintf(stdout,"Min Address %08X, Max Address %08X\n", iMinAddr, iMaxAddr);
	if(iMinAddr != ~0){
		elf->iBinSize = iMaxAddr - iMinAddr;
		elf->pElfBin[elf->iBinSize];
		if(elf->pElfBin != NULL){
			memset(elf->pElfBin, 0, elf->iBinSize);
			for(int iLoop = 0; iLoop < elf->iPHCount; iLoop++){
				ElfProgram* pProgram = &elf->programs[iLoop];
				if((pProgram->type == PT_LOAD) && (pProgram->pData != NULL)){
					fprintf(stdout,"Loading program %d 0x%08X\n", iLoop, pProgram->type);
					memcpy(elf->pElfBin + (pProgram->iVaddr - iMinAddr), pProgram->pData, pProgram->iFilesz);
				}
			}
			elf->baseAddr = iMinAddr;
			return 1;
		}
	}
	return 0;
}

int elf_buildBinaryImage(ElfCtx*elf){
	assert(elf->pElf);
	assert(elf->iElfSize);
	assert(!elf->pElfBin);
	assert(!elf->iBinSize);

	if (elf->header.type == ELF_MIPS_TYPE)
		return elf_buildBinaryImageFromSection(elf);
	else
		return elf_buildBinaryImageFromProgram(elf);
}

int elf_fillSection(ElfCtx*elf, ElfSection* elfSect, const Elf32_Shdr *pSection){
	assert(pSection);
	
	elfSect->iName       = LW(pSection->sh_name);
	elfSect->type       = LW(pSection->sh_type);
	elfSect->flags      = LW(pSection->sh_flags);
	elfSect->iAddr       = LW(pSection->sh_addr);
	elfSect->iOffset     = LW(pSection->sh_offset);
	elfSect->iSize       = LW(pSection->sh_size);
	elfSect->iLink       = LW(pSection->sh_link);
	elfSect->iInfo       = LW(pSection->sh_info);
	elfSect->iAddralign  = LW(pSection->sh_addralign);
	elfSect->iEntsize    = LW(pSection->sh_entsize);
	elfSect->pData       = elf->pElf + elfSect->iOffset;
	elfSect->pRelocs     = NULL;
	elfSect->iRelocCount = 0;

	if(((elfSect->pData + elfSect->iSize) > (elf->pElf + elf->iElfSize)) && (elfSect->type != SHT_NOBITS))
		return fprintf(stderr, "Section too big for file"),elfSect->pData = NULL,0;

	return 1;
}

int elf_validateHeader(ElfCtx*elf){
	assert(elf->pElf);
	assert(elf->iElfSize);
	
	Elf32_Ehdr* eh = (Elf32_Ehdr*) elf->pElf;
	elf->header=(ElfHeader){
		.magic     = LW(eh->e_magic),
		.iClass     =    eh->e_class,
		.iData      =    eh->e_data,
		.iIdver     =    eh->e_idver,
		.type      = LH(eh->e_type),
		.iMachine   = LH(eh->e_machine),
		.iVersion   = LW(eh->e_version),
		.entry     = LW(eh->e_entry),
		.PHoff     = LW(eh->e_phoff),
		.SHoff     = LW(eh->e_shoff),
		.flags     = LW(eh->e_flags),
		.EHsize    = LH(eh->e_ehsize),
		.PHentSize = LH(eh->e_phentsize),
		.PHnum     = LH(eh->e_phnum),
		.SHentSize = LH(eh->e_shentsize),
		.SHnum     = LH(eh->e_shnum),
		.SHstrIndex  = LH(eh->e_shstrndx),
	};
	if(elf->header.magic != ELF_MAGIC)
		return fprintf(stderr, "Magic value incorrect (not an ELF?)"),0;

	uint32_t iPhend = (elf->header.PHnum > 0)?elf->header.PHoff + (elf->header.PHentSize * elf->header.PHnum):0;
	uint32_t iShend = (elf->header.SHnum > 0)?elf->header.SHoff + (elf->header.SHentSize * elf->header.SHnum):0;

	if(iPhend > elf->iElfSize)
		return fprintf(stderr, "Program header information invalid"),0;
	if(iShend > elf->iElfSize)
		return fprintf(stderr, "Sections header information invalid"),0;
	//elf_dumpHeader(elf);
	return 0;
}

int elf_buildFakeSections(ElfCtx*elf, unsigned int dwDataBase){
	int blRet = 0;

	if(dwDataBase >= elf->iBinSize){
		// If invalid then set to 0
		fprintf(stdout, "Invalid data base address (%d), defaulting to 0\n", dwDataBase);
		dwDataBase = 0;
	}

	elf->sections[3];
	elf->iSHCount = 3;
	memset(elf->sections, 0, sizeof(ElfSection) * 3);
	elf->sections[1].type = SHT_PROGBITS;
	elf->sections[1].flags = SHF_ALLOC | SHF_EXECINSTR;
	elf->sections[1].pData = elf->pElfBin;
	elf->sections[1].iSize = dwDataBase>0?dwDataBase:elf->iBinSize;
	strcpy(elf->sections[1].szName, ".text");
	elf->sections[2].type = SHT_PROGBITS;
	elf->sections[2].flags = SHF_ALLOC;
	elf->sections[2].iAddr = dwDataBase;
	elf->sections[2].pData = elf->pElfBin + dwDataBase;
	elf->sections[2].iSize = dwDataBase>0?elf->iBinSize - dwDataBase:elf->iBinSize;
	strcpy(elf->sections[2].szName, ".data");
	return 1;
}

uint8_t* elf_loadFileToMem(ElfCtx*elf, const char *filename, uint32_t *lSize){
	FILE *fp = fopen(filename, "rb");
	if(!fp)
		return fprintf(stderr, "Could not open file %s\n", filename),NULL;
	fseek(fp, 0, SEEK_END);
	*lSize = ftell(fp);
	rewind(fp);
	
	if(*lSize >= sizeof(Elf32_Ehdr)){
		uint8_t pData[*lSize];
		if(fread(pData, 1, *lSize, fp) == *lSize){
			fprintf(stdout, "ELF Loaded (%d bytes)",*lSize);
		}else fprintf(stderr, "Could not read in file data");
	}else fprintf(stderr, "File not large enough to contain an ELF");
	fclose(fp);
	return NULL;//pData;
}

int elf_loadPrograms(ElfCtx*elf){
	if((!elf->header.PHoff) || (!elf->header.PHnum) || (!elf->header.PHentSize))
		return 0;
	uint8_t *pData = elf->pElf + elf->header.PHoff;
	elf->programs[elf->header.PHnum];
	if(!elf->programs)
		return 0;
	elf->iPHCount = elf->header.PHnum;
	fprintf(stdout, "Program Headers:");
	for(uint32_t iLoop = 0; iLoop < (uint32_t) elf->iPHCount; iLoop++){
		Elf32_Phdr *pHeader = (Elf32_Phdr *) pData;
		elf->programs[iLoop].type   = LW(pHeader->p_type);
		elf->programs[iLoop].iOffset = LW(pHeader->p_offset);
		elf->programs[iLoop].iVaddr  = LW(pHeader->p_vaddr);
		elf->programs[iLoop].iPaddr  = LW(pHeader->p_paddr);
		elf->programs[iLoop].iFilesz = LW(pHeader->p_filesz);
		elf->programs[iLoop].iMemsz  = LW(pHeader->p_memsz);
		elf->programs[iLoop].flags  = LW(pHeader->p_flags);
		elf->programs[iLoop].iAlign  = LW(pHeader->p_align);
		elf->programs[iLoop].pData   = elf->pElf + elf->programs[iLoop].iOffset;
		pData += elf->header.PHentSize;
	}
	elf_dumpPrograms(elf);
	return 1;
}

int elf_loadSymbols(ElfCtx*elf){
	fprintf(stdout,"Size %zu\n", sizeof(Elf32_Sym));
	ElfSection *pSymtab = elf_findSection(elf, ".symtab");
	if((!pSymtab) || (pSymtab->type != SHT_SYMTAB) || (!pSymtab->pData))
		return 0;
	uint32_t symidx = pSymtab->iLink;
	elf->symbolsCount = pSymtab->iSize / sizeof(Elf32_Sym);
	elf->symbols[elf->symbolsCount];
	
	Elf32_Sym *pSym = (Elf32_Sym*) pSymtab->pData;
	for(int iLoop = 0; iLoop < elf->symbolsCount; iLoop++){
		elf->symbols[iLoop].name    = LW(pSym[iLoop].st_name);
		elf->symbols[iLoop].symname = elf_getSymbolName(elf, elf->symbols[iLoop].name, symidx);
		elf->symbols[iLoop].value   = LW(pSym[iLoop].st_value);
		elf->symbols[iLoop].size    = LW(pSym[iLoop].st_size);
		elf->symbols[iLoop].info    = pSym[iLoop].st_info;
		elf->symbols[iLoop].other   = pSym[iLoop].st_other;
		elf->symbols[iLoop].shndx   = LH(pSym[iLoop].st_shndx);
	}
	elf_dumpSymbols(elf);
	return 0;
}

int elf_loadSections(ElfCtx*elf){
	int found = 1;

	assert(elf->pElf);

	if((!elf->header.SHoff) || (!elf->header.SHnum) || (!elf->header.SHentSize))
		return 1;
	elf->sections[elf->header.SHnum];
	elf->iSHCount = elf->header.SHnum;
	memset(elf->sections, 0, sizeof(ElfSection) * elf->iSHCount);
	uint8_t *pData = elf->pElf + elf->header.SHoff;

	for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++){
		if(!elf_fillSection(elf, &elf->sections[iLoop], (Elf32_Shdr*) pData)){
			found = 0;
			break;
		}
		pData += elf->header.SHentSize;
	}

	if((elf->header.SHstrIndex > 0) && (elf->header.SHstrIndex < (uint32_t) elf->iSHCount)){
		if(elf->sections[elf->header.SHstrIndex].type == SHT_STRTAB){
			//elf->strtab = &elf->sections[elf->header.SHstrIndex];
		}
	}

	if(found){
		// If we found a string table let's run through the sections fixing up names
		if(elf->strtab != NULL){
			for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++){
				strncpy(elf->sections[iLoop].szName, 
						(char *) (elf->strtab->pData + elf->sections[iLoop].iName), ELF_SECT_MAX_NAME - 1);
				elf->sections[iLoop].szName[ELF_SECT_MAX_NAME-1] = 0;
			}
		}
		elf_dumpSections(elf);
	}
	return 0;
}

int elf_loadFromFile(ElfCtx*elf, const char *filename){
	elf->pElf = elf_loadFileToMem(elf, filename, &elf->iElfSize);
	if(!elf->pElf || !elf_validateHeader(elf) || !elf_loadPrograms(elf) || !elf_loadSections(elf) || !elf_loadSymbols(elf) || !elf_buildBinaryImage(elf))
		return 0;
	strncpy(elf->filename, filename, PATH_MAX-1);
	elf->filename[PATH_MAX-1] = 0;
	elf->blElfLoaded = 1;

	return 1;
}

int elf_loadFromBinFile(ElfCtx*elf, const char *filename, unsigned int dwDataBase){
	if(!(elf->pElfBin = elf_loadFileToMem(elf, filename, &elf->iBinSize)) || (!elf_buildFakeSections(elf, dwDataBase)))
		return 0;
	strncpy(elf->filename, filename, PATH_MAX-1);
	elf->filename[PATH_MAX-1] = 0;
	elf->blElfLoaded = 1;
	return 1;
}
