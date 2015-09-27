enum {
	SERIALIZE_IMPORTS  = (1 << 0),
	SERIALIZE_EXPORTS  = (1 << 1),
	SERIALIZE_SECTIONS = (1 << 2),
	SERIALIZE_RELOCS   = (1 << 3),
	SERIALIZE_DOSYSLIB = (1 << 4),
	SERIALIZE_ALL	   = 0xFFFFFFFF
};
typedef enum{
	OUTPUT_NONE = 0,
	OUTPUT_IDC = 1,
	OUTPUT_MAP = 2,
	OUTPUT_XML = 3,
	OUTPUT_ELF = 4,
	OUTPUT_STUB = 6,
	OUTPUT_DEP = 7,
	OUTPUT_MOD = 8,
	OUTPUT_PSTUB = 9,
	OUTPUT_IMPEXP = 10,
	OUTPUT_SYMBOLS = 11,
	OUTPUT_DISASM  = 12,
	OUTPUT_XMLDB = 13,
	OUTPUT_ENT = 14,
}OutputMode;

char**       arg_ppInfiles;
int          arg_iInFiles;
char *       arg_pOutfile;
char *       arg_pNamefile;
char *       arg_pFuncfile;
int          arg_blDebug;
OutputMode   arg_outputMode;
uint32_t     arg_iSMask;
int          arg_newstubs;
uint32_t     arg_dwBase;
const char*  arg_disopts = "";
int          arg_loadbin = 0;
int          arg_xmlOutput = 0;
int          arg_aliasOutput = 0;
unsigned int arg_database = 0;
const char * arg_pDbTitle = "";

int arg_do_serialize(const char *arg){
	arg_iSMask = 0;
	for(int i=0;arg[i];i++){
		switch(tolower(arg[i])){
			case 'i' : arg_iSMask |= SERIALIZE_IMPORTS;break;
			case 'x' : arg_iSMask |= SERIALIZE_EXPORTS;break;
			case 'r' : arg_iSMask |= SERIALIZE_RELOCS;break;
			case 's' : arg_iSMask |= SERIALIZE_SECTIONS;break;
			case 'l' : arg_iSMask |= SERIALIZE_DOSYSLIB;break;
			default:   fprintf(stderr, "Unknown serialize option '%c'\n", tolower(arg[i]));
			return 0;
		}
	}

	return 1;
}

int arg_do_xmldb(const char *arg){
	arg_pDbTitle = arg;
	arg_outputMode = OUTPUT_XMLDB;
	return 1;
}

typedef struct{const char
  *full      ,  ch,type,req; void*argvoid              ; const int val ; const char *help;}ArgEntry;

