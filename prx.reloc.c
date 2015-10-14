
int PrxCountRelocs(PrxToolCtx* prx){
	int  iRelocCount = 0;

	for(int iLoop = 0; iLoop < prx->elf.iSHCount; iLoop++){
		if((prx->elf.sections[iLoop].type == SHT_PRXRELOC) || (prx->elf.sections[iLoop].type == SHT_REL)){
			if(prx->elf.sections[iLoop].iSize % sizeof(Elf32_Rel))
				fprintf(stdout,"Invalid Relocation section #%i\n",iLoop);
			iRelocCount += prx->elf.sections[iLoop].iSize / sizeof(Elf32_Rel);
		}
	}

	for(int iLoop = 0; iLoop < prx->elf.iPHCount; iLoop++){
		if(prx->elf.programs[iLoop].type != PT_PRXRELOC2)
			continue;
		if (prx->elf.programs[iLoop].pData[0] || prx->elf.programs[iLoop].pData[1])
			return fprintf(stdout,"Should start with 0x00 0x00\n"),0;
		
		uint8_t
			part1s = prx->elf.programs[iLoop].pData[2],
			//part2s = prx->elf.programs[iLoop].pData[3],
			block1s = prx->elf.programs[iLoop].pData[4],
			*block1 = &prx->elf.programs[iLoop].pData[4],
			*block2 = block1 + block1s,
			block2s = block2[0],
			*end = &prx->elf.programs[iLoop].pData[prx->elf.programs[iLoop].iFilesz];
		for (uint8_t*pos = block2 + block2s;pos < end;iRelocCount++) {
			uint32_t cmd = pos[0] | (pos[1] << 16);
			pos += 2;
			uint32_t temp = (cmd << (16 - part1s)) & 0xFFFF;
			temp = (temp >> (16 - part1s)) & 0xFFFF;
			if (temp >= block1s)
				return fprintf(stdout,"Invalid cmd1 index\n"),0;
			uint32_t part1 = block1[temp];
			if (!(part1 & 0x01) && ( part1 & 0x06 ) == 4 )
				pos += 4;
			else {
				switch (part1 & 0x06) {
				case 2:pos += 2;break;
				case 4:pos += 4;break;
				}
				switch (part1 & 0x38) {
				case 0x10:pos += 2;break;
				case 0x18:pos += 4;break;
				}
			}
		}
	}
	fprintf(stdout,"Relocation entries %d\n", iRelocCount);
	return iRelocCount;
}

int PrxLoadRelocsTypeA(PrxToolCtx* prx,ElfReloc *pRelocs){
	int iCurrRel = 0;
	
	for(int iLoop = 0; iLoop < prx->elf.iSHCount; iLoop++){
		if((prx->elf.sections[iLoop].type == SHT_PRXRELOC) || (prx->elf.sections[iLoop].type == SHT_REL)){
			const Elf32_Rel *reloc = (const Elf32_Rel *) prx->elf.sections[iLoop].pData;
			for(int i = 0; i < prx->elf.sections[iLoop].iSize / sizeof(Elf32_Rel); i++) {    
				pRelocs[iCurrRel].secname = prx->elf.sections[iLoop].szName;
				pRelocs[iCurrRel].base = 0;
				pRelocs[iCurrRel].type = ELF32_R_TYPE(LW(reloc->r_info));
				pRelocs[iCurrRel].symbol = ELF32_R_SYM(LW(reloc->r_info));
				pRelocs[iCurrRel].info = LW(reloc->r_info);
				pRelocs[iCurrRel].offset = reloc->r_offset;
				reloc++;
				iCurrRel++;
			}
		}
	}
	return iCurrRel;
}

