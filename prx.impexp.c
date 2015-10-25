int PrxLoadImport(PrxToolCtx* prx,PspModuleImport *pImport, uint32_t addr){
	char*pName,*dep,*slash;
	PspEntries pLib={
		.addr = addr,
		.stub.name = LW(pImport->name),
		.stub.flags = LW(pImport->flags),
		.stub.counts = LW(pImport->counts),
		.stub.nids = LW(pImport->nids),
		.stub.funcs = LW(pImport->funcs),
		.stub.vars = LW(pImport->vars),
	};
	if(!pLib.stub.name)// Shouldn't be zero, although technically it could be 
		return fprintf(stderr, "Import library must have a name"),0;
	if(!(pName = (char*) VmemGetPtr(&prx->vMem,pLib.stub.name)))
		return fprintf(stderr, "Invalid memory address for import name (0x%08X)\n", pLib.stub.name),0;

	strncpy(pLib.name, pName,sizeof(pLib.name));
	if((dep = db_nids_findPrxByLibName(prx->library,prx->library_count,pName))){
		if((slash = strrchr(dep, '/')))// Remove any path element
			dep = slash + 1;
		strcpy(pLib.file, dep);
	}

	fprintf(stdout,"Found import library '%s'\nFlags %08X, counts %08X, nids %08X, funcs %08X\n", pLib.name, pLib.stub.flags, pLib.stub.counts, pLib.stub.nids, pLib.stub.funcs);

	pLib.v_count = pLib.stub.counts >> 8;
	pLib.f_count = pLib.stub.counts >> 16;
	
	if(VmemGetSize(&prx->vMem,pLib.stub.nids) < (sizeof(uint32_t) * pLib.f_count))
		return fprintf(stderr, "Not enough space for library import nids"),0;

	if(VmemGetSize(&prx->vMem,pLib.stub.funcs) < (uint32_t) (8 * pLib.f_count))
		return fprintf(stderr, "Not enough space for library proto"),0;

	for(uint32_t nidAddr = pLib.stub.nids, iLoop = 0, funcAddr = pLib.stub.funcs; iLoop < pLib.f_count; iLoop++,nidAddr += 4,funcAddr += 8){
		pLib.funcs[iLoop].nid = VmemGetU32(&prx->vMem,nidAddr);
		strcpy(pLib.funcs[iLoop].name, db_nids_getFunctionName(prx->nids,prx->nid_count, pLib.name, pLib.funcs[iLoop].nid));
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
		strcpy(pLib.vars[iLoop].name, db_nids_getFunctionName(prx->nids,prx->nid_count, pLib.name, pLib.vars[iLoop].nid));
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
}

int PrxLoadImports(PrxToolCtx* prx){
	PspModuleInfo*i=&prx->module.info;
	#define PSP_IMPORT_BASE_SIZE (5*4)
	for(uint32_t count,base = i->imports;i->imports && (i->imp_end - base) >= PSP_IMPORT_BASE_SIZE;base += (count * sizeof(uint32_t)))
		if(!(count = PrxLoadImport(prx, VmemGetPtr(&prx->vMem,base), base)))
			return 0;
	return 1;
}

int PrxLoadExport(PrxToolCtx* prx,PspModuleExport *pExport, uint32_t addr){
	assert(pExport);

	PspEntries pLib = (PspEntries){
		.addr = addr,
		.export = {
			.name = LW(pExport->name),
			.flags = LW(pExport->flags),
			.counts = LW(pExport->counts),
			.exports = LW(pExport->exports),
		}
	};
	
	if(pLib.export.name == 0){
		// If 0 then this is the system, this should be the only one 
		strcpy(pLib.name, PSP_SYSTEM_EXPORT);
	}else{
		char *pName = (char*) VmemGetPtr(&prx->vMem, pLib.export.name);
		if(!pName)
			return fprintf(stderr, "Invalid memory address for export name (0x%08X)\n", pLib.export.name),0;
		strcpy(pLib.name, pName);
	}

	fprintf(stdout,"Found export library '%s'\n", pLib.name);
	fprintf(stdout,"Flags %08X, counts %08X, exports %08X\n", pLib.export.flags, pLib.export.counts, pLib.export.exports);

	pLib.v_count = (pLib.export.counts >> 8) & 0xFF;
	pLib.f_count = (pLib.export.counts >> 16) & 0xFFFF;
	int count = pLib.export.counts & 0xFF;
	uint32_t expAddr = pLib.export.exports;

	if(VmemGetSize(&prx->vMem,expAddr) < (sizeof(uint32_t) * (pLib.v_count + pLib.f_count)))
		return fprintf(stderr, "Invalid memory address for exports (0x%08X)\n", pLib.export.exports),0;

	for(int iLoop = 0; iLoop < pLib.f_count; iLoop++){
		// We will fix up the names later 
		pLib.funcs[iLoop].nid = VmemGetU32(&prx->vMem,expAddr);
		strcpy(pLib.funcs[iLoop].name, db_nids_getFunctionName(prx->nids,prx->library_count, pLib.name, pLib.funcs[iLoop].nid));
		pLib.funcs[iLoop].type = PSP_ENTRY_FUNC;
		pLib.funcs[iLoop].addr = VmemGetU32(&prx->vMem,expAddr + (sizeof(uint32_t) * (pLib.v_count + pLib.f_count)));
		pLib.funcs[iLoop].nid_addr = expAddr; 
		fprintf(stdout,"Found export nid:0x%08X func:0x%08X name:%s\n", pLib.funcs[iLoop].nid, pLib.funcs[iLoop].addr, pLib.funcs[iLoop].name);
		expAddr += 4;
	}

	for(int iLoop = 0; iLoop < pLib.v_count; iLoop++){
		// We will fix up the names later 
		pLib.vars[iLoop].nid = VmemGetU32(&prx->vMem,expAddr);
		strcpy(pLib.vars[iLoop].name, db_nids_getFunctionName(prx->nids,prx->library_count, pLib.name, pLib.vars[iLoop].nid));
		pLib.vars[iLoop].type = PSP_ENTRY_FUNC;
		pLib.vars[iLoop].addr = VmemGetU32(&prx->vMem,expAddr + (sizeof(uint32_t) * (pLib.v_count + pLib.f_count)));
		pLib.vars[iLoop].nid_addr = expAddr; 
		fprintf(stdout,"Found export nid:0x%08X var:0x%08X name:%s\n", pLib.vars[iLoop].nid, pLib.vars[iLoop].addr, pLib.vars[iLoop].name);
		expAddr += 4;
	}

	if(!prx->module.exports){
		prx->module.exports = &pLib;
	}else{
		// Search for the end of the list
		//while(PspEntries* prx->module.exports;pExport->next;pExport = pExport->next)
		//pExport->next = pLib;
	}

	return count;
}

int PrxLoadExports(PrxToolCtx* prx){
	assert(prx->module.exports == NULL);

	uint32_t exp_base = prx->module.info.exports;
	uint32_t exp_end =  prx->module.info.exp_end;
	if(!exp_base)
		return 0;
	while((exp_end - exp_base) >= sizeof(PspModuleExport)){
		PspModuleExport *pExport = (PspModuleExport*) VmemGetPtr(&prx->vMem,exp_base);
		if(!pExport)
			return 0;
		uint32_t count = PrxLoadExport(prx, pExport, exp_base);
		if(!count)
			return 0;
		exp_base += (count * sizeof(uint32_t));
	}
	return 1;
}

