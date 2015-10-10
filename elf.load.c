// Build a binary image of the elf file in memory
// Really should build the binary image from program headers if no section headers

int ElfBuildBinaryImageFromSection(ElfCProcessElf*elf){
	uint32_t iMinAddr = ~0,iMaxAddr = 0;
	long iMaxSize = 0;
	
	for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++){
		ElfSection* pSection = &elf->pElfSections[iLoop];
		if(pSection->iFlags & SHF_ALLOC){
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
				ElfSection* pSection = &elf->pElfSections[iLoop];
				if((pSection->iFlags & SHF_ALLOC) && (pSection->iType != SHT_NOBITS) && (pSection->pData)){
					memcpy(elf->pElfBin + (pSection->iAddr - iMinAddr), pSection->pData, pSection->iSize);
				}
			}
			elf->iBaseAddr = iMinAddr;
			return 1;
		}
	}
	return 0;
}

int ElfBuildBinaryImageFromProgram(ElfCProcessElf*elf){
	uint32_t iMinAddr = ~0,iMaxAddr = 0;
	
	for(int iLoop = 0; iLoop < elf->iPHCount; iLoop++){
		ElfProgram* pProgram = &elf->pElfPrograms[iLoop];
		if(pProgram->iType == PT_LOAD){
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
				ElfProgram* pProgram = &elf->pElfPrograms[iLoop];
				if((pProgram->iType == PT_LOAD) && (pProgram->pData != NULL)){
					fprintf(stdout,"Loading program %d 0x%08X\n", iLoop, pProgram->iType);
					memcpy(elf->pElfBin + (pProgram->iVaddr - iMinAddr), pProgram->pData, pProgram->iFilesz);
				}
			}
			elf->iBaseAddr = iMinAddr;
			return 1;
		}
	}
	return 0;
}

int ElfBuildBinaryImage(ElfCProcessElf*elf){
	assert(elf->pElf);
	assert(elf->iElfSize);
	assert(!elf->pElfBin);
	assert(!elf->iBinSize);

	if (elf->elfHeader.iType == ELF_MIPS_TYPE)
		return ElfBuildBinaryImageFromSection(elf);
	else
		return ElfBuildBinaryImageFromProgram(elf);
}

int ElfFillSection(ElfCProcessElf*elf, ElfSection* elfSect, const Elf32_Shdr *pSection){
	assert(pSection);
	
	elfSect->iName       = LW(pSection->sh_name);
	elfSect->iType       = LW(pSection->sh_type);
	elfSect->iFlags      = LW(pSection->sh_flags);
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

	if(((elfSect->pData + elfSect->iSize) > (elf->pElf + elf->iElfSize)) && (elfSect->iType != SHT_NOBITS))
		return fprintf(stderr, "Section too big for file"),elfSect->pData = NULL,0;

	return 1;
}

int ElfValidateHeader(ElfCProcessElf*elf){
	assert(elf->pElf);
	assert(elf->iElfSize);
	
	Elf32_Ehdr* eh = (Elf32_Ehdr*) elf->pElf;
	elf->elfHeader=(ElfHeader){
		.iMagic     = LW(eh->e_magic),
		.iClass     =    eh->e_class,
		.iData      =    eh->e_data,
		.iIdver     =    eh->e_idver,
		.iType      = LH(eh->e_type),
		.iMachine   = LH(eh->e_machine),
		.iVersion   = LW(eh->e_version),
		.iEntry     = LW(eh->e_entry),
		.iPhoff     = LW(eh->e_phoff),
		.iShoff     = LW(eh->e_shoff),
		.iFlags     = LW(eh->e_flags),
		.iEhsize    = LH(eh->e_ehsize),
		.iPhentsize = LH(eh->e_phentsize),
		.iPhnum     = LH(eh->e_phnum),
		.iShentsize = LH(eh->e_shentsize),
		.iShnum     = LH(eh->e_shnum),
		.iShstrndx  = LH(eh->e_shstrndx),
	};
	if(elf->elfHeader.iMagic != ELF_MAGIC)
		return fprintf(stderr, "Magic value incorrect (not an ELF?)"),0;

	uint32_t iPhend = (elf->elfHeader.iPhnum > 0)?elf->elfHeader.iPhoff + (elf->elfHeader.iPhentsize * elf->elfHeader.iPhnum):0;
	uint32_t iShend = (elf->elfHeader.iShnum > 0)?elf->elfHeader.iShoff + (elf->elfHeader.iShentsize * elf->elfHeader.iShnum):0;

	if(iPhend > elf->iElfSize)
		return fprintf(stderr, "Program header information invalid"),0;
	if(iShend > elf->iElfSize)
		return fprintf(stderr, "Sections header information invalid"),0;
	//ElfDumpHeader(elf);
	return 0;
}

