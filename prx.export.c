int prx_loadExports(PrxCtx* prx,PspModuleExport*exps,size_t*exps_count,PspModuleFunction*expfuncs,size_t*expfuncs_count,PspModuleVariable*expvars,size_t*expvars_count){
	assert(!prx->module.exports);
	assert(prx->module.info.exports<=prx->module.info.exp_end);
	
	PspModuleExport*cur = (PspModuleExport*)&prx->elf.elf[elf_translate(&prx->elf,prx->module.info.exports)];
	PspModuleExport*end = (PspModuleExport*)&prx->elf.elf[elf_translate(&prx->elf,prx->module.info.exp_end)];
	int e=0,f=0,v=0;
	for(uint32_t*exp_ = (uint32_t*)cur;cur->size && (cur<end);exp_+=cur->size,cur=(PspModuleExport*)exp_){
		if(exps_count && expfuncs_count && expvars_count){
			(*expfuncs_count)+=cur->funcs_count;
			(*expvars_count)+=cur->vars_count;
			(*exps_count)++;
		}
		if(exps && expfuncs && expvars){
			exps[e++]=*cur;
			printf(">>>%i\n",e);
			for(int i=0;i<cur->funcs_count;i++)
				expfuncs[f++]=(PspModuleFunction){
					cur->nids,cur->nids
				};
			for(int i=0;i<cur->vars_count;i++)
				expvars[v]=(PspModuleVariable){
					cur->nids,cur->nids//*((uint32_t*)(prx->elf.elf+elf_translate(&prx->elf,cur->nids))),
				};
		}
	}
	return 0;
}
