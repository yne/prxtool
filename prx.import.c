int prx_loadImports(PrxCtx* prx,PspModuleImport*imps,size_t*imps_count,PspModuleFunction*impfuncs,size_t*impfuncs_count,PspModuleVariable*impvars,size_t*impvars_count){
	assert(!prx->module.imports);
	assert(prx->module.info.imports<=prx->module.info.imp_end);
	
	PspModuleImport*cur = (PspModuleImport*)&prx->elf.elf[elf_translate(&prx->elf,prx->module.info.imports)];
	PspModuleImport*end = (PspModuleImport*)&prx->elf.elf[elf_translate(&prx->elf,prx->module.info.imp_end)];
	int imp=0,f=0,v=0;
	for(uint32_t*imp_ = (uint32_t*)cur;cur->size && (cur<end);imp_+=cur->size,cur=(PspModuleImport*)imp_){
		if(imps_count && impfuncs_count && impvars_count){
			(*impfuncs_count)+=cur->funcs_count;
			(*impvars_count)+=cur->vars_count;
			(*imps_count)++;
		}
		if(imps && impfuncs && impvars){
			imps[imp++]=*cur;
			for(int i=0;i<cur->funcs_count;i++)
				impfuncs[f++]=(PspModuleFunction){
					elf_at(&prx->elf,cur->nids)[i],
					cur->funcs+(i*4)
				};
			for(int i=0;i<cur->vars_count;i++)
				impvars[v++]=(PspModuleVariable){
					elf_at(&prx->elf,cur->nids)[cur->funcs_count+i],
					cur->vars+(i*4)
				};
		}
	}
	return 0;
}