int ElfBuildFakeSections(ElfCProcessElf*elf, unsigned int dwDataBase){
	int blRet = 0;

	if(dwDataBase >= elf->iBinSize){
		// If invalid then set to 0
		fprintf(stdout, "Invalid data base address (%d), defaulting to 0\n", dwDataBase);
		dwDataBase = 0;
	}

	elf->pElfSections[3];
	elf->iSHCount = 3;
	memset(elf->pElfSections, 0, sizeof(ElfSection) * 3);
	elf->pElfSections[1].iType = SHT_PROGBITS;
	elf->pElfSections[1].iFlags = SHF_ALLOC | SHF_EXECINSTR;
	elf->pElfSections[1].pData = elf->pElfBin;
	elf->pElfSections[1].iSize = dwDataBase>0?dwDataBase:elf->iBinSize;
	strcpy(elf->pElfSections[1].szName, ".text");
	elf->pElfSections[2].iType = SHT_PROGBITS;
	elf->pElfSections[2].iFlags = SHF_ALLOC;
	elf->pElfSections[2].iAddr = dwDataBase;
	elf->pElfSections[2].pData = elf->pElfBin + dwDataBase;
	elf->pElfSections[2].iSize = dwDataBase>0?elf->iBinSize - dwDataBase:elf->iBinSize;
	strcpy(elf->pElfSections[2].szName, ".data");
	return 1;
}

