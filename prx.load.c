//TODO
void PrxBuildSymbols(CProcessPrx* prx,Symbols *syms, uint32_t dwBase){
	// First map in imports and exports 
	// If we have a symbol table then no point building from imports/exports 
	if(prx->elf.symbols){
		for(int i = 0; i < prx->elf.symbolsCount; i++){
			int type = ELF32_ST_TYPE(prx->elf.symbols[i].info);
			if((type == STT_FUNC) || (type == STT_OBJECT)){
				SymbolEntry *s = NULL;//syms[prx->elf.symbols[i].value + dwBase];
				if(!s){
					syms->symbols[prx->elf.symbols[i].value + dwBase]=(SymbolEntry){
						.addr = prx->elf.symbols[i].value + dwBase,
						.type = type == STT_FUNC?SYMBOL_FUNC:SYMBOL_DATA,
						.type = SYMBOL_DATA,
						.size = prx->elf.symbols[i].size,
						//.name = prx->elf.symbols[i].symname,
					};
				}else{
					//if(strcmp(s->name, prx->elf.symbols[i].symname))
					//	s->alias.insert(s->alias.end(), prx->elf.symbols[i].symname);
				}
			}
		}
		return;
	}
	for(int exp_cnt=0;exp_cnt<prx->module.exports_count;exp_cnt++){
		PspEntries *pExport = &prx->module.exports[exp_cnt];
		if(pExport->f_count > 0){
			for(int iLoop = 0; iLoop < pExport->f_count; iLoop++){
				SymbolEntry *s = NULL;//syms.symbols[pExport->funcs[iLoop].addr + dwBase];
				if(s){
					//if(strcmp(s->name, pExport->funcs[iLoop].name))
					//		s->alias.insert(s->alias.end(), pExport->funcs[iLoop].name);
					//s->exported.insert(s->exported.end(), pExport);
				}else{
					syms->symbols[pExport->funcs[iLoop].addr + dwBase]=(SymbolEntry){
						.addr = pExport->funcs[iLoop].addr + dwBase,
						.type = SYMBOL_FUNC,
						.size = 0,
						//.name = pExport->funcs[iLoop].name,
						//.exported.insert(s->exported.end(), pExport),
					};
				}
			}
		}
		if(pExport->v_count > 0){
			for(int iLoop = 0; iLoop < pExport->v_count; iLoop++){
				SymbolEntry *s = NULL;//syms[pExport->vars[iLoop].addr + dwBase];
				if(s){
					//if(strcmp(s->name, pExport->vars[iLoop].name))
					//	s->alias.insert(s->alias.end(), pExport->vars[iLoop].name);
					//s->exported.insert(s->exported.end(), pExport);
				}else{
					syms->symbols[pExport->vars[iLoop].addr + dwBase]=(SymbolEntry){
						.addr = pExport->vars[iLoop].addr + dwBase,
						.type = SYMBOL_DATA,
						.size = 0,
						//.name = pExport->vars[iLoop].name,
						//.exported.insert(s->exported.end(), pExport),
					};
				}
			}
		}
	}
	for(int i = 0; i < prx->module.imports_count ;i++){
		PspEntries *pImport = &prx->module.imports[i];
		if(pImport->f_count){
			for(int iLoop = 0; iLoop < pImport->f_count; iLoop++){
				syms->symbols[pImport->funcs[iLoop].addr + dwBase] = (SymbolEntry){
					.addr = pImport->funcs[iLoop].addr + dwBase,
					.type = SYMBOL_FUNC,
					.size = 0,
					//.name = pImport->funcs[iLoop].name,
					//.imported.insert(s->imported.end(), pImport),
				};
			}
		}
		if(pImport->v_count){
			for(int iLoop = 0; iLoop < pImport->v_count; iLoop++){
				//imported.insert(s,s->imported.end(), pImport);
				syms->symbols[pImport->vars[iLoop].addr + dwBase] = (SymbolEntry){
					.addr = pImport->vars[iLoop].addr + dwBase,
					.type = SYMBOL_DATA,
					.size = 0,
					//.name = pImport->vars[iLoop].name,
				};
			}
		}
	}
}

int PrxFindFuncExtent(CProcessPrx*prx,uint32_t dwStart, uint8_t *pTouchMap){
	return 0;
}

void PrxMapFuncExtents(CProcessPrx*prx,Symbols*syms){
	uint8_t pTouchMap[prx->elf.iBinSize];
	memset(pTouchMap, 0, prx->elf.iBinSize);

	for(int i=0;i<syms->length;i++)
		if((syms[i].symbols->type == SYMBOL_FUNC) && (syms[i].symbols->size == 0))
			syms[i].symbols->size = PrxFindFuncExtent(prx, syms[i].symbols->addr, pTouchMap)?:syms[i].symbols->size;
}

