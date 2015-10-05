char**   arg_inFiles      = NULL;
int      arg_nbFiles      = 0;
char *   arg_outfile      = NULL;
int      arg_out_idc      = 0;
int      arg_out_map      = 0;
int      arg_out_xml      = 0;
int      arg_out_elf      = 0;
int      arg_out_stub     = 0;
int      arg_out_stubnew  = 0;
int      arg_out_dep      = 0;
int      arg_out_mod      = 0;
int      arg_out_pstub    = 0;
int      arg_out_pstubnew = 0;
int      arg_out_impexp   = 0;
int      arg_out_ent      = 0;
int      arg_out_disasm   = 0;
int      arg_out_symbols  = 0;
int      arg_out_xmldb    = 0;
char *   arg_funcfile     = "functions.txt";
char *   arg_nidsfile     = "psplibdoc.xml";
char *   arg_iSMask       = "ixrl";
uint32_t arg_dwBase       = 0;
char *   arg_dbTitle      = "";
int      arg_xmlOutput    = 0;
int      arg_loadbin      = 0;
uint32_t arg_database     = 0;
int      arg_aliasOutput  = 0;
int      arg_verbose      = 0;
int      arg_showusage    = 0;
char*    arg_disopts      = "";

typedef struct{
	void*   argvoid           ;char*label  ,type, *help;}ArgEntry;
ArgEntry cmd_options[] = {
	{(void*) &arg_outfile     ,"output"    , 's', "Output destination. (NULL=stdout)"},
	{(void*) &arg_out_idc     ,"outidc"    , 'i', "Output an IDC file"},
	{(void*) &arg_out_map     ,"outmap"    , 'i', "Output a MAP file"},
	{(void*) &arg_out_xml     ,"outxml"    , 'i', "Output an XML file"},
	{(void*) &arg_out_elf     ,"outelf"    , 'i', "Output an ELF from a PRX"},
	{(void*) &arg_out_stub    ,"outstub"   , 'i', "Output old stub files from an XML"},
	{(void*) &arg_out_stubnew ,"outstub2"  , 'i', "Output new stub files from an XML"},
	{(void*) &arg_out_dep     ,"outstubEx" , 'i', "Output old stub files from an XML exports" },
	{(void*) &arg_out_mod     ,"outstubEx2", 'i', "Output new stub files from an XML exports"},
	{(void*) &arg_out_pstub   ,"outdeps"   , 'i', "Output dependencies"},
	{(void*) &arg_out_pstubnew,"outinfo"   , 'i', "Output information"},
	{(void*) &arg_out_impexp  ,"outexp"    , 'i', "Output imports/exports"},
	{(void*) &arg_out_ent     ,"outent"    , 'i', "Output entries"},
	{(void*) &arg_out_disasm  ,"outdis"    , 'i', "Output disassembly"},
	{(void*) &arg_out_symbols ,"outsym"    , 'i', "Output a symbol file"},
	{(void*) &arg_out_xmldb   ,"outxdb"    , 'i', "Output an XML disassembly database"},
	{(void*) &arg_funcfile    ,"funcs"     , 's', "Path to a disassembly functions file"},
	{(void*) &arg_nidsfile    ,"xmlfile"   , 's', "Path to a NID table XML"},
	{(void*) &arg_iSMask      ,"serialize" , 's', "What to serialize: Imp,eXp,Rel,Sec,sysLibexp "},
	{(void*) &arg_dwBase      ,"reloc"     , 'i', "Relocate the PRX to a different address"},
	{(void*) &arg_dbTitle     ,"xmldb"     , 's', "XML disassembly database title" },
	{(void*) &arg_xmlOutput   ,"xmldis"    , 'i', "Enable XML disassembly output mode"},
	{(void*) &arg_loadbin     ,"inbin"     , 'i', "Load the file as binary for disassembly"},
	{(void*) &arg_database    ,"inbinoff"  , 'i', "Data section offset to disassemble"},
	{(void*) &arg_aliasOutput ,"alias"     , 'i', "Print aliases when using -f mode" },
	{(void*) &arg_verbose     ,"verbose"   , 'i', "Be verbose"},
	{(void*) &arg_showusage   ,"help"      , 'i', "Print the Usage screen"},
	{(void*) &arg_disopts     ,"disopts"   , 's', "Disassembler options"},
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

char **arg_parse(int *argc, char **argv, ArgEntry *entry, int entrycount){
	if(!argc || *argc<=1 || !argv || !entry)
		return fprintf(stderr, "No arguments provided\n"),NULL;
	
	for((*argc)--,argv++ ; *argc > 0 && *argv; (*argc)--,argv++){
		const char *arg = *argv;
		if(arg[0] != '-' || arg[1] != '-')break;//we've reached the first input file argv
		
		ArgEntry *ent=NULL;
		for(int i = 0;i < entrycount;i++)
			if(entry[i].label && !strncmp(entry[i].label, &arg[2], strlen(entry[i].label)))
				ent = &entry[i];
		if(!ent)
			return fprintf(stderr, "Invalid argument %s\n", arg),NULL;
		char*value = 2+argv[0]+1+strlen(ent->label);//'--','opt','=','val'
		if(ent->type=='i')
			*((int*) ent->argvoid) = value[-1]=='='?strtoul(value, NULL, 0):1;
		if(ent->type=='s' && value[-1]=='=')
			*((char**) ent->argvoid) = value;
	}
	return argv;
}
int arg_usage(ArgEntry *entry, int entrycount, ArgDisEntry *disentry, int disentrycount){
	fprintf(stdout, "Usage: prxtool [option=value ...] file\nOptions:\n");
	
	for(int i = 0; i < entrycount; i++){
		ArgEntry *ent=&entry[i];
		char argbuf[22];
		if(ent->type=='i')
			snprintf(argbuf,sizeof(argbuf),"--%s=%i",ent->label,*(int*)ent->argvoid);
		if(ent->type=='s')
			snprintf(argbuf,sizeof(argbuf),"--%s=\"%s\"",ent->label,*(char**)(ent->argvoid));
		fprintf(stdout,"%-*s %s\n",(int)sizeof(argbuf)-1,argbuf, ent->help);
	}
	fprintf(stdout, "\nDisassembler Options:\n");
	for(int i = 0; i < disentrycount; i++)
		fprintf(stdout,"%c - %s\n", disentry[i].label, disentry[i].help);
	return 1;
}

int arg_init(int argc, char **argv){
	struct stat s;
	if(stat(arg_nidsfile,&s))arg_nidsfile = NULL;
	if(stat(arg_funcfile,&s))arg_funcfile = NULL;
	
	arg_inFiles = arg_parse(&argc, argv, cmd_options, countof(cmd_options));
	arg_nbFiles = argc;
	if(!arg_inFiles || arg_showusage)
		arg_usage(cmd_options, countof(cmd_options),cmd_disopts, countof(cmd_disopts));
	return !arg_inFiles;
}