uint8_t* ElfLoadFileToMem(ElfCProcessElf*elf, const char *szFilename, uint32_t *lSize){
	FILE *fp = fopen(szFilename, "rb");
	if(!fp)
		return fprintf(stderr, "Could not open file %s\n", szFilename),NULL;
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

int ElfLoadPrograms(ElfCProcessElf*elf){
	if((!elf->elfHeader.iPhoff) || (!elf->elfHeader.iPhnum) || (!elf->elfHeader.iPhentsize))
		return 0;
	uint8_t *pData = elf->pElf + elf->elfHeader.iPhoff;
	elf->pElfPrograms[elf->elfHeader.iPhnum];
	if(!elf->pElfPrograms)
		return 0;
	elf->iPHCount = elf->elfHeader.iPhnum;
	fprintf(stdout, "Program Headers:");
	for(uint32_t iLoop = 0; iLoop < (uint32_t) elf->iPHCount; iLoop++){
		Elf32_Phdr *pHeader = (Elf32_Phdr *) pData;
		elf->pElfPrograms[iLoop].iType   = LW(pHeader->p_type);
		elf->pElfPrograms[iLoop].iOffset = LW(pHeader->p_offset);
		elf->pElfPrograms[iLoop].iVaddr  = LW(pHeader->p_vaddr);
		elf->pElfPrograms[iLoop].iPaddr  = LW(pHeader->p_paddr);
		elf->pElfPrograms[iLoop].iFilesz = LW(pHeader->p_filesz);
		elf->pElfPrograms[iLoop].iMemsz  = LW(pHeader->p_memsz);
		elf->pElfPrograms[iLoop].iFlags  = LW(pHeader->p_flags);
		elf->pElfPrograms[iLoop].iAlign  = LW(pHeader->p_align);
		elf->pElfPrograms[iLoop].pData   = elf->pElf + elf->pElfPrograms[iLoop].iOffset;
		pData += elf->elfHeader.iPhentsize;
	}
	ElfDumpPrograms(elf);
	return 1;
}

int ElfLoadSymbols(ElfCProcessElf*elf){
	fprintf(stdout,"Size %zu\n", sizeof(Elf32_Sym));
	ElfSection *pSymtab = ElfFindSection(elf, ".symtab");
	if((!pSymtab) || (pSymtab->iType != SHT_SYMTAB) || (!pSymtab->pData))
		return 0;
	uint32_t symidx = pSymtab->iLink;
	elf->iSymCount = pSymtab->iSize / sizeof(Elf32_Sym);
	elf->pElfSymbols[elf->iSymCount];
	
	Elf32_Sym *pSym = (Elf32_Sym*) pSymtab->pData;
	for(int iLoop = 0; iLoop < elf->iSymCount; iLoop++){
		elf->pElfSymbols[iLoop].name    = LW(pSym[iLoop].st_name);
		elf->pElfSymbols[iLoop].symname = ElfGetSymbolName(elf, elf->pElfSymbols[iLoop].name, symidx);
		elf->pElfSymbols[iLoop].value   = LW(pSym[iLoop].st_value);
		elf->pElfSymbols[iLoop].size    = LW(pSym[iLoop].st_size);
		elf->pElfSymbols[iLoop].info    = pSym[iLoop].st_info;
		elf->pElfSymbols[iLoop].other   = pSym[iLoop].st_other;
		elf->pElfSymbols[iLoop].shndx   = LH(pSym[iLoop].st_shndx);
	}
	ElfDumpSymbols(elf);
	return 0;
}

int ElfLoadSections(ElfCProcessElf*elf){
	int found = 1;

	assert(elf->pElf);

	if((!elf->elfHeader.iShoff) || (!elf->elfHeader.iShnum) || (!elf->elfHeader.iShentsize))
		return 1;
	elf->pElfSections[elf->elfHeader.iShnum];
	elf->iSHCount = elf->elfHeader.iShnum;
	memset(elf->pElfSections, 0, sizeof(ElfSection) * elf->iSHCount);
	uint8_t *pData = elf->pElf + elf->elfHeader.iShoff;

	for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++){
		if(!ElfFillSection(elf, &elf->pElfSections[iLoop], (Elf32_Shdr*) pData)){
			found = 0;
			break;
		}
		pData += elf->elfHeader.iShentsize;
	}

	if((elf->elfHeader.iShstrndx > 0) && (elf->elfHeader.iShstrndx < (uint32_t) elf->iSHCount)){
		if(elf->pElfSections[elf->elfHeader.iShstrndx].iType == SHT_STRTAB){
			//elf->pElfStrtab = &elf->pElfSections[elf->elfHeader.iShstrndx];
		}
	}

	if(found){
		// If we found a string table let's run through the sections fixing up names
		if(elf->pElfStrtab != NULL){
			for(int iLoop = 0; iLoop < elf->iSHCount; iLoop++){
				strncpy(elf->pElfSections[iLoop].szName, 
						(char *) (elf->pElfStrtab->pData + elf->pElfSections[iLoop].iName), ELF_SECT_MAX_NAME - 1);
				elf->pElfSections[iLoop].szName[ELF_SECT_MAX_NAME-1] = 0;
			}
		}
		ElfDumpSections(elf);
	}
	return 0;
}

int ElfLoadFromFile(ElfCProcessElf*elf, const char *szFilename){
	elf->pElf = ElfLoadFileToMem(elf, szFilename, &elf->iElfSize);
	if(!elf->pElf || !ElfValidateHeader(elf) || !ElfLoadPrograms(elf) || !ElfLoadSections(elf) || !ElfLoadSymbols(elf) || !ElfBuildBinaryImage(elf))
		return 0;
	strncpy(elf->szFilename, szFilename, PATH_MAX-1);
	elf->szFilename[PATH_MAX-1] = 0;
	elf->blElfLoaded = 1;

	return 1;
}

int ElfLoadFromBinFile(ElfCProcessElf*elf, const char *szFilename, unsigned int dwDataBase){
	if(!(elf->pElfBin = ElfLoadFileToMem(elf, szFilename, &elf->iBinSize)) || (!ElfBuildFakeSections(elf, dwDataBase)))
		return 0;
	strncpy(elf->szFilename, szFilename, PATH_MAX-1);
	elf->szFilename[PATH_MAX-1] = 0;
	elf->blElfLoaded = 1;
	return 1;
}