//TODO
int PrxBuildMaps(CProcessPrx*prx){
/*
	BuildSymbols(prx->syms, prx->dwBase);

	Imms::iterator start = prx->imms.begin();
	Imms::iterator end = prx->imms.end();

	while(start != end){
		ImmEntry *imm;
		uint32_t inst;

		imm = prx->imms[(*start).first];
		inst = VmemGetU32(imm->target - prx->dwBase);
		if(imm->text){
			SymbolEntry *s;

			s = prx->syms[imm->target];
			if(s == NULL){
				s = new SymbolEntry;
				char name[128];
				// Hopefully most functions will start with a SP assignment 
				if((inst >> 16) == 0x27BD){
					snprintf(name, sizeof(name), "sub_%08X", imm->target);
					s->type = SYMBOL_FUNC;
				}else{
					snprintf(name, sizeof(name), "loc_%08X", imm->target);
					s->type = SYMBOL_LOCAL;
				}
				s->addr = imm->target;
				s->size = 0;
				s->refs.insert(s->refs.end(), imm->addr);
				s->name = name;
				prx->syms[imm->target] = s;
			}else{
				s->refs.insert(s->refs.end(), imm->addr);
			}
		}

		start++;
	}

	// Build symbols for branches in the code 
	for(int iLoop = 0; iLoop < prx->iSHCount; iLoop++){
		if(prx->sections[iLoop].flags & SHF_EXECINSTR){
			uint32_t iILoop;
			uint32_t dwAddr;
			uint32_t *pInst;
			dwAddr = prx->sections[iLoop].iAddr;
			pInst  = (uint32_t*) VmemGetPtr(dwAddr);

			for(iILoop = 0; iILoop < (prx->sections[iLoop].iSize / 4); iILoop++){
				disasmAddBranchSymbols(LW(pInst[iILoop]), dwAddr + prx->dwBase, prx->syms, prx-instr, prx-instr_count);
				dwAddr += 4;
			}
		}
	}

	if(prx->syms[prx->header.entry + prx->dwBase] == NULL){
		SymbolEntry *s;
		s = new SymbolEntry;
		// Hopefully most functions will start with a SP assignment 
		s->type = SYMBOL_FUNC;
		s->addr = prx->header.entry + prx->dwBase;
		s->size = 0;
		s->name = "_start";
		prx->syms[prx->header.entry + prx->dwBase] = s;
	}

	MapFuncExtents(prx->syms);
*/
	return 1;
}

int PrxFillModule(CProcessPrx* prx,PspModuleInfo *pData, uint32_t iAddr){
	if(!pData)
		return 0;
	prx->module=(PspModule){
		.addr=iAddr,
		.info={
			.flags   = LW((*pData).flags),
			.gp      = LW((*pData).gp),
			.exports = LW((*pData).exports),
			.exp_end = LW((*pData).exp_end),
			.imports = LW((*pData).imports),
			.imp_end = LW((*pData).imp_end),
		}
	};
	prx->stubBottom   = prx->module.info.exports - 4; // ".lib.ent.top"
	fprintf(stdout,"Stub bottom 0x%08X\n", prx->stubBottom);

	fprintf(stdout,"Module Info:\n"
		"Name: %s\nAddr: 0x%08X\nFlags: 0x%08X\nGP: 0x%08X\n"
		"Exports: 0x%08X...0x%08X\nImports: 0x%08X...0x%08X\n",
		prx->module.info.name, prx->module.addr, prx->module.info.flags, prx->module.info.gp,
		prx->module.info.exports, prx->module.info.exp_end, prx->module.info.imports, prx->module.info.imp_end);
	return 1;
}

