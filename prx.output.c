int elf_getSize(PrxToolCtx* prx,size_t *iTotal, size_t *iSectCount, size_t *iStrSize){
	// Sect count 2 for NULL and string section 
	*iSectCount = 2;
	*iTotal = 0;
	// 1 for NUL for NULL section 
	*iStrSize = 2 + strlen(".shstrtab"); 

	for(int i = 1; i < prx->elf.SH_count; i++){
		if(prx->elf.section[i].flags & SHF_ALLOC){
			*iSectCount++;
			*iStrSize += strlen(prx->elf.section[i].szName) + 1;
		}
	}
	*iTotal = sizeof(Elf32_Ehdr) + (sizeof(Elf32_Shdr)* *iSectCount) + *iStrSize;
	return 0;
}

int prx_outputheader(PrxToolCtx* prx,FILE *fp, size_t iSectCount){
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

int prx_outputSections(PrxToolCtx* prx,FILE *fp, size_t iElfHeadSize, size_t iSectCount, size_t iStrSize){
	// Write NULL section 
	Elf32_Shdr shdr={};
	assert(fwrite(&shdr, 1, sizeof(shdr), fp) == sizeof(shdr));

	size_t iStrPointer = 1;
	char*pStrings=calloc(iStrSize,sizeof(*pStrings));
	for(int i = 1; i < prx->elf.SH_count; i++){
		if(!(prx->elf.section[i].flags & SHF_ALLOC))
			continue;//skip non-allocatable section
		SW(shdr.sh_name, iStrPointer);
		SW(shdr.sh_type, prx->elf.section[i].type);
		SW(shdr.sh_flags, prx->elf.section[i].flags);
		SW(shdr.sh_addr, prx->elf.section[i].iAddr + prx->base);
		SW(shdr.sh_offset, ((iElfHeadSize + 15) & ~15) + (prx->elf.section[i].type == SHT_NOBITS?prx->elf.elf_count:prx->elf.section[i].iAddr));
		SW(shdr.sh_size, prx->elf.section[i].iSize);
		SW(shdr.sh_link, 0);
		SW(shdr.sh_info, 0);
		SW(shdr.sh_addralign, prx->elf.section[i].iAddralign);
		SW(shdr.sh_entsize, 0);
		assert(fwrite(&shdr, 1, sizeof(shdr), fp) == sizeof(shdr));
		strcpy(&pStrings[iStrPointer], prx->elf.section[i].szName);
		iStrPointer += strlen(prx->elf.section[i].szName) + 1;
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

int prx_toElf(PrxToolCtx* prx,FILE *fp){
	size_t iElfHeadSize = 0,iSectCount = 0,iStrSize = 0;
	assert(fp && prx->elf.elf);
	assert(!elf_getSize(prx, &iElfHeadSize, &iSectCount, &iStrSize));
	assert(!prx_outputheader(prx, fp, iSectCount))
	assert(!prx_outputSections(prx, fp, iElfHeadSize, iSectCount, iStrSize));
	//fprintf(stdout, "size: %zu, sectcount: %zu, strsize: %zu\n", iElfHeadSize, iSectCount, iStrSize);
	if(iElfHeadSize & 15)// Align data size
		assert(fwrite((char[16]){}, 1, 16 - (iElfHeadSize & 15), fp) == 16 - (iElfHeadSize & 15));
	assert(fwrite(prx->elf.bin, 1, prx->elf.elf_count, fp) == prx->elf.elf_count)

	fflush(fp);
	return 1;
}
