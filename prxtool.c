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

#include "nid.c"
#include "elf.c"
#include "prx.c"
#include "arg.c"
#include "out.c"

#define PRXTOOL_VERSION "1.1"

int main(int argc, char **argv){
	fprintf(stdout, "PRXTool v%s : (c) TyRaNiD 2k6\n", PRXTOOL_VERSION);
	fprintf(stdout, "Built: %s %s\n", __DATE__, __TIME__);

	if(arg_init(argc, argv))
		return 1;
	
	FILE *out_fp = arg_outfile?fopen(arg_outfile, arg_out_elf?"wb":"wt"):stdout;
	if(!out_fp)
		return fprintf(stderr, "Couldn't open output file %s\n", arg_outfile),1;

	CNidMgr nids;
	if(arg_nidsfile){
		if((nids.libraries_count = NidsFromXml(NULL,arg_nidsfile))){
			LibraryEntry libraries[nids.libraries_count];
			NidsFromXml(nids.libraries = libraries,arg_nidsfile);
			fprintf(stderr,"%i nids loaded\n",nids.functions_count);
		}
	}
	if(arg_funcfile){
		if((nids.functions_count = FuncFromTxt(NULL,arg_funcfile))){
			FunctionType functions[nids.functions_count];
			FuncFromTxt(nids.functions = functions,arg_funcfile);
			fprintf(stderr,"%i prototypes loaded\n",nids.functions_count);
		}
	}
	return 0;
		//nids->FunctionFile = arg_funcfile;
	if(arg_out_elf){
		output_elf(arg_inFiles[0], out_fp);
	}
	if(arg_out_stub || arg_out_stubnew){
		//CNidMgr nidData;
		//nidData.XmlFile=arg_inFiles[0]
		//output_stubs_xml(&nidData);
	}
	if(arg_out_dep){
		for(int iLoop = 0; iLoop < arg_nbFiles; iLoop++){
			output_deps(arg_inFiles[iLoop], &nids);
		}
	}
	if(arg_out_mod){
		for(int iLoop = 0; iLoop < arg_nbFiles; iLoop++){
			output_mods(arg_inFiles[iLoop], &nids);
		}
	}
	if(arg_out_pstub || arg_out_pstubnew){
		for(int iLoop = 0; iLoop < arg_nbFiles; iLoop++){
			output_stubs_prx(arg_inFiles[iLoop], &nids);
		}
	}
	if(arg_out_impexp){
		for(int iLoop = 0; iLoop < arg_nbFiles; iLoop++){
			output_importexport(arg_inFiles[iLoop], &nids);
		}
	}
	if(arg_out_symbols){
		output_symbols(arg_inFiles[0], out_fp);
	}
	if(arg_out_xmldb){
		fprintf(out_fp, "<?xml version=\"1.0\" ?>\n");
		fprintf(out_fp, "<firmware title=\"%s\">\n", arg_dbTitle);
		for(int iLoop = 0; iLoop < arg_nbFiles; iLoop++){
			output_xmldb(arg_inFiles[iLoop], out_fp, &nids);
		}
		fprintf(out_fp, "</firmware>\n");
	}
	if(arg_out_ent){
		FILE *f = fopen("exports.exp", "w");

		if (f != NULL){
			fprintf(f, "# Export file automatically generated with prxtool\n");
			fprintf(f, "PSP_BEGIN_EXPORTS\n\n");
			output_ents(arg_inFiles[0], &nids, f);
			fprintf(f, "PSP_END_EXPORTS\n");
			fclose(f);
		}
	}
	if(arg_out_disasm){
		if(arg_nbFiles == 1){
			output_disasm(arg_inFiles[0], out_fp, &nids);
		}else{
			char path[PATH_MAX];

			for(int iLoop = 0; iLoop < arg_nbFiles; iLoop++){
				const char *file = strrchr(arg_inFiles[iLoop], '/');
				int len = snprintf(path, PATH_MAX, arg_xmlOutput?"%s.html":"%s.txt", file?file+1:arg_inFiles[iLoop]);
				if((len < 0) || (len >= PATH_MAX))
					continue;

				FILE *out = fopen(path, "w");
				if(!out){
					fprintf(stdout, "Could not open file %s for writing\n", path);
					continue;
				}
				output_disasm(arg_inFiles[iLoop], out, &nids);
				fclose(out);
			}
		}
	}
	//CSerializePrx *pSer=NULL;
	if(arg_out_xml)/*pSer = new CSerializePrxToXml(out_fp);*/;
	if(arg_out_map)/*pSer = new CSerializePrxToMap(out_fp);*/;
	if(arg_out_idc)/*pSer = new CSerializePrxToIdc(out_fp);*/;
	//pSer->Begin();
	//for(int iLoop = 0; iLoop < arg_nbFiles; iLoop++)
	//	serialize_file(arg_inFiles[iLoop], pSer, &nids);
	//pSer->End();

	if((arg_outfile) && (out_fp))
		fclose(out_fp);

	return fprintf(stdout, "Done"),0;
}
