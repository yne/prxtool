// Build a binary image of the elf file in memory
// Really should build the binary image from program headers if no section headers

int elf_loadFileToMem(ElfCtx*elf, FILE *fp){
	fseek(fp, 0, SEEK_END);
	elf->elf_count = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	assert(elf->elf_count >= sizeof(Elf32_Ehdr));
	assert(elf->elf = malloc(elf->elf_count));
	assert(fread(elf->elf, 1, elf->elf_count, fp) == elf->elf_count);
	fclose(fp);
	return 0;
}

int elf_loadHeader(ElfCtx*elf){
	assert(elf->elf);
	assert(elf->elf_count);
	
	Elf32_Ehdr* eh = (Elf32_Ehdr*) elf->elf;
	elf->header=(ElfHeader){
		.magic     = LW(eh->e_magic),
		.iClass    =    eh->e_class,
		.iData     =    eh->e_data,
		.iIdver    =    eh->e_idver,
		.type      = LH(eh->e_type),
		.iMachine  = LH(eh->e_machine),
		.iVersion  = LW(eh->e_version),
		.entry     = LW(eh->e_entry),
		.PHoff     = LW(eh->e_phoff),
		.SHoff     = LW(eh->e_shoff),
		.flags     = LW(eh->e_flags),
		.EHsize    = LH(eh->e_ehsize),
		.PHentSize = LH(eh->e_phentsize),
		.PHnum     = LH(eh->e_phnum),
		.SHentSize = LH(eh->e_shentsize),
		.SHnum     = LH(eh->e_shnum),
		.SHstrIndex= LH(eh->e_shstrndx),
	};
	assert(elf->header.magic == ELF_MAGIC);

	uint32_t iPhend = (elf->header.PHnum > 0)?elf->header.PHoff + (elf->header.PHentSize * elf->header.PHnum):0;
	uint32_t iShend = (elf->header.SHnum > 0)?elf->header.SHoff + (elf->header.SHentSize * elf->header.SHnum):0;

	assert(iPhend <= elf->elf_count);
	assert(iShend <= elf->elf_count);
	return 0;
}

int elf_loadPrograms(ElfCtx*elf){
	assert(elf->header.PHoff && elf->header.PHnum && elf->header.PHentSize);
	uint8_t *pData = elf->elf + elf->header.PHoff;
	elf->program=calloc(elf->header.PHnum,sizeof(*elf->program));
	assert(elf->program);
	elf->PH_count = elf->header.PHnum;
	for(ElfProgram*p=elf->program;p<elf->program+elf->PH_count;p++){
		Elf32_Phdr *pHeader = (Elf32_Phdr *) pData;
		p->type    = LW(pHeader->p_type);
		p->iOffset = LW(pHeader->p_offset);
		p->iVaddr  = LW(pHeader->p_vaddr);
		p->iPaddr  = LW(pHeader->p_paddr);
		p->iFilesz = LW(pHeader->p_filesz);
		p->iMemsz  = LW(pHeader->p_memsz);
		p->flags   = LW(pHeader->p_flags);
		p->iAlign  = LW(pHeader->p_align);
		p->pData   = elf->elf + p->iOffset;
		pData += elf->header.PHentSize;
	}
	return 0;
}

int elf_loadSections(ElfCtx*elf){
	assert(elf->elf);
	if(!elf->header.SHnum)
		return 0;
	assert(elf->header.SHoff && elf->header.SHentSize);
	
	elf->SH_count = elf->header.SHnum;
	elf->section = calloc(elf->SH_count,sizeof(*elf->section));

	uint8_t *pData = elf->elf + elf->header.SHoff;

	for(ElfSection*elfSect = elf->section; elfSect < elf->section+elf->SH_count; elfSect++){
		Elf32_Shdr*src = (Elf32_Shdr*)pData;
		*elfSect=(ElfSection){
			.iName       = LW(src->sh_name),
			.type        = LW(src->sh_type),
			.flags       = LW(src->sh_flags),
			.iAddr       = LW(src->sh_addr),
			.iOffset     = LW(src->sh_offset),
			.iSize       = LW(src->sh_size),
			.iLink       = LW(src->sh_link),
			.iInfo       = LW(src->sh_info),
			.iAddralign  = LW(src->sh_addralign),
			.iEntsize    = LW(src->sh_entsize),
			.pData       = elf->elf + LW(src->sh_offset),
			.pRelocs     = NULL,
			.iRelocCount = 0,
		};

		assert(((elfSect->pData + elfSect->iSize) <= (elf->elf + elf->elf_count)) || (elfSect->type == SHT_NOBITS));
		pData += elf->header.SHentSize;
	}

	if((elf->header.SHstrIndex > 0) && (elf->header.SHstrIndex < (uint32_t) elf->SH_count))
		if(elf->section[elf->header.SHstrIndex].type == SHT_STRTAB)
			elf->strtab = &elf->section[elf->header.SHstrIndex];

	if(elf->strtab)
		for(ElfSection*s=elf->section;s<elf->section+elf->SH_count; s++)
			strncpy(s->szName, (char *) (elf->strtab->pData + s->iName), sizeof(s->szName) - 1);
	return 0;
}

