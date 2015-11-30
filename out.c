#include "out.prx.c"

int compare_symbols(const void *left, const void *right){
	return ((int) ((ElfSymbol *) left)->value) - ((int) ((ElfSymbol *) right)->value);
}

int output_symbol   (PrxCtx* prx,FILE *out_fp){
	ElfSymbol *pSymbols=NULL;
	int symbol_count=0;//PrxGetSymbols(&pSymbols);
	if(!symbol_count)
		return fprintf(stderr, "No symbol available"),1;
	
	typedef struct{
		char magic[4];
		char modname[sizeof(((PspModule){}).info.name)];
		uint32_t  symcount;
		uint32_t  strstart;
		uint32_t  strsize;
	} __attribute__ ((packed)) SymfileHeader;
	SymfileHeader fileHead;
	ElfSymbol pSymCopy[symbol_count];

	int iSymCopyCount = 0;
	int iStrSize = 0;
	int iStrPos  = 0;
	// Calculate the sizes
	for(int i = 0; i < symbol_count; i++){
		int type = ELF32_ST_TYPE(pSymbols[i].info);
		if(((type == STT_FUNC) || (type == STT_OBJECT)) && (strlen(pSymbols[i].symname) > 0)){
			memcpy(&pSymCopy[iSymCopyCount++], &pSymbols[i], sizeof(ElfSymbol));
			iStrSize += strlen(pSymbols[i].symname) + 1;
		}
	}

	fprintf(stdout,"Removed %d symbol, leaving %d\n", symbol_count - iSymCopyCount, iSymCopyCount);
	fprintf(stdout,"String size %d/%d\n", symbol_count - iSymCopyCount, iSymCopyCount);
	qsort(pSymCopy, iSymCopyCount, sizeof(ElfSymbol), compare_symbols);
	memcpy(fileHead.magic, "SYMS", 4);
	// memcpy(fileHead.modname, PrxGetModuleInfo()->name, PspModuleInfo.name);
	typedef struct{
		uint32_t name;
		uint32_t addr;
		uint32_t size;
	} __attribute__((packed))SymfileEntry;
	
	SW(fileHead.symcount, iSymCopyCount);
	SW(fileHead.strstart, sizeof(fileHead) + (sizeof(SymfileEntry)*iSymCopyCount));
	SW(fileHead.strsize, iStrSize);
	fwrite(&fileHead, 1, sizeof(fileHead), out_fp);
	for(int i = 0; i < iSymCopyCount; i++){
		SymfileEntry sym;

		SW(sym.name, iStrPos);
		SW(sym.addr, pSymCopy[i].value);
		SW(sym.size, pSymCopy[i].size);
		iStrPos += strlen(pSymCopy[i].symname)+1;
		fwrite(&sym, 1, sizeof(sym), out_fp);
	}

	// Write out string table
	for(int i = 0; i < iSymCopyCount; i++)
		fwrite(pSymCopy[i].symname, 1, strlen(pSymCopy[i].symname)+1, out_fp);
	return 0;
}

int output_disasm   (PrxCtx* prx,FILE *out_fp,PrxToolArg* arg){
	PrxDump(prx,out_fp, arg->disopts);
	return 0;
}

int output_mod      (PrxCtx* prx,FILE *out_fp){
	return prx_dump(prx,stderr);
}

int output_elf      (PrxCtx* prx,FILE *out_fp){//TODO
	if(!prx_toElf(prx,out_fp))
		return fprintf(stderr, "Failed to create a fixed up ELF\n"),1;
	return 0;
}

int output_xml      (PrxCtx* prx,FILE *out_fp){
	return 0;
}

int output_map      (PrxCtx* prx,FILE *out_fp){
	return 0;
}

int output_idc      (PrxCtx* prx,FILE *out_fp){
	return 0;
}

