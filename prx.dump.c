int prx_dumpInfo(PrxCtx* prx,FILE *stream){
	PspModuleInfo*i=&prx->module.info;
	fprintf(stream,"\n[ModuleInfo]\nAddr:%X %.*s Flags:%08X GP:%X Imp:%X..%X Exp:%X..%X\n",prx->module.addr,
		sizeof(i->name),i->name,i->flags, i->gp, i->imports, i->imp_end, i->exports, i->exp_end);
	return 0;
}

int prx_dumpImports(PrxCtx* prx,FILE *stream){
	fprintf(stream,"\n[ImportedLibs]\nflags    sz var func nids   func   var    name\n");
	for(PspModuleImport*i=prx->module.imps;i<prx->module.imps+prx->module.imps_count;i++)
		fprintf(stream,"%08X %2i %3i %4i %06X %06X %06X %s\n",i->flags,i->size,i->vars_count,i->funcs_count,i->nids,i->funcs,i->vars,&prx->elf.elf[elf_translate(&prx->elf,i->name)]);
	fprintf(stream,"\n[ImportedFuncs]\nNids     Offset\n");
	for(PspModuleFunction*f=prx->module.impfuncs;f<prx->module.impfuncs+prx->module.impfuncs_count;f++)
		fprintf(stream,"%08X %06X\n",f->data_addr,f->nid_addr);
	fprintf(stream,"\n[ImportedVars]\nNids     Offset\n");
	for(PspModuleVariable*v=prx->module.impvars;v<prx->module.impvars+prx->module.impvars_count;v++)
		fprintf(stream,"%08X %06X\n",v->data_addr,v->nid_addr);
	return 0;
}

int prx_dumpExports(PrxCtx* prx,FILE *stream){
	fprintf(stream,"\n[ExportedLibs]\nflags    sz var func offset name\n");
	for(PspModuleExport*e=prx->module.exps;e<prx->module.exps+prx->module.exps_count;e++)
		fprintf(stream,"%08X %2i %3i %4i %06X %s\n",e->flags,e->size,e->vars_count,e->funcs_count,e->exports,elf_translate(&prx->elf,e->name)?(char*)&prx->elf.elf[elf_translate(&prx->elf,e->name)]:"syslib");
	fprintf(stream,"\n[ExportedFuncs]\nNids     Offset\n");
	for(PspModuleFunction*f=prx->module.expfuncs;f<prx->module.expfuncs+prx->module.expfuncs_count;f++)
		fprintf(stream,"%08X %06X\n",f->data_addr,f->nid_addr);
	fprintf(stream,"\n[ExportedVars]\nNids     Offset\n");
	for(PspModuleVariable*v=prx->module.expvars;v<prx->module.expvars+prx->module.expvars_count;v++)
		fprintf(stream,"%08X %06X\n",v->data_addr,v->nid_addr);
	return 0;
}

int prx_dump(PrxCtx* prx,FILE *stream){
	elf_dump(&prx->elf,stream);
	prx_dumpInfo(prx,stream);
	prx_dumpImports(prx,stream);
	prx_dumpExports(prx,stream);
	return 0;
}
