#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#define PATH_MAX 4096
#define assertf(cond,...) if(!(cond))return fprintf(stderr,__VA_ARGS__),-1;
#define assert(cond)      if(!(cond))return fprintf(stderr,"error in %s: %s\n", __func__, #cond),-1;
#define countof(X) (sizeof(X)/sizeof(X[0]))

#include "arg.c"
#include "prx.c"
#include "out.c"

#ifndef PRXTOOL_VERSION
#define PRXTOOL_VERSION ""
#endif

int main(int argc, char **argv){
	fprintf(stderr, "PRXTool "PRXTOOL_VERSION" ("__DATE__") \n(c) TyRaNiD 2k6\n");

	PrxToolCtx ctx={.arg={.print="ixrl",.modInfoName=".rodata.sceModuleInfo"}};
	assert(!parse_arg(argc,argv,&ctx.arg))

	
	if(ctx.arg.in.nid){
		if(!db_nids_import(NULL,&ctx.library_count,NULL,&ctx.nid_count,ctx.arg.in.nid)){
			ctx.library=malloc(ctx.library_count*sizeof(*ctx.library));
			ctx.nids=malloc(ctx.nid_count*sizeof(*ctx.nids));
			db_nids_import(ctx.library,NULL,ctx.nids,NULL,ctx.arg.in.nid);
		}
		fprintf(stderr,"loaded: %zu nids in %zu library\n",ctx.nid_count,ctx.library_count);
	}
	if(ctx.arg.in.func){
		if(!db_func_import(NULL,&ctx.proto_count,ctx.arg.in.func)){
			ctx.proto=malloc(ctx.proto_count*sizeof(ctx.proto));
			db_func_import(ctx.proto,NULL,ctx.arg.in.func);
		}
		fprintf(stderr,"loaded: %zu prototypes\n",ctx.proto_count);
	}
	if(ctx.arg.in.instr){
		if(!db_instr_import(NULL,&ctx.instr_count,NULL,&ctx.macro_count,ctx.arg.in.instr)){
			ctx.instr=malloc(ctx.instr_count*sizeof(*ctx.instr));
			ctx.macro=malloc(ctx.macro_count*sizeof(*ctx.macro));
			db_instr_import(ctx.instr,NULL,ctx.macro,NULL,ctx.arg.in.instr);
		}
		fprintf(stderr,"loaded: %zu instructions + %zu macro\n",ctx.instr_count,ctx.macro_count);
	}
	if(ctx.arg.in.prx){
		assert(!PrxLoadFromElf(&ctx,ctx.arg.in.prx))
	}
	if(ctx.arg.in.bin){
		assert(!PrxLoadFromBin(&ctx,ctx.arg.in.bin))
	}
	#define OUT(A) if(ctx.arg.out.A)output_##A(&ctx,ctx.arg.out.A,&ctx.arg);
	OUT(elf);
	OUT(stub);
	OUT(stub2);
	OUT(dep);
	OUT(mod);
	OUT(pstub);
	OUT(pstub2);
	OUT(impexp);
	OUT(symbols);
	//OUT(xmldb);
	OUT(ent);
	OUT(disasm);
	OUT(xml);
	OUT(map);
	OUT(idc);
	#undef OUT
	return 0;
}
