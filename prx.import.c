int prx_loadImports(PrxCtx* prx,PspModuleImport*imps,size_t*imps_count,PspModuleFunction*impfuncs,size_t*impfuncs_count,PspModuleVariable*impvars,size_t*impvars_count){
	assert(!prx->module.imports);
	assert(prx->module.info.imports<=prx->module.info.imp_end);
	
	PspModuleImport*imp = (PspModuleImport*)&prx->elf.elf[elf_translate(&prx->elf,prx->module.info.imports)];
	PspModuleImport*end = (PspModuleImport*)&prx->elf.elf[elf_translate(&prx->elf,prx->module.info.imp_end)];
	for(uint32_t*imp_ = (uint32_t*)imp;imp->size && (imp<end);imp_+=imp->size,imp=(PspModuleImport*)imp_){
		fprintf(stderr,"Imp:%i %i %i\n",imp->size,imp->vars_count,imp->funcs_count);
		*impfuncs_count+=imp->funcs_count;
		*impvars_count+=imp->vars_count;
		if(impfuncs)
			for(int i=0;i<imp->funcs_count;i++)
				impfuncs[i]=(PspModuleFunction){};
		if(impvars)
			for(int i=0;i<imp->vars_count;i++)
				impvars[i]=(PspModuleVariable){};
	}
	return 0;
}
