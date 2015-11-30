/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * ProcessPrx.C - Implementation of a class to manipulate a PRX
 **************************************************************
*/
#include <string.h>
#define PSP_SYSTEM_EXPORT "syslib"

#define RELOC_OFS_TEXT 0   // .text section relative reloc offset 
#define RELOC_OFS_DATA 1   // .data section relative reloc offset
#define RELOC_REL_DATA 256 // reloc'ed field should be fixed up relative to the data section base 

#define MINIMUM_STRING 4

typedef struct{
	uint32_t data_addr;
	uint32_t nid_addr;
}PspModuleFunction;

typedef struct{
	uint32_t data_addr;//type:6,data:26
	uint32_t nid_addr;
}PspModuleVariable;

typedef struct{
	uint32_t name;
	uint32_t flags;
	uint8_t  size;
	uint8_t  vars_count;
	uint16_t funcs_count;
	uint32_t exports;//Nid...,PspModuleVariable...,PspModuleFunction...
}PspModuleExport;

typedef struct{
	uint32_t name;
	uint32_t flags;
	uint8_t  size;
	uint8_t  vars_count;
	uint16_t funcs_count;
	uint32_t nids;//uint32_t*
	uint32_t funcs;//PspModuleFunction*
	uint32_t vars;//PspModuleVariable*(optional)
}PspModuleImport;

typedef struct{
	uint32_t flags;
	char name[28];
	uint32_t gp;
	uint32_t exports;//PspModuleExport*
	uint32_t exp_end;
	uint32_t imports;//PspModuleImport*
	uint32_t imp_end;
}PspModuleInfo;

//<Should me removed ?>
typedef enum {
	PSP_ENTRY_FUNC = 0,
	PSP_ENTRY_VAR = 1
}PspEntryType;

typedef struct{
	char name[128];
	uint32_t nid;
	PspEntryType type;
	uint32_t addr;
	uint32_t nid_addr;
}PspEntry;

typedef struct{
	char name[128],file[PATH_MAX];
	uint32_t addr;
	uint16_t f_count;
	uint8_t v_count;
	PspModuleImport stub;
	PspModuleExport export;
	PspEntry funcs[2000],vars[255];
}PspEntries;
//<Should me removed ?>

typedef struct{
	uint32_t addr;
	PspModuleInfo info;

	PspModuleExport*exps;size_t exps_count;
	PspModuleFunction*expfuncs;size_t expfuncs_count;
	PspModuleVariable*expvars ;size_t expvars_count;

	PspModuleImport*imps;size_t imps_count;
	PspModuleFunction*impfuncs;size_t impfuncs_count;
	PspModuleVariable*impvars ;size_t impvars_count;

	PspEntries *exports;size_t exports_count;
	PspEntries *imports;size_t imports_count;

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
	Library    *libs  ;size_t libs_count;
	Nid        *nids  ;size_t nids_count;
	Symbol     *symbol;size_t symbol_count;
	Imm        *imm   ;size_t imm_count;
	uint32_t   base;
}PrxCtx;

#include "prx.import.c"
#include "prx.export.c"
#include "prx.reloc.c"
#include "prx.load.c"
#include "prx.output.c"
#include "prx.disasm.c"

Symbol *prx_getSymbolEntryFromAddr(PrxCtx *pPrx,uint32_t dwAddr){
	return NULL;//prx->symbol[dwAddr];
}
