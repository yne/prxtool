enum {
	SERIALIZE_IMPORTS  = (1 << 0),
	SERIALIZE_EXPORTS  = (1 << 1),
	SERIALIZE_SECTIONS = (1 << 2),
	SERIALIZE_RELOCS   = (1 << 3),
	SERIALIZE_DOSYSLIB = (1 << 4),
	SERIALIZE_ALL	   = 0xFFFFFFFF
};
typedef enum{
	OUT_NONE,
	OUT_IDC,
	OUT_MAP,
	OUT_XML,
	OUT_ELF,
	OUT_STUB,
	OUT_STUBNEW,
	OUT_DEP,
	OUT_MOD,
	OUT_PSTUB,
	OUT_PSTUBNEW,
	OUT_IMPEXP,
	OUT_SYMBOLS,
	OUT_DISASM,
	OUT_XMLDB,
	OUT_ENT,
}OutType;

char**   arg_inFiles;
int      arg_nbFiles;
char *   arg_outfile;
char *   arg_nidsfile;
char *   arg_funcfile;
int      arg_debug;
OutType  arg_action;
uint32_t arg_iSMask;
uint32_t arg_dwBase;
char*    arg_disopts = "";
int      arg_loadbin = 0;
int      arg_xmlOutput = 0;
int      arg_aliasOutput = 0;
unsigned arg_database = 0;
char *   arg_dbTitle = "";

char*    def_namefile = "psplibdoc.xml";
char*    def_funcfile = "functions.txt";

void init_arguments(){
	struct stat s;
	arg_inFiles = NULL;
	arg_nbFiles = 0;
	arg_outfile = NULL;
	arg_debug = 0;
	arg_action = OUT_IDC;
	arg_iSMask = SERIALIZE_ALL & ~SERIALIZE_SECTIONS;
	arg_dwBase = 0;
	arg_nidsfile = stat(def_namefile,&s)?arg_nidsfile:def_namefile;
	arg_funcfile = stat(def_funcfile,&s)?arg_funcfile:def_funcfile;
}

int arg_do_serialize(const char *arg){
	arg_iSMask = 0;
	for(int i=0;arg[i];i++){
		switch(tolower((int)arg[i])){
			case 'i' : arg_iSMask |= SERIALIZE_IMPORTS;break;
			case 'x' : arg_iSMask |= SERIALIZE_EXPORTS;break;
			case 'r' : arg_iSMask |= SERIALIZE_RELOCS;break;
			case 's' : arg_iSMask |= SERIALIZE_SECTIONS;break;
			case 'l' : arg_iSMask |= SERIALIZE_DOSYSLIB;break;
			default  : fprintf(stderr, "Unknown serialize option '%c'\n", tolower((int)arg[i]));
			return 0;
		}
	}
	return 1;
}

int arg_do_xmldb(char *arg){
	arg_dbTitle = arg;
	arg_action = OUT_XMLDB;
	return 1;
}

typedef struct{const char
  *full      ,  lb,type,req; void*argvoid             ; int value   ; const char *help;}ArgEntry;

ArgEntry cmd_options[] = {
	{"output"  , 'o', 's', 1 , (void*) &arg_outfile     , 0           , "Outputfile. (stdout)"},
	{"idcout"  , 'c', 'i', 0 , (void*) &arg_action      , OUT_IDC     , "Output an IDC file (default)"},
	{"mapout"  , 'a', 'i', 0 , (void*) &arg_action      , OUT_MAP     , "Output a MAP file"},
	{"xmlout"  , 'x', 'i', 0 , (void*) &arg_action      , OUT_XML     , "Output an XML file"},
	{"elfout"  , 'e', 'i', 0 , (void*) &arg_action      , OUT_ELF     , "Output an ELF from a PRX"},
	{"debug"   , 'd', 'i', 0 , (void*) &arg_debug       , 1           , "Enable debug mode"},
	{"serial"  , 's', 'f', 1 , (void*) &arg_do_serialize, 0           , "Specify what to serialize (Imports,Exports,Relocs,Sections,SyslibExp)"},
	{"xmlfile" , 'n', 's', 1 , (void*) &arg_nidsfile    , 0           , "Specify a XML file containing the NID tables"},
	{"xmldis"  , 'g', 'i', 0 , (void*) &arg_xmlOutput   , 1           , "Enable XML disassembly output mode"},
	{"xmldb"   , 'w', 'f', 1 , (void*) &arg_do_xmldb    , 0           , "Output the PRX(es) as an XML database disassembly with a title" },
	{"xmlstubs", 't', 'i', 0 , (void*) &arg_action      , OUT_STUB    , "Emit old stub files for the XML file passed on the command line"},
	{"xmlstubs", 't', 'i', 0 , (void*) &arg_action      , OUT_STUBNEW , "Emit new stub files for the XML file passed on the command line"},
	{"prxstubs", 'u', 'i', 0 , (void*) &arg_action      , OUT_PSTUB   , "Emit old stub files based on the exports of the specified PRX files" },
	{"prxstubs", 'u', 'i', 0 , (void*) &arg_action      , OUT_PSTUBNEW, "Emit new stub files based on the exports of the specified PRX files"},
	{"depends" , 'q', 'i', 0 , (void*) &arg_action      , OUT_DEP     , "Print PRX dependencies. (Should have loaded an XML file to be useful"},
	{"modinfo" , 'm', 'i', 0 , (void*) &arg_action      , OUT_MOD     , "Print the module and library information to screen"},
	{"impexp"  , 'f', 'i', 0 , (void*) &arg_action      , OUT_IMPEXP  , "Print the imports and exports of a prx"},
	{"exports" , 'p', 'i', 0 , (void*) &arg_action      , OUT_ENT     , "Output an export file (.exp)"},
	{"disasm"  , 'w', 'i', 0 , (void*) &arg_action      , OUT_DISASM  , "Disasm the executable sections of the files (if more than one file output name is automatic)"},
	{"disopts" , 'i', 's', 1 , (void*) &arg_disopts     , 0           , "Specify options for disassembler"},
	{"binary"  , 'b', 'i', 0 , (void*) &arg_loadbin     , 1           , "Load the file as binary for disassembly"},
	{"database", 'l', 'i', 1 , (void*) &arg_database    , 0           , "Specify the offset of the data section in the file for binary disassembly"},
	{"reloc"   , 'r', 'i', 1 , (void*) &arg_dwBase      , 0           , "Relocate the PRX to a different address"},
	{"symbols" , 'y', 'i', 0 , (void*) &arg_action      , OUT_SYMBOLS , "Output a symbol file based on the input file"},
	{"funcs"   , 'z', 's', 1 , (void*) &arg_funcfile    , 0           , "Specify a functions file for disassembly"},
	{"alias"   , 'A', 'i', 0 , (void*) &arg_aliasOutput , 1           , "Print aliases when using -f mode" },
};
#define ARG_COUNT(x) (sizeof(x) / sizeof(ArgEntry))

