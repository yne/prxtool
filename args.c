enum ArgTypes{
	ARG_TYPE_INT,
	ARG_TYPE_BOOL,
	ARG_TYPE_STR,
	ARG_TYPE_FUNC,
};

enum ArgOpts{
	ARG_OPT_NONE,
	ARG_OPT_REQUIRED,
};

struct ArgEntry{
	const char *full;
	char ch;
	enum ArgTypes type;
	enum ArgOpts opt;
	void *argvoid;
	int val;
	const char *help;
};

#define ARG_COUNT(x) (sizeof(x) / sizeof(struct ArgEntry))

static struct ArgEntry cmd_options[] = {
	{"output", 'o', ARG_TYPE_STR, ARG_OPT_REQUIRED, (void*) &g_pOutfile, 0, 
		"outfile : Outputfile. If not specified uses stdout"},
	{"idcout", 'c', ARG_TYPE_INT, ARG_OPT_NONE, (void*) &g_outputMode, OUTPUT_IDC, 
		"        : Output an IDC file (default)"},
	{"mapout", 'a', ARG_TYPE_INT, ARG_OPT_NONE, (void*) &g_outputMode, OUTPUT_MAP, 
		"        : Output a MAP file"},
	{"xmlout", 'x', ARG_TYPE_INT, ARG_OPT_NONE, (void*) &g_outputMode, OUTPUT_XML, 
		"        : Output an XML file"},
	{"elfout", 'e', ARG_TYPE_INT, ARG_OPT_NONE, (void*) &g_outputMode, OUTPUT_ELF, 
		"        : Output an ELF from a PRX"},
	{"debug", 'd', ARG_TYPE_BOOL, ARG_OPT_NONE, (void*) &g_blDebug, 1,
		"        : Enable debug mode"},
	{"serial", 's', ARG_TYPE_FUNC, ARG_OPT_REQUIRED, (void*) &do_serialize, 0, 
		"ixrsl   : Specify what to serialize (Imports,Exports,Relocs,Sections,SyslibExp)"},
	{"xmlfile", 'n', ARG_TYPE_STR, ARG_OPT_REQUIRED, (void*) &g_pNamefile, 0, 
		"imp.xml : Specify a XML file containing the NID tables"},
	{"xmldis", 'g', ARG_TYPE_BOOL, ARG_OPT_NONE, (void*) &g_xmlOutput, 1, 
		"        : Enable XML disassembly output mode"},
	{"xmldb",  'w', ARG_TYPE_FUNC, ARG_OPT_REQUIRED, (void*) &do_xmldb, 0,
		"title   : Output the PRX(es) as an XML database disassembly with a title" },
	{"stubs", 't', ARG_TYPE_INT, ARG_OPT_NONE, (void*) &g_outputMode, OUTPUT_STUB, 
		"        : Emit stub files for the XML file passed on the command line"},
	{"prxstubs", 'u', ARG_TYPE_INT, ARG_OPT_NONE, (void*) &g_outputMode, OUTPUT_PSTUB, 
		"        : Emit stub files based on the exports of the specified PRX files" },
	{"newstubs", 'k', ARG_TYPE_BOOL, ARG_OPT_NONE, (void*) &g_newstubs, 1, 
		"        : Emit new style stubs for the SDK"},
	{"depends", 'q', ARG_TYPE_INT, ARG_OPT_NONE, (void*) &g_outputMode, OUTPUT_DEP, 
		"        : Print PRX dependencies. (Should have loaded an XML file to be useful"},
	{"modinfo", 'm', ARG_TYPE_INT, ARG_OPT_NONE, (void*) &g_outputMode, OUTPUT_MOD, 
		"        : Print the module and library information to screen"},
	{"impexp", 'f', ARG_TYPE_INT, ARG_OPT_NONE, (void*) &g_outputMode, OUTPUT_IMPEXP, 
		"        : Print the imports and exports of a prx"},
	{"exports", 'p', ARG_TYPE_INT, ARG_OPT_NONE, (void*) &g_outputMode, OUTPUT_ENT,
		"        : Output an export file (.exp)"},
	{"disasm", 'w', ARG_TYPE_INT, ARG_OPT_NONE, (void*) &g_outputMode, OUTPUT_DISASM, 
		"        : Disasm the executable sections of the files (if more than one file output name is automatic)"},
	{"disopts", 'i', ARG_TYPE_STR, ARG_OPT_REQUIRED, (void*) &g_disopts, 0, 
		"opts    : Specify options for disassembler"},
	{"binary", 'b', ARG_TYPE_BOOL, ARG_OPT_NONE, (void*) &g_loadbin, 1, 
		"        : Load the file as binary for disassembly"},
	{"database", 'l', ARG_TYPE_INT, ARG_OPT_REQUIRED, (void*) &g_database, 0, 
		"        : Specify the offset of the data section in the file for binary disassembly"},
	{"reloc", 'r', ARG_TYPE_INT, ARG_OPT_REQUIRED, (void*) &g_dwBase, 0, 
		"addr    : Relocate the PRX to a different address"},
	{"symbols", 'y', ARG_TYPE_INT, ARG_OPT_NONE, (void*) &g_outputMode, OUTPUT_SYMBOLS, 
		"Output a symbol file based on the input file"},
	{"funcs", 'z', ARG_TYPE_STR, ARG_OPT_REQUIRED, (void*) &g_pFuncfile, 0, 
		"        : Specify a functions file for disassembly"},
	{"alias", 'A', ARG_TYPE_BOOL, ARG_OPT_NONE, (void*) &g_aliasOutput, 1, 
		"        : Print aliases when using -f mode" },
};

