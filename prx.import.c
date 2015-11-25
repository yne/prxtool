int prx_loadImports(PrxCtx* prx,PspModuleImport*imps,size_t*imps_count,PspModuleFunction*impfuncs,size_t*impfuncs_count,PspModuleVariable*impvars,size_t*impvars_count){
	assert(!prx->module.imports);
	assert(prx->module.info.imports<=prx->module.info.imp_end);
	
	PspModuleImport*cur = (PspModuleImport*)&prx->elf.elf[elf_translate(&prx->elf,prx->module.info.imports)];
	PspModuleImport*end = (PspModuleImport*)&prx->elf.elf[elf_translate(&prx->elf,prx->module.info.imp_end)];
	for(uint32_t*imp_ = (uint32_t*)cur;cur->size && (cur<end);imp_+=cur->size,cur=(PspModuleImport*)imp_){
		if( impfuncs_count)
			(*impfuncs_count)+=cur->funcs_count;
		if( impvars_count)
			(*impvars_count)+=cur->vars_count;
		if( imps_count)
			(*imps_count)++;
		if( imps)
			(*imps)=*cur;
		if(impfuncs)
			for(int i=0;i<cur->funcs_count;i++)
				impfuncs[i]=(PspModuleFunction){
					*((uint32_t*)(prx->elf.elf+elf_translate(&prx->elf,cur->nids))),
				};
		if(impvars)
			for(int i=0;i<cur->vars_count;i++)
				impvars[i]=(PspModuleVariable){
					*((uint32_t*)(prx->elf.elf+elf_translate(&prx->elf,cur->nids))),
				};
	}
	return 0;
}