int elf_loadSymbols(ElfCtx*elf){
	#define VALID_NAME (pSymtab->iLink && (pSymtab->iLink < elf->SH_count)\
	                && (sym->type == SHT_STRTAB) && (s->name < sym->iSize))
	ElfSection *pSymtab = elf_findSection(elf, ".symtab");
	if(!pSymtab)
		return 0;
	assert((pSymtab->type == SHT_SYMTAB) && pSymtab->pData);
	ElfSection *sym = &elf->section[pSymtab->iLink];
	elf->symbol_count = pSymtab->iSize / sizeof(Elf32_Sym);
	elf->symbol=calloc(elf->symbol_count,sizeof(*elf->symbol));
	assert(elf->symbol);
	Elf32_Sym*pSym = (Elf32_Sym*) pSymtab->pData;
	for(ElfSymbol*s=elf->symbol;s < elf->symbol+elf->symbol_count;s++,pSym++)
		*s=(ElfSymbol){
			.name    = LW(pSym->st_name),
			.value   = LW(pSym->st_value),
			.size    = LW(pSym->st_size),
			.info    = pSym->st_info,
			.other   = pSym->st_other,
			.shndx   = LH(pSym->st_shndx),
			.symname = VALID_NAME?(const char*)(sym->pData + s->name):"",
		};
	return 0;
}

int elf_buildBinaryImageFromSection(ElfCtx*elf){
	uint32_t iMinAddr = ~0,iMaxAddr = 0, iMaxSize = 0;
	
	for(ElfSection* s = elf->section; s < elf->section+elf->SH_count;s++){
		if(s->flags & SHF_ALLOC){
			if((s->iAddr + s->iSize) > (iMaxAddr + iMaxSize)){
				iMaxAddr = s->iAddr;
				iMaxSize = s->iSize;
			}
			if(s->iAddr < iMinAddr)
				iMinAddr = s->iAddr;
		}
	}
	//fprintf(stdout,"Min Address %08X, Max Address %08X, Max Size %08X\n", iMinAddr, iMaxAddr, iMaxSize);
	assert(iMinAddr != ~0);
	elf->bin_count = iMaxAddr - iMinAddr + iMaxSize;
	elf->bin = calloc(elf->bin_count,sizeof(*elf->bin));
	for(ElfSection* s = elf->section; s < elf->section+elf->SH_count;s++)
		if((s->flags & SHF_ALLOC) && (s->type != SHT_NOBITS) && (s->pData))
			memcpy(elf->bin + (s->iAddr - iMinAddr), s->pData, s->iSize);
	elf->baseAddr = iMinAddr;
	return 0;
}

int elf_buildBinaryImageFromProgram(ElfCtx*elf){
	uint32_t iMinAddr = ~0,iMaxAddr = 0;
	
	for(ElfProgram* p = elf->program;p < elf->program+elf->PH_count; p++){
		if(p->type == PT_LOAD){
			if((p->iVaddr + p->iMemsz) > iMaxAddr)
				iMaxAddr = p->iVaddr + p->iMemsz;
			if(p->iVaddr < iMinAddr)
				iMinAddr = p->iVaddr;
		}
	}
	//fprintf(stdout,"Min Address %08X, Max Address %08X\n", iMinAddr, iMaxAddr);
	assert(iMinAddr != ~0);
	elf->bin_count = iMaxAddr - iMinAddr;
	elf->bin = calloc(elf->bin_count,sizeof(*elf->bin));
	for(ElfProgram* p = elf->program;p < elf->program+elf->PH_count; p++){
		if((p->type == PT_LOAD) && (p->pData))
			memcpy(elf->bin + (p->iVaddr - iMinAddr), p->pData, p->iFilesz);
	}
	elf->baseAddr = iMinAddr;
	return 0;
}

