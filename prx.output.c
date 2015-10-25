int PrxCalcElfSize(PrxToolCtx* prx,size_t *iTotal, size_t *iSectCount, size_t *iStrSize){
	// Sect count 2 for NULL and string sections 
	*iSectCount = 2;
	*iTotal = 0;
	// 1 for NUL for NULL section 
	*iStrSize = 2 + strlen(".shstrtab"); 

	for(int i = 1; i < prx->elf.iSHCount; i++){
		if(prx->elf.sections[i].flags & SHF_ALLOC){
			*iSectCount++;
			*iStrSize += strlen(prx->elf.sections[i].szName) + 1;
		}
	}
	*iTotal = sizeof(Elf32_Ehdr) + (sizeof(Elf32_Shdr)* *iSectCount) + *iStrSize;
	return 0;
}

int PrxOutputheader(PrxToolCtx* prx,FILE *fp, size_t iSectCount){
	Elf32_Ehdr hdr={.e_class = 1,.e_data = 1,.e_idver = 1};
	SW(hdr.e_magic, ELF_MAGIC);
	SH(hdr.e_type, ELF_MIPS_TYPE);
	SH(hdr.e_machine, 8); 
	SW(hdr.e_version, 1);
	SW(hdr.e_entry, prx->base + prx->elf.header.entry); 
	SW(hdr.e_phoff, 0);
	SW(hdr.e_shoff, sizeof(Elf32_Ehdr));
	SW(hdr.e_flags, 0x10a23001);
	SH(hdr.e_ehsize, sizeof(Elf32_Ehdr));
	SH(hdr.e_phentsize, sizeof(Elf32_Phdr));
	SH(hdr.e_phnum, 0);
	SH(hdr.e_shentsize, sizeof(Elf32_Shdr));
	SH(hdr.e_shnum, iSectCount);
	SH(hdr.e_shstrndx, iSectCount-1);

	assert(fwrite(&hdr, 1, sizeof(hdr), fp) == sizeof(hdr));
	return 0;
}

int PrxOutputSections(PrxToolCtx* prx,FILE *fp, size_t iElfHeadSize, size_t iSectCount, size_t iStrSize){
	// Write NULL section 
	Elf32_Shdr shdr={};
	assert(fwrite(&shdr, 1, sizeof(shdr), fp) == sizeof(shdr));

	size_t iStrPointer = 1;
	char*pStrings=calloc(iStrSize,sizeof(*pStrings));
	for(int i = 1; i < prx->elf.iSHCount; i++){
		if(!(prx->elf.sections[i].flags & SHF_ALLOC))
			continue;//skip non-allocatable sections
		SW(shdr.sh_name, iStrPointer);
		SW(shdr.sh_type, prx->elf.sections[i].type);
		SW(shdr.sh_flags, prx->elf.sections[i].flags);
		SW(shdr.sh_addr, prx->elf.sections[i].iAddr + prx->base);
		SW(shdr.sh_offset, ((iElfHeadSize + 15) & ~15) + (prx->elf.sections[i].type == SHT_NOBITS?prx->elf.iElfSize:prx->elf.sections[i].iAddr));
		SW(shdr.sh_size, prx->elf.sections[i].iSize);
		SW(shdr.sh_link, 0);
		SW(shdr.sh_info, 0);
		SW(shdr.sh_addralign, prx->elf.sections[i].iAddralign);
		SW(shdr.sh_entsize, 0);
		assert(fwrite(&shdr, 1, sizeof(shdr), fp) == sizeof(shdr));
		strcpy(&pStrings[iStrPointer], prx->elf.sections[i].szName);
		iStrPointer += strlen(prx->elf.sections[i].szName) + 1;
	}

	// Write string section 
	SW(shdr.sh_name, iStrPointer);
	SW(shdr.sh_type, SHT_STRTAB);
	SW(shdr.sh_flags, 0);
	SW(shdr.sh_addr, 0);
	SW(shdr.sh_offset, sizeof(Elf32_Ehdr) + (sizeof(Elf32_Shdr)*iSectCount));
	SW(shdr.sh_size, iStrSize);
	SW(shdr.sh_link, 0);
	SW(shdr.sh_info, 0);
	SW(shdr.sh_addralign, 1);
	SW(shdr.sh_entsize, 0);
	assert(fwrite(&shdr, 1, sizeof(shdr), fp) == sizeof(shdr));
	strcpy(&pStrings[iStrPointer], ".shstrtab");
	assert(iStrSize == iStrPointer + strlen(".shstrtab") + 1);
	assert(fwrite(pStrings, 1, iStrSize, fp) == (unsigned) iStrSize);
	free(pStrings);
	return 0;
}

int PrxToElf(PrxToolCtx* prx,FILE *fp){
	size_t iElfHeadSize = 0,iSectCount = 0,iStrSize = 0;
	assert(fp && prx->elf.pElf);
	assert(!PrxCalcElfSize(prx, &iElfHeadSize, &iSectCount, &iStrSize));
	assert(!PrxOutputheader(prx, fp, iSectCount))
	assert(!PrxOutputSections(prx, fp, iElfHeadSize, iSectCount, iStrSize));
	//fprintf(stdout, "size: %zu, sectcount: %zu, strsize: %zu\n", iElfHeadSize, iSectCount, iStrSize);
	if(iElfHeadSize & 15)// Align data size
		assert(fwrite((char[16]){}, 1, 16 - (iElfHeadSize & 15), fp) == 16 - (iElfHeadSize & 15));
	assert(fwrite(prx->elf.pElfBin, 1, prx->elf.iElfSize, fp) == prx->elf.iElfSize)

	fflush(fp);
	return 1;
}