int output_impexp   (PrxCtx* prx,FILE *out_fp,PrxToolArg* arg){
	PspModule pMod;
	// PrxGetModuleInfo(&pMod);
	fprintf(stdout, "Module information\n");
	fprintf(stdout, "Name:    %s\n", pMod.info.name);
	fprintf(stdout, "Attrib:  %04X\n", pMod.info.flags & 0xFFFF);
	fprintf(stdout, "Version: %d.%d\n", (pMod.info.flags >> 24) & 0xFF, (pMod.info.flags >> 16) & 0xFF);
	fprintf(stdout, "GP:      %08X\n", pMod.info.gp);

	fprintf(stdout, "\nExports:\n");
	
	int count = 0;
	for(PspEntries *pExport = pMod.exports;pExport;){
		fprintf(stdout, "Export %d, Name %s, Functions %d, Variables %d, flags %08X\n", 
				count++, pExport->name, pExport->f_count, pExport->v_count, pExport->stub.flags);

		if(pExport->f_count > 0){
			fprintf(stdout, "Functions:\n");
			for(int i = 0; i < pExport->f_count; i++){

				fprintf(stdout, "0x%08X [0x%08X] - %s", pExport->funcs[i].nid, 
						pExport->funcs[i].addr, pExport->funcs[i].name);
				if(arg->aliased){
					Symbol pSym;
					// prx_getSymbolEntryFromAddr(pExport->funcs[i].addr,&pSym);
					if(pSym.alias > 0){
						if(strcmp(pSym.name, pExport->funcs[i].name)){
							fprintf(stdout, " => %s", pSym.name);
						}else{
							fprintf(stdout, " => %s", *(pSym.alias));
						}
					}
				}
				fprintf(stdout, "\n");
			}
		}

		if(pExport->v_count > 0){
			fprintf(stdout, "Variables:\n");
			for(int i = 0; i < pExport->v_count; i++){
				fprintf(stdout, "0x%08X [0x%08X] - %s\n", pExport->vars[i].nid, 
						pExport->vars[i].addr, pExport->vars[i].name);
			}
		}
	}

	fprintf(stdout, "\nImports:\n");
	count = 0;
	for(PspEntries *pImport = pMod.imports;pImport;){
		fprintf(stdout, "Import %d, Name %s, Functions %d, Variables %d, flags %08X\n", 
				count++, pImport->name, pImport->f_count, pImport->v_count, pImport->stub.flags);

		if(pImport->f_count > 0){
			fprintf(stdout, "Functions:\n");
			for(int i = 0; i < pImport->f_count; i++){
				fprintf(stdout, "0x%08X [0x%08X] - %s\n", 
						pImport->funcs[i].nid, pImport->funcs[i].addr, 
						pImport->funcs[i].name);
			}
		}

		if(pImport->v_count > 0){
			fprintf(stdout, "Variables:\n");
			for(int i = 0; i < pImport->v_count; i++){
				fprintf(stdout, "0x%08X [0x%08X] - %s\n", 
						pImport->vars[i].nid, pImport->vars[i].addr, 
						pImport->vars[i].name);
			}
		}
	}
	return 0;
}

int output_dep      (PrxCtx* prx,FILE *out_fp){
	char path[PATH_MAX];
	int i = 0;
	fprintf(stdout, "Dependency list\n");
	// PrxGetImports(&pHead);
	for(PspEntries*pHead=NULL;pHead;){
		if(strlen(pHead->file) > 0)
			strcpy(path, pHead->file);
		else
			snprintf(path, PATH_MAX, "Unknown (%s)", pHead->name);
		fprintf(stdout, "Dependency %d for%s: %s\n", i++, pHead->name, path);
	}
	return 0;
}

