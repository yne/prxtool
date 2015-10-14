int compare_symbols(const void *left, const void *right){
	return ((int) ((ElfSymbol *) left)->value) - ((int) ((ElfSymbol *) right)->value);
}

int output_symbols  (PrxToolCtx* prx,FILE *out_fp){
	ElfSymbol *pSymbols=NULL;
	int symbolsCount=0;//PrxGetSymbols(&pSymbols);
	if(!symbolsCount)
		return fprintf(stderr, "No symbols available"),1;
	SymfileHeader fileHead;
	ElfSymbol pSymCopy[symbolsCount];

	int iSymCopyCount = 0;
	int iStrSize = 0;
	int iStrPos  = 0;
	// Calculate the sizes
	for(int i = 0; i < symbolsCount; i++){
		int type = ELF32_ST_TYPE(pSymbols[i].info);
		if(((type == STT_FUNC) || (type == STT_OBJECT)) && (strlen(pSymbols[i].symname) > 0)){
			memcpy(&pSymCopy[iSymCopyCount++], &pSymbols[i], sizeof(ElfSymbol));
			iStrSize += strlen(pSymbols[i].symname) + 1;
		}
	}

	fprintf(stdout,"Removed %d symbols, leaving %d\n", symbolsCount - iSymCopyCount, iSymCopyCount);
	fprintf(stdout,"String size %d/%d\n", symbolsCount - iSymCopyCount, iSymCopyCount);
	qsort(pSymCopy, iSymCopyCount, sizeof(ElfSymbol), compare_symbols);
	memcpy(fileHead.magic, SYMFILE_MAGIC, 4);
	//memcpy(fileHead.modname, PrxGetModuleInfo()->name, PSP_MODULE_MAX_NAME);
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

int output_disasm   (PrxToolCtx* prx,FILE *out_fp){
	return PrxDump(&prx,out_fp, arg_disopts, arg_xmlOutput);
}

int output_xmldb    (PrxToolCtx* prx,FILE *out_fp){
	fprintf(out_fp, "<?xml version=\"1.0\" ?>\n");
	fprintf(out_fp, "<firmware title=\"%s\">\n", arg_dbTitle);
	PrxDumpXML(prx,out_fp, arg_disopts);
	fprintf(out_fp, "</firmware>\n");
	return 0;
}

int output_mods     (PrxToolCtx* prx,FILE *out_fp){
	//PrxGetModuleInfo(prx);
	fprintf(stdout, "Module information\n");
	fprintf(stdout, "Name:    %s\n", prx.module.info.name);
	fprintf(stdout, "Attrib:  %04X\n", prx.module.info.flags & 0xFFFF);
	fprintf(stdout, "Version: %d.%d\n", (prx.module.info.flags >> 24) & 0xFF, (prx.module.info.flags >> 16) & 0xFF);
	fprintf(stdout, "GP:      %08X\n", prx.module.info.gp);

	fprintf(stdout, "\nExports:\n");
	for(int ex = 0; ex < prx.module.exports_count; ex++)
		fprintf(stdout, "Export %d, Name %s, Functions %d, Variables %d, flags %08X\n", ex, prx.module.exports[ex]->name, prx.module.exports[ex]->f_count, prx.module.exports[ex]->v_count, prx.module.exports[ex]->stub.flags);

	fprintf(stdout, "\nImports:\n");
	for(int im = 0; im < prx.module.imports_count; im++)
		fprintf(stdout, "Import %d, Name %s, Functions %d, Variables %d, flags %08X\n", im, prx.module.imports[im]->name, prx.module.imports[im]->f_count, prx.module.imports[im]->v_count, prx.module.imports[im]->stub.flags);
	return 0;
}

int output_elf      (PrxToolCtx* prx,FILE *out_fp){//TODO
	if(!PrxToElf(prx,out_fp))
		return fprintf(stderr, "Failed to create a fixed up ELF\n"),1;
	return 0;
}

int output_xml      (PrxToolCtx* prx,FILE *out_fp){
	return 0;
}

int output_map      (PrxToolCtx* prx,FILE *out_fp){
	return 0;
}

int output_idc      (PrxToolCtx* prx,FILE *out_fp){
	return 0;
}

int output_impexp   (PrxToolCtx* prx,FILE *out_fp){
	PrxToolCtx prx;

	PrxSetNidMgr(&prx,pNids);
	if(!PrxLoadFromFile(&prx,file))
		return fprintf(stderr, "Couldn't load prx file structures\n"),1;
	PspModule pMod;
	//PrxGetModuleInfo(&pMod);
	fprintf(stdout, "Module information\n");
	fprintf(stdout, "Name:    %s\n", pMod.info.name);
	fprintf(stdout, "Attrib:  %04X\n", pMod.info.flags & 0xFFFF);
	fprintf(stdout, "Version: %d.%d\n", (pMod.info.flags >> 24) & 0xFF, (pMod.info.flags >> 16) & 0xFF);
	fprintf(stdout, "GP:      %08X\n", pMod.info.gp);

	fprintf(stdout, "\nExports:\n");
	
	int count = 0;
	for(PspEntries *pExport = pMod.exports;pExport;/*pExport = pExport->next;*/){
		fprintf(stdout, "Export %d, Name %s, Functions %d, Variables %d, flags %08X\n", 
				count++, pExport->name, pExport->f_count, pExport->v_count, pExport->stub.flags);

		if(pExport->f_count > 0){
			fprintf(stdout, "Functions:\n");
			for(int i = 0; i < pExport->f_count; i++){

				fprintf(stdout, "0x%08X [0x%08X] - %s", pExport->funcs[i].nid, 
						pExport->funcs[i].addr, pExport->funcs[i].name);
				if(arg_aliasOutput){
					Symbol pSym;
					//PrxGetSymbolEntryFromAddr(pExport->funcs[i].addr,&pSym);
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
	for(PspEntries *pImport = pMod.imports;pImport;/*pImport = pImport->next*/){
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

int output_deps     (PrxToolCtx* prx,FILE *out_fp){
	PrxToolCtx prx;

	PrxSetNidMgr(&prx,pNids);
	if(!PrxLoadFromFile(&prx,file))
		return fprintf(stderr, "Couldn't load prx file structures\n"),1;
	
	char path[PATH_MAX];
	int i = 0;
	fprintf(stdout, "Dependency list for%s\n", file);
	//PrxGetImports(&pHead);
	for(PspEntries*pHead=NULL;pHead;/*pHead = pHead.next*/){
		if(strlen(pHead->file) > 0)
			strcpy(path, pHead->file);
		else
			snprintf(path, PATH_MAX, "Unknown (%s)", pHead->name);
		fprintf(stdout, "Dependency %d for%s: %s\n", i++, pHead->name, path);
	}
	return 0;
}

int output_stub     (PrxToolCtx* prx,FILE *out_fp){
	fprintf(stdout,"Library %s\n", pExp->name);
	if(pExp->v_count != 0)
		fprintf(stderr, "%s: Stub output does not currently support variables\n", pExp->name);

	char szPath[PATH_MAX];
	strcpy(szPath, file);
	strcat(szPath, pExp->name);
	strcat(szPath, ".S");

	FILE *fp = fopen(szPath, "w");
	if(!fp)
		return fprintf(stderr, "Unable to Open %s\n",szPath),1;
	fprintf(fp, "\t.set noreorder\n\n");
	fprintf(fp, "#include \"pspstub.s\"\n\n");
	fprintf(fp, "\tSTUB_START\t\"%s\",0x%08X,0x%08X\n", pExp->name, pExp->stub.flags, (pExp->f_count << 16) | 5);

	for(int i = 0; i < pExp->f_count; i++){
		Symbol *pSym = pPrx?PrxGetSymbolEntryFromAddr(pPrx,pExp->funcs[i].addr):NULL;
		if((arg_aliasOutput) && (pSym) && (pSym->alias > 0))
			fprintf(fp, "\tSTUB_FUNC_WITH_ALIAS\t0x%08X,%s,%s\n", pExp->funcs[i].nid, pExp->funcs[i].name, strcmp(pSym->name, pExp->funcs[i].name)?pSym->name:pSym->alias[0]);
		else
			fprintf(fp, "\tSTUB_FUNC\t0x%08X,%s\n", pExp->funcs[i].nid, pExp->funcs[i].name);
	}

	fprintf(fp, "\tSTUB_END\n");
	fclose(fp);
	return 0;
}

int output_ent      (PrxToolCtx* prx,FILE *out_fp){
	fprintf(stdout,"Library %s\n", pExp->name);
	
	fprintf(out_fp, "PSP_EXPORT_START(%s, 0x%04X, 0x%04X)\n", pExp->name, pExp->stub.flags & 0xFFFF, pExp->stub.flags >> 16);

	char nidName[sizeof(pExp->name) + 10];
	int nidName_size=strlen(pExp->name) + 10;
	for(int i = 0; i < pExp->f_count; i++){
		snprintf(nidName, nidName_size, "%s_%08X", pExp->name, pExp->funcs[i].nid);
		if (strcmp(nidName, pExp->funcs[i].name))
			fprintf(out_fp, "PSP_EXPORT_FUNC_HASH(%s)\n", pExp->funcs[i].name);
		else
			fprintf(out_fp, "PSP_EXPORT_FUNC_NID(%s, 0x%08X)\n", pExp->funcs[i].name, pExp->funcs[i].nid);
	}
	for(int i = 0; i < pExp->v_count; i++){
		snprintf(nidName, nidName_size, "%s_%08X", pExp->name, pExp->vars[i].nid);
		if (strcmp(nidName, pExp->vars[i].name))
			fprintf(out_fp, "PSP_EXPORT_VAR_HASH(%s)\n", pExp->vars[i].name);
		else
			fprintf(out_fp, "PSP_EXPORT_VAR_NID(%s, 0x%08X)\n", pExp->vars[i].name, pExp->vars[i].nid);
	}
	fprintf(out_fp, "PSP_EXPORT_END\n\n");
	return 0;
}

int output_stub_new (PrxToolCtx* prx,FILE *out_fp){
	fprintf(stdout,"Library %s\n", pExp->name);
	if(!pExp->v_count)
		fprintf(stderr, "%s: Stub output does not currently support variables\n", pExp->name);

	char szPath[PATH_MAX];
	strcpy(szPath, file);
	strcat(szPath, pExp->name);
	strcat(szPath, ".S");

	FILE *fp = fopen(szPath, "w");
	
	if(!fp)
		return fprintf(stderr, "Unable to Open %s\n",szPath),1;
	
	fprintf(fp, "\t.set noreorder\n\n");
	fprintf(fp, "#include \"pspimport.s\"\n\n");

	fprintf(fp, "// Build List\n");
	fprintf(fp, "// %s_0000.o ", pExp->name);
	for(int i = 0; i < pExp->f_count; i++)
		fprintf(fp, "%s_%04d.o ", pExp->name, i + 1);
	fprintf(fp, "\n\n");

	fprintf(fp, "#ifdef F_%s_0000\n", pExp->name);
	fprintf(fp, "\tIMPORT_START\t\"%s\",0x%08X\n", pExp->name, pExp->stub.flags);
	fprintf(fp, "#endif\n");

	for(int i = 0; i < pExp->f_count; i++){
		fprintf(fp, "#ifdef F_%s_%04d\n", pExp->name, i + 1);
		Symbol *pSym = /*pPrx?PrxGetSymbolEntryFromAddr(pPrx,pExp->funcs[i].addr):*/NULL;
		if((arg_aliasOutput) && (pSym) && (pSym->alias > 0))
			fprintf(fp, "\tIMPORT_FUNC_WITH_ALIAS\t\"%s\",0x%08X,%s,%s\n", pExp->name, pExp->funcs[i].nid, pExp->funcs[i].name, strcmp(pSym->name, pExp->funcs[i].name)?pSym->name:pSym->alias[0]);
		else
			fprintf(fp, "\tIMPORT_FUNC\t\"%s\",0x%08X,%s\n", pExp->name, pExp->funcs[i].nid, pExp->funcs[i].name);
		fprintf(fp, "#endif\n");
	}
	fclose(fp);
	return 0;
}

int output_stubs_prx(PrxToolCtx* prx,FILE *out_fp){
	PrxToolCtx prx;
	PrxSetNidMgr(&prx,pNids);
	if(!PrxLoadFromFile(&prx,file))
		return fprintf(stderr, "Couldn't load prx file structures\n"),1;
	fprintf(stdout, "Dependency list for%s\n", file);
	
	//PrxGetExports(&prx,&pHead);
	for(PspEntries*pHead=prx.module.exports;pHead;/*pHead = pHead->next*/){
		if(strcmp(pHead->name, PSP_SYSTEM_EXPORT)){
			if(arg_out_pstubnew)
				write_stub_new("", pHead, NULL);
			else
				write_stub("", pHead, NULL);
		}
	}
	return 0;
}

int output_ents     (PrxToolCtx* prx,FILE *out_fp){
	PrxToolCtx prx;

	fprintf(f, "# Export file automatically generated with prxtool\n");
	fprintf(f, "PSP_BEGIN_EXPORTS\n\n");
	PrxSetNidMgr(&prx,pNids);
	if(!PrxLoadFromFile(&prx,file))
		return fprintf(stderr, "Couldn't load prx file structures\n"),1;
	fprintf(stdout, "Dependency list for%s\n", file);
	
	//PrxGetExports(&prx,&pHead);
	for(PspEntries*pHead=prx.module.exports;pHead;/*pHead = pHead->next*/)
		write_ent(pHead, f);
	fprintf(f, "PSP_END_EXPORTS\n");
	return 0;
}

int output_stubs_xml(PrxToolCtx* prx,FILE *out_fp){
	for(LibraryEntry *pLib = pNids->libraries;pLib;/*pLib = pLib->pNext;*/){
		// Convery the LibraryEntry into a valid PspEntries
		PspEntries pExp={};
		strcpy(pExp.name, pLib->lib_name);
		pExp.f_count = pLib->fcount;
		pExp.v_count = pLib->vcount;
		pExp.stub.flags = pLib->flags;

		for(int i = 0; i < pExp.f_count; i++){
			//pExp.funcs[i].nid = pLib->pNids[i].nid;
			//strcpy(pExp.funcs[i].name, pLib->pNids[i].name);
		}

		if(arg_out_stubnew)
			write_stub_new("", &pExp, NULL);
		else
			write_stub("", &pExp, NULL);
	}
	return 0;
}
