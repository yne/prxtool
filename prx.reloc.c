ElfSection *elf_findSectionByAddr(ElfCtx*elf, unsigned int dwAddr){
	if((!elf->sections) || (!elf->iSHCount) || (!elf->strtab))
		return NULL;
	for(ElfSection*s = elf->sections; s < elf->sections+elf->iSHCount; s++)
		if((s->flags & SHF_ALLOC) && (dwAddr >= s->iAddr) && (dwAddr < (s->iAddr + s->iSize)))
			return s;
	return NULL;
}
int elf_addrIsText(ElfCtx*elf, unsigned int dwAddr){
	ElfSection *sect = elf_findSectionByAddr(elf, dwAddr);
	return sect && (sect->flags & SHF_EXECINSTR);
}

//TODO
int elf_fixupRelocs(ElfCtx* elf,uint32_t base, Imm* imm, size_t imm_count,Vmem*vMem){
	//uint32_t regs[32];
	if(!elf->reloc)
		return 0;
	// Fixup the elf file and output it to fp 
	assert(elf->pElf);
	assert(elf->header.PHnum && elf->header.PHentSize && elf->header.PHoff);
	// We dont support ELF reloc as they are not very special 
	assert(elf->header.type == ELF_PRX_TYPE);
	for(int iLoop = 0; iLoop < elf->reloc_count; iLoop++){
		ElfReloc *rel = &elf->reloc[iLoop];
		int iOfsPH = rel->symbol & 0xFF;
		int iValPH = (rel->symbol >> 8) & 0xFF;
		if((iOfsPH >= elf->iPHCount) || (iValPH >= elf->iPHCount)){
			fprintf(stdout,"Invalid relocation PH sets (%d, %d)\n", iOfsPH, iValPH);
			continue;
		}
		uint32_t dwRealOfs = rel->offset + elf->programs[iOfsPH].iVaddr;
		uint32_t dwCurrBase = base + elf->programs[iValPH].iVaddr;
		uint32_t *pData = (uint32_t*) VmemGetPtr(vMem, dwRealOfs);
		if(!pData){
			fprintf(stdout,"Invalid offset for relocation (%08X)\n", dwRealOfs);
			continue;
		}

		switch(elf->reloc[iLoop].type){
			case R_MIPS_HI16: {
				uint32_t inst;
				int base = iLoop;
				int lowaddr, hiaddr, addr;
				int loinst;
				int ofsph = elf->programs[iOfsPH].iVaddr;
				
				inst = LW(*pData);
				addr = ((inst & 0xFFFF) << 16) + dwCurrBase;
				fprintf(stdout,"Hi at (%08X) %d\n", dwRealOfs, iLoop);
				while (++iLoop < elf->reloc_count) {
					if (elf->reloc[iLoop].type != R_MIPS_HI16) break;
				}
				fprintf(stdout,"Matching low at %d\n", iLoop);
				if (iLoop < elf->reloc_count) {
					loinst = LW(*((uint32_t*) VmemGetPtr(vMem, elf->reloc[iLoop].offset+ofsph)));
				} else {
					loinst = 0;
				}

				addr = (int32_t) addr + (int16_t) (loinst & 0xFFFF);
				lowaddr = addr & 0xFFFF;
				hiaddr = (((addr >> 15) + 1) >> 1) & 0xFFFF;
				while (base < iLoop) {
					inst = LW(*((uint32_t*)VmemGetPtr(vMem, elf->reloc[base].offset+ofsph)));
					inst = (inst & ~0xFFFF) | hiaddr;
					SW(*((uint32_t*)VmemGetPtr(vMem, elf->reloc[base].offset+ofsph)), inst);
					base++;
				}
				while (iLoop < elf->reloc_count) {
					inst = LW(*((uint32_t*)VmemGetPtr(vMem, elf->reloc[iLoop].offset+ofsph)));
					if ((inst & 0xFFFF) != (loinst & 0xFFFF)) break;
					inst = (inst & ~0xFFFF) | lowaddr;
					SW(*((uint32_t*)VmemGetPtr(vMem, elf->reloc[iLoop].offset+ofsph)), inst);
									
					Imm*imm=NULL;
					imm->addr = base + ofsph + elf->reloc[iLoop].offset;
					imm->target = addr;
					imm->text = elf_addrIsText(elf, addr - base);
					//imm[base + ofsph + elf->reloc[iLoop].offset].imm = *imm;

					if (elf->reloc[++iLoop].type != R_MIPS_LO16) break;
				}
				iLoop--;
				fprintf(stdout,"Finished at %d\n", iLoop);
			}
			break;
			case R_MIPS_16:
			case R_MIPS_LO16: {
				uint32_t loinst = LW(*pData);
				uint32_t addr = ((int16_t) (loinst & 0xFFFF) & 0xFFFF) + dwCurrBase;
				fprintf(stdout,"Low at (%08X)\n", dwRealOfs);

				Imm*imm=NULL;
				imm->addr = dwRealOfs + base;
				imm->target = addr;
				imm->text = elf_addrIsText(elf, addr - base);
				//imm[dwRealOfs + base] = imm;

				loinst &= ~0xFFFF;
				loinst |= addr;
				SW(*pData, loinst);
			}
			break;
			case R_MIPS_X_HI16: {
				uint32_t hiinst = LW(*pData);
				uint32_t addr = (hiinst & 0xFFFF) << 16;
				addr += rel->base + dwCurrBase;
				uint32_t hiaddr = (((addr >> 15) + 1) >> 1) & 0xFFFF;
				fprintf(stdout,"Extended hi at (%08X)\n", dwRealOfs);

				Imm*imm=NULL;
				imm->addr = dwRealOfs + base;
				imm->target = addr;
				imm->text = elf_addrIsText(elf, addr - base);
				//imm[dwRealOfs + base] = imm;

				hiinst &= ~0xFFFF;
				hiinst |= (hiaddr & 0xFFFF);
				SW(*pData, hiinst);			
			}
			break;
			case R_MIPS_X_J26: {
				uint32_t off = 0;
				int base = iLoop;
				ElfReloc *rel2 = NULL;
				uint32_t offs2 = 0;
				while (++iLoop < elf->reloc_count){
					rel2 = &elf->reloc[iLoop];
					if (rel2->type == R_MIPS_X_JAL26 && (base + elf->programs[(rel2->symbol >> 8) & 0xFF].iVaddr) == dwCurrBase)
						break;
				}

				if (iLoop < elf->reloc_count) {
					offs2 = rel2->offset + elf->programs[rel2->symbol & 0xFF].iVaddr;
					off = LW(*(uint32_t*) VmemGetPtr(vMem, offs2));
				}

				uint32_t dwInst = LW(*pData);
				uint32_t dwData = dwInst + (dwCurrBase >> 16);
				SW(*pData, dwData);

				if (off & 0x8000)
				    dwInst--;

				if ((dwData >> 26) != 2){// not J instruction
					Imm*imm=NULL;
					imm->addr = dwRealOfs + base;
					imm->target = dwCurrBase + (((dwInst & 0xFFFF) << 16) | (off & 0xFFFF));
					imm->text = elf_addrIsText(elf, imm->target - base);
					//imm[dwRealOfs + base] = imm;
				}
				// already add the JAL26 symbol so we don't have to search for the J26 there
				if (iLoop < elf->reloc_count && (dwData >> 26) != 3){// not JAL instruction
					Imm*imm=NULL;
					imm->addr = offs2 + base;
					imm->target = dwCurrBase + (((dwInst & 0xFFFF) << 16) | (off & 0xFFFF));
					imm->text = elf_addrIsText(elf, imm->target - base);
					//imm[offs2 + base] = imm;
				}

				iLoop = base;
			}
			break;
			case R_MIPS_X_JAL26: {
				uint32_t dwInst = LW(*pData);
				uint32_t dwData = dwInst + (dwCurrBase & 0xFFFF);
				SW(*pData, dwData);
			}
			break;
			case R_MIPS_26: {
				uint32_t dwInst = LW(*pData);
				uint32_t dwAddr = (dwInst & 0x03FFFFFF) << 2;
				dwAddr += dwCurrBase;
				dwInst &= ~0x03FFFFFF;
				dwAddr = (dwAddr >> 2) & 0x03FFFFFF;
				dwInst |= dwAddr;
				SW(*pData, dwInst);
			}
			break;
			case R_MIPS_32: {
				uint32_t dwData = LW(*pData);
				dwData += (dwCurrBase & 0x03FFFFFF);
				dwData += (base >> 2) & 0x03FFFFFF;
				SW(*pData, dwData);

				if ((dwData >> 26) != 2){// not J instruction
					Imm*imm=NULL;
					imm->addr = dwRealOfs + base;
					imm->target = (dwData & 0x03FFFFFF) << 2;;
					imm->text = elf_addrIsText(elf, dwData - base);
					//imm[dwRealOfs + base] = imm;
				}
			}
			break;
			default: // Do nothing 
			break;
		};
	}
	return 0;
}