int output_stub     (PrxCtx* prx,FILE *out_fp,PrxToolArg* arg){
	fprintf(stdout,"Library %s\n", prx->module.exports->name);
	if(prx->module.exports->v_count != 0)
		fprintf(stderr, "%s: Stub output does not currently support variables\n", prx->module.exports->name);

	char szPath[PATH_MAX];
	//strcpy(szPath, file);
	strcat(szPath, prx->module.exports->name);
	strcat(szPath, ".S");

	FILE *fp = fopen(szPath, "w");
	if(!fp)
		return fprintf(stderr, "Unable to Open %s\n",szPath),1;
	fprintf(fp, "\t.set noreorder\n\n");
	fprintf(fp, "#include \"pspstub.s\"\n\n");
	fprintf(fp, "\tSTUB_START\t\"%s\",0x%08X,0x%08X\n", prx->module.exports->name, prx->module.exports->stub.flags, (prx->module.exports->f_count << 16) | 5);

	for(int i = 0; i < prx->module.exports->f_count; i++){
		Symbol *pSym = prx?prx_getSymbolEntryFromAddr(prx,prx->module.exports->funcs[i].addr):NULL;
		if((arg->aliased) && (pSym) && (pSym->alias > 0))
			fprintf(fp, "\tSTUB_FUNC_WITH_ALIAS\t0x%08X,%s,%s\n", prx->module.exports->funcs[i].nid, prx->module.exports->funcs[i].name, strcmp(pSym->name, prx->module.exports->funcs[i].name)?pSym->name:pSym->alias[0]);
		else
			fprintf(fp, "\tSTUB_FUNC\t0x%08X,%s\n", prx->module.exports->funcs[i].nid, prx->module.exports->funcs[i].name);
	}

	fprintf(fp, "\tSTUB_END\n");
	fclose(fp);
	return 0;
}

int output_ent      (PrxCtx* prx,FILE *out_fp){
	fprintf(stdout,"Library %s\n", prx->module.exports->name);
	
	fprintf(out_fp, "PSP_EXPORT_START(%s, 0x%04X, 0x%04X)\n", prx->module.exports->name, prx->module.exports->stub.flags & 0xFFFF, prx->module.exports->stub.flags >> 16);

	char nidName[sizeof(prx->module.exports->name) + 10];
	int nidName_size=strlen(prx->module.exports->name) + 10;
	for(int i = 0; i < prx->module.exports->f_count; i++){
		snprintf(nidName, nidName_size, "%s_%08X", prx->module.exports->name, prx->module.exports->funcs[i].nid);
		if (strcmp(nidName, prx->module.exports->funcs[i].name))
			fprintf(out_fp, "PSP_EXPORT_FUNC_HASH(%s)\n", prx->module.exports->funcs[i].name);
		else
			fprintf(out_fp, "PSP_EXPORT_FUNC_NID(%s, 0x%08X)\n", prx->module.exports->funcs[i].name, prx->module.exports->funcs[i].nid);
	}
	for(int i = 0; i < prx->module.exports->v_count; i++){
		snprintf(nidName, nidName_size, "%s_%08X", prx->module.exports->name, prx->module.exports->vars[i].nid);
		if (strcmp(nidName, prx->module.exports->vars[i].name))
			fprintf(out_fp, "PSP_EXPORT_VAR_HASH(%s)\n", prx->module.exports->vars[i].name);
		else
			fprintf(out_fp, "PSP_EXPORT_VAR_NID(%s, 0x%08X)\n", prx->module.exports->vars[i].name, prx->module.exports->vars[i].nid);
	}
	fprintf(out_fp, "PSP_EXPORT_END\n\n");
	return 0;
}