typedef enum{
	LEVEL_INFO = 0,
	LEVEL_WARNING = 1,
	LEVEL_ERROR = 2,
	LEVEL_DEBUG = 3
}OutputLevel;
typedef int (*ArgFunc)(const char *opt);

void DoOutput(OutputLevel level, const char *str){
	switch(level){
		case LEVEL_INFO: fprintf(stderr, "%s", str);break;
		case LEVEL_WARNING: fprintf(stderr, "Warning: %s", str);break;
		case LEVEL_ERROR: fprintf(stderr, "Error: %s", str);break;
		case LEVEL_DEBUG: fprintf(stderr, "Debug: %s", str);break;
		default: fprintf(stderr, "Unknown Level: %s", str);break;
	};
}

char **GetArgs(int *argc, char **argv, struct ArgEntry *entry, int argcount){
	int error = 0;

	if((argc == NULL) || (argv == NULL) || (entry == NULL)){
		return NULL;
	}

	(*argc)--;
	argv++;

	while(*argc > 0){
		const char *arg;
		struct ArgEntry *ent;

		if(*argv == NULL){
			error = 1;
			break;
		}
		arg = *argv;

		if(arg[0] != '-'){
			break;
		}

		if(arg[1] == '-'){
			/* Long arg */
			int i;

			ent = NULL;
			for(i = 0; i < argcount; i++){
				if(entry[i].full){
					if(strcmp(entry[i].full, &arg[2]) == 0){
						ent = &entry[i];
					}
				}
			}
		}
		else if(arg[1] != 0){
			int i;

			ent = NULL;

			/* Error check, short options should be 2 characters */
			if(strlen(arg) == 2){
				for(i = 0; i < argcount; i++){
					if(entry[i].ch == arg[1]){
						ent = &entry[i];
					}
				}
			}
		}else{
			/* Single - means stop processing */
			(*argc)--;
			argv++;
			break;
		}

		if(ent == NULL){
			fprintf(stderr, "Invalid argument %s\n", arg);
			error = 1;
			break;
		}

		if(ent->argvoid == NULL){
			fprintf(stderr, "Internal error processing %s\n", arg);
			error = 1;
			break;
		}

		if(ent->opt == ARG_OPT_NONE){
			switch(ent->type){
				case ARG_TYPE_BOOL: { 
										int *argint = (int *) ent->argvoid;
										*argint = ent->val;
									}
									break;
				case ARG_TYPE_INT: {
										int *argint = (int *) ent->argvoid;
										*argint = ent->val;
									 }
									 break;
				case ARG_TYPE_FUNC: { ArgFunc argfunc = (ArgFunc) ent->argvoid; 
								    if(argfunc(NULL) == 0){
										fprintf(stderr, "Error processing argument for %s\n", arg);
										error = 1;
									}
									}
									break;
				default: fprintf(stderr, "Invalid type for option %s\n", arg);
						 error = 1;
						 break;
			};

			if(error){
				break;
			}
		}
		else if(ent->opt == ARG_OPT_REQUIRED){
			if(*argc <= 1){
				fprintf(stderr, "No argument passed for %s\n", arg);
				error = 1;
				break;
			}
			(*argc)--;
			argv++;

			switch(ent->type){
				case ARG_TYPE_INT: { int *argint = (int*) ent->argvoid;
								   *argint = strtoul(argv[0], NULL, 0);
								   }
								   break;
				case ARG_TYPE_STR: { const char **argstr = (const char **) ent->argvoid;
								   *argstr = argv[0];
								   }
								   break;
				case ARG_TYPE_FUNC: { ArgFunc argfunc = (ArgFunc) ent->argvoid; 
								    if(argfunc(argv[0]) == 0){
										fprintf(stderr, "Error processing argument for %s\n", arg);
										error = 1;
									}
									}
									break;
				default: fprintf(stderr, "Invalid type for option %s\n", arg);
						 error = 1;
						 break;
			};

			if(error){
				break;
			}
		}else{
			fprintf(stderr, "Internal options error processing %s\n", arg);
			error = 1;
			break;
		}

		(*argc)--;
		argv++;
	}

	if(error){
		return NULL;
	}else{
		return argv;
	}
}

