/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * main.C - Main function for PRXTool
 ***************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdint.h>
#include <limits.h>
#include <sys/stat.h>

#define PRXTOOL_VERSION "1.1"

#include "nid.c"
#include "elf.c"
#include "prx.c"

enum {
	SERIALIZE_IMPORTS  = (1 << 0),
	SERIALIZE_EXPORTS  = (1 << 1),
	SERIALIZE_SECTIONS = (1 << 2),
	SERIALIZE_RELOCS   = (1 << 3),
	SERIALIZE_DOSYSLIB = (1 << 4),
	SERIALIZE_ALL	   = 0xFFFFFFFF
};
typedef enum{
	OUTPUT_NONE = 0,
	OUTPUT_IDC = 1,
	OUTPUT_MAP = 2,
	OUTPUT_XML = 3,
	OUTPUT_ELF = 4,
	OUTPUT_STUB = 6,
	OUTPUT_DEP = 7,
	OUTPUT_MOD = 8,
	OUTPUT_PSTUB = 9,
	OUTPUT_IMPEXP = 10,
	OUTPUT_SYMBOLS = 11,
	OUTPUT_DISASM  = 12,
	OUTPUT_XMLDB = 13,
	OUTPUT_ENT = 14,
}OutputMode;

char **g_ppInfiles;
int  g_iInFiles;
char *g_pOutfile;
char *g_pNamefile;
char *g_pFuncfile;
int g_blDebug;
OutputMode g_outputMode;
uint32_t g_iSMask;
int g_newstubs;
uint32_t g_dwBase;
const char *g_disopts = "";
char g_namepath[PATH_MAX]="psplibdoc.xml";
char g_funcpath[PATH_MAX]="functions.txt";
int g_loadbin = 0;
int g_xmlOutput = 0;
int g_aliasOutput = 0;
const char *g_pDbTitle;
unsigned int g_database = 0;


int do_serialize(const char *arg){
	int i;

	i = 0;
	g_iSMask = 0;
	while(arg[i]){
		switch(tolower(arg[i])){
			case 'i' : g_iSMask |= SERIALIZE_IMPORTS;break;
			case 'x' : g_iSMask |= SERIALIZE_EXPORTS;break;
			case 'r' : g_iSMask |= SERIALIZE_RELOCS;break;
			case 's' : g_iSMask |= SERIALIZE_SECTIONS;break;
			case 'l' : g_iSMask |= SERIALIZE_DOSYSLIB;break;
			default:   fprintf(stderr, "Unknown serialize option '%c'\n", tolower(arg[i]));
			return 0;
		};
		i++;
	}

	return 1;
}

int do_xmldb(const char *arg){
	g_pDbTitle = arg;
	g_outputMode = OUTPUT_XMLDB;

	return 1;
}

#include "args.c"


void output_elf(const char *file, FILE *out_fp){//TODO
	//CProcessPrx prx(g_dwBase);

	fprintf(stdout, "Loading %s\n", file);
	if(PrxLoadFromFile(file) == 0){
		fprintf(stderr, "Couldn't load prx file structures\n");
	}else{
		if(PrxPrxToElf(out_fp) == 0){
			fprintf(stderr, "Failed to create a fixed up ELF\n");
		}
	}
}

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
	//CProcessPrx prx(g_dwBase);

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
	//CProcessPrx prx(g_dwBase);
	int blRet;

	fprintf(stdout, "Loading %s\n", file);
	//PrxSetNidMgr(nids);
	if(g_loadbin){
		blRet = PrxLoadFromBinFile(file, g_database);
	}else{
		blRet = PrxLoadFromFile(file);
	}

	if(g_xmlOutput){
		//PrxSetXmlDump();
	}

	if(blRet == 0){
		fprintf(stderr, "Couldn't load elf file structures");
	}else{
		PrxDump(out_fp, g_disopts);
	}
}
void output_xmldb(const char *file, FILE *out_fp/*, CNidMgr *nids*/){
	//CProcessPrx prx(g_dwBase);
	int blRet;

	fprintf(stdout, "Loading %s\n", file);
//	PrxSetNidMgr(nids);
	if(g_loadbin){
		blRet = PrxLoadFromBinFile(file, g_database);
	}else{
		blRet = PrxLoadFromFile(file);
	}

	if(blRet == 0){
		fprintf(stderr, "Couldn't load elf file structures");
	}else{
		PrxDumpXML(out_fp, g_disopts);
	}
}

