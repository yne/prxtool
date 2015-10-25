//TODO
void PrxBuildSymbols(PrxToolCtx* prx,Symbol *symbol,size_t *syms_count, uint32_t base){
	// First map in imports and exports 
	// If we have a symbol table then no point building from imports/exports 
	if(prx->elf.symbols){
		for(int i = 0; i < prx->elf.symbolsCount; i++){
			int type = ELF32_ST_TYPE(prx->elf.symbols[i].info);
			if((type == STT_FUNC) || (type == STT_OBJECT)){
				Symbol *s = NULL;//symbol[prx->elf.symbols[i].value + base];
				if(!s){
					symbol[prx->elf.symbols[i].value + base]=(Symbol){
						.addr = prx->elf.symbols[i].value + base,
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
				Symbol *s = NULL;//symbol.symbols[pExport->funcs[iLoop].addr + base];
				if(s){
					//if(strcmp(s->name, pExport->funcs[iLoop].name))
					//		s->alias.insert(s->alias.end(), pExport->funcs[iLoop].name);
					//s->exported.insert(s->exported.end(), pExport);
				}else{
					symbol[pExport->funcs[iLoop].addr + base]=(Symbol){
						.addr = pExport->funcs[iLoop].addr + base,
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
				Symbol *s = NULL;//symbol[pExport->vars[iLoop].addr + base];
				if(s){
					//if(strcmp(s->name, pExport->vars[iLoop].name))
					//	s->alias.insert(s->alias.end(), pExport->vars[iLoop].name);
					//s->exported.insert(s->exported.end(), pExport);
				}else{
					symbol[pExport->vars[iLoop].addr + base]=(Symbol){
						.addr = pExport->vars[iLoop].addr + base,
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
				symbol[pImport->funcs[iLoop].addr + base] = (Symbol){
					.addr = pImport->funcs[iLoop].addr + base,
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
				symbol[pImport->vars[iLoop].addr + base] = (Symbol){
					.addr = pImport->vars[iLoop].addr + base,
					.type = SYMBOL_DATA,
					.size = 0,
					//.name = pImport->vars[iLoop].name,
				};
			}
		}
	}
}

int PrxFindFuncExtent(PrxToolCtx*prx,uint32_t dwStart, uint8_t *pTouchMap){
	return 0;
}

void PrxMapFuncExtents(PrxToolCtx*prx,Symbol*symbol,size_t syms_count){
	uint8_t*pTouchMap=calloc(prx->elf.iBinSize,sizeof(*pTouchMap));

	for(int i=0;i<syms_count;i++)
		if((symbol[i].type == SYMBOL_FUNC) && (symbol[i].size == 0))
			symbol[i].size = PrxFindFuncExtent(prx, symbol[i].addr, pTouchMap)?:symbol[i].size;
}

//TODO
int PrxBuildMaps(PrxToolCtx*prx){
/*
	BuildSymbols(prx->symbol, prx->base);

	Imms::iterator start = prx->imm.begin();
	Imms::iterator end = prx->imm.end();

	while(start != end){
		Imm *imm;
		uint32_t inst;

		imm = prx->imm[(*start).first];
		inst = VmemGetU32(imm->target - prx->base);
		if(imm->text){
			Symbol *s;

			s = prx->symbol[imm->target];
			if(s == NULL){
				s = new Symbol;
				char name[128];
				// Hopefully most proto will start with a SP assignment 
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
				prx->symbol[imm->target] = s;
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
				disasmAddBranchSymbols(LW(pInst[iILoop]), dwAddr + prx->base, prx->symbol, prx-instr, prx-instr_count);
				dwAddr += 4;
			}
		}
	}

	if(prx->symbol[prx->header.entry + prx->base] == NULL){
		Symbol *s;
		s = new Symbol;
		// Hopefully most proto will start with a SP assignment 
		s->type = SYMBOL_FUNC;
		s->addr = prx->header.entry + prx->base;
		s->size = 0;
		s->name = "_start";
		prx->symbol[prx->header.entry + prx->base] = s;
	}

	MapFuncExtents(prx->symbol);
*/
	return 1;
}

int PrxFillModule(ElfCtx* elf,PspModule*mod,char*secname){
	PspModuleInfo*modinfo = NULL;
	uint32_t iAddr = 0;
	ElfSection *pInfoSect = elf_findSection(elf, secname);
	if(!pInfoSect && elf->iPHCount > 0){// Get from program headers 
		iAddr = (elf->programs[0].iPaddr & 0x7FFFFFFF) - elf->programs[0].iOffset;
		modinfo = (PspModuleInfo*)elf->pElfBin + iAddr;
	}else{
		iAddr = pInfoSect->iAddr;
		modinfo = (PspModuleInfo*)pInfoSect->pData;
	}
	assert(modinfo);
	*mod=(PspModule){
		.addr=iAddr,
		.info={
			.flags   = LW(modinfo->flags),
			.gp      = LW(modinfo->gp),
			.exports = LW(modinfo->exports),
			.exp_end = LW(modinfo->exp_end),
			.imports = LW(modinfo->imports),
			.imp_end = LW(modinfo->imp_end),
		}
	};
	memcpy(mod->info.name, ((PspModuleInfo*)modinfo)->name, sizeof(mod->info.name));
	return 0;
}

int PrxCreateFakeSections(ElfCtx*elf,ElfProgram*p,uint32_t stubBtm){
	// If we have no section headers let's build some fake sections 
	if(!elf->iSHCount)
		return 1;
	assert(elf->iPHCount >= 3)

	elf->iSHCount = (p[2].type == PT_PRXRELOC)?6:5;
	
	ElfSection fake_sections[]={{},{
		.type  = SHT_PROGBITS,
		.flags = SHF_ALLOC | SHF_EXECINSTR,
		.iAddr = p[0].iVaddr,
		.pData = elf->pElf + p[0].iOffset,
		.iSize = stubBtm,
		.szName= ".text",
	},{
		.type  = SHT_PROGBITS,
		.flags = SHF_ALLOC,
		.iAddr = stubBtm,
		.pData = elf->pElf + p[0].iOffset + stubBtm,
		.iSize = p[0].iMemsz - stubBtm,
		.szName=".rodata",
	},{
		.type  = SHT_PROGBITS,
		.flags = SHF_ALLOC | SHF_WRITE,
		.iAddr = p[1].iVaddr,
		.pData = elf->pElf + p[1].iOffset,
		.iSize = p[1].iFilesz,
		.szName= ".data",
	},{
		.type  = SHT_NOBITS,
		.flags = SHF_ALLOC | SHF_WRITE,
		.iAddr = p[1].iVaddr + p[1].iFilesz,
		.pData = elf->pElf + p[1].iOffset + p[1].iFilesz,
		.iSize = p[1].iMemsz - p[1].iFilesz,
		.szName= ".bss",
	},{
		.type  = SHT_PRXRELOC,
		.flags = 0,
		.iAddr = 0,
		.pData = elf->pElf + p[2].iOffset,
		.iSize = p[2].iFilesz,
		.iInfo = 1,// Bind to section 1, not that is matters 
		.szName= ".reloc",
	}};
	memcpy(elf->sections,fake_sections,elf->iSHCount*sizeof(*elf->sections));
	return 1;
}

int PrxLoadFromElf(PrxToolCtx* prx,FILE *fp){
	assert(!elf_loadFromElfFile(&prx->elf, fp));
	prx->vMem = (Vmem){prx->elf.pElfBin, prx->elf.iBinSize, prx->elf.baseAddr, MEM_LITTLE_ENDIAN};
	assert(!elf_loadModInfo(&prx->elf,&prx->module,prx->arg.modInfoName));
	assert(!elf_loadRelocs(&prx->elf));
	assert(!elf_fixupRelocs(&prx->elf, prx->base, prx->imm,prx->imm_count,&prx->vMem));
	//assert(!PrxLoadExports(prx));
	//fprintf(stderr,"%i\n",__LINE__);
	//assert(!PrxLoadImports(prx));
	//fprintf(stderr,"%i\n",__LINE__);
	//assert(!PrxCreateFakeSections(&prx->elf,prx->elf.programs,prx->mod->info.exports - 4));
	//fprintf(stderr,"%i\n",__LINE__);
	//assert(!PrxBuildMaps(prx));
	return 0;
}

int PrxLoadFromBin(PrxToolCtx* prx,FILE *fp){
	assert(!elf_loadFromBinFile(&prx->elf, fp, prx->base));
	prx->vMem = (Vmem){prx->elf.pElfBin, prx->elf.iBinSize, prx->elf.baseAddr, MEM_LITTLE_ENDIAN};
	assert(PrxBuildMaps(prx));
	return 0;
}

