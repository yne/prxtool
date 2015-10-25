#include <string.h>
#include <stdlib.h>

typedef struct{
	int        xmldis,aliased,verbose,help;
	uint32_t   base,dataOff;
	char       *print,*disopts,*dbTitle,*modInfoName;
	struct{FILE*prx,*bin,*func,*nid,*instr;}in;
	struct{FILE*idc,*map,*xml,*elf,*stub,*stub2,*dep,*mod,*pstub,*pstub2,*impexp,*ent,*disasm,*symbols,*xmldb;}out;
}PrxToolArg;

int parse_arg(int argc,char**argv,PrxToolArg*arg){
	assert(argc>1 && "No arguments provided (use --help)\n");
	
	//bind each arg struct to an ArgEntry, so we can handle they value and print help about them
	typedef struct{void*argvoid;char*label,*type,*help;}ArgEntry;
	ArgEntry cmds[] = {
		#define ARG(name,type,desc) {(void*)&arg-> name,#name,type,desc}
		ARG(in.prx      ,"rb","Input PRX file"),//first = default argument
		ARG(in.bin      ,"rb","Input bin file"),
		ARG(in.instr    ,"r" ,"Instructions list (TSV)"),
		ARG(in.func     ,"r" ,"Prototypes list (TSV)"),
		ARG(in.nid      ,"r" ,"NIDs list (XML,YML)"),
		ARG(out.idc     ,"w" ,"IDC file"),
		ARG(out.map     ,"w" ,"MAP file"),
		ARG(out.xml     ,"w" ,"XML file"),
		ARG(out.elf     ,"wb","ELF file"),
		ARG(out.stub    ,"w" ,"stub file (old) files from an XML"),
		ARG(out.stub2   ,"w" ,"stub file (new) files from an XML"),
		ARG(out.dep     ,"w" ,"stub file (old) files from an XML exports" ),
		ARG(out.mod     ,"w" ,"stub file (new) files from an XML exports"),
		ARG(out.pstub   ,"w" ,"dependencies"),
		ARG(out.pstub2  ,"w" ,"information"),
		ARG(out.impexp  ,"w" ,"imports/exports"),
		ARG(out.ent     ,"w" ,"entries"),
		ARG(out.disasm  ,"w" ,"disassembly"),
		ARG(out.symbols ,"w" ,"symbol file"),
		ARG(out.xmldb   ,"w" ,"XML disassembly database"),
		ARG(xmldis      ,"i" ,"Enable XML disassembly output mode"),
		ARG(aliased     ,"i" ,"Print aliases when using -f mode" ),
		ARG(verbose     ,"i" ,"Be verbose"),
		ARG(help        ,"i" ,"Print the Usage screen"),
		ARG(modInfoName ,"s" ,"Name of the ModuleInfo section to lookup"),
		ARG(print       ,"s" ,"What to print: Imp,eXp,Rel,Sec,sysLibexp "),
		ARG(base        ,"i" ,"Relocate the PRX to a different address"),
		ARG(dbTitle     ,"s" ,"XML disassembly database title" ),
		ARG(dataOff     ,"i" ,"Data section offset to disassemble"),
		ARG(disopts     ,"s" ,"Disassembler options"),
		#undef ARG
	};
	typedef struct{char label,*help;}ArgDisEntry;
	ArgDisEntry cmd_disopts[]={
		{'x',"Print immediate all in hex (not just appropriate ones"},
		{'d',"When combined with 'x' prints the hex as signed"},
		{'r',"Print CPU registers using rN format rather than mnemonics (i.e. $a0)"},
		{'s',"Print the PC as a symbol if possible"},
		{'m',"Disable macro instructions (e.g. nop, beqz etc."},
		{'w',"Indicate PC, opcode information goes after the instruction disasm"},
	};
	
	for(int i=1 ; i < argc ; i++){
		// if the argument is command-less (no --cmd=...) treat it like the first/default command
		ArgEntry* cmd = argv[i][0]=='-'?NULL:&cmds[0];
		char*   value = argv[i];
		// lookup the command
		for(int c = 0;!cmd && c < countof(cmds);c++){
			if(cmds[c].label && !strncmp(cmds[c].label, argv[i]+2, strlen(cmds[c].label)))
				value += 2+strlen((cmd = &cmds[c])->label)+1;
		}
		assert(cmd && "Invalid argument\n");
		// parse it value according to the type
		// Integer : use 1 if no value provided
		if(*cmd->type=='i')
			*((int*) cmd->argvoid) = value[-1]=='='?strtoul(value, NULL, 0):1;
		// String : use empty string if no value provided
		if(*cmd->type=='s')
			*((char**) cmd->argvoid) = value[-1]=='='?value:"";
		// File : use stdin/stdout if no file specified
		if(*cmd->type=='r' || *cmd->type=='w'){
			*((FILE**) cmd->argvoid) = (value[-1]=='=')?fopen(value, cmd->type):(*cmd->type=='w'?stdin:stdout);
			assert(*((FILE**) cmd->argvoid) && "Unable to open the file\n");
		}
	}
	if(arg->help){
		fprintf(stdout, "Usage: prxtool [[--option=]value ...] \nOptions:\n");
		for(int i = 0; i < countof(cmds); i++){
			ArgEntry *ent=&cmds[i];
			char argbuf[26]={};
			if(ent->type[0]=='i')
				snprintf(argbuf,sizeof(argbuf),"--%s=%i",ent->label,*(int*)ent->argvoid);
			if(*ent->type=='s')
				snprintf(argbuf,sizeof(argbuf),"--%s=\"%s\"",ent->label,*(char**)(ent->argvoid)?:"");
			if(*ent->type=='r')
				snprintf(argbuf,sizeof(argbuf),"--%s=%s",ent->label,*(char**)(ent->argvoid)?"...":"<inFile>");
			if(*ent->type=='w')
				snprintf(argbuf,sizeof(argbuf),"--%s=%s",ent->label,*(char**)(ent->argvoid)?"...":"<outFile>");
			fprintf(stdout,"%-*s %s\n",(int)sizeof(argbuf)-1,argbuf, ent->help);
		}
		fprintf(stdout, "\nDisassembler Options:\n");
		for(int i = 0; i < countof(cmd_disopts); i++)
			fprintf(stdout,"%c - %s\n", cmd_disopts[i].label, cmd_disopts[i].help);
	}
	return 0;
}
