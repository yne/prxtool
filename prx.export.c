int prx_loadExport(PrxToolCtx* prx,PspModuleExport *pExport, uint32_t addr){
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
	fprintf(stderr,"%i %i\n",pLib.export.counts& 0xFF,pLib.export.exports);
	if(!pLib.export.name){// name less mean system
		strcpy(pLib.name, PSP_SYSTEM_EXPORT);//this should be the only one 
	}else{
		char *pName = (char*) VmemGetPtr(&prx->vMem, pLib.export.name);
		if(!pName)
			return fprintf(stderr, "Invalid memory address for export name (0x%08X)\n", pLib.export.name),0;
		strcpy(pLib.name, pName);
	}

	fprintf(stderr,"Found export library '%s'\n", pLib.name);
	fprintf(stderr,"Flags %08X, counts %08X, exports %08X\n", pLib.export.flags, pLib.export.counts, pLib.export.exports);
	pLib.v_count = (pLib.export.counts >> 8) & 0xFF;
	pLib.f_count = (pLib.export.counts >> 16) & 0xFFFF;
	uint32_t expAddr = pLib.export.exports;

	fprintf(stderr,">>> %i\n",__LINE__);
	if(VmemGetSize(&prx->vMem,expAddr) < (sizeof(uint32_t) * (pLib.v_count + pLib.f_count)))
		return fprintf(stderr, "Invalid memory address for exports (0x%08X)\n", pLib.export.exports),0;

	fprintf(stderr,">>> %i <%i>\n",__LINE__,pLib.f_count);
	for(PspEntry*f=pLib.funcs;f<pLib.funcs+pLib.f_count;f++){
	// We will fix up the names later 
		f->nid = VmemGetU32(&prx->vMem,expAddr);
		strcpy(f->name, db_nids_getFunctionName(prx->nids,prx->library_count, pLib.name, f->nid));
		f->type = PSP_ENTRY_FUNC;
		f->addr = VmemGetU32(&prx->vMem,expAddr + (sizeof(uint32_t) * (pLib.v_count + pLib.f_count)));
		f->nid_addr = expAddr; 
		fprintf(stderr,"Found export nid:0x%08X func:0x%08X name:%s\n", f->nid, f->addr, f->name);
		expAddr += 4;
	}

	fprintf(stderr,">>> %i <%i>\n",__LINE__,pLib.v_count);
	for(PspEntry*v=pLib.vars;v<pLib.vars+pLib.v_count;v++){
		// We will fix up the names later 
		v->nid = VmemGetU32(&prx->vMem,expAddr);
		strcpy(v->name, db_nids_getFunctionName(prx->nids,prx->library_count, pLib.name, v->nid));
		v->type = PSP_ENTRY_FUNC;
		v->addr = VmemGetU32(&prx->vMem,expAddr + (sizeof(uint32_t) * (pLib.v_count + pLib.f_count)));
		v->nid_addr = expAddr; 
		fprintf(stderr,"Found export nid:0x%08X var:0x%08X name:%s\n", v->nid, v->addr, v->name);
		expAddr += 4;
	}
	fprintf(stderr,">>> %i\n",__LINE__);
	if(!prx->module.exports){
		prx->module.exports = &pLib;
	}else{
		// Search for the end of the list
		//while(PspEntries* prx->module.exports;pExport->next;pExport = pExport->next)
		//pExport->next = pLib;
	}

	return pLib.export.counts & 0xFF;
}

int prx_loadExports(PrxToolCtx* prx){
	assert(!prx->module.exports);
	assert(prx->module.info.exports);

	uint32_t addr = prx->module.info.exports;
	for(uint32_t count=0;(prx->module.info.exp_end - addr) >= sizeof(PspModuleExport);addr += (count * sizeof(uint32_t)))
		assert(count = prx_loadExport(prx, (PspModuleExport*) VmemGetPtr(&prx->vMem,addr), addr));
	return 0;
}