typedef int (*ArgFunc)(const char *opt);

char **GetArgs(int *argc, char **argv, ArgEntry *entry, int argcount){
	if((!argc) || (!argv) || (!entry))
		return NULL;
	
	for((*argc)--,argv++ ; *argc > 0 ; (*argc)--,argv++){
		if(!*argv)
			return NULL;
		ArgEntry *ent;
		const char *arg = *argv;
		if(arg[0] != '-')break;
		if(arg[1] == '-'){// Long arg
			ent = NULL;
			for(int i = 0;i < argcount;i++){
				if(entry[i].full && !strcmp(entry[i].full, &arg[2]))
					ent = &entry[i];
			}
		}else if(arg[1]){
			ent = NULL;
			/* Error check, short options should be 2 characters */
			if(strlen(arg) == 2){
				for(int i = 0;i < argcount;i++)
					if(entry[i].lb == arg[1])
						ent = &entry[i];
			}
		}else{// Single - means stop processing
			(*argc)--;
			argv++;
			break;
		}
		if(!ent)
			return fprintf(stderr, "Invalid argument %sn", arg),NULL;
		if(!ent->argvoid)
			return fprintf(stderr, "Internal error processing %sn", arg),NULL;
		
		if(ent->req){
			switch(ent->type){
				case 'i':
					*((int *) ent->argvoid) = ent->value;
				break;
				case 'f':
					if(!((ArgFunc) ent->argvoid)(NULL))
						return fprintf(stderr, "Error processing argument for %sn", arg),NULL;
				break;
				default:
					return fprintf(stderr, "Invalid type for option %sn", arg),NULL;
			}
		}else{
			if(*argc <= 1)
				return fprintf(stderr, "No argument passed for %sn", arg),NULL;
			(*argc)--;
			argv++;
			switch(ent->type){
				case 'i':
					*((int*) ent->argvoid) = strtoul(argv[0], NULL, 0);
				break;
				case 's':
					*((const char **) ent->argvoid) = argv[0];
				break;
				case 'f':
					if(!((ArgFunc) ent->argvoid)(argv[0]))
						return fprintf(stderr, "Error processing argument for %sn", arg),NULL;
				break;
				default:
					return fprintf(stderr, "Invalid type for option %sn", arg),NULL;
			}
		}
	}
	return argv;
}
int arg_usage(char* title){
	fprintf(stdout, "%s\n",title?title:"");
	fprintf(stdout, "Usage: prxtool [options...] file\n");
	fprintf(stdout, "Options:\n");

	for(unsigned i = 0; i < ARG_COUNT(cmd_options); i++){
		if(cmd_options[i].help)
			fprintf(stdout, "--%-10s -%c %s\n", cmd_options[i].full, cmd_options[i].lb, cmd_options[i].help);
	}
	fprintf(stdout, "\n");
	fprintf(stdout, "Disassembler Options:\n");
	fprintf(stdout, "x - Print immediates all in hex (not just appropriate ones\n");
	fprintf(stdout, "d - When combined with 'x' prints the hex as signed\n");
	fprintf(stdout, "r - Print CPU registers using rN format rather than mnemonics (i.e. $a0)\n");
	fprintf(stdout, "s - Print the PC as a symbol if possible\n");
	fprintf(stdout, "m - Disable macro instructions (e.g. nop, beqz etc.\n");
	fprintf(stdout, "w - Indicate PC, opcode information goes after the instruction disasm\n");
	return 1;
}


int arg_apply(int argc, char **argv){
	if(argc<=1)
		return arg_usage("No arguments provided");
	
	init_arguments();
	arg_inFiles = GetArgs(&argc, argv, cmd_options, ARG_COUNT(cmd_options));
	if(!arg_inFiles)
		return arg_usage("Error while parsing arguments");
	
	arg_nbFiles = argc;
	return 0;
}
