FILE*    arg_in_prx       = NULL;
FILE*    arg_in_func      = NULL;
FILE*    arg_in_nids      = NULL;
FILE*    arg_in_instr     = NULL;

FILE*    arg_out_idc      = NULL;
FILE*    arg_out_map      = NULL;
FILE*    arg_out_xml      = NULL;
FILE*    arg_out_elf      = NULL;
FILE*    arg_out_stub     = NULL;
FILE*    arg_out_stubnew  = NULL;
FILE*    arg_out_dep      = NULL;
FILE*    arg_out_mod      = NULL;
FILE*    arg_out_pstub    = NULL;
FILE*    arg_out_pstubnew = NULL;
FILE*    arg_out_impexp   = NULL;
FILE*    arg_out_ent      = NULL;
FILE*    arg_out_disasm   = NULL;
FILE*    arg_out_symbols  = NULL;
FILE*    arg_out_xmldb    = NULL;

char*    arg_iSMask       = "ixrl";
uint32_t arg_dwBase       = 0;
char*    arg_dbTitle      = "";
int      arg_xmlOutput    = 0;
int      arg_loadbin      = 0;
uint32_t arg_database     = 0;
int      arg_aliasOutput  = 0;
int      arg_verbose      = 0;
int      arg_showusage    = 0;
char*    arg_disopts      = "";
typedef struct{
	void*   argvoid           ;char*label  ,*type, *help;}ArgEntry;
ArgEntry cmd_options[] = {//String,Integer,Read,Write(upper=binary)
	{(void*) &arg_prxfile     ,"prx"       , "rb", "Input PRX file"},//default argument
	{(void*) &arg_instrfile   ,"instr"     , "r" , "Disassembly instruction list"},
	{(void*) &arg_funcfile    ,"funcs"     , "r" , "Disassembly functions"},
	{(void*) &arg_nidsfile    ,"nid"       , "r" , "NIDs table (XML/YML)"},
	
	{(void*) &arg_out_idc     ,"idc"       , "w" , "IDC file"},
	{(void*) &arg_out_map     ,"map"       , "w" , "MAP file"},
	{(void*) &arg_out_xml     ,"xml"       , "w" , "XML file"},
	{(void*) &arg_out_elf     ,"elf"       , "wb", "ELF file"},
	{(void*) &arg_out_stub    ,"stub"      , "w" , "stub file (old) files from an XML"},
	{(void*) &arg_out_stubnew ,"stub2"     , "w" , "stub file (new) files from an XML"},
	{(void*) &arg_out_dep     ,"stubEx"    , "w" , "stub file (old) files from an XML exports" },
	{(void*) &arg_out_mod     ,"stubEx2"   , "w" , "stub file (new) files from an XML exports"},
	{(void*) &arg_out_pstub   ,"deps"      , "w" , "dependencies"},
	{(void*) &arg_out_pstubnew,"info"      , "w" , "information"},
	{(void*) &arg_out_impexp  ,"exp"       , "w" , "imports/exports"},
	{(void*) &arg_out_ent     ,"ent"       , "w" , "entries"},
	{(void*) &arg_out_disasm  ,"dis"       , "w" , "disassembly"},
	{(void*) &arg_out_symbols ,"sym"       , "w" , "a symbol file"},
	{(void*) &arg_out_xmldb   ,"xdb"       , "w" , "an XML disassembly database"},
	
	{(void*) &arg_iSMask      ,"serialize" , "s" , "What to serialize: Imp,eXp,Rel,Sec,sysLibexp "},
	{(void*) &arg_dwBase      ,"reloc"     , "i" , "Relocate the PRX to a different address"},
	{(void*) &arg_dbTitle     ,"xmldb"     , "s" , "XML disassembly database title" },
	{(void*) &arg_xmlOutput   ,"xmldis"    , "i" , "Enable XML disassembly output mode"},
	{(void*) &arg_loadbin     ,"inbin"     , "i" , "Load the file as binary for disassembly"},
	{(void*) &arg_database    ,"inbinoff"  , "i" , "Data section offset to disassemble"},
	{(void*) &arg_aliasOutput ,"alias"     , "i" , "Print aliases when using -f mode" },
	{(void*) &arg_verbose     ,"verbose"   , "i" , "Be verbose"},
	{(void*) &arg_showusage   ,"help"      , "i" , "Print the Usage screen"},
	{(void*) &arg_disopts     ,"disopts"   , "s" , "Disassembler options"},
};

