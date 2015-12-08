#include <string.h>
#include <ctype.h>

typedef struct{
	char name[128];
	char args[128];
	char ret[32];
}Protoype;

char *db_func_strip_whitesp(char *str){
	while(isspace((int)*str))str++;
	int len = strlen(str);
	while((len > 0) && (isspace((int)str[len-1])))
		str[--len] = 0;
	return len?str:NULL;
}

int db_func_import(Protoype *func,size_t*count,FILE* fp){
	char line[512],field_sep='\t';
	for(int i=0; fgets(line, sizeof(line), fp);){
		char *name,*args,*ret = NULL;

		if(!(name = db_func_strip_whitesp(line)) || name[0] == '#')
			continue;
		
		if((args = strchr(name, field_sep))){
			*args++ = 0;
			if((ret = strchr(args, field_sep)))
				*ret++ = 0;
		}
		if(func){//fill mode
			if(name)strncpy(func[i].name, name, sizeof(func[i].name));
			if(args)strncpy(func[i].args, args, sizeof(func[i].args));
			if(ret )strncpy(func[i].ret , ret , sizeof(func[i].ret ));
			//fprintf(stdout,">>>%s %s(%s)\n", func[i].ret, func[i].name, func[i].args);
		}
		i++;
		if(count)*count=i;//count mode
	}
	return 0;
}

Protoype *db_func_find(Protoype *func,size_t func_count,const char *name){
	for(unsigned i = 0; i < func_count; i++)
		if(!strcmp(name, func[i].name))
			return &func[i];
	return NULL;
}
