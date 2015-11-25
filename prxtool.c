#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#define PATH_MAX 4096
#define assert(cond) if(!(cond))return fprintf(stderr,"error in %s: %s\n", __func__, #cond),-1;
#define foreach(Type,name,list) for(Type name = list; name < list+list##_count; name++)

#include "arg.c"
#include "prx.c"
#include "out.c"

#ifndef PRXTOOL_VERSION
#define PRXTOOL_VERSION ""
#endif

int main(int argc, char **argv){
	fprintf(stderr, "PRXTool "PRXTOOL_VERSION" ("__DATE__") \n(c) TyRaNiD 2k6\n");

	
	PrxToolArg arg = {.print="ixrl",.modInfoName=".rodata.sceModuleInfo"};
	assert(!parse_arg(argc,argv,&arg))

	Library    *libs=NULL;size_t libs_count=0;
	Nid        *nids=NULL;size_t nids_count=0;
	if(arg.in.nid){
		if(!db_nids_import(NULL,&libs_count,NULL,&nids_count,arg.in.nid)){
			libs=calloc(libs_count,sizeof(*libs));
			nids=calloc(nids_count,sizeof(*nids));
			db_nids_import(libs,NULL,nids,NULL,arg.in.nid);
		}
		fprintf(stderr,"loaded: %zu nids in %zu libs\n",nids_count,libs_count);
	}
	
	Protoype   *proto=NULL;size_t proto_count=0;
	if(arg.in.func){
		if(!db_func_import(NULL,&proto_count,arg.in.func)){
			proto=calloc(proto_count,sizeof(proto));
			db_func_import(proto,NULL,arg.in.func);
		}
		fprintf(stderr,"loaded: %zu prototypes\n",proto_count);
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
	}
	PrxCtx prx = {};
	if(arg.in.prx){
		assert(!prx_loadFromElf(&prx,arg.in.prx,instr,instr_count,arg.modInfoName))
	}
	if(arg.in.bin){
		assert(!prx_loadFromBin(&prx,arg.in.bin,instr,instr_count))
	}
	return 0;
	#define OUT(A) if(arg.out.A)output_##A(&prx,arg.out.A,&arg);
	OUT(elf);
	OUT(stub);
	OUT(stub2);
	OUT(dep);
	OUT(mod);
	OUT(pstub);
	OUT(pstub2);
	OUT(impexp);
	OUT(symbol);
	//OUT(xmldb);
	OUT(ent);
	OUT(disasm);
	OUT(xml);
	OUT(map);
	OUT(idc);
	#undef OUT
	return 0;
}