int elf_buildBinaryImage(ElfCtx*elf){
	assert(elf->elf && elf->elf_count && !elf->bin && !elf->bin_count);

	if (elf->header.type == ELF_MIPS_TYPE)
		return elf_buildBinaryImageFromSection(elf);
	else
		return elf_buildBinaryImageFromProgram(elf);
}

int elf_buildFakeSections(ElfCtx*elf, unsigned int dwDataBase){
	if(dwDataBase >= elf->bin_count){
		// If invalid then set to 0
		fprintf(stdout, "Invalid data base address (%d), defaulting to 0\n", dwDataBase);
		dwDataBase = 0;
	}

	elf->SH_count = 3;
	elf->section[0]=(ElfSection){};
	elf->section[1]=(ElfSection){
		.type  = SHT_PROGBITS,
		.flags = SHF_ALLOC | SHF_EXECINSTR,
		.pData = elf->bin,
		.szName= ".text",
		.iSize = dwDataBase>0?dwDataBase:elf->bin_count,
	};
	elf->section[2]=(ElfSection){
		.type  = SHT_PROGBITS,
		.flags = SHF_ALLOC,
		.iAddr = dwDataBase,
		.pData = elf->bin + dwDataBase,
		.iSize = dwDataBase>0?elf->bin_count - dwDataBase:elf->bin_count,
		.szName= ".data",
	};
	return 0;
}

int elf_loadRelocsSTD(ElfCtx*elf,ElfReloc *pRelocs){
	int count = 0;
	
	for(ElfSection*s =elf->section; s < elf->section+elf->SH_count; s++)
		if((s->type == SHT_PRXRELOC) || (s->type == SHT_REL))
			for(Elf32_Rel *r = (Elf32_Rel *) s->pData; r < (Elf32_Rel *)(s->pData+s->iSize); r++)
				if (!pRelocs)count++;
				else pRelocs[count++]=(ElfReloc){
					.secname = s->szName,
					.base = 0,
					.type = ELF32_R_TYPE(LW(r->r_info)),
					.symbol = ELF32_R_SYM(LW(r->r_info)),
					.info = LW(r->r_info),
					.offset = r->r_offset,
				};
	return count;
}

