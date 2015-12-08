#include <stdio.h>
//#include <unistd.h>
#include <stdint.h>

#define assert(cond) if(!(cond))return fprintf(stderr,"error in %s: %s\n", __func__, #cond),-1;
#define foreach(Type,name,list) for(Type name = list; name < list+list##_count; name++)

#include "prx.c"
#include "prxtool.out.c"
#include "prxtool.arg.c"

#ifndef PRXTOOL_VERSION
#define PRXTOOL_VERSION ""
#endif

int main(int argc, char **argv){
	fprintf(stderr, "PRXTool "PRXTOOL_VERSION" ("__DATE__") \n(c) TyRaNiD 2k6\n");

	
	PrxToolArg arg = {.print="ixrl",.modInfoName=".rodata.sceModuleInfo"};
	assert(!arg_parse(argc,argv,&arg))

	Library    *libs=NULL;size_t libs_count=0;
	Nid        *nids=NULL;size_t nids_count=0;
	if(arg.in.nid){
		if(!db_nids_import(NULL,&libs_count,NULL,&nids_count,arg.in.nid)){
			libs=calloc(libs_count,sizeof(*libs));
			nids=calloc(nids_count,sizeof(*nids));
			db_nids_import(libs,NULL,nids,NULL,arg.in.nid);
		}
		fprintf(stderr,"loaded: %zu nids in %zu libs\n",nids_count,libs_count);
		fclose(arg.in.nid);
	}
	
	Protoype   *proto=NULL;size_t proto_count=0;
	if(arg.in.func){
		if(!db_func_import(NULL,&proto_count,arg.in.func)){
			proto=calloc(proto_count,sizeof(proto));
			db_func_import(proto,NULL,arg.in.func);
		}
		fprintf(stderr,"loaded: %zu prototypes\n",proto_count);
		fclose(arg.in.func);
	}
	
	Instruction*macro=NULL;size_t macro_count=0;
	Instruction*instr=NULL;size_t instr_count=0;
	if(arg.in.instr){
		if(!db_instr_import(NULL,&instr_count,NULL,&macro_count,arg.in.instr)){
			instr=calloc(instr_count,sizeof(*instr));
			macro=calloc(macro_count,sizeof(*macro));
			db_instr_import(instr,NULL,macro,NULL,arg.in.instr);
		}
		fprintf(stderr,"loaded: %zu instructions + %zu macro\n",instr_count,macro_count);
		fclose(arg.in.instr);
	}

	PrxCtx prx = {};
	if(arg.in.prx){
		assert(!prx_loadFromElf(&prx,arg.in.prx,instr,instr_count,arg.modInfoName))
	}
	if(arg.in.bin){
		assert(!prx_loadFromBin(&prx,arg.in.bin,instr,instr_count))
	}
	#define OUT(A,...) if(arg.out.A)output_##A(&prx,arg.out.A, ##__VA_ARGS__);
	OUT(elf);
	OUT(S  ,arg.aliased);
	OUT(exp);
	OUT(mod);
	OUT(sym);
	OUT(xml);
	OUT(htm);
	OUT(asm,arg.disopts);
	OUT(xml);
	OUT(map);
	OUT(idc);
	#undef OUT
	return 0;
}