int PrxLoadRelocsTypeB(PrxToolCtx* prx,ElfReloc *pRelocs){
	uint8_t *block1, *block2, *pos, *end;
	uint32_t block1s, block2s, part1s, part2s;
	uint32_t cmd, part1, part2, lastpart2;
	uint32_t addend = 0, offset = 0;
	uint32_t ofsbase = 0xFFFFFFFF, addrbase;
	uint32_t temp1, temp2;
	uint32_t nbits;
	int iLoop, iCurrRel = 0;
	
	for(iLoop = 0; iLoop < prx->elf.iPHCount; iLoop++){
		if(prx->elf.programs[iLoop].type == PT_PRXRELOC2){
			part1s = prx->elf.programs[iLoop].pData[2];
			part2s = prx->elf.programs[iLoop].pData[3];
			block1s =prx->elf.programs[iLoop].pData[4];
			block1 = &prx->elf.programs[iLoop].pData[4];
			block2 = block1 + block1s;
			block2s = block2[0];
			pos = block2 + block2s;
			end = &prx->elf.programs[iLoop].pData[prx->elf.programs[iLoop].iFilesz];
			
			for (nbits = 1; (1 << nbits) < iLoop; nbits++) {
				if (nbits >= 33) {
					fprintf(stdout,"Invalid nbits\n");
					return 0;
				}
			}

	
			lastpart2 = block2s;
			while (pos < end) {
				cmd = pos[0] | (pos[1] << 8);
				pos += 2;
				temp1 = (cmd << (16 - part1s)) & 0xFFFF;
				temp1 = (temp1 >> (16 - part1s)) & 0xFFFF;
				if (temp1 >= block1s) {
					fprintf(stdout,"Invalid part1 index\n");
					return 0;
				}
				part1 = block1[temp1];
				if ((part1 & 0x01) == 0) {
					ofsbase = (cmd << (16 - part1s - nbits)) & 0xFFFF;
					ofsbase = (ofsbase >> (16 - nbits)) & 0xFFFF;
					if (!(ofsbase < iLoop)) {
						fprintf(stdout,"Invalid offset base\n");
						return 0;
					}

					if ((part1 & 0x06) == 0) {
						offset = cmd >> (part1s + nbits);
					} else if ((part1 & 0x06) == 4) {
						offset = pos[0] | (pos[1] << 8) | (pos[2] << 16) | (pos[3] << 24);
						pos += 4;
					} else {
						fprintf(stdout,"Invalid size\n");
						return 0;
					}
				} else {
					temp2 = (cmd << (16 - (part1s + nbits + part2s))) & 0xFFFF;
					temp2 = (temp2 >> (16 - part2s)) & 0xFFFF;
					if (temp2 >= block2s) {
						fprintf(stdout,"Invalid part2 index\n");
						return 0;
					}

					addrbase = (cmd << (16 - part1s - nbits)) & 0xFFFF;
					addrbase = (addrbase >> (16 - nbits)) & 0xFFFF;
					if (!(addrbase < iLoop)) {
						fprintf(stdout,"Invalid address base\n");
						return 0;
					}
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
						fprintf(stdout,"invalid part1 size\n");
						return 0;
					}
					
					if (!(offset < prx->elf.programs[ofsbase].iFilesz)) {
						fprintf(stdout,"invalid relocation offset\n");
						return 0;
					}
					
					switch (part1 & 0x38) {
					case 0x00:
						addend = 0;
						break;
					case 0x08:
						if ((lastpart2 ^ 0x04) != 0) {
							addend = 0;
						}
						break;
					case 0x10:
						addend = pos[0] | (pos[1] << 8);
						pos += 2;
						break;
					case 0x18:
						addend = pos[0] | (pos[1] << 8) | (pos[2] << 16) | (pos[3] << 24);
						pos += 4;
						fprintf(stdout,"invalid addendum size\n");
						return 0;
					default:
						fprintf(stdout,"invalid addendum size\n");
						return 0;
					}

					lastpart2 = part2;
					pRelocs[iCurrRel].secname = NULL;
					pRelocs[iCurrRel].base = 0;
					pRelocs[iCurrRel].symbol = ofsbase | (addrbase << 8);
					pRelocs[iCurrRel].info = (ofsbase << 8) | (addrbase << 8);
					pRelocs[iCurrRel].offset = offset;

					switch (part2) {
					case 2:
						pRelocs[iCurrRel].type = R_MIPS_32;
						break;
					case 0:
						continue;
					case 3:
						pRelocs[iCurrRel].type = R_MIPS_26;
						break;
					case 6:
						pRelocs[iCurrRel].type = R_MIPS_X_J26;
						break;
					case 7:
						pRelocs[iCurrRel].type = R_MIPS_X_JAL26;
						break;
					case 4:
						pRelocs[iCurrRel].type = R_MIPS_X_HI16;
						pRelocs[iCurrRel].base = (int16_t) addend;
						break;
					case 1:
					case 5:
						pRelocs[iCurrRel].type = R_MIPS_LO16;
						break;
					default:
						fprintf(stdout,"invalid relocation type\n");
						return 0;
					}
					temp1 = (cmd << (16 - part1s)) & 0xFFFF;
					temp1 = (temp1 >> (16 - part1s)) & 0xFFFF;
					temp2 = (cmd << (16 - (part1s + nbits + part2s))) & 0xFFFF;
					temp2 = (temp2 >> (16 - part2s)) & 0xFFFF;					
					fprintf(stdout,"CMD=0x%04X I1=0x%02X I2=0x%02X PART1=0x%02X PART2=0x%02X\n", cmd, temp1, temp2, part1, part2);
					pRelocs[iCurrRel].info |= pRelocs[iCurrRel].type;
					iCurrRel++;
				}
			}
		}
	}
	return iCurrRel;
}

