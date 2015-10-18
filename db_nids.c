#include <string.h>
#include <stdlib.h>

typedef struct{
	char prx[PATH_MAX];
	char prx_name[64];
	char lib_name[64];
	int  flags;
	int  vcount,fcount;
}Library;

typedef struct{
	uint32_t nid;
	char name[128];
	Library*owner;
}Nid;


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

void db_nids_print(Library *library, Nid *nids,int nid_count){
	printf("[%s:%s] %s %08X (%i var & %i func)\n",library->prx,library->prx_name,library->lib_name,library->flags,library->vcount,library->fcount);
	for(int i=0;i<nid_count;i++)
		if(nids[i].owner == library)
			printf("	%08X %s\n",nids[i].nid,nids[i].name);
}

// find a function name by owner name + nid
char* db_nids_getFunctionName (Nid   * nids, size_t nids_length    ,char*lib_name, uint32_t nid){
	for(int i=0;i<nids_length;i++)
		if((nids[i].nid == nid) && !strcmp(nids[i].owner->lib_name,lib_name))
			return nids[i].name;
	return NULL;
}
char* db_nids_findPrxByLibName(Library * libs, size_t library_count,char*lib_name){
	for(int i=0;i<library_count;i++)
		if(!strcmp(libs[i].lib_name,lib_name))
			return libs[i].prx;
	return NULL;
}

int db_nids_import_xml(Library *library,size_t*library_count, Nid *nids,size_t*nid_count, FILE* fp){
	char buffer[512],*pos;
	Library lib={};
	Nid nid={};
	unsigned lib_pos = 0, nid_pos = 0;
	for(fseek(fp,0,SEEK_SET);fgets(buffer, sizeof(buffer),fp);){
		if(!nids && !library && library_count && nid_count){//counting mode
			if(strstr(buffer,"<LIBRARY>"))
				(*library_count)++;
			if(strstr(buffer,"<NID>"))
				(*nid_count)++;
		}else{//filling mode
			if((pos=strstr(buffer,"<PRX>")))
				strcpy(lib.prx,db_nids_trim_xml(pos));
			if((pos=strstr(buffer,"<PRXNAME>")))
				strcpy(lib.prx_name,db_nids_trim_xml(pos));
			if((pos=strstr(buffer,"<NAME>")))
				strcpy(lib.lib_name[0]?nid.name:lib.lib_name,db_nids_trim_xml(pos));
			if((pos=strstr(buffer,"<NID>"))){
				nid.nid=strtoul(pos+5,NULL,0);
				nid.owner=&library[lib_pos];
			}
			if((pos=strstr(buffer,"<FLAGS>")))
				lib.flags=strtoul(pos+7,NULL,0);
			if((pos=strstr(buffer,"</LIBRARY>"))){
				library[lib_pos++]=lib;
				//clear LIBRARY related attribute (but not PRX one !)
				lib.vcount=lib.fcount=lib.flags=lib.lib_name[0]=0;
			}
			if((pos=strstr(buffer,"</FUNCTION>"))){
				lib.fcount++;
				nids[nid_pos++]=nid;
			}
			if((pos=strstr(buffer,"</VARIABLE>"))){
				lib.vcount++;
				nids[nid_pos++]=nid;
			}
		}
	}
	return 0;
}

int db_nids_import_yml(Library *library,size_t*library_count, Nid *nids,size_t*nid_count, FILE* fp){
	char buffer[512];
	unsigned lib_pos = 0, nid_pos = 0;
	for(fseek(fp,0,SEEK_SET);fgets(buffer, sizeof(buffer),fp);){
		if(!nids && !library && library_count && nid_count){//counting mode
			if(!strncmp(buffer,"  - ",4))
				(*library_count)++;
			if(!strncmp(buffer,"    - ",6))
				(*nid_count)++;
		}else{
			if(strstr(buffer,".prx ")){//new prx = retreive the prx,prx_name
				char* prx_name = strchr(buffer,' ')?:buffer;
				prx_name[0] = 0;
				prx_name[strlen(prx_name+1)-1]=0;
				strcpy(library[lib_pos].prx,buffer);
				strcpy(library[lib_pos].prx_name,prx_name+1);
			}
			if(!strncmp(buffer,"  - ",4)){//new lib = 
				char* lib_name = strchr(buffer,' ')?:buffer;
				lib_name[0] = '\0';
				strcpy(library[lib_pos+1].prx_name,library[lib_pos].prx_name);
				strcpy(library[lib_pos+1].prx,library[lib_pos].prx);
				strcpy(library[lib_pos].lib_name,lib_name+1);
				library[lib_pos].flags=strtoul(lib_name+1,NULL,0);
			}
			if(!strncmp(buffer,"    - ",6) && lib_pos){//new nid
				char* nid_name = (strchr(buffer+6,' ')?:buffer)+1;
				nids[nid_pos].nid=strtoul(buffer+6,NULL,0);
				int is_var = nid_name[0]=='*';//variable nid name start with a *
				if(is_var)nid_name++;
				nid_name[(strlen(nid_name)?:1)-1]=0;
				strcpy(nids[nid_pos].name,nid_name);
				nids[nid_pos++].owner = &library[lib_pos-1];
				is_var?library[lib_pos-1].vcount++:library[lib_pos-1].fcount++;
			}
		}
	}
	return 0;
}

int db_nids_import(Library *library,size_t*library_count, Nid *nids,size_t*nid_count, FILE* fp){
	char header[5];
	assert(!fseek(fp,0,SEEK_SET) && fread(header,sizeof(char),sizeof(header),fp)==sizeof(header))
	if(!memcmp(header,"<?xml",sizeof(header)))
		return db_nids_import_xml(library, library_count, nids, nid_count, fp);
	else
		return db_nids_import_yml(library, library_count, nids, nid_count, fp);
}