void init_arguments(){
	g_ppInfiles = NULL;
	g_iInFiles = 0;
	g_pOutfile = NULL;
	g_blDebug = 0;
	g_outputMode = OUTPUT_IDC;
	g_iSMask = SERIALIZE_ALL & ~SERIALIZE_SECTIONS;
	g_newstubs = 0;
	g_dwBase = 0;

	struct stat s;

	if(stat(g_namepath, &s) == 0){
		g_pNamefile = g_namepath;
	}
	if(stat(g_funcpath, &s) == 0){
		g_pFuncfile = g_funcpath;
	}
}
char **GetArgs(int *argc, char **argv, struct ArgEntry *entry, int argcount){
	int error = 0;

	if((argc == NULL) || (argv == NULL) || (entry == NULL)){
		return NULL;
	}

	(*argc)--;
	argv++;

	while(*argc > 0){
		const char *arg;
		struct ArgEntry *ent;

		if(*argv == NULL){
			error = 1;
			break;
		}
		arg = *argv;

		if(arg[0] != '-'){
			break;
		}

		if(arg[1] == '-'){
			/* Long arg */
			int i;

			ent = NULL;
			for(i = 0; i < argcount; i++){
				if(entry[i].full){
					if(strcmp(entry[i].full, &arg[2]) == 0){
						ent = &entry[i];
					}
				}
			}
		}
		else if(arg[1] != 0){
			int i;

			ent = NULL;

			/* Error check, short options should be 2 characters */
			if(strlen(arg) == 2){
				for(i = 0; i < argcount; i++){
					if(entry[i].ch == arg[1]){
						ent = &entry[i];
					}
				}
			}
		}else{
			/* Single - means stop processing */
			(*argc)--;
			argv++;
			break;
		}

		if(ent == NULL){
			fprintf(stderr, "Invalid argument %s\n", arg);
			error = 1;
			break;
		}

		if(ent->argvoid == NULL){
			fprintf(stderr, "Internal error processing %s\n", arg);
			error = 1;
			break;
		}

		if(ent->opt == ARG_OPT_NONE){
			switch(ent->type){
				case ARG_TYPE_BOOL: { 
										int *argint = (int *) ent->argvoid;
										*argint = ent->val;
									}
									break;
				case ARG_TYPE_INT: {
										int *argint = (int *) ent->argvoid;
										*argint = ent->val;
									 }
									 break;
				case ARG_TYPE_FUNC: { ArgFunc argfunc = (ArgFunc) ent->argvoid; 
								    if(argfunc(NULL) == 0){
										fprintf(stderr, "Error processing argument for %s\n", arg);
										error = 1;
									}
									}
									break;
				default: fprintf(stderr, "Invalid type for option %s\n", arg);
						 error = 1;
						 break;
			};

			if(error){
				break;
			}
		}
		else if(ent->opt == ARG_OPT_REQUIRED){
			if(*argc <= 1){
				fprintf(stderr, "No argument passed for %s\n", arg);
				error = 1;
				break;
			}
			(*argc)--;
			argv++;

			switch(ent->type){
				case ARG_TYPE_INT: { int *argint = (int*) ent->argvoid;
								   *argint = strtoul(argv[0], NULL, 0);
								   }
								   break;
				case ARG_TYPE_STR: { const char **argstr = (const char **) ent->argvoid;
								   *argstr = argv[0];
								   }
								   break;
				case ARG_TYPE_FUNC: { ArgFunc argfunc = (ArgFunc) ent->argvoid; 
								    if(argfunc(argv[0]) == 0){
										fprintf(stderr, "Error processing argument for %s\n", arg);
										error = 1;
									}
									}
									break;
				default: fprintf(stderr, "Invalid type for option %s\n", arg);
						 error = 1;
						 break;
			};

			if(error){
				break;
			}
		}else{
			fprintf(stderr, "Internal options error processing %s\n", arg);
			error = 1;
			break;
		}

		(*argc)--;
		argv++;
	}

	if(error){
		return NULL;
	}else{
		return argv;
	}
}

int process_args(int argc, char **argv){
	init_arguments();

	g_ppInfiles = GetArgs(&argc, argv, cmd_options, ARG_COUNT(cmd_options));
	if((g_ppInfiles) && (argc > 0)){
		g_iInFiles = argc;
	}else{
		return 0;
	}

	return 1;
}

void print_help(){
	unsigned int i;
	fprintf(stdout, "Usage: prxtool [options...] file\n");
	fprintf(stdout, "Options:\n");

	for(i = 0; i < ARG_COUNT(cmd_options); i++){
		if(cmd_options[i].help){
			fprintf(stdout, "--%-10s -%c %s\n", cmd_options[i].full, cmd_options[i].ch, cmd_options[i].help);
		}
	}
	fprintf(stdout, "\n");
	fprintf(stdout, "Disassembler Options:\n");
	fprintf(stdout, "x - Print immediates all in hex (not just appropriate ones\n");
	fprintf(stdout, "d - When combined with 'x' prints the hex as signed\n");
	fprintf(stdout, "r - Print CPU registers using rN format rather than mnemonics (i.e. $a0)\n");
	fprintf(stdout, "s - Print the PC as a symbol if possible\n");
	fprintf(stdout, "m - Disable macro instructions (e.g. nop, beqz etc.\n");
	fprintf(stdout, "w - Indicate PC, opcode information goes after the instruction disasm\n");
}