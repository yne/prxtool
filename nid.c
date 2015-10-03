/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * NidMgr.C - Implementation of a class to manipulate a list
 * of NID Libraries.
 **************************************************************
*/
#include <string.h>

#define LIB_NAME_MAX 64
#define LIB_SYMBOL_NAME_MAX 128
#define FUNCTION_NAME_MAX   128
#define FUNCTION_ARGS_MAX   128
#define FUNCTION_RET_MAX    64
#define LIB_NID_MAX         1050//Max Nid (paf/pafmini)

typedef struct{
	uint32_t nid;
	char name[LIB_SYMBOL_NAME_MAX];
}LibraryNid;

typedef struct{
	char name[FUNCTION_NAME_MAX];
	char args[FUNCTION_ARGS_MAX];
	char ret[FUNCTION_RET_MAX];
}FunctionType;

typedef struct{
	char prx_name[LIB_NAME_MAX];
	char lib_name[LIB_NAME_MAX];
	char prx[PATH_MAX];
	int  flags;
	int  entry_count;
	int  vcount;
	int  fcount;
	LibraryNid pNids[LIB_NID_MAX];
}LibraryEntry;


typedef struct{
	unsigned libraries_count,functions_count;
	LibraryEntry *libraries;
	FunctionType *functions;
}CNidMgr;

typedef struct{unsigned nid;char *name;}SyslibEntry;
SyslibEntry g_syslib[] = {
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

static char *strip_whitesp(char *str){
	while(isspace((int)*str))
		str++;

	int len = strlen(str);
	while((len > 0) && (isspace((int)str[len-1])))
		str[--len] = 0;

	return len?str:NULL;
}

int FuncFromTxt(FunctionType *func,const char* szFilename){
	FILE *fp = fopen(szFilename, "r");
	if(!fp)
		return fprintf(stderr,"Unable to Open \"%s\"\n", szFilename),0;
	
	char line[1024];
	int num;
	for(num=0; fgets(line, sizeof(line), fp);){
		char *args,*name,*ret = NULL;

		if(!(name = strip_whitesp(line)))
			continue;
		
		if((args = strchr(name, '|'))){
			*args++ = 0;
			if((ret = strchr(args, '|')))
				*ret++ = 0;
		}

		if(func && name && (name[0] != '#')){
			memset(&func[num], 0, sizeof(FunctionType));
			snprintf(func[num].name, FUNCTION_NAME_MAX, "%s", name);
			if(args)
				snprintf(func[num].args, FUNCTION_ARGS_MAX, "%s", args);
			if(ret)
				snprintf(func[num].ret, FUNCTION_RET_MAX, "%s", ret);
			//fprintf(stdout,"Function: %s %s(%s)\n", func[num].ret, func[num].name, func[num].args);
			num++;
		}
	}
	fclose(fp);
	return num;
}

FunctionType *NidFindFunctionType(CNidMgr *mgr,const char *name){
	for(unsigned int i = 0; i < mgr->functions_count; i++)
		if(!strcmp(name, mgr->functions[i].name))
			return &mgr->functions[i];
	return NULL;
}
int NidsFromXml(LibraryEntry *libraries,const char* szFilename){
	FILE *fp = fopen(szFilename, "r");
	if(!fp)
		return fprintf(stderr,"Unable to Open \"%s\"\n", szFilename),0;
}


// Search the NID list for a function and return the name 
const char *NidFindLibName(LibraryEntry *m_pMasterNids,const char *lib, uint32_t nid){
	const char *pName = NULL;
/*
	// Very lazy, could be sped up using a hash table 
	for(LibraryEntry *pLib = m_pMasterNids?m_pMasterNids:m_pLibHead;pLib;pLib = m_pMasterNids?NULL:pLib->pNext)
		if((strcmp(lib, pLib->lib_name) == 0) || (m_pMasterNids))
			for(int iNidLoop = 0; iNidLoop < pLib->entry_count; iNidLoop++)
				if(pLib->pNids[iNidLoop].nid == nid)
					if((pName = pLib->pNids[iNidLoop].name))
						return pName;
	// First check special case system library stuff 
	if(strcmp(lib, PSP_SYSTEM_EXPORT) == 0)
		for(int i = 0; i < sizeof(g_syslib) / sizeof(SyslibEntry); i++)
			if(nid == g_syslib[i].nid)
				if((pName = g_syslib[i].name))
					return pName;
	snprintf(pName, LIB_SYMBOL_NAME_MAX, "%s_%08X", lib?lib:"syslib", nid);
*/
	return pName;
}

// Read the NID data from the XML file 
const char* NidReadNid(void *pElement, uint32_t *nid){
//	TiXmlHandle nidHandle(pElement);
//	TiXmlText *pNid = nidHandle.FirstChild("NID").FirstChild().Text();
//	TiXmlText *pName = nidHandle.FirstChild("NAME").FirstChild().Text();
//
//	if(pNid && pName){
//		*nid = strtoul(pNid->Value(), NULL, 16);
//		return pName->Value();
//	}

	return NULL;
}

// Count the number of nids in the current element 
int NidCountNids(void *pElement, const char *name){
	uint32_t nid;
	int iCount = 0;
	
	//for(TiXmlElement *pIterator = pElement;pIterator;pIterator = pIterator->NextSiblingElement(name))
	//	if(ReadNid(pIterator, nid))
	//		iCount++;
	return iCount;
}

// Process a library XML element 
void NidProcessLibrary(void *pLibrary, const char *prx_name, const char *prx){
/*
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
*/
}

// Process a PRXFILE XML element 
void NidProcessPrxfile(void*pPrxfile){
//	TiXmlHandle prxHandle(pPrxfile);
//	TiXmlText *txtName = prxHandle.FirstChild("PRXNAME").FirstChild().Text();
//	TiXmlText *txtPrx = prxHandle.FirstChild("PRX").FirstChild().Text();
//	for(TiXmlElement *elmLibrary = prxHandle.FirstChild("LIBRARIES").FirstChild("LIBRARY").Element();elmLibrary;elmLibrary = elmLibrary->NextSiblingElement("LIBRARY"))
//		if(txtName)
//			ProcessLibrary(elmLibrary, txtName->Value(), txtPrx?txtPrx->Value():"unknown.prx");
}

// Add an XML file to the current library list 
int NidAddXmlFile(const char *szFilename){
//	TiXmlDocument doc(szFilename);
//	if(doc.LoadFile()){
//		fprintf(stdout,"Loaded XML file %s", szFilename);
//		TiXmlHandle docHandle(&doc);
//		while(TiXmlElement *elmPrxfile = docHandle.FirstChild("PSPLIBDOC").FirstChild("PRXFILES").FirstChild("PRXFILE").Element();elmPrxfile;elmPrxfile = elmPrxfile->NextSiblingElement("PRXFILE"))
//			ProcessPrxfile(elmPrxfile);
//		return 1;
//	}
	return fprintf(stderr, "Couldn't load xml file %s\n", szFilename),0;
}

// Find the name of the dependency library for a specified lib 
const char *NidFindDependency(const char *lib){
//	for(LibraryEntry *pLib = m_pLibHead;pLib;pLib = pLib->pNext)
//		if(strcmp(pLib->lib_name, lib) == 0)
//			return pLib->prx;
	return NULL;
}