ArgEntry cmd_options[] = {
	{"output"  , 'o', 's', 1 , (void*) &arg_pOutfile     , 0             , "outfile : Outputfile. If not specified uses stdout"},
	{"idcout"  , 'c', 'i', 0 , (void*) &arg_outputMode   , OUTPUT_IDC    , "        : Output an IDC file (default)"},
	{"mapout"  , 'a', 'i', 0 , (void*) &arg_outputMode   , OUTPUT_MAP    , "        : Output a MAP file"},
	{"xmlout"  , 'x', 'i', 0 , (void*) &arg_outputMode   , OUTPUT_XML    , "        : Output an XML file"},
	{"elfout"  , 'e', 'i', 0 , (void*) &arg_outputMode   , OUTPUT_ELF    , "        : Output an ELF from a PRX"},
	{"debug"   , 'd', 'i', 0 , (void*) &arg_blDebug      , 1             , "        : Enable debug mode"},
	{"serial"  , 's', 'f', 1 , (void*) &arg_do_serialize , 0             , "ixrsl   : Specify what to serialize (Imports,Exports,Relocs,Sections,SyslibExp)"},
	{"xmlfile" , 'n', 's', 1 , (void*) &arg_pNamefile    , 0             , "imp.xml : Specify a XML file containing the NID tables"},
	{"xmldis"  , 'g', 'i', 0 , (void*) &arg_xmlOutput    , 1             , "        : Enable XML disassembly output mode"},
	{"xmldb"   , 'w', 'f', 1 , (void*) &arg_do_xmldb     , 0             , "title   : Output the PRX(es) as an XML database disassembly with a title" },
	{"stubs"   , 't', 'i', 0 , (void*) &arg_outputMode   , OUTPUT_STUB   , "        : Emit stub files for the XML file passed on the command line"},
	{"prxstubs", 'u', 'i', 0 , (void*) &arg_outputMode   , OUTPUT_PSTUB  , "        : Emit stub files based on the exports of the specified PRX files" },
	{"newstubs", 'k', 'i', 0 , (void*) &arg_newstubs     , 1             , "        : Emit new style stubs for the SDK"},
	{"depends" , 'q', 'i', 0 , (void*) &arg_outputMode   , OUTPUT_DEP    , "        : Print PRX dependencies. (Should have loaded an XML file to be useful"},
	{"modinfo" , 'm', 'i', 0 , (void*) &arg_outputMode   , OUTPUT_MOD    , "        : Print the module and library information to screen"},
	{"impexp"  , 'f', 'i', 0 , (void*) &arg_outputMode   , OUTPUT_IMPEXP , "        : Print the imports and exports of a prx"},
	{"exports" , 'p', 'i', 0 , (void*) &arg_outputMode   , OUTPUT_ENT    , "        : Output an export file (.exp)"},
	{"disasm"  , 'w', 'i', 0 , (void*) &arg_outputMode   , OUTPUT_DISASM , "        : Disasm the executable sections of the files (if more than one file output name is automatic)"},
	{"disopts" , 'i', 's', 1 , (void*) &arg_disopts      , 0             , "opts    : Specify options for disassembler"},
	{"binary"  , 'b', 'i', 0 , (void*) &arg_loadbin      , 1             , "        : Load the file as binary for disassembly"},
	{"database", 'l', 'i', 1 , (void*) &arg_database     , 0             , "        : Specify the offset of the data section in the file for binary disassembly"},
	{"reloc"   , 'r', 'i', 1 , (void*) &arg_dwBase       , 0             , "addr    : Relocate the PRX to a different address"},
	{"symbols" , 'y', 'i', 0 , (void*) &arg_outputMode   , OUTPUT_SYMBOLS, "        : Output a symbol file based on the input file"},
	{"funcs"   , 'z', 's', 1 , (void*) &arg_pFuncfile    , 0             , "        : Specify a functions file for disassembly"},
	{"alias"   , 'A', 'i', 0 , (void*) &arg_aliasOutput  , 1             , "        : Print aliases when using -f mode" },
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
					if(entry[i].ch == arg[1])
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
					*((int *) ent->argvoid) = ent->val;
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

void init_arguments(){
	arg_ppInfiles = NULL;
	arg_iInFiles = 0;
	arg_pOutfile = NULL;
	arg_blDebug = 0;
	arg_outputMode = OUTPUT_IDC;
	arg_iSMask = SERIALIZE_ALL & ~SERIALIZE_SECTIONS;
	arg_newstubs = 0;
	arg_dwBase = 0;

	struct stat s;

	char*def_namepath = "psplibdoc.xml";
	if(!stat(def_namepath, &s))
		arg_pNamefile = def_namepath;
	char*def_funcpath = "functions.txt";
	if(!stat(def_funcpath, &s))
		arg_pFuncfile = def_funcpath;
}

int process_args(int argc, char **argv){
	init_arguments();

	arg_ppInfiles = GetArgs(&argc, argv, cmd_options, ARG_COUNT(cmd_options));
	if((arg_ppInfiles) && (argc > 0)){
		arg_iInFiles = argc;
		return 0;
	}
	return 1;
}

int print_help(){
	fprintf(stdout, "Usage: prxtool [options...] file\n");
	fprintf(stdout, "Options:\n");

	for(unsigned i = 0; i < ARG_COUNT(cmd_options); i++){
		if(cmd_options[i].help)
			fprintf(stdout, "--%-10s -%c %s\n", cmd_options[i].full, cmd_options[i].ch, cmd_options[i].help);
	}
	fprintf(stdout, "\n");
	fprintf(stdout, "Disassembler Options:\n");
	fprintf(stdout, "x - Print immediates all in hex (not just appropriate ones\n");
	fprintf(stdout, "d - When combined with 'x' prints the hex as signed\n");
	fprintf(stdout, "r - Print CPU registers using rN format rather than mnemonics (i.e. $a0)\n");
	fprintf(stdout, "s - Print the PC as a symbol if possible\n");
	fprintf(stdout, "m - Disable macro instructions (e.g. nop, beqz etc.\n");
	fprintf(stdout, "w - Indicate PC, opcode information goes after the instruction disasm\n");
	return 0;
}
