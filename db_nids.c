#include <string.h>
#include <stdlib.h>

typedef struct{
	char prx_name[64];
	char lib_name[64];
	char prx[PATH_MAX];
	int  flags;
	int  vcount,fcount;
}LibraryEntry;

typedef struct{
	uint32_t nid;
	char name[128];
	LibraryEntry*owner;
}LibraryNid;


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
char*db_nids_trim_xml(char*str){
	char*end=strrchr(str,'<');//locate last '<'
	char*beg=strchr (str,'>');//locate first '>'
	if(end)*end=0;
	return beg?beg+1:str;
}

void db_nids_print(LibraryEntry *lib, LibraryNid *nids,int nids_count){
	printf("[%s:%s] %s %08X (%i var & %i func)\n",lib->prx,lib->prx_name,lib->lib_name,lib->flags,lib->vcount,lib->fcount);
	for(int i=0;i<nids_count;i++)
		if(nids[i].owner==lib)
			printf("	%08X %s\n",nids[i].nid,nids[i].name);
}

// get function name by Nid

int db_nids_import_xml(LibraryEntry *libraries,int*libraries_count, LibraryNid *nids,int*nids_count, const char* szFilename){
	FILE *fp = fopen(szFilename, "r");
	if(!fp)
		return fprintf(stderr,"Unable to Open \"%s\"\n", szFilename),1;
	char buffer[512];
	LibraryEntry lib={};
	LibraryNid nid={};
	for(int line = 0,l = 0, n = 0;fgets(buffer, sizeof(buffer),fp);line++){
		if(!nids && !libraries){//counting mode
			if(strstr(buffer,"<LIBRARY>"))
				(*libraries_count)++;
			if(strstr(buffer,"<NID>"))
				(*nids_count)++;
		}else{//filling mode
			char*pos;
			if((pos=strstr(buffer,"<PRX>")))
				strcpy(lib.prx,db_nids_trim_xml(pos));
			if((pos=strstr(buffer,"<PRXNAME>")))
				strcpy(lib.prx_name,db_nids_trim_xml(pos));
			if((pos=strstr(buffer,"<NAME>"))){
				if(!lib.lib_name[0])//first NAME is the library name
					strcpy(lib.lib_name,db_nids_trim_xml(pos));
				else{//others NAMEs are nids related
					strcpy(nid.name,db_nids_trim_xml(pos));
				}
			}
			if((pos=strstr(buffer,"<NID>"))){
				nid.nid=strtoul(pos+5,NULL,0);
				nid.owner=&libraries[l];
			}
			if((pos=strstr(buffer,"<FLAGS>")))
				lib.flags=strtoul(pos+7,NULL,0);
			if((pos=strstr(buffer,"</LIBRARY>"))){
				libraries[l]=lib;
				//clear LIBRARY related attribute (and not PRX one !)
				lib.vcount=lib.fcount=lib.flags=lib.lib_name[0]=0;
				l++;
			}
			if((pos=strstr(buffer,"</FUNCTION>"))){
				lib.fcount++;
				nids[n++]=nid;
			}
			if((pos=strstr(buffer,"</VARIABLE>"))){
				lib.vcount++;
				nids[n++]=nid;
			}
		}
	}
	fclose(fp);
	return 0;
}
