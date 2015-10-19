#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#define PATH_MAX 4096
#define assertf(cond,...) if(!(cond))return fprintf(stderr,__VA_ARGS__),-1;
#define assert(cond)      if(!(cond))return fprintf(stderr,"%s error: %s\n",__func__, #cond),-1;
#define countof(X) (sizeof(X)/sizeof(X[0]))

#include "arg.c"
#include "prx.c"
#include "out.c"

#ifndef PRXTOOL_VERSION
#define PRXTOOL_VERSION ""
#endif

int main(int argc, char **argv){
	fprintf(stdout, "PRXTool "PRXTOOL_VERSION" ("__DATE__") \n");
	fprintf(stdout, "(c) TyRaNiD 2k6\n");

	PrxToolArg arg={.print="ixrl"};
	assert(!parse_arg(argc,argv,&arg))

	PrxToolCtx prx={};
	
	if(arg.in.nid){
		if(!db_nids_import(NULL,&prx.library_count,NULL,&prx.nid_count,arg.in.nid)){
			Library l[prx.library_count];
			Nid     n[prx.nid_count];
			db_nids_import(prx.library=l,NULL,prx.nids=n,NULL,arg.in.nid);
		}
		fprintf(stderr,"loaded: %zu nids in %zu library\n",prx.nid_count,prx.library_count);
	}
	if(arg.in.func){
		if(!db_func_import(NULL,&prx.proto_count,arg.in.func)){
			Protoype f[prx.proto_count];
			db_func_import(prx.proto=f,NULL,arg.in.func);
		}
		fprintf(stderr,"loaded: %zu prototypes\n",prx.proto_count);
	}
	if(arg.in.instr){
		if(!db_instr_import(NULL,&prx.instr_count,NULL,&prx.macro_count,arg.in.instr)){
			Instruction instr[prx.instr_count];
			Instruction macro[prx.macro_count];
			db_instr_import(prx.instr=instr,NULL,prx.macro=macro,NULL,arg.in.instr);
		}
		fprintf(stderr,"loaded: %zu instructions + %zu macro\n",prx.instr_count,prx.macro_count);
	}
	if(arg.in.prx){
		assert(!PrxLoadFromElf(&prx,arg.in.prx))
	}
	if(arg.in.bin){
		assert(!PrxLoadFromBin(&prx,arg.in.bin))
	}
	#define OUT(A) if(arg.out.A)output_##A(&prx,arg.out.A,&arg);
	OUT(elf);
	OUT(stub);
	OUT(stub2);
	OUT(dep);
	OUT(mod);
	OUT(pstub);
	OUT(pstub2);
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
