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
			for(int i=0;i<cur->funcs_count;i++)
				expfuncs[f++]=(PspModuleFunction){
					elf_at(&prx->elf,cur->exports)[i],
					elf_at(&prx->elf,cur->exports)[cur->funcs_count+cur->vars_count+i]
				};
			for(int i=0;i<cur->vars_count;i++)
				expvars[v++]=(PspModuleVariable){
					elf_at(&prx->elf,cur->exports)[cur->funcs_count+i],
					elf_at(&prx->elf,cur->exports)[cur->funcs_count*2+cur->vars_count+i]
				};
		}
	}
	return 0;
}
/*
syslib
	Functions:
		NID: 0xD632ACDB  VADDR: 0x00000008 NAME: module_start
	Variables:
		NID: 0xF01D73A7  VADDR: 0x00000080 NAME: module_info

MyExportLibName
	Functions:
		NID: 0x19CB347B  VADDR: 0x00000000
*/