int PrxLoadRelocs(PrxToolCtx* prx){
	int  iRelocCount = PrxCountRelocs(prx);
	int  iCurrRel = 0;

	if(iRelocCount <= 0)
		return 1;

	prx->reloc[iRelocCount];
	if(!prx->reloc)
		return 1;

	memset(prx->reloc, 0, sizeof(ElfReloc) * iRelocCount);
	
	fprintf(stdout,"Loading Type A reloc\n");
	iCurrRel += PrxLoadRelocsTypeA (prx, &prx->reloc[iCurrRel]);

	fprintf(stdout,"Loading Type B reloc\n");
	iCurrRel += PrxLoadRelocsTypeB (prx, &prx->reloc[iCurrRel]);
	prx->iRelocCount = iCurrRel;
	
	fprintf(stdout,"Dumping reloc %d\n", prx->iRelocCount);
	
	for(int iLoop = 0; iLoop < prx->iRelocCount; iLoop++){
		if(prx->reloc[iLoop].type < 16){
			fprintf(stdout,"Reloc %s:%d Type:%s Symbol:%d Offset %08X Info:%08X\n", 
					prx->reloc[iLoop].secname, iLoop, g_szRelTypes[prx->reloc[iLoop].type],
					prx->reloc[iLoop].symbol, prx->reloc[iLoop].offset, prx->reloc[iLoop].info);
		}else{
			fprintf(stdout,"Reloc %s:%d Type:%d Symbol:%d Offset %08X\n", 
					prx->reloc[iLoop].secname, iLoop, prx->reloc[iLoop].type,
					prx->reloc[iLoop].symbol, prx->reloc[iLoop].offset);
		}
	}

	return 1;
}
//TODO
int PrxFixupRelocs(PrxToolCtx* prx,uint32_t base, Imm* imms, size_t imms_count){
	//uint32_t regs[32];
	// Fixup the elf file and output it to fp 
	if((prx->isPrxLoaded == 0))
		return 1;
	if((prx->elf.header.PHnum < 1) || (prx->elf.header.PHentSize == 0) || (prx->elf.header.PHoff == 0))
		return 1;
	// We dont support ELF reloc as they are not very special 
	if(prx->elf.header.type != ELF_PRX_TYPE)
		return 1;
	for(int iLoop = 0; iLoop < prx->iRelocCount; iLoop++){
		ElfReloc *rel = &prx->reloc[iLoop];
		uint32_t dwRealOfs;
		uint32_t dwCurrBase;
		int iOfsPH;
		int iValPH;

		iOfsPH = rel->symbol & 0xFF;
		iValPH = (rel->symbol >> 8) & 0xFF;
		if((iOfsPH >= prx->elf.iPHCount) || (iValPH >= prx->elf.iPHCount)){
			fprintf(stdout,"Invalid relocation PH sets (%d, %d)\n", iOfsPH, iValPH);
			continue;
		}
		dwRealOfs = rel->offset + prx->elf.programs[iOfsPH].iVaddr;
		dwCurrBase = base + prx->elf.programs[iValPH].iVaddr;
		uint32_t *pData = (uint32_t*) VmemGetPtr(&prx->vMem, dwRealOfs);
		if(pData == NULL){
			fprintf(stdout,"Invalid offset for relocation (%08X)\n", dwRealOfs);
			continue;
		}

		switch(prx->reloc[iLoop].type){
			case R_MIPS_HI16: {
				uint32_t inst;
				int base = iLoop;
				int lowaddr, hiaddr, addr;
				int loinst;
				int ofsph = prx->elf.programs[iOfsPH].iVaddr;
				
				inst = LW(*pData);
				addr = ((inst & 0xFFFF) << 16) + dwCurrBase;
				fprintf(stdout,"Hi at (%08X) %d\n", dwRealOfs, iLoop);
				while (++iLoop < prx->iRelocCount) {
					if (prx->reloc[iLoop].type != R_MIPS_HI16) break;
				}
				fprintf(stdout,"Matching low at %d\n", iLoop);
				if (iLoop < prx->iRelocCount) {
					loinst = LW(*((uint32_t*) VmemGetPtr(&prx->vMem, prx->reloc[iLoop].offset+ofsph)));
				} else {
					loinst = 0;
				}

				addr = (int32_t) addr + (int16_t) (loinst & 0xFFFF);
				lowaddr = addr & 0xFFFF;
				hiaddr = (((addr >> 15) + 1) >> 1) & 0xFFFF;
				while (base < iLoop) {
					inst = LW(*((uint32_t*)VmemGetPtr(&prx->vMem, prx->reloc[base].offset+ofsph)));
					inst = (inst & ~0xFFFF) | hiaddr;
					SW(*((uint32_t*)VmemGetPtr(&prx->vMem, prx->reloc[base].offset+ofsph)), inst);
					base++;
				}
				while (iLoop < prx->iRelocCount) {
					inst = LW(*((uint32_t*)VmemGetPtr(&prx->vMem, prx->reloc[iLoop].offset+ofsph)));
					if ((inst & 0xFFFF) != (loinst & 0xFFFF)) break;
					inst = (inst & ~0xFFFF) | lowaddr;
					SW(*((uint32_t*)VmemGetPtr(&prx->vMem, prx->reloc[iLoop].offset+ofsph)), inst);
									
					Imm*imm=NULL;
					imm->addr = base + ofsph + prx->reloc[iLoop].offset;
					imm->target = addr;
					imm->text = elf_addrIsText(&prx->elf, addr - base);
					//imms[base + ofsph + prx->reloc[iLoop].offset].imms = *imm;

					if (prx->reloc[++iLoop].type != R_MIPS_LO16) break;
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
				imm->text = elf_addrIsText(&prx->elf, addr - base);
				//imms[dwRealOfs + base] = imm;

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
				imm->text = elf_addrIsText(&prx->elf, addr - base);
				//imms[dwRealOfs + base] = imm;

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
				while (++iLoop < prx->iRelocCount){
					rel2 = &prx->reloc[iLoop];
					if (rel2->type == R_MIPS_X_JAL26 && (base + prx->elf.programs[(rel2->symbol >> 8) & 0xFF].iVaddr) == dwCurrBase)
						break;
				}

				if (iLoop < prx->iRelocCount) {
					offs2 = rel2->offset + prx->elf.programs[rel2->symbol & 0xFF].iVaddr;
					off = LW(*(uint32_t*) VmemGetPtr(&prx->vMem, offs2));
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
					imm->text = elf_addrIsText(&prx->elf, imm->target - base);
					//imms[dwRealOfs + base] = imm;
				}
				// already add the JAL26 symbol so we don't have to search for the J26 there
				if (iLoop < prx->iRelocCount && (dwData >> 26) != 3){// not JAL instruction
					Imm*imm=NULL;
					imm->addr = offs2 + base;
					imm->target = dwCurrBase + (((dwInst & 0xFFFF) << 16) | (off & 0xFFFF));
					imm->text = elf_addrIsText(&prx->elf, imm->target - base);
					//imms[offs2 + base] = imm;
				}

				iLoop = base;
			}
			break;
			case R_MIPS_X_JAL26: {
				uint32_t dwData, dwInst;
				dwInst = LW(*pData);
				dwData = dwInst + (dwCurrBase & 0xFFFF);
				SW(*pData, dwData);
			}
			break;
			case R_MIPS_26: {
				uint32_t dwAddr;
				uint32_t dwInst;

				dwInst = LW(*pData);
				dwAddr = (dwInst & 0x03FFFFFF) << 2;
				dwAddr += dwCurrBase;
				dwInst &= ~0x03FFFFFF;
				dwAddr = (dwAddr >> 2) & 0x03FFFFFF;
				dwInst |= dwAddr;
				SW(*pData, dwInst);
			}
			break;
			case R_MIPS_32: {
				uint32_t dwData;

				dwData = LW(*pData);
				dwData += (dwCurrBase & 0x03FFFFFF);
				dwData += (base >> 2) & 0x03FFFFFF;
				SW(*pData, dwData);

				if ((dwData >> 26) != 2){// not J instruction
					Imm*imm=NULL;
					imm->addr = dwRealOfs + base;
					imm->target = (dwData & 0x03FFFFFF) << 2;;
					imm->text = elf_addrIsText(&prx->elf, dwData - base);
					//imms[dwRealOfs + base] = imm;
				}
			}
			break;
			default: // Do nothing 
			break;
		};
	}
	return 0;
}
