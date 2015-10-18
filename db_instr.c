#include <string.h>
#include <ctype.h>

int db_instr_import(Instruction *instr,size_t*instr_count,Instruction *macro,size_t*macro_count,FILE* fp){
	char line[512],field_sep='\t',mode='i';
	size_t instr_pos=0,macro_pos=0;
	for(fseek(fp,0,SEEK_SET);fgets(line, sizeof(line), fp);){
		if(line[0] == '#'){//comment may indicate instruction types (macro/instr)
			mode=strstr(line+1,"MACRO")?'m':'i';
			continue;
		}
		if(instr && macro){//fill mode
			Instruction tmp={
				.opcode=strtoul(line,NULL,0),
				.mask=strtoul(line+11,NULL,0),
				.addrtype=line[22],
				.type=line[24],
			};
			char*fmt=strrchr(line,field_sep);
			strncpy(tmp.name,line+26,fmt-(line+26));
			strncpy(tmp.fmt,fmt+1,strlen(fmt+1)-1);//-1 to remove the newline
			//printf("%08X %08X %c %c %s %s\n",tmp.opcode,tmp.mask,tmp.addrtype,tmp.type,tmp.name,tmp.fmt);
			if(mode=='i')instr[instr_pos]=tmp;
			if(mode=='m')macro[macro_pos]=tmp;
		}
		if(mode=='i')instr_pos++;
		if(mode=='m')macro_pos++;
	}
	if(instr_count)*instr_count=instr_pos;
	if(macro_count)*macro_count=macro_pos;
	return 0;
}