typedef struct{
	char label,*help;}ArgDisEntry;
ArgDisEntry cmd_disopts[]={
	{'x',"Print immediate all in hex (not just appropriate ones"},
	{'d',"When combined with 'x' prints the hex as signed"},
	{'r',"Print CPU registers using rN format rather than mnemonics (i.e. $a0)"},
	{'s',"Print the PC as a symbol if possible"},
	{'m',"Disable macro instructions (e.g. nop, beqz etc."},
	{'w',"Indicate PC, opcode information goes after the instruction disasm"},
};

int arg_parse(int argc, char **argv, ArgEntry *cmds, int entrycount){
	if(!argc || argc<=1 || !argv || !cmds)
		return fprintf(stderr, "No arguments provided\n"),0;
	
	for(int i=0 ; i < argc ; i++){
		// if the argument is command-less (no --cmd=...) use the first one
		ArgEntry* cmd = argv[i][0]=='-'?NULL:&cmds[0];
		char*   value = argv[i];
		// lookup the command
		for(int c = 0;!cmd && c < entrycount;c++)
			if(cmds[c].label && !strncmp(cmds[c].label, &(*argv)[2], strlen(cmds[c].label)))
				value += 2+strlen((cmd = &cmds[c])->label)+1;
		if(!cmd)
			return fprintf(stderr, "Invalid argument %s\n", *argv),0;
		// parse it value according to the type
		// Integer : use 1 if no value provided
		if(*cmd->type=='i')
			*((int*) cmd->argvoid) = value[-1]=='='?strtoul(value, NULL, 0):1;
		// String : use empty string if no value provided
		if(*cmd->type=='s')
			*((char**) cmd->argvoid) = value[-1]=='='?value:"";
		// File : use stdin/stdout if no file specified
		if(*cmd->type=='r' || *cmd->type=='w'){
			*((FILE**) cmd->argvoid) = value[-1]=='='?fopen(value, cmd->type):(*cmd->type=='w'?stdin:stdout);
			if(!cmd->argvoid)
				return fprintf(stderr, "Unable to open %s\n",value),0;
		}
	}
	return 1;
}

int arg_usage(ArgEntry *entry, int entrycount, ArgDisEntry *disentry, int disentrycount){
	fprintf(stdout, "Usage: prxtool [option=value ...] file\nOptions:\n");
	
	for(int i = 0; i < entrycount; i++){
		ArgEntry *ent=&entry[i];
		char argbuf[22];
		if(*ent->type=='i')
			snprintf(argbuf,sizeof(argbuf),"--%s=%i",ent->label,*(int*)ent->argvoid);
		if(*ent->type=='s')
			snprintf(argbuf,sizeof(argbuf),"--%s=\"%s\"",ent->label,*(char**)(ent->argvoid)?:"");
		fprintf(stdout,"%-*s %s\n",(int)sizeof(argbuf)-1,argbuf, ent->help);
	}
	fprintf(stdout, "\nDisassembler Options:\n");
	for(int i = 0; i < disentrycount; i++)
		fprintf(stdout,"%c - %s\n", disentry[i].label, disentry[i].help);
	return 1;
}

int arg_init(int argc, char **argv){
	int res = arg_parse(argc, argv, cmd_options, countof(cmd_options));
	if(res || arg_showusage)
		arg_usage(cmd_options, countof(cmd_options),cmd_disopts, countof(cmd_disopts));
	return res;
}
