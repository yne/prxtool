int compare_symbols(const void *left, const void *right){
	return ((int) ((ElfSymbol *) left)->value) - ((int) ((ElfSymbol *) right)->value);
}
void output_symbols2(ElfSymbol *pSymbols,int iSymCount, FILE*out_fp){
	int iSymCopyCount;
	int iStrSize;
	int iStrPos;
	SymfileHeader fileHead;
	ElfSymbol pSymCopy[iSymCount];

	iSymCopyCount = 0;
	iStrSize = 0;
	iStrPos  = 0;
	// Calculate the sizes
	for(int i = 0; i < iSymCount; i++){
		int type;

		type = ELF32_ST_TYPE(pSymbols[i].info);
		if(((type == STT_FUNC) || (type == STT_OBJECT)) && (strlen(pSymbols[i].symname) > 0)){
			memcpy(&pSymCopy[iSymCopyCount], &pSymbols[i], sizeof(ElfSymbol));
			iSymCopyCount++;
			iStrSize += strlen(pSymbols[i].symname) + 1;
		}
	}

	fprintf(stdout,"Removed %d symbols, leaving %d\n", iSymCount - iSymCopyCount, iSymCopyCount);
	fprintf(stdout,"String size %d\n", iSymCount - iSymCopyCount, iSymCopyCount);
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
	for(int i = 0; i < iSymCopyCount; i++){
		fwrite(pSymCopy[i].symname, 1, strlen(pSymCopy[i].symname)+1, out_fp);
	}
}
void output_symbols(const char *file, FILE *out_fp){
	//CProcessPrx prx(arg_dwBase);

	fprintf(stdout, "Loading %s\n", file);
	if(PrxLoadFromFile(file) == 0){
		fprintf(stderr, "Couldn't load elf file structures");
	}else{
		ElfSymbol *pSymbols;
		int iSymCount=0;//PrxGetSymbols(&pSymbols);
		if(iSymCount){
			output_symbols2(pSymbols, iSymCount, out_fp);
		}else{
			fprintf(stderr, "No symbols available");
		}
	}
}
void output_disasm(const char *file, FILE *out_fp/*, CNidMgr *nids*/){
	//CProcessPrx prx(arg_dwBase);
	int blRet;

	fprintf(stdout, "Loading %s\n", file);
	//PrxSetNidMgr(nids);
	if(arg_loadbin){
		blRet = PrxLoadFromBinFile(file, arg_database);
	}else{
		blRet = PrxLoadFromFile(file);
	}

	if(arg_xmlOutput){
		//PrxSetXmlDump();
	}

	if(blRet == 0){
		fprintf(stderr, "Couldn't load elf file structures");
	}else{
		PrxDump(out_fp, arg_disopts);
	}
}
void output_xmldb(const char *file, FILE *out_fp/*, CNidMgr *nids*/){
	//CProcessPrx prx(arg_dwBase);
	int blRet;

	fprintf(stdout, "Loading %s\n", file);
//	PrxSetNidMgr(nids);
	if(arg_loadbin){
		blRet = PrxLoadFromBinFile(file, arg_database);
	}else{
		blRet = PrxLoadFromFile(file);
	}

	if(blRet == 0){
		fprintf(stderr, "Couldn't load elf file structures");
	}else{
		PrxDumpXML(out_fp, arg_disopts);
	}
}
void output_mods(const char *file/*, CNidMgr *pNids*/){
	//CProcessPrx prx(arg_dwBase);

	//PrxSetNidMgr(pNids);
	if(PrxLoadFromFile(file) == 0){
		fprintf(stderr, "Couldn't load prx file structures\n");
	}else{
		PspModule pMod;
		PspLibExport *pExport;
		PspLibImport *pImport;
		int count;

		//PrxGetModuleInfo(&pMod);
		fprintf(stdout, "Module information\n");
		fprintf(stdout, "Name:    %s\n", pMod.name);
		fprintf(stdout, "Attrib:  %04X\n", pMod.info.flags & 0xFFFF);
		fprintf(stdout, "Version: %d.%d\n", 
				(pMod.info.flags >> 24) & 0xFF, (pMod.info.flags >> 16) & 0xFF);
		fprintf(stdout, "GP:      %08X\n", pMod.info.gp);

		fprintf(stdout, "\nExports:\n");
		pExport = pMod.exp_head;
		count = 0;
		while(pExport != NULL){
			fprintf(stdout, "Export %d, Name %s, Functions %d, Variables %d, flags %08X\n", 
					count++, pExport->name, pExport->f_count, pExport->v_count, pExport->stub.flags);
			pExport = pExport->next;
		}

		fprintf(stdout, "\nImports:\n");
		pImport = pMod.imp_head;
		count = 0;
		while(pImport != NULL){
			fprintf(stdout, "Import %d, Name %s, Functions %d, Variables %d, flags %08X\n", 
					count++, pImport->name, pImport->f_count, pImport->v_count, pImport->stub.flags);
			pImport = pImport->next;
		}

	}
}
void output_elf(const char *file, FILE *out_fp){//TODO
	//CProcessPrx prx(arg_dwBase);

	fprintf(stdout, "Loading %s\n", file);
	if(PrxLoadFromFile(file) == 0){
		fprintf(stderr, "Couldn't load prx file structures\n");
	}else{
		if(PrxPrxToElf(out_fp) == 0){
			fprintf(stderr, "Failed to create a fixed up ELF\n");
		}
	}
}
void serialize_file(const char *file/*, CSerializePrx *pSer, CNidMgr *pNids*/){
	//CProcessPrx prx(arg_dwBase);

	//PrxSetNidMgr(pNids);
	fprintf(stdout, "Loading %s\n", file);
	if(PrxLoadFromFile(file) == 0){
		fprintf(stderr, "Couldn't load prx file structures\n");
	}else{
		//PrxSerSerializePrx(prx, arg_iSMask);
	}
}

