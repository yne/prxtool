/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * main.C - Main function for PRXTool
 ***************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#define PATH_MAX 4096
#define countof(x) (sizeof(x) / sizeof(x[0]))

#include "db_func.c"
#include "db_nids.c"
#include "db_instr.c"
#include "elf.c"
#include "prx.c"
#include "arg.c"
#include "out.c"

#ifndef PRXTOOL_VERSION
#define PRXTOOL_VERSION ""
#endif

int main(int argc, char **argv){
	fprintf(stdout, "PRXTool "PRXTOOL_VERSION" ("__DATE__") \n");
	fprintf(stdout, "(c) TyRaNiD 2k6\n");

	if(arg_init(argc, argv))
		return 1;
	
	PrxToolCtx prx;
	if(arg_nidsfile){
		if(!db_nids_import(NULL,&prx.db.libraries_count,NULL,&prx.db.nids_count,arg_nidsfile)){
			LibraryEntry l[prx.db.libraries_count];
			LibraryNid   n[prx.db.nids_count];
			db_nids_import(prx.db.libraries=l,NULL,prx.db.nids=n,NULL,arg_nidsfile);
		}
		fprintf(stderr,"%uz nids loaded in %uz libraries\n",prx.db.nids_count,prx.db.libraries_count);
	}
	if(arg_funcfile){
		if(!db_func_import(NULL,&prx.db.functions_count,arg_funcfile)){
			FunctionType f[prx.db.functions_count];
			db_func_import(prx.db.functions=f,NULL,arg_funcfile);
		}
		fprintf(stderr,"%i prototypes loaded\n",prx.db.functions_count);
	}
	if(arg_prxfile && !PrxLoadFromFile(&prx,arg_prxfile)){
		return fprintf(stderr, "Couldn't load elf file structures"),1;
	}
	if(arg_loadbin && !PrxLoadFromBinFile(&prx,arg_loadbin)){
		return fprintf(stderr, "Couldn't load elf file structures"),1;
	}

	PrxSetNidMgr(&prx,pNids);
	#define OUT(arg) if(arg_out_#arg)output_#arg(&prx,arg_out_#arg);
	OUT(elf);
	OUT(stub);
	OUT(stubnew);
	OUT(dep);
	OUT(mod);
	OUT(pstub);
	OUT(pstubnew);
	OUT(impexp);
	OUT(symbols);
	OUT(xmldb);
	OUT(ent);
	OUT(disasm);
	OUT(xml);
	OUT(map);
	OUT(idc);
	#undef OUT
	return 0;
}
