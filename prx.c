/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * ProcessPrx.C - Implementation of a class to manipulate a PRX
 **************************************************************
*/
#include <string.h>
#define PSP_MODULE_MAX_NAME 28
#define PSP_MODULE_INFO_NAME ".rodata.sceModuleInfo"
#define PSP_SYSTEM_EXPORT "syslib"
#define PSP_IMPORT_BASE_SIZE (5*4)
#define SYMFILE_MAGIC "SYMS"

#define RELOC_OFS_TEXT 0   // .text section relative reloc offset 
#define RELOC_OFS_DATA 1   // .data section relative reloc offset
#define RELOC_REL_DATA 256 // reloc'ed field should be fixed up relative to the data section base 

#define MINIMUM_STRING 4

typedef struct{
	char name[128];
	uint32_t nid;
	enum {PSP_ENTRY_FUNC,PSP_ENTRY_VAR}isvar;
	uint32_t addr;
	uint32_t nid_addr;
}PspEntry;

typedef struct{
	char name[128],file[PATH_MAX];
	uint32_t addr;
	uint16_t f_count;
	uint8_t  v_count;
	struct{//import
		uint32_t name;
		uint32_t flags;
		uint32_t counts;
		uint32_t nids;
		uint32_t funcs;
		uint32_t vars;
	}stub;
	struct{//export
		uint32_t name;
		uint32_t flags;
		uint32_t counts;
		uint32_t exports;
	}export;
	PspEntry funcs[2000],vars[255];
}PspEntries;

typedef struct{
	struct{
		uint32_t flags;
		char     name[PSP_MODULE_MAX_NAME];
		uint32_t gp;
		uint32_t exports;
		uint32_t exp_end;
		uint32_t imports;
		uint32_t imp_end;
	}info;
	uint32_t addr;
	PspEntries*exports;size_t exports_count;
	PspEntries*imports;size_t imports_count;
}PspModule;

#include "vmem.c"
#include "asm.c"
#include "elf.c"
#include "db_func.c"
#include "db_nids.c"
#include "db_instr.c"

typedef struct{
	ElfCtx     elf;
	PspModule  module;
	Vmem       vMem;
	Library    *library;size_t library_count;
	Nid        *nids   ;size_t nid_count;
	Protoype   *proto  ;size_t proto_count;
	Instruction*macro  ;size_t macro_count;
	Instruction*instr  ;size_t instr_count;
	Symbol     *symbol ;size_t symbol_count;
	Imm        *imm    ;size_t imm_count;
	ElfReloc   *reloc  ;size_t reloc_count;
	int        isPrxLoaded;
	int        isXmlDump;
	uint32_t   stubBottom;
	uint32_t   base;
}PrxToolCtx;

//#include "prx.impexp.c"
//#include "prx.reloc.c"
//#include "prx.load.c"
//#include "prx.output.c"
//#include "prx.disasm.c"

//Symbol *PrxGetSymbolEntryFromAddr(PrxToolCtx *pPrx,uint32_t dwAddr){
//	return NULL;//prx->symbol[dwAddr];
//}