void serialize_file(const char *file/*, CSerializePrx *pSer, CNidMgr *pNids*/){
	//CProcessPrx prx(g_dwBase);

	//PrxSetNidMgr(pNids);
	fprintf(stdout, "Loading %s\n", file);
	if(PrxLoadFromFile(file) == 0){
		fprintf(stderr, "Couldn't load prx file structures\n");
	}else{
		//PrxSerSerializePrx(prx, g_iSMask);
	}
}

void output_mods(const char *file/*, CNidMgr *pNids*/){
	//CProcessPrx prx(g_dwBase);

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
//	CProcessPrx prx(g_dwBase);
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
					if(g_aliasOutput){
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
//	CProcessPrx prx(g_dwBase);

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

			if((g_aliasOutput) && (pSym) && (pSym->alias > 0)){
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

void write_ent(PspLibExport *pExp, FILE *fp)
{      
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

void write_stub_new(const char *szDirectory, PspLibExport *pExp/*, CProcessPrx *pPrx*/){
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
		fprintf(fp, "#include \"pspimport.s\"\n\n");

		fprintf(fp, "// Build List\n");
		fprintf(fp, "// %s_0000.o ", pExp->name);
		for(int i = 0; i < pExp->f_count; i++){
			fprintf(fp, "%s_%04d.o ", pExp->name, i + 1);
		}
		fprintf(fp, "\n\n");

		fprintf(fp, "#ifdef F_%s_0000\n", pExp->name);
		fprintf(fp, "\tIMPORT_START\t\"%s\",0x%08X\n", pExp->name, pExp->stub.flags);
		fprintf(fp, "#endif\n");

		for(int i = 0; i < pExp->f_count; i++){
			SymbolEntry *pSym;

			fprintf(fp, "#ifdef F_%s_%04d\n", pExp->name, i + 1);
			if(/*pPrx*/0){
				//pSym = pPrx->GetSymbolEntryFromAddr(pExp->funcs[i].addr);
			}else{
				//pSym = NULL;
			}

			if((g_aliasOutput) && (pSym) && (pSym->alias > 0)){
				if(strcmp(pSym->name, pExp->funcs[i].name)){
					fprintf(fp, "\tIMPORT_FUNC_WITH_ALIAS\t\"%s\",0x%08X,%s,%s\n", pExp->name, 
							pExp->funcs[i].nid, pExp->funcs[i].name, pSym->name);
				}else{
					fprintf(fp, "\tIMPORT_FUNC_WITH_ALIAS\t\"%s\",0x%08X,%s,%s\n", pExp->name, 
							pExp->funcs[i].nid, pExp->funcs[i].name, pSym->alias[0]);
				}
			}else{
				fprintf(fp, "\tIMPORT_FUNC\t\"%s\",0x%08X,%s\n", pExp->name, pExp->funcs[i].nid, pExp->funcs[i].name);
			}

			fprintf(fp, "#endif\n");
		}
			
		fclose(fp);
	}
}


void output_stubs_prx(const char *file/*, CNidMgr *pNids*/){
	//CProcessPrx prx(g_dwBase);

	//PrxSetNidMgr(pNids);
	if(PrxLoadFromFile(file) == 0){
		fprintf(stderr, "Couldn't load prx file structures\n");
	}else{
		PspLibExport pHead;

		fprintf(stdout, "Dependancy list for %s\n", file);
		//PrxGetExports(&pHead);
		while(pHead != NULL){
			if(strcmp(pHead.name, PSP_SYSTEM_EXPORT) != 0){
				if(g_newstubs){
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
	//CProcessPrx prx(g_dwBase);

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

void output_stubs_xml(/*CNidMgr *pNids*/){
	LibraryEntry *pLib = NULL;
	PspLibExport *pExp = NULL;

	//pLib = pNids->GetLibraries();
	//pExp = new PspLibExport;

	while(pLib != NULL){
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

		if(g_newstubs){
			write_stub_new("", pExp/*, NULL*/);
		}else{
			write_stub("", pExp/*, NULL*/);
		}

		//pLib = pLib->pNext;
	}
}

int main(int argc, char **argv){
	//CSerializePrx *pSer;
	//CNidMgr nids;
	FILE *out_fp;

	out_fp = stdout;
	//COutput::SetOutputHandler(DoOutput);
	fprintf(stdout, "PRXTool v%s : (c) TyRaNiD 2k6\n", PRXTOOL_VERSION);
	fprintf(stdout, "Built: %s %s\n", __DATE__, __TIME__);

	if(process_args(argc, argv)){
		//COutput::SetDebug(g_blDebug);
		if(g_pOutfile != NULL){
			switch(g_outputMode){
				case OUTPUT_ELF :
					out_fp = fopen(g_pOutfile, "wb");
					break;
				default:
					out_fp = fopen(g_pOutfile, "wt");
					break;
			}
			if(out_fp == NULL){
				fprintf(stderr, "Couldn't open output file %s\n", g_pOutfile);
				return 1;
			}
		}

		//switch(g_outputMode){
		//	case OUTPUT_XML : pSer = new CSerializePrxToXml(out_fp);break;
		//	case OUTPUT_MAP : pSer = new CSerializePrxToMap(out_fp);break;
		//	case OUTPUT_IDC : pSer = new CSerializePrxToIdc(out_fp);break;
		//	default: pSer = NULL;break;
		//};

		if(g_pNamefile != NULL){
			//(void) NidsAddXmlFile(g_pNamefile);
		}
		if(g_pFuncfile != NULL){
			//(void) NidsAddFunctionFile(g_pFuncfile);
		}
		switch(g_outputMode){
		case OUTPUT_ELF:{
			output_elf(g_ppInfiles[0], out_fp);
		}
		case OUTPUT_STUB:{
			//CNidMgr nidData;
			//if(nidData.AddXmlFile(g_ppInfiles[0])){
			//	output_stubs_xml(&nidData);
			//}
		}
		case OUTPUT_DEP:{
			int iLoop;

			for(iLoop = 0; iLoop < g_iInFiles; iLoop++){
				output_deps(g_ppInfiles[iLoop]/*, &nids*/);
			}
		}
		case OUTPUT_MOD:{
			int iLoop;

			for(iLoop = 0; iLoop < g_iInFiles; iLoop++){
				output_mods(g_ppInfiles[iLoop]/*, &nids*/);
			}
		}
		case OUTPUT_PSTUB:{
			int iLoop;

			for(iLoop = 0; iLoop < g_iInFiles; iLoop++){
				output_stubs_prx(g_ppInfiles[iLoop]/*, &nids*/);
			}
		}
		case OUTPUT_IMPEXP:{
			for(int iLoop = 0; iLoop < g_iInFiles; iLoop++){
				output_importexport(g_ppInfiles[iLoop]/*, &nids*/);
			}
		}
		case OUTPUT_SYMBOLS:{
			output_symbols(g_ppInfiles[0], out_fp);
		}
		case OUTPUT_XMLDB:{
			int iLoop;

			fprintf(out_fp, "<?xml version=\"1.0\" ?>\n");
			fprintf(out_fp, "<firmware title=\"%s\">\n", g_pDbTitle);
			for(iLoop = 0; iLoop < g_iInFiles; iLoop++){
				output_xmldb(g_ppInfiles[iLoop], out_fp/*, &nids*/);
			}
			fprintf(out_fp, "</firmware>\n");
		}
		case OUTPUT_ENT:{
			FILE *f = fopen("exports.exp", "w");

			if (f != NULL){
				fprintf(f, "# Export file automatically generated with prxtool\n");
				fprintf(f, "PSP_BEGIN_EXPORTS\n\n");
				output_ents(g_ppInfiles[0]/*, &nids*/, f);
				fprintf(f, "PSP_END_EXPORTS\n");
				fclose(f);
			}
		}
		case OUTPUT_DISASM:{
			int iLoop;

			if(g_iInFiles == 1){
				output_disasm(g_ppInfiles[0], out_fp/*, &nids*/);
			}else{
				char path[PATH_MAX];
				int len;

				for(iLoop = 0; iLoop < g_iInFiles; iLoop++){
					FILE *out;
					const char *file;

					file = strrchr(g_ppInfiles[iLoop], '/');
					if(file){
						file++;
					}else{
						file = g_ppInfiles[iLoop];
					}

					if(g_xmlOutput){
						len = snprintf(path, PATH_MAX, "%s.html", file);
					}else{
						len = snprintf(path, PATH_MAX, "%s.txt", file);
					}

					if((len < 0) || (len >= PATH_MAX)){
						continue;
					}

					out = fopen(path, "w");
					if(out == NULL){
						fprintf(stdout, "Could not open file %s for writing\n", path);
						continue;
					}

					output_disasm(g_ppInfiles[iLoop], out/*, &nids*/);
					fclose(out);
				}
			}
		}
		default:{
			int iLoop;

			//pSer->Begin();
			//for(iLoop = 0; iLoop < g_iInFiles; iLoop++){
			//	serialize_file(g_ppInfiles[iLoop], pSer, &nids);
			//}
			//pSer->End();
		}
		}

		if((g_pOutfile != NULL) && (out_fp != NULL)){
			fclose(out_fp);
		}

		fprintf(stdout, "Done");
	}else{
		print_help();
	}
}
