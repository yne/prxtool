int prx_loadExports(PrxCtx* prx,PspModuleExport*exps,size_t*exps_count,PspModuleFunction*expfuncs,size_t*expfuncs_count,PspModuleVariable*expvars,size_t*expvars_count){
	assert(!prx->module.exports);
	assert(prx->module.info.exports<=prx->module.info.exp_end);
	
	PspModuleExport*cur = (PspModuleExport*)&prx->elf.elf[elf_translate(&prx->elf,prx->module.info.exports)];
	PspModuleExport*end = (PspModuleExport*)&prx->elf.elf[elf_translate(&prx->elf,prx->module.info.exp_end)];
	for(uint32_t*exp_ = (uint32_t*)cur;cur->size && (cur<end);exp_+=cur->size,cur=(PspModuleExport*)exp_){
		if(expfuncs_count)
			(*expfuncs_count)+=cur->funcs_count;
		if(expvars_count)
			(*expvars_count)+=cur->vars_count;
		if(exps_count)
			(*exps_count)++;
		if(exps)
			(*exps)=*cur;
		if(expfuncs)
			for(int i=0;i<cur->funcs_count;i++)
				expfuncs[i]=(PspModuleFunction){
					*((uint32_t*)(prx->elf.elf+elf_translate(&prx->elf,cur->nids))),
				};
		if(expvars)
			for(int i=0;i<cur->vars_count;i++)
				expvars[i]=(PspModuleVariable){
					*((uint32_t*)(prx->elf.elf+elf_translate(&prx->elf,cur->nids))),
				};
	}
	return 0;
}
