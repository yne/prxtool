int compare_symbols(const void *left, const void *right){
	return ((int) ((ElfSymbol *) left)->value) - ((int) ((ElfSymbol *) right)->value);
}

int output_symbols2(ElfSymbol *pSymbols,int iSymCount, FILE*out_fp){
	SymfileHeader fileHead;
	ElfSymbol pSymCopy[iSymCount];

	int iSymCopyCount = 0;
	int iStrSize = 0;
	int iStrPos  = 0;
	// Calculate the sizes
	for(int i = 0; i < iSymCount; i++){
		int type = ELF32_ST_TYPE(pSymbols[i].info);
		if(((type == STT_FUNC) || (type == STT_OBJECT)) && (strlen(pSymbols[i].symname) > 0)){
			memcpy(&pSymCopy[iSymCopyCount], &pSymbols[i], sizeof(ElfSymbol));
			iSymCopyCount++;
			iStrSize += strlen(pSymbols[i].symname) + 1;
		}
	}

	fprintf(stdout,"Removed %d symbols, leaving %d\n", iSymCount - iSymCopyCount, iSymCopyCount);
	fprintf(stdout,"String size %d/%d\n", iSymCount - iSymCopyCount, iSymCopyCount);
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

int output_symbols(const char *file, FILE *out_fp){
	CProcessPrx prx;

	fprintf(stdout, "Loading %s\n", file);
	if(!PrxLoadFromFile(&prx,file))
		return fprintf(stderr, "Couldn't load elf file structures"),0;
	ElfSymbol *pSymbols;
	int iSymCount=0;//PrxGetSymbols(&pSymbols);
	if(iSymCount)
		output_symbols2(pSymbols, iSymCount, out_fp);
	else
		fprintf(stderr, "No symbols available");
	return 0;
}

int output_disasm(const char *file, FILE *out_fp, DataBase *nids){
	CProcessPrx prx;
	int blRet;

	fprintf(stdout, "Loading %s\n", file);
	PrxSetNidMgr(&prx, nids);
	if(arg_loadbin){
		blRet = PrxLoadFromBinFile(&prx,file, arg_database);
	}else{
		blRet = PrxLoadFromFile(&prx,file);
	}

	if(arg_xmlOutput){
		//PrxSetXmlDump();
	}

	if(!blRet)
		return fprintf(stderr, "Couldn't load elf file structures"),1;
	PrxDump(&prx,out_fp, arg_disopts);
	return 0;
}

int output_xmldb(const char *file, FILE *out_fp, DataBase *nids){
	CProcessPrx prx;
	PrxSetNidMgr(&prx,nids);

	if(!(arg_loadbin?PrxLoadFromBinFile(&prx,file, arg_database):PrxLoadFromFile(&prx,file)))
		return fprintf(stderr, "Couldn't load elf file structures"),1;
	PrxDumpXML(&prx,out_fp, arg_disopts);
	return 0;
}

int output_mods(const char *file, DataBase *pNids){
	CProcessPrx prx;
	PrxSetNidMgr(&prx,pNids);

	if(!PrxLoadFromFile(&prx,file))
		return fprintf(stderr, "Couldn't load prx file structures\n"),1;

	PspModule pMod;
	//PrxGetModuleInfo(&pMod);
	fprintf(stdout, "Module information\n");
	fprintf(stdout, "Name:    %s\n", pMod.name);
	fprintf(stdout, "Attrib:  %04X\n", pMod.info.flags & 0xFFFF);
	fprintf(stdout, "Version: %d.%d\n", (pMod.info.flags >> 24) & 0xFF, (pMod.info.flags >> 16) & 0xFF);
	fprintf(stdout, "GP:      %08X\n", pMod.info.gp);

	fprintf(stdout, "\nExports:\n");
	int count = 0;
	for(PspEntries *pExport = pMod.exports;pExport;/*pExport = pExport->next*/)
		fprintf(stdout, "Export %d, Name %s, Functions %d, Variables %d, flags %08X\n", 
				count++, pExport->name, pExport->f_count, pExport->v_count, pExport->stub.flags);

	fprintf(stdout, "\nImports:\n");
	count = 0;
	for(PspEntries *pImport=pMod.imports;pImport;/*pImport = pImport->next*/)
		fprintf(stdout, "Import %d, Name %s, Functions %d, Variables %d, flags %08X\n", 
				count++, pImport->name, pImport->f_count, pImport->v_count, pImport->stub.flags);
	return 0;
}

int output_elf(const char *file, FILE *out_fp){//TODO
	CProcessPrx prx;

	if(!PrxLoadFromFile(&prx,file))
		return fprintf(stderr, "Couldn't load prx file structures\n"),1;
	if(!PrxPrxToElf(&prx,out_fp))
		return fprintf(stderr, "Failed to create a fixed up ELF\n"),1;
	return 0;
}

int serialize_file(const char *file, DataBase *pNids/* CSerializePrx *pSer */){
	CProcessPrx prx;

	PrxSetNidMgr(&prx,pNids);
	if(!PrxLoadFromFile(&prx,file))
		return fprintf(stderr, "Couldn't load prx file structures\n"),1;
	//PrxSerSerializePrx(prx, arg_iSMask);
	return 0;
}

int output_importexport(const char *file, DataBase *pNids){
	CProcessPrx prx;

	PrxSetNidMgr(&prx,pNids);
	if(!PrxLoadFromFile(&prx,file))
		return fprintf(stderr, "Couldn't load prx file structures\n"),1;
	PspModule pMod;
	//PrxGetModuleInfo(&pMod);
	fprintf(stdout, "Module information\n");
	fprintf(stdout, "Name:    %s\n", pMod.name);
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
					SymbolEntry pSym;
					//PrxGetSymbolEntryFromAddr(pExport->funcs[i].addr,&pSym);
					if(pSym.alias > 0){
						if(strcmp(pSym.name, pExport->funcs[i].name)){
							fprintf(stdout, " => %s", pSym.name);
						}else{
							fprintf(stdout, " => %s", pSym.alias);
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

int output_deps(const char *file, DataBase *pNids){
	CProcessPrx prx;

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
}

int write_stub(const char *szDirectory, PspEntries *pExp, CProcessPrx *pPrx){
	fprintf(stdout,"Library %s\n", pExp->name);
	if(pExp->v_count != 0)
		fprintf(stderr, "%s: Stub output does not currently support variables\n", pExp->name);

	char szPath[PATH_MAX];
	strcpy(szPath, szDirectory);
	strcat(szPath, pExp->name);
	strcat(szPath, ".S");

	FILE *fp = fopen(szPath, "w");
	if(!fp)
		return fprintf(stderr, "Unable to Open %s\n",szPath),1;
	fprintf(fp, "\t.set noreorder\n\n");
	fprintf(fp, "#include \"pspstub.s\"\n\n");
	fprintf(fp, "\tSTUB_START\t\"%s\",0x%08X,0x%08X\n", pExp->name, pExp->stub.flags, (pExp->f_count << 16) | 5);

	for(int i = 0; i < pExp->f_count; i++){
		SymbolEntry *pSym = pPrx?PrxGetSymbolEntryFromAddr(pPrx,pExp->funcs[i].addr):NULL;
		if((arg_aliasOutput) && (pSym) && (pSym->alias > 0))
			fprintf(fp, "\tSTUB_FUNC_WITH_ALIAS\t0x%08X,%s,%s\n", pExp->funcs[i].nid, pExp->funcs[i].name, strcmp(pSym->name, pExp->funcs[i].name)?pSym->name:pSym->alias);
		else
			fprintf(fp, "\tSTUB_FUNC\t0x%08X,%s\n", pExp->funcs[i].nid, pExp->funcs[i].name);
	}

	fprintf(fp, "\tSTUB_END\n");
	fclose(fp);
	return 0;
}

int write_ent(PspEntries *pExp, FILE *fp){
	fprintf(stdout,"Library %s\n", pExp->name);
	if(!fp)
		return fprintf(stderr, "Unable to Open stream\n"),1;
	
	fprintf(fp, "PSP_EXPORT_START(%s, 0x%04X, 0x%04X)\n", pExp->name, pExp->stub.flags & 0xFFFF, pExp->stub.flags >> 16);

	char nidName[sizeof(pExp->name) + 10];
	int nidName_size=strlen(pExp->name) + 10;
	for(int i = 0; i < pExp->f_count; i++){
		snprintf(nidName, nidName_size, "%s_%08X", pExp->name, pExp->funcs[i].nid);
		if (strcmp(nidName, pExp->funcs[i].name))
			fprintf(fp, "PSP_EXPORT_FUNC_HASH(%s)\n", pExp->funcs[i].name);
		else
			fprintf(fp, "PSP_EXPORT_FUNC_NID(%s, 0x%08X)\n", pExp->funcs[i].name, pExp->funcs[i].nid);
	}
	for(int i = 0; i < pExp->v_count; i++){
		snprintf(nidName, nidName_size, "%s_%08X", pExp->name, pExp->vars[i].nid);
		if (strcmp(nidName, pExp->vars[i].name))
			fprintf(fp, "PSP_EXPORT_VAR_HASH(%s)\n", pExp->vars[i].name);
		else
			fprintf(fp, "PSP_EXPORT_VAR_NID(%s, 0x%08X)\n", pExp->vars[i].name, pExp->vars[i].nid);
	}
	fprintf(fp, "PSP_EXPORT_END\n\n");
	return 0;
}

int write_stub_new(const char *szDirectory, PspEntries *pExp, CProcessPrx *pPrx){
	fprintf(stdout,"Library %s\n", pExp->name);
	if(!pExp->v_count)
		fprintf(stderr, "%s: Stub output does not currently support variables\n", pExp->name);

	char szPath[PATH_MAX];
	strcpy(szPath, szDirectory);
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
		SymbolEntry *pSym = /*pPrx?PrxGetSymbolEntryFromAddr(pPrx,pExp->funcs[i].addr):*/NULL;
		if((arg_aliasOutput) && (pSym) && (pSym->alias > 0))
			fprintf(fp, "\tIMPORT_FUNC_WITH_ALIAS\t\"%s\",0x%08X,%s,%s\n", pExp->name, pExp->funcs[i].nid, pExp->funcs[i].name, strcmp(pSym->name, pExp->funcs[i].name)?pSym->name:pSym->alias);
		else
			fprintf(fp, "\tIMPORT_FUNC\t\"%s\",0x%08X,%s\n", pExp->name, pExp->funcs[i].nid, pExp->funcs[i].name);
		fprintf(fp, "#endif\n");
	}
	fclose(fp);
	return 0;
}

int output_stubs_prx(const char *file, DataBase *pNids){
	CProcessPrx prx;
	PrxSetNidMgr(&prx,pNids);
	if(!PrxLoadFromFile(&prx,file))
		return fprintf(stderr, "Couldn't load prx file structures\n"),1;
	fprintf(stdout, "Dependency list for%s\n", file);
	
	//PrxGetExports(&prx,&pHead);
	for(PspEntries*pHead=prx.modInfo.exports;pHead;/*pHead = pHead->next*/){
		if(strcmp(pHead->name, PSP_SYSTEM_EXPORT)){
			if(arg_out_pstubnew)
				write_stub_new("", pHead, NULL);
			else
				write_stub("", pHead, NULL);
		}
	}
	return 0;
}

int output_ents(const char *file, DataBase *pNids, FILE *f){
	CProcessPrx prx;

	PrxSetNidMgr(&prx,pNids);
	if(!PrxLoadFromFile(&prx,file))
		return fprintf(stderr, "Couldn't load prx file structures\n"),1;
	fprintf(stdout, "Dependency list for%s\n", file);
	
	//PrxGetExports(&prx,&pHead);
	for(PspEntries*pHead=prx.modInfo.exports;pHead;/*pHead = pHead->next*/)
		write_ent(pHead, f);
}

int output_stubs_xml(DataBase *pNids){
	PspEntries *pExp;

	for(LibraryEntry *pLib = pNids->libraries;pLib;/*pLib = pLib->pNext;*/){
		// Convery the LibraryEntry into a valid PspEntries
		memset(pExp, 0, sizeof(PspEntries));
		strcpy(pExp->name, pLib->lib_name);
		pExp->f_count = pLib->fcount;
		pExp->v_count = pLib->vcount;
		pExp->stub.flags = pLib->flags;

		for(int i = 0; i < pExp->f_count; i++){
			//pExp->funcs[i].nid = pLib->pNids[i].nid;
			//strcpy(pExp->funcs[i].name, pLib->pNids[i].name);
		}

		if(arg_out_stubnew)
			write_stub_new("", pExp, NULL);
		else
			write_stub("", pExp, NULL);
	}
	return 0;
}
