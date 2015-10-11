/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * main.C - Main function for PRXTool
 ***************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>

#define PATH_MAX 4096
#define countof(x) (sizeof(x) / sizeof(x[0]))

#include "db.c"
#include "elf.c"
#include "prx.c"
#include "arg.c"
#include "out.c"

#ifndef PRXTOOL_VERSION
#define PRXTOOL_VERSION ""
#endif

int main(int argc, char **argv){
	fprintf(stdout, "PRXTool "PRXTOOL_VERSION" ("__DATE__") \n");
	fprintf(stdout, "(c) TyRaNiD 2k6\n");

	if(arg_init(argc, argv))
		return 1;
	
	DataBase db = {};
	if(arg_nidsfile){
		if(!db_nids_import(NULL,&db.libraries_count,NULL,&db.nids_count,arg_nidsfile)){
			LibraryEntry l[db.libraries_count];
			LibraryNid   n[db.nids_count];
			db_nids_import(db.libraries=l,NULL,db.nids=n,NULL,arg_nidsfile);
		}
		fprintf(stderr,"%i nids loaded in %i libraries\n",db.nids_count,db.libraries_count);
	}
	if(arg_funcfile){
		if(!db_func_import(NULL,&db.functions_count,arg_funcfile)){
			FunctionType f[db.functions_count];
			db_func_import(db.functions=f,NULL,arg_funcfile);
		}
		fprintf(stderr,"%i prototypes loaded\n",db.functions_count);
	}
	//db_nids_print(&db.libraries[217],db.nids,db.nids_count);
	
	FILE *out_fp = arg_outfile?fopen(arg_outfile, arg_out_elf?"wb":"wt"):stdout;
	if(!out_fp)
		return fprintf(stderr, "Couldn't open output file %s\n", arg_outfile),1;
	
	if(arg_out_elf){
		output_elf(arg_inFiles[0], out_fp);
	}
	if(arg_out_stub || arg_out_stubnew){
		//DataBase nidData;
		//nidData.XmlFile=arg_inFiles[0]
		//output_stubs_xml(&nidData);
	}
	if(arg_out_dep){
		for(int i = 0; i < arg_nbFiles; i++){
			output_deps(arg_inFiles[i], &db);
		}
	}
	if(arg_out_mod){
		for(int i = 0; i < arg_nbFiles; i++){
			output_mods(arg_inFiles[i], &db);
		}
	}
	if(arg_out_pstub || arg_out_pstubnew){
		for(int i = 0; i < arg_nbFiles; i++){
			output_stubs_prx(arg_inFiles[i], &db);
		}
	}
	if(arg_out_impexp){
		for(int i = 0; i < arg_nbFiles; i++){
			output_importexport(arg_inFiles[i], &db);
		}
	}
	if(arg_out_symbols){
		output_symbols(arg_inFiles[0], out_fp);
	}
	if(arg_out_xmldb){
		fprintf(out_fp, "<?xml version=\"1.0\" ?>\n");
		fprintf(out_fp, "<firmware title=\"%s\">\n", arg_dbTitle);
		for(int i = 0; i < arg_nbFiles; i++){
			output_xmldb(arg_inFiles[i], out_fp, &db);
		}
		fprintf(out_fp, "</firmware>\n");
	}
	if(arg_out_ent){
		FILE *f = fopen("exports.exp", "w");

		if (f != NULL){
			fprintf(f, "# Export file automatically generated with prxtool\n");
			fprintf(f, "PSP_BEGIN_EXPORTS\n\n");
			output_ents(arg_inFiles[0], &db, f);
			fprintf(f, "PSP_END_EXPORTS\n");
			fclose(f);
		}
	}
	if(arg_out_disasm){
		if(arg_nbFiles == 1){
			output_disasm(arg_inFiles[0], out_fp, &db);
		}else{
			char path[PATH_MAX];
			for(int i = 0; i < arg_nbFiles; i++){
				const char *file = strrchr(arg_inFiles[i], '/');
				int len = snprintf(path, PATH_MAX, arg_xmlOutput?"%s.html":"%s.txt", file?file+1:arg_inFiles[i]);
				if((len < 0) || (len >= PATH_MAX))
					continue;
				FILE *out = fopen(path, "w");
				if(!out){
					fprintf(stdout, "Could not open file %s for writing\n", path);
					continue;
				}
				output_disasm(arg_inFiles[i], out, &db);
				fclose(out);
			}
		}
	}
	//CSerializePrx *pSer=NULL;
	if(arg_out_xml)/*pSer = new CSerializePrxToXml(out_fp);*/;
	if(arg_out_map)/*pSer = new CSerializePrxToMap(out_fp);*/;
	if(arg_out_idc)/*pSer = new CSerializePrxToIdc(out_fp);*/;
	//pSer->Begin();
	//for(int i = 0; i < arg_nbFiles; i++)
	//	serialize_file(arg_inFiles[i], pSer, &db);
	//pSer->End();

	if((arg_outfile) && (out_fp))
		fclose(out_fp);
	
	return 0;
}
