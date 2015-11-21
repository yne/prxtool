int prx_buildSymbols(PrxCtx* prx,Symbol *symbol,size_t *syms_count, uint32_t base){//TODO
	// First map in imports and exports 
	// If we have a symbol table then no point building from imports/exports 
	if(prx->elf.symbol){
		for(int i = 0; i < prx->elf.symbol_count; i++){
			int type = ELF32_ST_TYPE(prx->elf.symbol[i].info);
			if((type != STT_FUNC) && (type == STT_OBJECT))
				continue;
			Symbol *s = NULL;//symbol[prx->elf.symbol[i].value + base];
			if(!s){
				symbol[prx->elf.symbol[i].value + base]=(Symbol){
					.addr = prx->elf.symbol[i].value + base,
					.type = type == STT_FUNC?SYMBOL_FUNC:SYMBOL_DATA,
					.type = SYMBOL_DATA,
					.size = prx->elf.symbol[i].size,
					//.name = prx->elf.symbol[i].symname,
				};
			}else{
				//if(strcmp(s->name, prx->elf.symbol[i].symname))
				//	s->alias.insert(s->alias.end(), prx->elf.symbol[i].symname);
			}
		}
		return 0;
	}
	for(int exp_cnt=0;exp_cnt<prx->module.exports_count;exp_cnt++){
		PspEntries *pExport = &prx->module.exports[exp_cnt];
		for(int iLoop = 0; iLoop < pExport->f_count; iLoop++){
			Symbol *s = NULL;//symbol.symbol[pExport->funcs[iLoop].addr + base];
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
	for(int i = 0; i < prx->module.imports_count ;i++){
		PspEntries *pImport = &prx->module.imports[i];
		for(int iLoop = 0; iLoop < pImport->f_count; iLoop++){
			symbol[pImport->funcs[iLoop].addr + base] = (Symbol){
				.addr = pImport->funcs[iLoop].addr + base,
				.type = SYMBOL_FUNC,
				.size = 0,
				//.name = pImport->funcs[iLoop].name,
				//.imported.insert(s->imported.end(), pImport),
			};
		}
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
	return 0;
}

int prx_mapFuncExtent(PrxCtx*prx,uint32_t dwStart, uint8_t *pTouchMap){
	return 0;
}

int prx_mapFuncExtents(PrxCtx*prx,Symbol*symbol,size_t syms_count){
	uint8_t*pTouchMap=calloc(prx->elf.bin_count,sizeof(*pTouchMap));

	for(int i=0;i<syms_count;i++)
		if((symbol[i].type == SYMBOL_FUNC) && (symbol[i].size == 0))
			symbol[i].size = prx_mapFuncExtent(prx, symbol[i].addr, pTouchMap)?:symbol[i].size;
	return 0;
}

int prx_buildMaps(PrxCtx*prx, Instruction*instr, size_t instr_count){//TODO:refs
	for(Imm *imm = prx->imm; imm < prx->imm+prx->symbol_count; imm++){
		uint32_t inst = VmemGetU32(&prx->vMem,imm->target - prx->base);
		if(!imm->text)
			continue;
		if(!prx->symbol[imm->target].type){
			prx->symbol[imm->target] = (Symbol){
				.type = (inst >> 16) == 0x27BD?SYMBOL_FUNC:SYMBOL_LOCAL,
				.addr = imm->target,
				.size = 0,
			};
			snprintf(prx->symbol[imm->target].name, sizeof((Symbol){}.name), "%s_%08X",prx->symbol[imm->target].type==SYMBOL_FUNC?"sub":"loc",imm->target);
		}
		//s->refs.insert(s->refs.end(), imm->addr);
	}

	// Build symbol for branches in the code 
	for(int i = 0; i < prx->elf.SH_count; i++){
		if(!(prx->elf.section[i].flags & SHF_EXECINSTR))
			continue;
		uint32_t dwAddr = prx->elf.section[i].iAddr;
		uint32_t *pInst = (uint32_t*) VmemGetPtr(&prx->vMem,dwAddr);

		for(uint32_t j = 0; j < (prx->elf.section[i].iSize / 4); j++,dwAddr += 4)
			disasmAddBranchSymbols(LW(pInst[j]), dwAddr + prx->base, prx->symbol,&prx->symbol_count, instr, instr_count);
	}

	if(!prx->symbol[prx->elf.header.entry + prx->base].type)
		prx->symbol[prx->elf.header.entry + prx->base] = (Symbol){
			.type = SYMBOL_FUNC,
			.addr = prx->elf.header.entry + prx->base,
			.size = 0,
			.name = "_start",
		};
	return 0;
}

int elf_loadModInfo(ElfCtx* elf,PspModule*mod,char*secname){
	PspModuleInfo*modinfo = NULL;
	uint32_t iAddr = 0;
	ElfSection *modInfoSec = elf_findSection(elf, secname);
	if(modInfoSec){
		iAddr = modInfoSec->iAddr;
		modinfo = (PspModuleInfo*)modInfoSec->pData;
	}else if(elf->PH_count){//If no ModInfo section found => use PH
		iAddr = (elf->program[0].iPaddr & 0x7FFFFFFF) - elf->program[0].iOffset;
		modinfo = (PspModuleInfo*)elf->bin + iAddr;
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

int elf_createFakeSections(ElfCtx*elf,ElfProgram*p,uint32_t stubBtm){
	// If we have no section headers let's build some fake section 
	if(!elf->SH_count)
		return 1;
	assert(elf->PH_count >= 3)

	elf->SH_count = (p[2].type == PT_PRXRELOC)?6:5;
	
	ElfSection fake_sections[]={{},{
		.type  = SHT_PROGBITS,
		.flags = SHF_ALLOC | SHF_EXECINSTR,
		.iAddr = p[0].iVaddr,
		.pData = elf->elf + p[0].iOffset,
		.iSize = stubBtm,
		.szName= ".text",
	},{
		.type  = SHT_PROGBITS,
		.flags = SHF_ALLOC,
		.iAddr = stubBtm,
		.pData = elf->elf + p[0].iOffset + stubBtm,
		.iSize = p[0].iMemsz - stubBtm,
		.szName=".rodata",
	},{
		.type  = SHT_PROGBITS,
		.flags = SHF_ALLOC | SHF_WRITE,
		.iAddr = p[1].iVaddr,
		.pData = elf->elf + p[1].iOffset,
		.iSize = p[1].iFilesz,
		.szName= ".data",
	},{
		.type  = SHT_NOBITS,
		.flags = SHF_ALLOC | SHF_WRITE,
		.iAddr = p[1].iVaddr + p[1].iFilesz,
		.pData = elf->elf + p[1].iOffset + p[1].iFilesz,
		.iSize = p[1].iMemsz - p[1].iFilesz,
		.szName= ".bss",
	},{
		.type  = SHT_PRXRELOC,
		.flags = 0,
		.iAddr = 0,
		.pData = elf->elf + p[2].iOffset,
		.iSize = p[2].iFilesz,
		.iInfo = 1,// Bind to section 1, not that is matters 
		.szName= ".reloc",
	}};
	memcpy(elf->section,fake_sections,elf->SH_count*sizeof(*elf->section));
	return 1;
}

int prx_loadFromElf(PrxCtx* prx,FILE *fp, Instruction*inst, size_t inst_count,char*modInfoName){
	assert(!elf_loadFromElfFile(&prx->elf, fp));
	prx->vMem = (Vmem){prx->elf.elf, prx->elf.bin_count, prx->elf.baseAddr, MEM_LITTLE_ENDIAN};
	//fprintf(stderr,"pData %p, iSize %x, iBaseAddr 0x%08X, endian %d\n", prx->vMem.data, prx->vMem.size, prx->vMem.baseAddr, prx->vMem.endian);
	assert(!elf_loadModInfo(&prx->elf,&prx->module,modInfoName));
	assert(!elf_loadRelocs(&prx->elf));
	elf_dump(&prx->elf,stderr);
	//assert(!elf_fixupRelocs(&prx->elf, prx->base, prx->imm,prx->imm_count,&prx->vMem));
	PspModuleInfo*i=&prx->module.info;
	fprintf(stderr,"\n[PRX]\nAddr:%08X %.*s Flags:%08X GP:%08X Imp:%X..%X Exp:%X..%X\n",prx->module.addr,
		sizeof(i->name),i->name,i->flags, i->gp, i->imports, i->imp_end, i->exports, i->exp_end);
	
	assert(!prx_loadImports(prx));
	fprintf(stderr,">>>IMPORT\n");
	return 0;
	assert(!prx_loadExports(prx));
	fprintf(stderr,">>>EXPORT:OK\n",__LINE__);
	assert(!elf_createFakeSections(&prx->elf,prx->elf.program,prx->module.info.exports - 4));
	fprintf(stderr,">>>FAKESEC\n",__LINE__);
	assert(!prx_buildSymbols(prx,prx->symbol,&prx->symbol_count, prx->base));
	assert(!prx_buildMaps(prx,inst,inst_count));
	assert(!prx_mapFuncExtents(prx,prx->symbol,prx->symbol_count));
	return 0;
}

int prx_loadFromBin(PrxCtx* prx,FILE *fp, Instruction*inst, size_t inst_count){
	assert(!elf_loadFromBinFile(&prx->elf, fp, prx->base));
	prx->vMem = (Vmem){prx->elf.bin, prx->elf.bin_count, prx->elf.baseAddr, MEM_LITTLE_ENDIAN};
	assert(!prx_buildSymbols(prx,prx->symbol,&prx->symbol_count, prx->base));
	assert(!prx_buildMaps(prx,inst,inst_count));
	assert(!prx_mapFuncExtents(prx,prx->symbol,prx->symbol_count));
	return 0;
}