int output_stub2    (PrxCtx* prx,FILE *out_fp,PrxToolArg* arg){
	fprintf(stdout,"Library %s\n", prx->module.exports->name);
	if(!prx->module.exports->v_count)
		fprintf(stderr, "%s: Stub output does not currently support variables\n", prx->module.exports->name);

	char szPath[PATH_MAX];
	//strcpy(szPath, file);
	strcat(szPath, prx->module.exports->name);
	strcat(szPath, ".S");

	FILE *fp = fopen(szPath, "w");
	
	if(!fp)
		return fprintf(stderr, "Unable to Open %s\n",szPath),1;
	
	fprintf(fp, "\t.set noreorder\n\n");
	fprintf(fp, "#include \"pspimport.s\"\n\n");

	fprintf(fp, "// Build List\n");
	fprintf(fp, "// %s_0000.o ", prx->module.exports->name);
	for(int i = 0; i < prx->module.exports->f_count; i++)
		fprintf(fp, "%s_%04d.o ", prx->module.exports->name, i + 1);
	fprintf(fp, "\n\n");

	fprintf(fp, "#ifdef F_%s_0000\n", prx->module.exports->name);
	fprintf(fp, "\tIMPORT_START\t\"%s\",0x%08X\n", prx->module.exports->name, prx->module.exports->stub.flags);
	fprintf(fp, "#endif\n");

	for(int i = 0; i < prx->module.exports->f_count; i++){
		fprintf(fp, "#ifdef F_%s_%04d\n", prx->module.exports->name, i + 1);
		Symbol *pSym = NULL; //pPrx?prx_getSymbolEntryFromAddr(pPrx,prx->module.exports->funcs[i].addr)
		if((arg->aliased) && (pSym) && (pSym->alias > 0))
			fprintf(fp, "\tIMPORT_FUNC_WITH_ALIAS\t\"%s\",0x%08X,%s,%s\n", prx->module.exports->name, prx->module.exports->funcs[i].nid, prx->module.exports->funcs[i].name, strcmp(pSym->name, prx->module.exports->funcs[i].name)?pSym->name:pSym->alias[0]);
		else
			fprintf(fp, "\tIMPORT_FUNC\t\"%s\",0x%08X,%s\n", prx->module.exports->name, prx->module.exports->funcs[i].nid, prx->module.exports->funcs[i].name);
		fprintf(fp, "#endif\n");
	}
	fclose(fp);
	return 0;
}

int output_pstub    (PrxCtx* prx,FILE *out_fp){
	fprintf(stdout, "Dependency list\n");
	// PrxGetExports(&prx,&pHead);
	for(PspEntries*pHead=prx->module.exports;pHead;){
		if(strcmp(pHead->name, PSP_SYSTEM_EXPORT)){
				//write_stub("", pHead, NULL);
		}
	}
	return 0;
}

int output_pstub2   (PrxCtx* prx,FILE *out_fp){
	fprintf(stdout, "Dependency list\n");
	// PrxGetExports(&prx,&pHead);
	for(PspEntries*pHead=prx->module.exports;pHead;){
		if(strcmp(pHead->name, PSP_SYSTEM_EXPORT)){
				//write_stub_new("", pHead, NULL);
		}
	}
	return 0;
}

int output_ents     (PrxCtx* prx,FILE *out_fp){
	fprintf(out_fp, "# Export file automatically generated with prxtool\n");
	fprintf(out_fp, "PSP_BEGIN_EXPORTS\n\n");
	fprintf(stdout, "Dependency list\n");
	
	// PrxGetExports(&prx,&pHead);
	for(PspEntries*pHead=prx->module.exports;pHead;)
		;//write_ent(pHead, out_fp);
	fprintf(out_fp, "PSP_END_EXPORTS\n");
	return 0;
}

int output_stubs_xml(PrxCtx* prx,FILE *out_fp){
/*
	for(Library *pLib = pNids->libs;pLib;){
		// Convery the Library into a valid PspEntries
		PspEntries prx->module.exports={};
		strcpy(prx->module.exports.name, pLib->lib_name);
		prx->module.exports.f_count = pLib->fcount;
		prx->module.exports.v_count = pLib->vcount;
		prx->module.exports.stub.flags = pLib->flags;

		for(int i = 0; i < prx->module.exports.f_count; i++){
			// prx->module.exports.funcs[i].nid = pLib->pNids[i].nid;
			// strcpy(prx->module.exports.funcs[i].name, pLib->pNids[i].name);
		}

		if(arg_out_stubnew)
			write_stub_new("", &prx->module.exports, NULL);
		else
			write_stub("", &prx->module.exports, NULL);
	}
*/
	return 0;
}
