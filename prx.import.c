int prx_loadImport(PrxCtx* prx,PspModuleImport *imp,uint32_t*pos){
	fprintf(stderr,"%08X %i %i %i %08X %08X %08X %s@%X\n",
		imp->flags,
		imp->size,
		imp->vars_count,
		imp->funcs_count,
		imp->nids,
		imp->funcs,
		imp->vars,
		&prx->elf.elf[elf_translate(&prx->elf,imp->name)],
		imp->name);
	assert(imp->size);
	*pos += imp->size*4;
	return 0;
}
int prx_loadImport1(PrxCtx* prx,PspModuleImport *pImport){
#if 0
	char*pName,*dep,*slash;
	PspEntries pLib={
		.stub.name = LW(pImport->name),
		.stub.flags = LW(pImport->flags),
		.stub.counts = LW(pImport->counts),
		
		
		.stub.nids = LW(pImport->nids),
		.stub.funcs = LW(pImport->funcs),
		.stub.vars = LW(pImport->vars),
	};
	if(!pLib.stub.name)// Shouldn't be zero, although technically it could be 
		return fprintf(stderr, "Import libs must have a name"),0;
	if(!(pName = (char*) VmemGetPtr(&prx->vMem,pLib.stub.name)))
		return fprintf(stderr, "Invalid memory address for import name (0x%08X)\n", pLib.stub.name),0;

	strncpy(pLib.name, pName,sizeof(pLib.name));
	if((dep = db_nids_findPrxByLibName(prx->libs,prx->libs_count,pName))){
		if((slash = strrchr(dep, '/')))// Remove any path element
			dep = slash + 1;
		strcpy(pLib.file, dep);
	}

	fprintf(stdout,"Found import libs '%s'\nFlags %08X, counts %08X, nids %08X, funcs %08X\n", pLib.name, pLib.stub.flags, pLib.stub.counts, pLib.stub.nids, pLib.stub.funcs);

	pLib.v_count = pLib.stub.counts >> 8;
	pLib.f_count = pLib.stub.counts >> 16;
	
	if(VmemGetSize(&prx->vMem,pLib.stub.nids) < (sizeof(uint32_t) * pLib.f_count))
		return fprintf(stderr, "Not enough space for libs import nids"),0;

	if(VmemGetSize(&prx->vMem,pLib.stub.funcs) < (uint32_t) (8 * pLib.f_count))
		return fprintf(stderr, "Not enough space for libs proto"),0;

	for(uint32_t nidAddr = pLib.stub.nids, iLoop = 0, funcAddr = pLib.stub.funcs; iLoop < pLib.f_count; iLoop++,nidAddr += 4,funcAddr += 8){
		pLib.funcs[iLoop].nid = VmemGetU32(&prx->vMem,nidAddr);
		strcpy(pLib.funcs[iLoop].name, db_nids_getFunctionName(prx->nids,prx->nids_count, pLib.name, pLib.funcs[iLoop].nid));
		pLib.funcs[iLoop].type = PSP_ENTRY_FUNC;
		pLib.funcs[iLoop].addr = funcAddr;
		pLib.funcs[iLoop].nid_addr = nidAddr;
		fprintf(stdout,"Found import nid:0x%08X func:0x%08X name:%s\n", pLib.funcs[iLoop].nid, pLib.funcs[iLoop].addr, pLib.funcs[iLoop].name);
	}
	
	for(uint32_t varAddr = pLib.stub.vars, iLoop = 0; iLoop < pLib.v_count; iLoop++,varAddr += 8){
		pLib.vars[iLoop].addr = VmemGetU32(&prx->vMem,varAddr);
		pLib.vars[iLoop].nid = VmemGetU32(&prx->vMem,varAddr+4);
		pLib.vars[iLoop].type = PSP_ENTRY_VAR;
		pLib.vars[iLoop].nid_addr = varAddr+4;
		strcpy(pLib.vars[iLoop].name, db_nids_getFunctionName(prx->nids,prx->nids_count, pLib.name, pLib.vars[iLoop].nid));
		fprintf(stdout,"Found variable nid:0x%08X addr:0x%08X name:%s\n", pLib.vars[iLoop].nid, pLib.vars[iLoop].addr, pLib.vars[iLoop].name);
		uint32_t varFixup = pLib.vars[iLoop].addr;
		for(uint32_t varData;(varData = VmemGetU32(&prx->vMem,varFixup));varFixup += 4)
			fprintf(stdout,"Variable Fixup: addr:%08X type:%08X\n", (varData & 0x3FFFFFF) << 2, varData >> 26);
		
	}
	//append module imports to the global symbol list
	if(prx->module.imports){
		// Search for the end of the list
		//for(PspEntries* pImport = prx->module.imports;pImport->next;pImport = pImport->next);
		//pImport->next = pLib;
		//pLib.prev = pImport;
		//pLib.next = NULL;
	}else{
		//pLib.next = NULL;
		//pLib.prev = NULL;
		//prx->module.imports = pLib;
	}

	return pLib.stub.counts & 0xFF;
#endif
	return 0;
}

int prx_loadImports(PrxCtx* prx){
	assert(!prx->module.imports);
	assert(prx->module.info.imports);
	PspModuleInfo*i=&prx->module.info;
	
	for(uint32_t pos = i->imports;i->imp_end - pos;)
		assert(!prx_loadImport(prx,(PspModuleImport*)&prx->elf.elf[elf_translate(&prx->elf,pos)],&pos))
	/*
	for(uint32_t count,current = i->imports;i->imp_end - current >= sizeof(PspModuleImport);current += (count * sizeof(uint32_t))){
		fprintf(stderr,">>>>%08X\n",i->imports);
		assert(count = prx_loadImport2(prx, VmemGetPtr(&prx->vMem,current), current))
	}
	*/
	return 0;
}
