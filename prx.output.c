void PrxCalcElfSize(PrxToolCtx* prx,size_t *iTotal, size_t *iSectCount, size_t *iStrSize){
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

	if(fwrite(&hdr, 1, sizeof(hdr), fp) != sizeof(hdr))
		return 0;
	return 1;
}

int PrxOutputSections(PrxToolCtx* prx,FILE *fp, size_t iElfHeadSize, size_t iSectCount, size_t iStrSize){
	Elf32_Shdr shdr;
	size_t iStrPointer = 1;
	char pStrings[iStrSize];
	memset(pStrings,0,iStrSize);
	
	size_t iBinBase = (iElfHeadSize + 15) & ~15;
	memset(&shdr, 0, sizeof(shdr));
	// Write NULL section 
	if(fwrite(&shdr, 1, sizeof(shdr), fp) != sizeof(shdr)){
		return 0;
	}

	for(int i = 1; i < prx->elf.iSHCount; i++){
		if(prx->elf.sections[i].flags & SHF_ALLOC){
			SW(shdr.sh_name, iStrPointer);
			SW(shdr.sh_type, prx->elf.sections[i].type);
			SW(shdr.sh_flags, prx->elf.sections[i].flags);
			SW(shdr.sh_addr, prx->elf.sections[i].iAddr + prx->base);
			if(prx->elf.sections[i].type == SHT_NOBITS){
				SW(shdr.sh_offset, iBinBase + prx->elf.iElfSize);
			}else{
				SW(shdr.sh_offset, iBinBase + prx->elf.sections[i].iAddr);
			}
			SW(shdr.sh_size, prx->elf.sections[i].iSize);
			SW(shdr.sh_link, 0);
			SW(shdr.sh_info, 0);
			SW(shdr.sh_addralign, prx->elf.sections[i].iAddralign);
			SW(shdr.sh_entsize, 0);
			if(fwrite(&shdr, 1, sizeof(shdr), fp) != sizeof(shdr)){
				return 0;
			}
			strcpy(&pStrings[iStrPointer], prx->elf.sections[i].szName);
			iStrPointer += strlen(prx->elf.sections[i].szName) + 1;
		}
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
	if(fwrite(&shdr, 1, sizeof(shdr), fp) != sizeof(shdr)){
		return 0;
	}

	strcpy(&pStrings[iStrPointer], ".shstrtab");
	iStrPointer += strlen(".shstrtab") + 1;

	assert(iStrSize == iStrPointer);

	if(fwrite(pStrings, 1, iStrSize, fp) != (unsigned) iStrSize)
		return 0;

	return 1;
}

int PrxToElf(PrxToolCtx* prx,FILE *fp){
	// Fixup the elf file and output it to fp 
	if((!fp) || (!prx->isPrxLoaded))
		return 0;

	size_t iElfHeadSize = 0;
	size_t iSectCount = 0;
	size_t iStrSize = 0;
	PrxCalcElfSize(prx, &iElfHeadSize, &iSectCount, &iStrSize);
	fprintf(stdout, "size: %uz, sectcount: %uz, strsize: %uz\n", iElfHeadSize, iSectCount, iStrSize);
	if(!PrxOutputheader(prx, fp, iSectCount)){
		fprintf(stdout, "Could not write ELF header\n");
		return 0;
	}

	if(!PrxOutputSections(prx, fp, iElfHeadSize, iSectCount, iStrSize))
		return fprintf(stdout, "Could not write ELF sections\n"),0;

	// Align data size 
	size_t iAlign = iElfHeadSize & 15;
	if(iAlign > 0){
		char align[16];

		iAlign = 16 - iAlign;
		memset(align, 0, sizeof(align));
		if(fwrite(align, 1, iAlign, fp) != iAlign)
			return fprintf(stdout, "Could not write alignment\n"),0;
	}

	if(fwrite(prx->elf.pElfBin, 1, prx->elf.iElfSize, fp) != prx->elf.iElfSize)
		return fprintf(stdout, "Could not write out binary image\n"),0;

	fflush(fp);
	return 1;
}