int elf_loadRelocsSCE(ElfCtx*elf,ElfReloc *pRelocs){
	uint8_t *block1, *block2, *pos, *end;
	uint32_t block1s, block2s, part1s, part2s;
	uint32_t cmd, part1, part2, lastpart2;
	uint32_t addend = 0, offset = 0;
	uint32_t ofsbase = 0xFFFFFFFF, addrbase;
	uint32_t temp1, temp2;
	uint32_t nbits;
	int iLoop, iCurrRel = 0;
	
	for(iLoop = 0; iLoop < elf->PH_count; iLoop++){
		if(elf->program[iLoop].type != PT_PRXRELOC2)
			continue;
		part1s = elf->program[iLoop].pData[2];
		part2s = elf->program[iLoop].pData[3];
		block1s =elf->program[iLoop].pData[4];
		block1 = &elf->program[iLoop].pData[4];
		block2 = block1 + block1s;
		block2s = block2[0];
		pos = block2 + block2s;
		end = &elf->program[iLoop].pData[elf->program[iLoop].iFilesz];
		
		for (nbits = 1; (1 << nbits) < iLoop; nbits++) {
			assert(nbits <= 32);
		}


		lastpart2 = block2s;
		while (pos < end) {
			cmd = pos[0] | (pos[1] << 8);
			pos += 2;
			temp1 = (cmd << (16 - part1s)) & 0xFFFF;
			temp1 = (temp1 >> (16 - part1s)) & 0xFFFF;
			assert(temp1 < block1s);
			part1 = block1[temp1];
			if ((part1 & 0x01) == 0) {
				ofsbase = (cmd << (16 - part1s - nbits)) & 0xFFFF;
				ofsbase = (ofsbase >> (16 - nbits)) & 0xFFFF;
				assert(ofsbase < iLoop);
				if ((part1 & 0x06) == 0) {
					offset = cmd >> (part1s + nbits);
				} else if ((part1 & 0x06) == 4) {
					offset = pos[0] | (pos[1] << 8) | (pos[2] << 16) | (pos[3] << 24);
					pos += 4;
				} else {
					assert(!"Invalid size\n");
				}
				continue;
			}
			temp2 = (cmd << (16 - (part1s + nbits + part2s))) & 0xFFFF;
			temp2 = (temp2 >> (16 - part2s)) & 0xFFFF;
			assert(temp2 < block2s);

			addrbase = (cmd << (16 - part1s - nbits)) & 0xFFFF;
			addrbase = (addrbase >> (16 - nbits)) & 0xFFFF;
			assert(addrbase < iLoop);
			part2 = block2[temp2];
			
			switch (part1 & 0x06) {
			case 0:
				temp1 = cmd;
				if (temp1 & 0x8000) {
					temp1 |= ~0xFFFF;
					temp1 >>= part1s + part2s + nbits;
					temp1 |= ~0xFFFF;
				} else {
					temp1 >>= part1s + part2s + nbits;
				}
				offset += temp1;
				break;
			case 2:
				temp1 = cmd;
				if (temp1 & 0x8000) temp1 |= ~0xFFFF;
				temp1 = (temp1 >> (part1s + part2s + nbits)) << 16;
				temp1 |= pos[0] | (pos[1] << 8);
				offset += temp1;
				pos += 2;
				break;
			case 4:
				offset = pos[0] | (pos[1] << 8) | (pos[2] << 16) | (pos[3] << 24);
				pos += 4;
				break;
			default:
				assert(!"invalid part1 size\n");
			}
			
			assert(offset < elf->program[ofsbase].iFilesz);
			
			switch (part1 & 0x38) {
			case 0x00:
				addend = 0;
				break;
			case 0x08:
				if ((lastpart2 ^ 0x04))
					addend = 0;
				break;
			case 0x10:
				addend = pos[0] | (pos[1] << 8);
				pos += 2;
				break;
			case 0x18:
				addend = pos[0] | (pos[1] << 8) | (pos[2] << 16) | (pos[3] << 24);
				pos += 4;
				assert(!"invalid addendum size\n");
			default:
				assert(!"invalid addendum size\n");
			}
			uint32_t part2type[]={0,R_MIPS_LO16,R_MIPS_32,R_MIPS_26,R_MIPS_X_HI16,R_MIPS_LO16,R_MIPS_X_J26,R_MIPS_X_JAL26};
			lastpart2 = part2;
			assert(part2<(sizeof(part2type)/sizeof(part2type[0])));
			if(pRelocs)pRelocs[iCurrRel]=(ElfReloc){
				.secname = NULL,
				.base   = part2==4?addend:0,
				.symbol = ofsbase | (addrbase << 8),
				.info   = (ofsbase << 8) | (addrbase << 8) | part2type[part2],
				.offset = offset,
				.type   = part2type[part2],
			};
			if(!part2)continue;
			temp1 = (cmd << (16 - part1s)) & 0xFFFF;
			temp1 = (temp1 >> (16 - part1s)) & 0xFFFF;
			temp2 = (cmd << (16 - (part1s + nbits + part2s))) & 0xFFFF;
			temp2 = (temp2 >> (16 - part2s)) & 0xFFFF;
			//fprintf(stdout,"CMD=0x%04X I1=0x%02X I2=0x%02X PART1=0x%02X PART2=0x%02X\n", cmd, temp1, temp2, part1, part2);
			iCurrRel++;
		}
	}
	return iCurrRel;
}

int elf_loadRelocs(ElfCtx*elf){
	elf->reloc_count = elf_loadRelocsSTD(elf,NULL)+elf_loadRelocsSCE(elf,NULL);
	if(!elf->reloc_count)
		return 0;
	elf->reloc = calloc(elf->reloc_count,sizeof(*elf->reloc));
	assert(elf->reloc);
	size_t cout = 0;
	cout += elf_loadRelocsSTD (elf, &elf->reloc[cout]);
	cout += elf_loadRelocsSCE (elf, &elf->reloc[cout]);
	assert(elf->reloc_count == cout);
	return 0;
}

int elf_loadFromElfFile(ElfCtx*elf, FILE *fp){
	assert(!elf_loadFileToMem(elf,fp));
	assert(!elf_loadHeader(elf));
	assert(!elf_loadPrograms(elf));
	assert(!elf_loadSections(elf));
	assert(!elf_loadSymbols(elf));
	assert(!elf_buildBinaryImage(elf));
	return 0;
}

int elf_loadFromBinFile(ElfCtx*elf, FILE *fp, unsigned int dwDataBase){
	assert(!elf_loadFileToMem(elf, fp))
	assert(!elf_buildFakeSections(elf, dwDataBase))
	return 0;
}
