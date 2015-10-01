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
#include <limits.h>
#include <sys/stat.h>

#include "nid.c"
#include "elf.c"
#include "prx.c"
#include "arg.c"
#include "out.c"

#define PRXTOOL_VERSION "1.1"

int main(int argc, char **argv){
	fprintf(stdout, "PRXTool v%s : (c) TyRaNiD 2k6\n", PRXTOOL_VERSION);
	fprintf(stdout, "Built: %s %s\n", __DATE__, __TIME__);

	if(process_args(argc, argv))
		return print_help();
	
	FILE *out_fp = arg_pOutfile?fopen(arg_pOutfile, arg_outputMode==OUTPUT_ELF?"wb":"wt"):stdout;
	if(!out_fp)
		return fprintf(stderr, "Couldn't open output file %s\n", arg_pOutfile),1;

	CNidMgr nids;
	if(arg_pNamefile){
		LibraryEntry libraries[NidsFromXml(NULL,arg_pNamefile)];
		NidsFromXml(nids.libraries = libraries,arg_pNamefile);
	}
	if(arg_pFuncfile){
		FunctionType functions[FuncFromTxt(NULL,arg_pFuncfile)];
		FuncFromTxt(nids.functions = functions,arg_pFuncfile);
	}
		//nids->FunctionFile = arg_pFuncfile;
	switch(arg_outputMode){
		case OUTPUT_ELF:{
			output_elf(arg_ppInfiles[0], out_fp);
		}break;
		case OUTPUT_STUB:{
			//CNidMgr nidData;
			//nidData.XmlFile=arg_ppInfiles[0]
			//output_stubs_xml(&nidData);
		}break;
		case OUTPUT_DEP:{
			for(int iLoop = 0; iLoop < arg_iInFiles; iLoop++){
				output_deps(arg_ppInfiles[iLoop]/*, &nids*/);
			}
		}break;
		case OUTPUT_MOD:{
			for(int iLoop = 0; iLoop < arg_iInFiles; iLoop++){
				output_mods(arg_ppInfiles[iLoop]/*, &nids*/);
			}
		}break;
		case OUTPUT_PSTUB:{
			for(int iLoop = 0; iLoop < arg_iInFiles; iLoop++){
				output_stubs_prx(arg_ppInfiles[iLoop]/*, &nids*/);
			}
		}break;
		case OUTPUT_IMPEXP:{
			for(int iLoop = 0; iLoop < arg_iInFiles; iLoop++){
				output_importexport(arg_ppInfiles[iLoop]/*, &nids*/);
			}
		}break;
		case OUTPUT_SYMBOLS:{
			output_symbols(arg_ppInfiles[0], out_fp);
		}break;
		case OUTPUT_XMLDB:{
			fprintf(out_fp, "<?xml version=\"1.0\" ?>\n");
			fprintf(out_fp, "<firmware title=\"%s\">\n", arg_pDbTitle);
			for(int iLoop = 0; iLoop < arg_iInFiles; iLoop++){
				output_xmldb(arg_ppInfiles[iLoop], out_fp/*, &nids*/);
			}
			fprintf(out_fp, "</firmware>\n");
		}break;
		case OUTPUT_ENT:{
			FILE *f = fopen("exports.exp", "w");

			if (f != NULL){
				fprintf(f, "# Export file automatically generated with prxtool\n");
				fprintf(f, "PSP_BEGIN_EXPORTS\n\n");
				output_ents(arg_ppInfiles[0]/*, &nids*/, f);
				fprintf(f, "PSP_END_EXPORTS\n");
				fclose(f);
			}
		}break;
		case OUTPUT_DISASM:{
			if(arg_iInFiles == 1){
				output_disasm(arg_ppInfiles[0], out_fp/*, &nids*/);
			}else{
				char path[PATH_MAX];

				for(int iLoop = 0; iLoop < arg_iInFiles; iLoop++){
					const char *file = strrchr(arg_ppInfiles[iLoop], '/');
					int len = snprintf(path, PATH_MAX, arg_xmlOutput?"%s.html":"%s.txt", file?file+1:arg_ppInfiles[iLoop]);
					if((len < 0) || (len >= PATH_MAX))
						continue;

					FILE *out = fopen(path, "w");
					if(!out){
						fprintf(stdout, "Could not open file %s for writing\n", path);
						continue;
					}
					output_disasm(arg_ppInfiles[iLoop], out/*, &nids*/);
					fclose(out);
				}
			}
		}break;
		default:{
			//CSerializePrx *pSer;
			switch(arg_outputMode){
				case OUTPUT_XML : /*pSer = new CSerializePrxToXml(out_fp);*/break;
				case OUTPUT_MAP : /*pSer = new CSerializePrxToMap(out_fp);*/break;
				case OUTPUT_IDC : /*pSer = new CSerializePrxToIdc(out_fp);*/break;
				default: /*pSer = NULL;*/break;
			};
			//pSer->Begin();
			//for(int iLoop = 0; iLoop < arg_iInFiles; iLoop++){
			//	serialize_file(arg_ppInfiles[iLoop], pSer, &nids);
			//}
			//pSer->End();
		}
	}

	if((arg_pOutfile) && (out_fp)){
		fclose(out_fp);
	}

	fprintf(stdout, "Done");
}
