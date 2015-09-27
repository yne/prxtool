/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * NidMgr.C - Implementation of a class to manipulate a list
 * of NID Libraries.
 **************************************************************
*/
#define LIB_NAME_MAX 64
#define LIB_SYMBOL_NAME_MAX 128
#define FUNCTION_NAME_MAX   128
#define FUNCTION_ARGS_MAX   128
#define FUNCTION_RET_MAX    64

struct LibraryEntry;
typedef struct{
	uint32_t nid;
	char name[LIB_SYMBOL_NAME_MAX];
	struct LibraryEntry *pParentLib;
}LibraryNid;

struct FunctionType{
	char name[FUNCTION_NAME_MAX];
	char args[FUNCTION_ARGS_MAX];
	char ret[FUNCTION_RET_MAX];
};

typedef struct{
	struct LibraryEntry* pNext;
	char prx_name[LIB_NAME_MAX];
	char lib_name[LIB_NAME_MAX];
	char prx[PATH_MAX];
	int  flags;
	int  entry_count;
	int  vcount;
	int  fcount;
	LibraryNid *pNids;
}LibraryEntry;

typedef struct{
	unsigned int nid;
	const char *name;
}SyslibEntry;

static SyslibEntry g_syslib[] = {
	{ 0xd3744be0, "module_bootstart" },
	{ 0xf01d73a7, "module_info" },
	{ 0x2f064fa6, "module_reboot_before" },
	{ 0xadf12745, "module_reboot_phase" },
	{ 0xd632acdb, "module_start" },
	{ 0x0f7c276c, "module_start_thread_parameter" },
	{ 0xcee8593c, "module_stop" },
	{ 0xcf0cc697, "module_stop_thread_parameter" },
	{ 0x11b97506, "module_sdk_version" },
};

#define MASTER_NID_MAPPER "MasterNidMapper"