typedef enum{
	SYMBOL_NOSYM = 0,
	SYMBOL_UNK,
	SYMBOL_FUNC,
	SYMBOL_LOCAL,
	SYMBOL_DATA,
}SymbolType;

typedef struct{
	unsigned int addr;
	SymbolType type;
	unsigned int size;
	char* name;
	unsigned int*refs;
	char** alias;
	PspLibExport* exported;
	PspLibImport* imported;
}SymbolEntry;


struct ImmEntry{
	unsigned int addr;
	unsigned int target;
	/* Does this entry point to a text section ? */
	int text;
};

void output_importexport(const char *file/*, CNidMgr *pNids*/){
//	CProcessPrx prx(arg_dwBase);
	int iLoop;

//	PrxSetNidMgr(pNids);
	if(PrxLoadFromFile(file) == 0){
		fprintf(stderr, "Couldn't load prx file structures\n");
	}else{
		PspModule pMod;
		PspLibExport *pExport;
		PspLibImport *pImport;
		int count;

		//PrxGetModuleInfo(&pMod);
		fprintf(stdout, "Module information\n");
		fprintf(stdout, "Name:    %s\n", pMod.name);
		fprintf(stdout, "Attrib:  %04X\n", pMod.info.flags & 0xFFFF);
		fprintf(stdout, "Version: %d.%d\n", 
				(pMod.info.flags >> 24) & 0xFF, (pMod.info.flags >> 16) & 0xFF);
		fprintf(stdout, "GP:      %08X\n", pMod.info.gp);

		fprintf(stdout, "\nExports:\n");
		pExport = pMod.exp_head;
		count = 0;
		while(pExport != NULL){
			fprintf(stdout, "Export %d, Name %s, Functions %d, Variables %d, flags %08X\n", 
					count++, pExport->name, pExport->f_count, pExport->v_count, pExport->stub.flags);

			if(pExport->f_count > 0){
				fprintf(stdout, "Functions:\n");
				for(iLoop = 0; iLoop < pExport->f_count; iLoop++){

					fprintf(stdout, "0x%08X [0x%08X] - %s", pExport->funcs[iLoop].nid, 
							pExport->funcs[iLoop].addr, pExport->funcs[iLoop].name);
					if(arg_aliasOutput){
						SymbolEntry pSym;
						//PrxGetSymbolEntryFromAddr(pExport->funcs[iLoop].addr,&pSym);
						if(pSym.alias > 0){
							if(strcmp(pSym.name, pExport->funcs[iLoop].name)){
								fprintf(stdout, " => %s", pSym.name);
							}else{
								fprintf(stdout, " => %s", pSym.alias[0]);
							}
						}
					}
					fprintf(stdout, "\n");
				}
			}

			if(pExport->v_count > 0){
				fprintf(stdout, "Variables:\n");
				for(iLoop = 0; iLoop < pExport->v_count; iLoop++){
					fprintf(stdout, "0x%08X [0x%08X] - %s\n", pExport->vars[iLoop].nid, 
							pExport->vars[iLoop].addr, pExport->vars[iLoop].name);
				}
			}

			pExport = pExport->next;
		}

		fprintf(stdout, "\nImports:\n");
		pImport = pMod.imp_head;
		count = 0;
		while(pImport != NULL){
			fprintf(stdout, "Import %d, Name %s, Functions %d, Variables %d, flags %08X\n", 
					count++, pImport->name, pImport->f_count, pImport->v_count, pImport->stub.flags);

			if(pImport->f_count > 0){
				fprintf(stdout, "Functions:\n");
				for(iLoop = 0; iLoop < pImport->f_count; iLoop++){
					fprintf(stdout, "0x%08X [0x%08X] - %s\n", 
							pImport->funcs[iLoop].nid, pImport->funcs[iLoop].addr, 
							pImport->funcs[iLoop].name);
				}
			}

			if(pImport->v_count > 0){
				fprintf(stdout, "Variables:\n");
				for(iLoop = 0; iLoop < pImport->v_count; iLoop++){
					fprintf(stdout, "0x%08X [0x%08X] - %s\n", 
							pImport->vars[iLoop].nid, pImport->vars[iLoop].addr, 
							pImport->vars[iLoop].name);
				}
			}

			pImport = pImport->next;
		}

	}

}
void output_deps(const char *file/*, CNidMgr *pNids*/){
//	CProcessPrx prx(arg_dwBase);

//	PrxSetNidMgr(pNids);
	if(PrxLoadFromFile(file) == 0){
		fprintf(stderr, "Couldn't load prx file structures\n");
	}else{
		PspLibImport pHead;
		char path[PATH_MAX];
		int i;

		i = 0;
		fprintf(stdout, "Dependancy list for %s\n", file);
		//PrxGetImports(&pHead);
		while(pHead){
			if(strlen(pHead.file) > 0){
				strcpy(path, pHead.file);
			}else{
				snprintf(path, PATH_MAX, "Unknown (%s)", pHead.name);
			}
			fprintf(stdout, "Dependancy %d for %s: %s\n", i++, pHead.name, path);
			//pHead = pHead.next;
		}
	}
}
void write_stub(const char *szDirectory, PspLibExport *pExp/*, CProcessPrx *pPrx*/){
	char szPath[PATH_MAX];
	FILE *fp;
	fprintf(stdout,"Library %s\n", pExp->name);
	if(pExp->v_count != 0){
		fprintf(stderr, "%s: Stub output does not currently support variables\n", pExp->name);
	}

	strcpy(szPath, szDirectory);
	strcat(szPath, pExp->name);
	strcat(szPath, ".S");

	fp = fopen(szPath, "w");
	if(fp != NULL){
		fprintf(fp, "\t.set noreorder\n\n");
		fprintf(fp, "#include \"pspstub.s\"\n\n");
		fprintf(fp, "\tSTUB_START\t\"%s\",0x%08X,0x%08X\n", pExp->name, pExp->stub.flags, (pExp->f_count << 16) | 5);

		for(int i = 0; i < pExp->f_count; i++){
			SymbolEntry *pSym;

			// if(pPrx){
				// pSym = pPrx->GetSymbolEntryFromAddr(pExp->funcs[i].addr);
			// }else{
				// pSym = NULL;
			// }

			if((arg_aliasOutput) && (pSym) && (pSym->alias > 0)){
				if(strcmp(pSym->name, pExp->funcs[i].name)){
					fprintf(fp, "\tSTUB_FUNC_WITH_ALIAS\t0x%08X,%s,%s\n", pExp->funcs[i].nid, pExp->funcs[i].name,
							pSym->name);
				}else{
					fprintf(fp, "\tSTUB_FUNC_WITH_ALIAS\t0x%08X,%s,%s\n", pExp->funcs[i].nid, pExp->funcs[i].name,
							pSym->alias[0]);
				}
			}else{
				fprintf(fp, "\tSTUB_FUNC\t0x%08X,%s\n", pExp->funcs[i].nid, pExp->funcs[i].name);
			}
		}

		fprintf(fp, "\tSTUB_END\n");
		fclose(fp);
	}
}
void write_ent(PspLibExport *pExp, FILE *fp){
	char szPath[PATH_MAX];
	fprintf(stdout,"Library %s\n", pExp->name);
	if(fp != NULL){
		int i;
		char *nidName = (char*)malloc(strlen(pExp->name) + 10);

		fprintf(fp, "PSP_EXPORT_START(%s, 0x%04X, 0x%04X)\n", pExp->name, pExp->stub.flags & 0xFFFF, pExp->stub.flags >> 16);

		for(i = 0; i < pExp->f_count; i++){
			sprintf(nidName, "%s_%08X", pExp->name, pExp->funcs[i].nid);
			if (strcmp(nidName, pExp->funcs[i].name) == 0)
				fprintf(fp, "PSP_EXPORT_FUNC_NID(%s, 0x%08X)\n", pExp->funcs[i].name, pExp->funcs[i].nid);
			else
				fprintf(fp, "PSP_EXPORT_FUNC_HASH(%s)\n", pExp->funcs[i].name);
		}
		for (i = 0; i < pExp->v_count; i++){
			sprintf(nidName, "%s_%08X", pExp->name, pExp->vars[i].nid);
			if (strcmp(nidName, pExp->vars[i].name) == 0)
				fprintf(fp, "PSP_EXPORT_VAR_NID(%s, 0x%08X)\n", pExp->vars[i].name, pExp->vars[i].nid);
			else
				fprintf(fp, "PSP_EXPORT_VAR_HASH(%s)\n", pExp->vars[i].name);
		}

		fprintf(fp, "PSP_EXPORT_END\n\n");
		free(nidName);
	}
}
void write_stub_new(const char *szDirectory, PspLibExport *pExp, CProcessPrx *pPrx){
	char szPath[PATH_MAX];
	fprintf(stdout,"Library %s\n", pExp->name);
	if(pExp->v_count != 0){
		fprintf(stderr, "%s: Stub output does not currently support variables\n", pExp->name);
	}

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
			fprintf(fp, "\tIMPORT_FUNC_WITH_ALIAS\t\"%s\",0x%08X,%s,%s\n", pExp->name, pExp->funcs[i].nid, pExp->funcs[i].name, strcmp(pSym->name, pExp->funcs[i].name)?pSym->name:pSym->alias[0]);
		else
			fprintf(fp, "\tIMPORT_FUNC\t\"%s\",0x%08X,%s\n", pExp->name, pExp->funcs[i].nid, pExp->funcs[i].name);
		fprintf(fp, "#endif\n");
	}
	fclose(fp);
	return 0;
}
void output_stubs_prx(const char *file/*, CNidMgr *pNids*/){
	//CProcessPrx prx(arg_dwBase);

	//PrxSetNidMgr(pNids);
	if(PrxLoadFromFile(file) == 0){
		fprintf(stderr, "Couldn't load prx file structures\n");
	}else{
		PspLibExport pHead;

		fprintf(stdout, "Dependancy list for %s\n", file);
		//PrxGetExports(&pHead);
		while(pHead != NULL){
			if(strcmp(pHead.name, PSP_SYSTEM_EXPORT) != 0){
				if(arg_newstubs){
					//write_stub_new("", pHead, &prx);
				}else{
					//write_stub("", pHead, &prx);
				}
			}
			//pHead = pHead->next;
		}
	}
}
void output_ents(const char *file/*, CNidMgr *pNids*/, FILE *f){
	//CProcessPrx prx(arg_dwBase);

	//PrxSetNidMgr(pNids);
	if(PrxLoadFromFile(file) == 0){
		fprintf(stderr, "Couldn't load prx file structures\n");
	}else{
		PspLibExport pHead;

		fprintf(stdout, "Dependancy list for %s\n", file);
		//PrxGetExports(&pHead);
		while(pHead != NULL){
			write_ent(&pHead, f);
			//pHead = pHead->next;
		}
	}
}
void output_stubs_xml(CNidMgr *pNids){

	LibraryEntry *pLib = pNids->libraries;
	PspLibExport *pExp;

	while(pLib){
		// Convery the LibraryEntry into a valid PspLibExport
		int i;

		memset(pExp, 0, sizeof(PspLibExport));
		strcpy(pExp->name, pLib->lib_name);
		pExp->f_count = pLib->fcount;
		pExp->v_count = pLib->vcount;
		pExp->stub.flags = pLib->flags;

		for(i = 0; i < pExp->f_count; i++){
			pExp->funcs[i].nid = pLib->pNids[i].nid;
			strcpy(pExp->funcs[i].name, pLib->pNids[i].name);
		}

		(arg_newstubs?write_stub_new:write_stub)("", pExp, NULL);

		//pLib = pLib->pNext;
	}
}