int PrxCreateFakeSections(CProcessPrx* prx){
	// If we have no section headers let's build some fake sections 
	if(!prx->elf.iSHCount)
		return 1;
	if(prx->elf.iPHCount < 3)
		return fprintf(stderr, "Not enough program headers (%zu<3)\n", prx->elf.iPHCount),0;

	prx->elf.iSHCount = prx->elf.programs[2].type == PT_PRXRELOC?6:5;
	prx->elf.sections[0]=(ElfSection){};
	prx->elf.sections[1]=(ElfSection){
		.type = SHT_PROGBITS,
		.flags = SHF_ALLOC | SHF_EXECINSTR,
		.iAddr = prx->elf.programs[0].iVaddr,
		.pData = prx->elf.pElf + prx->elf.programs[0].iOffset,
		.iSize = prx->stubBottom,
		.szName= ".text",
	};
	prx->elf.sections[2]=(ElfSection){
	.type = SHT_PROGBITS,
	.flags = SHF_ALLOC,
	.iAddr = prx->stubBottom,
	.pData = prx->elf.pElf + prx->elf.programs[0].iOffset + prx->stubBottom,
	.iSize = prx->elf.programs[0].iMemsz - prx->stubBottom,
	.szName=".rodata",
	};
	prx->elf.sections[3]=(ElfSection){
		.type = SHT_PROGBITS,
		.flags = SHF_ALLOC | SHF_WRITE,
		.iAddr = prx->elf.programs[1].iVaddr,
		.pData = prx->elf.pElf + prx->elf.programs[1].iOffset,
		.iSize = prx->elf.programs[1].iFilesz,
		.szName= ".data",
	};
	prx->elf.sections[4]=(ElfSection){
		.type = SHT_NOBITS,
		.flags = SHF_ALLOC | SHF_WRITE,
		.iAddr = prx->elf.programs[1].iVaddr + prx->elf.programs[1].iFilesz,
		.pData = prx->elf.pElf + prx->elf.programs[1].iOffset + prx->elf.programs[1].iFilesz,
		.iSize = prx->elf.programs[1].iMemsz - prx->elf.programs[1].iFilesz,
		.szName= ".bss",
	};
	prx->elf.sections[5]=prx->elf.programs[2].type == PT_PRXRELOC?(ElfSection){
		.type = SHT_PRXRELOC,
		.flags = 0,
		.iAddr = 0,
		.pData = prx->elf.pElf + prx->elf.programs[2].iOffset,
		.iSize = prx->elf.programs[2].iFilesz,
		.iInfo = 1,// Bind to section 1, not that is matters 
		.szName= ".reloc",
	}:(ElfSection){};

	//elf_dumpSections();
	return 1;
}

int PrxLoadFromFile(CProcessPrx* prx,const char *filename){
	if(!elf_loadFromFile(&prx->elf, filename))
		return 1;
	// Do PRX specific stuff 
	uint8_t *pData = NULL;
	uint32_t iAddr = 0;

	prx->blPrxLoaded = 0;

	prx->vMem = (Vmem){prx->elf.pElfBin, prx->elf.iBinSize, prx->elf.baseAddr, MEM_LITTLE_ENDIAN};

	ElfSection *pInfoSect = elf_findSection(&prx->elf, PSP_MODULE_INFO_NAME);
	if(!pInfoSect && prx->elf.iPHCount > 0){
		// Get from program headers 
		iAddr = (prx->elf.programs[0].iPaddr & 0x7FFFFFFF) - prx->elf.programs[0].iOffset;
		pData = prx->elf.pElfBin + iAddr;
	}else{
		iAddr = pInfoSect->iAddr;
		pData = pInfoSect->pData;
	}

	if(!pData)
		return fprintf(stderr, "Could not find module section\n"),1;
	if(!PrxFillModule(prx, (PspModuleInfo *)pData, iAddr))
		return fprintf(stderr, "Could not fill module\n"),1;
	if(!PrxLoadRelocs(prx))
		return fprintf(stderr, "Could not load relocs\n"),1;
	prx->blPrxLoaded = 1;
	if(prx->pElfRelocs){
		PrxFixupRelocs(prx, prx->dwBase/*, prx->imms*/);
	}
	if(!PrxLoadExports(prx))
		return fprintf(stderr, "Could not load exports\n"),1;
	if(!PrxLoadImports(prx))
		return fprintf(stderr, "Could not load imports\n"),1;
	if(!PrxCreateFakeSections(prx))
		return fprintf(stderr, "Could not create fake sections\n"),1;
		
	fprintf(stdout, "Loaded PRX %s successfully\n", filename);
	PrxBuildMaps(prx);
	return 0;
}

int PrxLoadFromBinFile(CProcessPrx* prx,const char *filename, unsigned int dwDataBase){
	if(!elf_loadFromBinFile(&prx->elf, filename, dwDataBase))
		return 0;
	prx->blPrxLoaded = 0;

	prx->vMem = (Vmem){prx->elf.pElfBin, prx->elf.iBinSize, prx->elf.baseAddr, MEM_LITTLE_ENDIAN};

	fprintf(stdout, "Loaded BIN %s successfully\n", filename);
	prx->blPrxLoaded = 1;
	PrxBuildMaps(prx);
	return 1;
}

