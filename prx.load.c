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

int prx_dumpInfo(PrxCtx* prx,FILE *stream){
	PspModuleInfo*i=&prx->module.info;
	fprintf(stream,"\n[ModuleInfo]\nAddr:%08X %.*s Flags:%08X GP:%08X Imp:%X..%X Exp:%X..%X\n",prx->module.addr,
		sizeof(i->name),i->name,i->flags, i->gp, i->imports, i->imp_end, i->exports, i->exp_end);
	return 0;
}

int prx_dumpImports(PrxCtx* prx,FILE *stream){
	fprintf(stream,"\n[ImportedLibs]\nflags    sz var func nids func  var name\n");
	for(PspModuleImport*i=prx->module.imps;i<prx->module.imps+prx->module.imps_count;i++)
		fprintf(stream,"%08X %2i %3i %4i %04X %04X %04X %s\n",i->flags,i->size,i->vars_count,i->funcs_count,i->nids,i->funcs,i->vars,&prx->elf.elf[elf_translate(&prx->elf,i->name)]);
	fprintf(stream,"[ImportedFuncs]\n");
	for(PspModuleFunction*f=prx->module.impfuncs;f<prx->module.impfuncs+prx->module.impfuncs_count;f++)
		fprintf(stream,"%08X at %08X from lib#%i\n",f->data_addr,f->nid_addr,0);
	fprintf(stream,"[ImportedVars]\n");
	for(PspModuleVariable*v=prx->module.impvars;v<prx->module.impvars+prx->module.impvars_count;v++)
		fprintf(stream,"%08X at %08X from lib#%i\n",v->data_addr,v->nid_addr,0);
	return 0;
}

int prx_dumpExports(PrxCtx* prx,FILE *stream){
	fprintf(stream,"\n[ExportedLibs]\nflags    sz var func addr name\n");
	for(PspModuleExport*e=prx->module.exps;e<prx->module.exps+prx->module.exps_count;e++)
		fprintf(stream,"%08X %2i %3i %4i %04X %s\n",e->flags,e->size,e->vars_count,e->funcs_count,e->exports,elf_translate(&prx->elf,e->name)?(char*)&prx->elf.elf[elf_translate(&prx->elf,e->name)]:"syslib");
	fprintf(stream,"[ExportedFuncs]\n");
	for(PspModuleFunction*f=prx->module.expfuncs;f<prx->module.expfuncs+prx->module.expfuncs_count;f++)
		fprintf(stream,"%08X at %08X as lib#%i\n",f->data_addr,f->nid_addr,0);
	fprintf(stream,"[ExportedVars]\n");
	for(PspModuleVariable*v=prx->module.expvars;v<prx->module.expvars+prx->module.expvars_count;v++)
		fprintf(stream,"%08X at %08X as lib#%i\n",v->data_addr,v->nid_addr,0);
	return 0;
}

int prx_dump(PrxCtx* prx,FILE *stream){
	elf_dump(&prx->elf,stream);
	prx_dumpInfo(prx,stream);
	prx_dumpImports(prx,stream);
	prx_dumpExports(prx,stream);
	return 0;
}

int prx_loadFromElf(PrxCtx* prx,FILE *fp,Instruction*inst, size_t inst_count,char*modInfoName){
	assert(!elf_loadFromElfFile(&prx->elf, fp));
	prx->vMem = (Vmem){prx->elf.elf, prx->elf.bin_count, prx->elf.baseAddr, MEM_LITTLE_ENDIAN};
	//fprintf(stderr,"pData %p, iSize %x, iBaseAddr 0x%08X, endian %d\n", prx->vMem.data, prx->vMem.size, prx->vMem.baseAddr, prx->vMem.endian);
	assert(!elf_loadModInfo(&prx->elf,&prx->module,modInfoName));
	assert(!elf_loadRelocs(&prx->elf));
	//assert(!elf_fixupRelocs(&prx->elf, prx->base, prx->imm,prx->imm_count,&prx->vMem));
	
	assert(!prx_loadImports(prx,NULL,&prx->module.imps_count,NULL,&prx->module.impfuncs_count,NULL,&prx->module.impvars_count));
	prx->module.imps=calloc(prx->module.imps_count,sizeof(PspModuleImport));
	prx->module.impfuncs=calloc(prx->module.expfuncs_count,sizeof(PspModuleFunction));
	prx->module.impvars=calloc(prx->module.expvars_count,sizeof(PspModuleVariable));
	assert(!prx_loadImports(prx,prx->module.imps,NULL,prx->module.impfuncs,NULL,prx->module.impvars,NULL));
	
	assert(!prx_loadExports(prx,NULL,&prx->module.exps_count,	NULL,&prx->module.expfuncs_count,NULL,&prx->module.expvars_count));
	prx->module.exps=calloc(prx->module.exps_count,sizeof(PspModuleExport));
	prx->module.expfuncs=calloc(prx->module.expfuncs_count,sizeof(PspModuleFunction));
	prx->module.expvars=calloc(prx->module.expvars_count,sizeof(PspModuleVariable));
	assert(!prx_loadExports(prx,prx->module.exps,NULL,prx->module.expfuncs,NULL,prx->module.expvars,NULL));
	
	return 0;
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
