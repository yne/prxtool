#include <string.h>
#include <stdlib.h>

typedef struct{
	char prx[PATH_MAX];
	char prx_name[64];
	char lib_name[64];
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

void db_nids_print(LibraryEntry *library, LibraryNid *nids,int nids_count){
	printf("[%s:%s] %s %08X (%i var & %i func)\n",library->prx,library->prx_name,library->lib_name,library->flags,library->vcount,library->fcount);
	for(int i=0;i<nids_count;i++)
		if(nids[i].owner == library)
			printf("	%08X %s\n",nids[i].nid,nids[i].name);
}

// find a function name by owner name + nid
char* db_nids_getFunctionName(LibraryNid * nids, unsigned nids_length,char*lib_name, uint32_t nid){
	for(int i=0;i<nids_length;i++)
		if((nids[i].nid == nid) && !strcmp(nids[i].owner->lib_name,lib_name))
			return nids[i].name;
	return NULL;
}
char* db_nids_findPrxByLibName(LibraryEntry * libs, unsigned libraries_count,char*lib_name){
	for(int i=0;i<libraries_count;i++)
		if(!strcmp(libs[i].lib_name,lib_name))
			return libs[i].prx;
	return NULL;
}

int db_nids_import_xml(LibraryEntry *libraries,int*libraries_count, LibraryNid *nids,int*nids_count, const char* filename){
	FILE *fp = fopen(filename, "r");
	if(!fp)
		return fprintf(stderr,"Unable to Open \"%s\"\n", filename),1;
	char buffer[512],*pos;
	LibraryEntry lib={};
	LibraryNid nid={};
	for(unsigned line = 0,l = 0, n = 0;fgets(buffer, sizeof(buffer),fp);line++){
		if(!nids && !libraries && libraries_count && nids_count){//counting mode
			if(strstr(buffer,"<LIBRARY>"))
				(*libraries_count)++;
			if(strstr(buffer,"<NID>"))
				(*nids_count)++;
		}else{//filling mode
			if((pos=strstr(buffer,"<PRX>")))
				strcpy(lib.prx,db_nids_trim_xml(pos));
			if((pos=strstr(buffer,"<PRXNAME>")))
				strcpy(lib.prx_name,db_nids_trim_xml(pos));
			if((pos=strstr(buffer,"<NAME>")))
				strcpy(lib.lib_name[0]?nid.name:lib.lib_name,db_nids_trim_xml(pos));
			if((pos=strstr(buffer,"<NID>"))){
				nid.nid=strtoul(pos+5,NULL,0);
				nid.owner=&libraries[l];
			}
			if((pos=strstr(buffer,"<FLAGS>")))
				lib.flags=strtoul(pos+7,NULL,0);
			if((pos=strstr(buffer,"</LIBRARY>"))){
				libraries[l++]=lib;
				//clear LIBRARY related attribute (but not PRX one !)
				lib.vcount=lib.fcount=lib.flags=lib.lib_name[0]=0;
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

int db_nids_import_yml(LibraryEntry *libraries,int*libraries_count, LibraryNid *nids,int*nids_count, const char* filename){
	FILE *fp = fopen(filename, "r");
	if(!fp)
		return fprintf(stderr,"Unable to Open \"%s\"\n", filename),1;
	char buffer[512];
	for(unsigned line = 0,l = 0, n = 0;fgets(buffer, sizeof(buffer),fp);line++){
		if(!nids && !libraries && libraries_count && nids_count){//counting mode
			if(!strncmp(buffer,"  - ",4))
				(*libraries_count)++;
			if(!strncmp(buffer,"    - ",6))
				(*nids_count)++;
		}else{
			if(strstr(buffer,".prx ")){//new prx = retreive the prx,prx_name
				char* prx_name = strchr(buffer,' ')?:buffer;
				prx_name[0] = 0;
				prx_name[strlen(prx_name+1)-1]=0;
				strcpy(libraries[l].prx,buffer);
				strcpy(libraries[l].prx_name,prx_name+1);
			}
			if(!strncmp(buffer,"  - ",4)){//new lib = 
				char* lib_name = strchr(buffer,' ')?:buffer;
				lib_name[0] = '\0';
				strcpy(libraries[l+1].prx_name,libraries[l].prx_name);
				strcpy(libraries[l+1].prx,libraries[l].prx);
				strcpy(libraries[l].lib_name,lib_name+1);
				libraries[l].flags=strtoul(lib_name+1,NULL,0);
			}
			if(!strncmp(buffer,"    - ",6) && l){//new nid
				char* nid_name = (strchr(buffer+6,' ')?:buffer)+1;
				nids[n].nid=strtoul(buffer+6,NULL,0);
				int is_var = nid_name[0]=='*';//variable nid name start with a *
				if(is_var)nid_name++;
				nid_name[(strlen(nid_name)?:1)-1]=0;
				strcpy(nids[n].name,nid_name);
				nids[n++].owner = &libraries[l-1];
				is_var?libraries[l-1].vcount++:libraries[l-1].fcount++;
			}
		}
	}
	return 0;
}

int db_nids_import(LibraryEntry *libraries,int*libraries_count, LibraryNid *nids,int*nids_count, const char* filename){
	if(!strcmp(strrchr(filename,'.')?:filename,".xml"))
		return db_nids_import_xml(libraries, libraries_count, nids, nids_count, filename);
	if(!strcmp(strrchr(filename,'.')?:filename,".yml"))
		return db_nids_import_yml(libraries, libraries_count, nids, nids_count, filename);
	return fprintf(stderr,"Unsupported NID library format\n"),1;
}