/*
// Generate a simple name based on the library and the nid 
const char *NidGenName(char *m_szCurrName,const char *lib, uint32_t nid){
	if(lib == NULL){
		snprintf(m_szCurrName, LIB_SYMBOL_NAME_MAX, "syslib_%08X", nid);
	}else{
		snprintf(m_szCurrName, LIB_SYMBOL_NAME_MAX, "%s_%08X", lib, nid);
	}

	return m_szCurrName;
}

// Search the NID list for a function and return the name 
const char *NidSearchLibs(LibraryEntry *m_pMasterNids,const char *lib, uint32_t nid){
	const char *pName = NULL;
	LibraryEntry *pLib;

	if(m_pMasterNids){
		pLib = m_pMasterNids;
	}else{
		pLib = m_pLibHead;
	}

	// Very lazy, could be sped up using a hash table 
	while(pLib != NULL){
		if((strcmp(lib, pLib->lib_name) == 0) || (m_pMasterNids)){
			int iNidLoop;

			for(iNidLoop = 0; iNidLoop < pLib->entry_count; iNidLoop++){
				if(pLib->pNids[iNidLoop].nid == nid){
					pName = pLib->pNids[iNidLoop].name;
					fprintf(stdout,"Using %s, nid %08X\n", pName, nid);
					break;
				}
			}

			if(pName != NULL){
				break;
			}
		}

		if(m_pMasterNids) {
			pLib = NULL;
		}else{
			pLib = pLib->pNext;
		}
	}

	if(pName == NULL){
		// First check special case system library stuff 
		if(strcmp(lib, PSP_SYSTEM_EXPORT) == 0){
			int size;
			int i;

			size = sizeof(g_syslib) / sizeof(SyslibEntry);
			for(i = 0; i < size; i++){
				if(nid == g_syslib[i].nid){
					pName = g_syslib[i].name;
					break;
				}
			}
		}

		if(pName == NULL){
			COutput::Puts(LEVEL_DEBUG, "Using default name");
			pName = GenName(lib, nid);
		}
	}

	return pName;
}

// Read the NID data from the XML file 
const char* NidReadNid(TiXmlElement *pElement, uint32_t &nid){
	TiXmlHandle nidHandle(pElement);
	TiXmlText *pNid;
	TiXmlText *pName;
	const char* szName;

	szName = NULL;
	pNid = nidHandle.FirstChild("NID").FirstChild().Text();
	pName = nidHandle.FirstChild("NAME").FirstChild().Text();

	if((pNid != NULL) && (pName != NULL)){
		nid = strtoul(pNid->Value(), NULL, 16);
		szName = pName->Value();
	}

	return szName;
}

// Count the number of nids in the current element 
int NidCountNids(TiXmlElement *pElement, const char *name){
	TiXmlElement *pIterator;
	uint32_t nid;
	int iCount = 0;

	pIterator = pElement;
	while(pIterator != NULL){
		if(ReadNid(pIterator, nid) != NULL){
			iCount++;
		}
		pIterator = pIterator->NextSiblingElement(name);
	}

	return iCount;
}

// Process a library XML element 
void NidProcessLibrary(TiXmlElement *pLibrary, const char *prx_name, const char *prx){
	TiXmlHandle libHandle(pLibrary);
	TiXmlText *elmName;
	TiXmlText *elmFlags;
	TiXmlElement *elmFunction;
	TiXmlElement *elmVariable;
	int fCount;
	int vCount;
	int blMasterNids = 0;
	
	assert(prx_name != NULL);
	assert(prx != NULL);

	elmName = libHandle.FirstChild("NAME").FirstChild().Text();
	elmFlags = libHandle.FirstChild("FLAGS").FirstChild().Text();
	if(elmName){
		LibraryEntry *pLib;

		fprintf(stdout,"Library %s\n", elmName->Value());
		SAFE_ALLOC(pLib, LibraryEntry);
		if(pLib != NULL){
			memset(pLib, 0, sizeof(LibraryEntry));
			strcpy(pLib->lib_name, elmName->Value());
			if(strcmp(pLib->lib_name, MASTER_NID_MAPPER) == 0){
				blMasterNids = 1;
				fprintf(stdout,"Found master NID table\n");
			}

			if(elmFlags){
				pLib->flags = strtoul(elmFlags->Value(), NULL, 16);
			}

			strcpy(pLib->prx_name, prx_name);
			strcpy(pLib->prx, prx);
			elmFunction = libHandle.FirstChild("FUNCTIONS").FirstChild("FUNCTION").Element();
			elmVariable = libHandle.FirstChild("VARIABLES").FirstChild("VARIABLE").Element();
			fCount = CountNids(elmFunction, "FUNCTION");
			vCount = CountNids(elmVariable, "VARIABLE");
			pLib->vcount = vCount;
			pLib->fcount = fCount;
			if((fCount+vCount) > 0){
				SAFE_ALLOC(pLib->pNids, LibraryNid[vCount+fCount]);
				if(pLib->pNids != NULL){
					int iLoop;
					const char *pName;

					memset(pLib->pNids, 0, sizeof(LibraryNid) * (vCount+fCount));
					pLib->entry_count = vCount + fCount;
					iLoop = 0;
					while(elmFunction != NULL){
						pName = ReadNid(elmFunction, pLib->pNids[iLoop].nid);
						if(pName){
							pLib->pNids[iLoop].pParentLib = pLib;
							strcpy(pLib->pNids[iLoop].name, pName);
							fprintf(stdout,"Read func:%s nid:0x%08X\n", pLib->pNids[iLoop].name, pLib->pNids[iLoop].nid);
							iLoop++;
						}

						elmFunction = elmFunction->NextSiblingElement("FUNCTION");
					}

					while(elmVariable != NULL){
						pName = ReadNid(elmVariable, pLib->pNids[iLoop].nid);
						if(pName){
							strcpy(pLib->pNids[iLoop].name, pName);
							fprintf(stdout,"Read var:%s nid:0x%08X\n", pLib->pNids[iLoop].name, pLib->pNids[iLoop].nid);
							iLoop++;
						}

						elmVariable = elmVariable->NextSiblingElement("VARIABLE");
					}
				}
			}

			// Link into list 
			if(m_pLibHead == NULL){
				m_pLibHead = pLib;
			}else{
				pLib->pNext = m_pLibHead;
				m_pLibHead = pLib;
			}

			if(blMasterNids){
				m_pMasterNids = pLib;
			}
		}

		// Allocate library memory 
	}
}

// Process a PRXFILE XML element 
void NidProcessPrxfile(TiXmlElement *pPrxfile){
	TiXmlHandle prxHandle(pPrxfile);
	TiXmlElement *elmLibrary;
	TiXmlText *txtName;
	TiXmlText *txtPrx;
	const char *szPrx;

	txtPrx = prxHandle.FirstChild("PRX").FirstChild().Text();
	txtName = prxHandle.FirstChild("PRXNAME").FirstChild().Text();

	elmLibrary = prxHandle.FirstChild("LIBRARIES").FirstChild("LIBRARY").Element();
	while(elmLibrary){
		COutput::Puts(LEVEL_DEBUG, "Found LIBRARY");

		if(txtPrx == NULL){
			szPrx = "unknown.prx";
		}else{
			szPrx = txtPrx->Value();
		}

		if(txtName != NULL){
			ProcessLibrary(elmLibrary, txtName->Value(), szPrx);
		}

		elmLibrary = elmLibrary->NextSiblingElement("LIBRARY");
	}
}

// Add an XML file to the current library list 
int NidAddXmlFile(const char *szFilename){
	TiXmlDocument doc(szFilename);
	int blRet = 0;

	if(doc.LoadFile()){
		fprintf(stdout,"Loaded XML file %s", szFilename);
		TiXmlHandle docHandle(&doc);
		TiXmlElement *elmPrxfile;

		elmPrxfile = docHandle.FirstChild("PSPLIBDOC").FirstChild("PRXFILES").FirstChild("PRXFILE").Element();
		while(elmPrxfile){
			COutput::Puts(LEVEL_DEBUG, "Found PRXFILE");
			ProcessPrxfile(elmPrxfile);

			elmPrxfile = elmPrxfile->NextSiblingElement("PRXFILE");
		}
		blRet = 1;
	}else{
		fprintf(stderr, "Couldn't load xml file %s\n", szFilename);
	}

	return blRet;
}

// Find the name based on our list of names 
const char *NidFindLibName(const char *lib, uint32_t nid){
	return SearchLibs(lib, nid);
}

LibraryEntry *NidGetLibraries(void){
	return m_pLibHead;
}

// Find the name of the dependany library for a specified lib 
const char *NidFindDependancy(const char *lib){
	LibraryEntry *pLib;

	pLib = m_pLibHead;

	while(pLib != NULL){
		if(strcmp(pLib->lib_name, lib) == 0){
			return pLib->prx;
		}

		pLib = pLib->pNext;
	}

	return NULL;
}

static char *strip_whitesp(char *str){
	int len;

	while(isspace(*str)){
		str++;
	}

	len = strlen(str);
	while((len > 0) && (isspace(str[len-1]))){
		str[len-1] = 0;
		len--;
	}

	if(len == 0){
		return NULL;
	}
	
	return str;
}

int NidAddFunctionFile(const char *szFilename){
	FILE *fp;

	fp = fopen(szFilename, "r");
	if(fp){
		char line[1024];

		while(fgets(line, sizeof(line), fp)){
			char *name;
			char *args = NULL;
			char *ret = NULL;

			name = strip_whitesp(line);
			if(name == NULL){
				continue;
			}

			args = strchr(name, '|');
			if(args){
				*args++ = 0;
				ret = strchr(args, '|');
				if(ret){
					*ret++ = 0;
				}
			}

			if((name) && (name[0] != '#')){
				FunctionType *p = new FunctionType;

				memset(p, 0, sizeof(FunctionType));
				snprintf(p->name, FUNCTION_NAME_MAX, "%s", name);
				if(args){
					snprintf(p->args, FUNCTION_ARGS_MAX, "%s", args);
				}
				if(ret){
					snprintf(p->ret, FUNCTION_RET_MAX, "%s", ret);
				}
				m_funcMap.insert(m_funcMap.end(), p);
				fprintf(stdout,"Function: %s %s(%s)\n", p->ret, p->name, p->args);
			}
		}
		fclose(fp);
		return 1;
	}

	return 0;
}

FunctionType *NidFindFunctionType(const char *name){
	FunctionType *ret = NULL;

	for(unsigned int i = 0; i < m_funcMap.size(); i++){
		FunctionType *p = NULL;
		p = m_funcMap[i];
		if((p) && (strcmp(name, p->name) == 0)){
			ret = p;
			break;
		}
	}

	return ret;
}
*/