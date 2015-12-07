#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct{
	int        xmldis,aliased,verbose,help;
	uint32_t   base,dataOff;
	char       *print,*disopts,*dbTitle,*modInfoName;
	struct{FILE*prx,*bin,*func,*nid,*instr;}in;
	struct{FILE*idc,*map,*htm,*elf,*S,*dep,*mod,*exp,*asm,*sym,*xml;}out;
}PrxToolArg;

int arg_parse(int argc,char**argv,PrxToolArg*arg){
	if(argc<2 || !argv)//print usage if no argument provided
		argc=2,argv=(char*[]){"prx","--help"};
	//bind each arg struct to an ArgEntry, so we can handle they value and print help about them
	typedef struct{void*argvoid;char*label,*type,*help;}ArgEntry;
	ArgEntry cmds[] = {
		#define ARG(name,type,desc) {(void*)&arg-> name,#name,type,desc}
		ARG(in.prx      ,"frb","Input PRX file"),//first = default argument
		ARG(in.bin      ,"frb","Input bin file"),
		ARG(in.instr    ,"fr" ,"Instructions list (TSV)"),
		ARG(in.func     ,"fr" ,"Prototypes list (TSV)"),
		ARG(in.nid      ,"fr" ,"NIDs list (XML,YML)"),
		ARG(out.idc     ,"fw" ,"IDC file"),
		ARG(out.map     ,"fw" ,"MAP file"),
		ARG(out.htm     ,"fw" ,"HTM file"),
		ARG(out.elf     ,"fwb","ELF file"),
		ARG(out.S       ,"fw" ,"Stub file"),
		ARG(out.exp     ,"fw" ,"Export file" ),
		ARG(out.mod     ,"fw" ,"PRX Information"),
		ARG(out.asm     ,"fw" ,"disassembly"),
		ARG(out.sym     ,"fw" ,"symbol file"),
		ARG(out.xml     ,"fw" ,"XML database"),
		ARG(xmldis      ,"i"  ,"Enable XML disassembly output mode"),
		ARG(aliased     ,"i"  ,"Print aliases when using -f mode" ),
		ARG(help        ,"i"  ,"Print the Usage screen"),
		ARG(modInfoName ,"s"  ,"Name of the ModuleInfo section to lookup"),
		ARG(print       ,"s"  ,"What to print: Imp,eXp,Rel,Sec,sysLibexp "),
		ARG(base        ,"i"  ,"Relocate the PRX to a different address"),
		ARG(dbTitle     ,"s"  ,"XML disassembly database title" ),
		ARG(dataOff     ,"i"  ,"Data section offset to disassemble"),
		ARG(disopts     ,"s"  ,"Disassembler options"),
		#undef ARG
	};
	struct{char label,*help;} cmd_disopts[]={
		{'x',"Print immediate all in hex (not just appropriate ones"},
		{'d',"When combined with 'x' prints the hex as signed"},
		{'r',"Print CPU registers using rN format rather than mnemonics (i.e. $a0)"},
		{'s',"Print the PC as a symbol if possible"},
		{'m',"Disable macro instructions (e.g. nop, beqz etc."},
		{'w',"Indicate PC, opcode information goes after the instruction disasm"},
	};
	
	for(int i=1 ; i < argc ; i++){
		// if the argument is name-less (no --name=...) treat it like the first/default command
		ArgEntry* cmd = argv[i][0]=='-'?NULL:&cmds[0];
		char*   value = argv[i];
		// lookup the command
		for(int c = 0;!cmd && c < sizeof(cmds)/sizeof(*cmd);c++){
			if(cmds[c].label && !strncmp(cmds[c].label, argv[i]+2, strlen(cmds[c].label)))
				value += 2+strlen((cmd = &cmds[c])->label)+1;
		}
		assert(cmd && "Invalid argument\n");
		// parse it value according to the type
		if(*cmd->type=='i')// Integer : use 1 if no value provided
			*((int*  ) cmd->argvoid) = value[-1]=='='?strtoul(value, NULL, 0):1;
		if(*cmd->type=='s')// String : use empty string if no value provided
			*((char**) cmd->argvoid) = value[-1]=='='?value:"";
		if(*cmd->type=='f'){// File : use std-in/out if no file specified
			*((FILE**) cmd->argvoid) = value[-1]=='='?fopen(value, cmd->type+1):(cmd->type[1]=='w'?stdout:stdin);
			assert(*((FILE**)cmd->argvoid) && "Unable to open the file\n");
		}
	}
	
	if(arg->help){
		fprintf(stderr, "Usage: %s [[--option=]value ...] \nOptions:\n",argv[0]);
		for(int i = 0; i < sizeof(cmds)/sizeof(*cmds); i++){
			ArgEntry *ent=&cmds[i];
			char argbuf[26]={};
			if(ent->type[0]=='i')
				snprintf(argbuf,sizeof(argbuf),"--%s=%i",ent->label,*(int*)ent->argvoid);
			if(*ent->type=='s')
				snprintf(argbuf,sizeof(argbuf),"--%s=%s",ent->label,*(char**)(ent->argvoid)?:"");
			if(*ent->type=='f')
				snprintf(argbuf,sizeof(argbuf),"--%s=%s",ent->label,*(char**)(ent->argvoid)?"...":"<File>");
			fprintf(stderr,"%-*s %s\n",(int)sizeof(argbuf)-1,argbuf, ent->help);
		}
		fprintf(stderr, "\nDisassembler Options:\n");
		for(int i = 0; i < sizeof(cmd_disopts)/sizeof(*cmd_disopts); i++)
			fprintf(stderr,"%c - %s\n", cmd_disopts[i].label, cmd_disopts[i].help);
	}
	return 0;
}
