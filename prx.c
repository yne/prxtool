/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * ProcessPrx.C - Implementation of a class to manipulate a PRX
 **************************************************************
*/

#define PSP_MODULE_MAX_NAME 28
#define PSP_MODULE_INFO_NAME ".rodata.sceModuleInfo"
#define PSP_SYSTEM_EXPORT "syslib"
#define PSP_IMPORT_BASE_SIZE (5*4)
#define SYMFILE_MAGIC "SYMS"

typedef enum {
	PSP_ENTRY_FUNC = 0,
	PSP_ENTRY_VAR = 1
}PspEntryType;

typedef struct{
	uint32_t name;
	uint32_t flags;
	uint32_t counts;
	uint32_t exports;
}PspModuleExport;

typedef struct{
	uint32_t name;
	uint32_t flags;
	uint32_t counts;
	uint32_t nids;
	uint32_t funcs;
	uint32_t vars;
}PspModuleImport;

typedef struct{
	uint32_t flags;
	char name[PSP_MODULE_MAX_NAME];
	uint32_t gp;
	uint32_t exports;
	uint32_t exp_end;
	uint32_t imports;
	uint32_t imp_end;
}PspModuleInfo;

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
	//PspModuleExport stub;
	PspEntry funcs[2000],vars[255];
}PspEntries;

typedef struct{
	PspModuleInfo info;
	uint32_t addr;
	PspEntries *exports,*imports;
}PspModule;

typedef struct{
	char magic[4];
	char modname[sizeof(((PspModuleInfo){}).name)];
	uint32_t  symcount;
	uint32_t  strstart;
	uint32_t  strsize;
} __attribute__ ((packed)) SymfileHeader;

typedef struct {
	uint32_t name;
	uint32_t addr;
	uint32_t size;
} __attribute__((packed))SymfileEntry;

#include "vmem.c"
#include "asm.c"

static const char* g_szRelTypes[16] = {
	"R_NONE",
	"R_16",
	"R_32",
	"R_REL32",
	"R_26",
	"R_HI16",
	"R_LO16",
	"R_GPREL16",
	"R_LITERAL",
	"R_GOT16",
	"R_PC16",
	"R_CALL16",
	"R_GPREL32",
	"X_HI16",
	"X_J26",
	"X_JAL26"
	
};

// Flag indicates the reloc offset field is relative to the text section base 
#define RELOC_OFS_TEXT 0
// Flag indicates the reloc offset field is relative to the data section base 
#define RELOC_OFS_DATA 1
// Flag indicates the reloc'ed field should be fixed up relative to the data section base 
#define RELOC_REL_DATA 256

// Minimum string size 
#define MINIMUM_STRING 4

typedef struct{
	ElfCProcessElf elf;
	PspModule  module;
	DataBase   defNidMgr;
	DataBase*  pCurrNidMgr;
	Vmem vMem;
	int blPrxLoaded;
	ElfReloc  *pElfRelocs;// Pointer to the allocated relocation entries, if available 
	int iRelocCount;// Number of relocations 
	//Imms imms;
	//Symbols syms;
	uint32_t dwBase;
	uint32_t stubBottom;
	int blXmlDump;
}CProcessPrx;

int PrxLoadImport(CProcessPrx* prx,PspModuleImport *pImport, uint32_t addr){
	char*pName,*dep,*slash;
	PspEntries pLib={
		.addr = addr,
		.stub.name = LW(pImport->name),
		.stub.flags = LW(pImport->flags),
		.stub.counts = LW(pImport->counts),
		.stub.nids = LW(pImport->nids),
		.stub.funcs = LW(pImport->funcs),
		.stub.vars = LW(pImport->vars),
	};
	if(!pLib.stub.name)// Shouldn't be zero, although technically it could be 
		return fprintf(stderr, "Import libraries must have a name"),0;
	if(!(pName = (char*) VmemGetPtr(&prx->vMem,pLib.stub.name)))
		return fprintf(stderr, "Invalid memory address for import name (0x%08X)\n", pLib.stub.name),0;

	strncpy(pLib.name, pName,sizeof(pLib.name));
	if((dep = db_nids_findPrxByLibName(prx->pCurrNidMgr->libraries,prx->pCurrNidMgr->libraries_count,pName))){
		if((slash = strrchr(dep, '/')))// Remove any path element
			dep = slash + 1;
		strcpy(pLib.file, dep);
	}

	fprintf(stdout,"Found import library '%s'\nFlags %08X, counts %08X, nids %08X, funcs %08X\n", pLib.name, pLib.stub.flags, pLib.stub.counts, pLib.stub.nids, pLib.stub.funcs);

	pLib.v_count = pLib.stub.counts >> 8;
	pLib.f_count = pLib.stub.counts >> 16;
	
	if(VmemGetSize(&prx->vMem,pLib.stub.nids) < (sizeof(uint32_t) * pLib.f_count))
		return fprintf(stderr, "Not enough space for library import nids"),0;

	if(VmemGetSize(&prx->vMem,pLib.stub.funcs) < (uint32_t) (8 * pLib.f_count))
		return fprintf(stderr, "Not enough space for library functions"),0;

	for(uint32_t nidAddr = pLib.stub.nids, iLoop = 0, funcAddr = pLib.stub.funcs; iLoop < pLib.f_count; iLoop++,nidAddr += 4,funcAddr += 8){
		pLib.funcs[iLoop].nid = VmemGetU32(&prx->vMem,nidAddr);
		strcpy(pLib.funcs[iLoop].name, db_nids_getFunctionName(prx->pCurrNidMgr->nids,prx->pCurrNidMgr->nids_count, pLib.name, pLib.funcs[iLoop].nid));
		pLib.funcs[iLoop].type = PSP_ENTRY_FUNC;
		pLib.funcs[iLoop].addr = funcAddr;
		pLib.funcs[iLoop].nid_addr = nidAddr;
		fprintf(stdout,"Found import nid:0x%08X func:0x%08X name:%s\n", pLib.funcs[iLoop].nid, pLib.funcs[iLoop].addr, pLib.funcs[iLoop].name);
	}
	
	for(uint32_t varAddr = pLib.stub.vars, iLoop = 0; iLoop < pLib.v_count; iLoop++,varAddr += 8){
		pLib.vars[iLoop].addr = VmemGetU32(&prx->vMem,varAddr);
		pLib.vars[iLoop].nid = VmemGetU32(&prx->vMem,varAddr+4);
		pLib.vars[iLoop].type = PSP_ENTRY_VAR;
		pLib.vars[iLoop].nid_addr = varAddr+4;
		strcpy(pLib.vars[iLoop].name, db_nids_getFunctionName(prx->pCurrNidMgr->nids,prx->pCurrNidMgr->nids_count, pLib.name, pLib.vars[iLoop].nid));
		fprintf(stdout,"Found variable nid:0x%08X addr:0x%08X name:%s\n", pLib.vars[iLoop].nid, pLib.vars[iLoop].addr, pLib.vars[iLoop].name);
		uint32_t varFixup = pLib.vars[iLoop].addr;
		for(uint32_t varData;(varData = VmemGetU32(&prx->vMem,varFixup));varFixup += 4)
			fprintf(stdout,"Variable Fixup: addr:%08X type:%08X\n", (varData & 0x3FFFFFF) << 2, varData >> 26);
		
	}
	//append module imports to the global symbol list
	if(prx->module.imports){
		// Search for the end of the list
		//for(PspEntries* pImport = prx->module.imports;pImport->next;pImport = pImport->next);
		//pImport->next = pLib;
		//pLib.prev = pImport;
		//pLib.next = NULL;
	}else{
		//pLib.next = NULL;
		//pLib.prev = NULL;
		//prx->module.imports = pLib;
	}

	return pLib.stub.counts & 0xFF;
}

int PrxLoadImports(CProcessPrx* prx){
	PspModuleInfo*i=&prx->module.info;
	uint32_t count;void*ptr;
	for(uint32_t base = i->imports;i->imports && (i->imp_end - base) >= PSP_IMPORT_BASE_SIZE;base += (count * sizeof(uint32_t)))
		if(!(ptr=VmemGetPtr(&prx->vMem,base)) || !(count = PrxLoadImport(prx, ptr, base)))
			return 0;
	return 1;
}

int PrxLoadExport(CProcessPrx* prx,PspModuleExport *pExport, uint32_t addr){
	int blError = 1;
	int count = 0;
	int iLoop;
	PspEntries* pLib = NULL;
	uint32_t expAddr;

/*
	assert(pExport != NULL);

	SAFE_ALLOC(pLib, PspEntries);
	if(pLib != NULL){
		do{
			memset(pLib, 0, sizeof(PspEntries));
			pLib->addr = addr;
			pLib->stub.name = LW(pExport->name);
			pLib->stub.flags = LW(pExport->flags);
			pLib->stub.counts = LW(pExport->counts);
			pLib->stub.exports = LW(pExport->exports);

			if(pLib->stub.name == 0){
				// If 0 then this is the system, this should be the only one 
				strcpy(pLib->name, PSP_SYSTEM_EXPORT);
			}else{
				char *pName = (char*) VmemGetPtr(pLib->stub.name);
				if(pName == NULL){
					fprintf(stderr, "Invalid memory address for export name (0x%08X)\n", pLib->stub.name);
					break;
				}

				strcpy(pLib->name, pName);
			}

			fprintf(stdout,"Found export library '%s'\n", pLib->name);
			fprintf(stdout,"Flags %08X, counts %08X, exports %08X\n", 
					pLib->stub.flags, pLib->stub.counts, pLib->stub.exports);

			pLib->v_count = (pLib->stub.counts >> 8) & 0xFF;
			pLib->f_count = (pLib->stub.counts >> 16) & 0xFFFF;
			count = pLib->stub.counts & 0xFF;
			expAddr = pLib->stub.exports;

			if(VmemGetSize(expAddr) < (sizeof(uint32_t) * (pLib->v_count + pLib->f_count))){
				fprintf(stderr, "Invalid memory address for exports (0x%08X)\n", pLib->stub.exports);
				break;
			}

			for(iLoop = 0; iLoop < pLib->f_count; iLoop++){
				// We will fix up the names later 
				pLib->funcs[iLoop].nid = VmemGetU32(expAddr);
				strcpy(pLib->funcs[iLoop].name, prx->pCurrNidMgr->FindLibName(pLib->name, pLib->funcs[iLoop].nid));
				pLib->funcs[iLoop].type = PSP_ENTRY_FUNC;
				pLib->funcs[iLoop].addr = VmemGetU32(expAddr + (sizeof(uint32_t) * (pLib->v_count + pLib->f_count)));
				pLib->funcs[iLoop].nid_addr = expAddr; 
				fprintf(stdout,"Found export nid:0x%08X func:0x%08X name:%s\n", 
											pLib->funcs[iLoop].nid, pLib->funcs[iLoop].addr, pLib->funcs[iLoop].name);
				expAddr += 4;
			}

			for(iLoop = 0; iLoop < pLib->v_count; iLoop++){
				// We will fix up the names later 
				pLib->vars[iLoop].nid = VmemGetU32(expAddr);
				strcpy(pLib->vars[iLoop].name, prx->pCurrNidMgr->FindLibName(pLib->name, pLib->vars[iLoop].nid));
				pLib->vars[iLoop].type = PSP_ENTRY_FUNC;
				pLib->vars[iLoop].addr = VmemGetU32(expAddr + (sizeof(uint32_t) * (pLib->v_count + pLib->f_count)));
				pLib->vars[iLoop].nid_addr = expAddr; 
				fprintf(stdout,"Found export nid:0x%08X var:0x%08X name:%s\n", 
											pLib->vars[iLoop].nid, pLib->vars[iLoop].addr, pLib->vars[iLoop].name);
				expAddr += 4;
			}

			if(prx->module.exports == NULL){
				pLib->next = NULL;
				pLib->prev = NULL;
				prx->module.exports = pLib;
			}else{
				// Search for the end of the list
				PspEntries* pExport;

				pExport = prx->module.exports;
				while(pExport->next != NULL){
					pExport = pExport->next;
				}

				pExport->next = pLib;
				pLib->prev = pExport;
				pLib->next = NULL;
			}

			blError = 0;

		}
		while(0);
	}else{
		fprintf(stderr, "Couldn't allocate memory for export\n");
	}

	if(blError){
		count = 0;
		if(pLib != NULL){
			delete pLib;
			pLib = NULL;
		}
	}
*/
	return count;
}

int PrxLoadExports(CProcessPrx* prx){
	int blRet = 1;
	uint32_t exp_base;
	uint32_t exp_end;
/*
	assert(prx->module.exports == NULL);

	exp_base = prx->module.info.exports;
	exp_end =  prx->module.info.exp_end;
	if(exp_base != 0){
		while((exp_end - exp_base) >= sizeof(PspModuleExport)){
			uint32_t count;
			PspModuleExport *pExport;

			pExport = (PspModuleExport*) VmemGetPtr(exp_base);

			if(pExport != NULL){
				count = LoadSingleExport(pExport, exp_base);
				if(count > 0){
					exp_base += (count * sizeof(uint32_t));
				}else{
					blRet = 0;
					break;
				}
			}else{
				blRet = 0;
				break;
			}
		}
	}
*/
	return blRet;
}

int PrxFillModule(CProcessPrx* prx,PspModuleInfo *pData, uint32_t iAddr){
	if(!pData)
		return 0;
	prx->module=(PspModule){
		.addr=iAddr,
		.info={
			.flags   = LW((*pData).flags),
			.gp      = LW((*pData).gp),
			.exports = LW((*pData).exports),
			.exp_end = LW((*pData).exp_end),
			.imports = LW((*pData).imports),
			.imp_end = LW((*pData).imp_end),
		}
	};
	prx->stubBottom   = prx->module.info.exports - 4; // ".lib.ent.top"
	fprintf(stdout,"Stub bottom 0x%08X\n", prx->stubBottom);

	fprintf(stdout,"Module Info:\n"
		"Name: %s\nAddr: 0x%08X\nFlags: 0x%08X\nGP: 0x%08X\n"
		"Exports: 0x%08X...0x%08X\nImports: 0x%08X...0x%08X\n",
		prx->module.info.name, prx->module.addr, prx->module.info.flags, prx->module.info.gp,
		prx->module.info.exports, prx->module.info.exp_end, prx->module.info.imports, prx->module.info.imp_end);
	return 1;
}

int PrxCreateFakeSections(CProcessPrx* prx){
	// If we have no section headers let's build some fake sections 
	if(!prx->elf.m_iSHCount)
		return 1;
	if(prx->elf.m_iPHCount < 3)
		return fprintf(stderr, "Not enough program headers (%d<3)\n", prx->elf.m_iPHCount),0;

	prx->elf.m_iSHCount = prx->elf.m_pElfPrograms[2].iType == PT_PRXRELOC?6:5;
	prx->elf.m_pElfSections[0]=(ElfSection){};
	prx->elf.m_pElfSections[1]=(ElfSection){
		.iType = SHT_PROGBITS,
		.iFlags = SHF_ALLOC | SHF_EXECINSTR,
		.iAddr = prx->elf.m_pElfPrograms[0].iVaddr,
		.pData = prx->elf.m_pElf + prx->elf.m_pElfPrograms[0].iOffset,
		.iSize = prx->stubBottom,
		.szName= ".text",
	};
	prx->elf.m_pElfSections[2]=(ElfSection){
	.iType = SHT_PROGBITS,
	.iFlags = SHF_ALLOC,
	.iAddr = prx->stubBottom,
	.pData = prx->elf.m_pElf + prx->elf.m_pElfPrograms[0].iOffset + prx->stubBottom,
	.iSize = prx->elf.m_pElfPrograms[0].iMemsz - prx->stubBottom,
	.szName=".rodata",
	};
	prx->elf.m_pElfSections[3]=(ElfSection){
		.iType = SHT_PROGBITS,
		.iFlags = SHF_ALLOC | SHF_WRITE,
		.iAddr = prx->elf.m_pElfPrograms[1].iVaddr,
		.pData = prx->elf.m_pElf + prx->elf.m_pElfPrograms[1].iOffset,
		.iSize = prx->elf.m_pElfPrograms[1].iFilesz,
		.szName= ".data",
	};
	prx->elf.m_pElfSections[4]=(ElfSection){
		.iType = SHT_NOBITS,
		.iFlags = SHF_ALLOC | SHF_WRITE,
		.iAddr = prx->elf.m_pElfPrograms[1].iVaddr + prx->elf.m_pElfPrograms[1].iFilesz,
		.pData = prx->elf.m_pElf + prx->elf.m_pElfPrograms[1].iOffset + prx->elf.m_pElfPrograms[1].iFilesz,
		.iSize = prx->elf.m_pElfPrograms[1].iMemsz - prx->elf.m_pElfPrograms[1].iFilesz,
		.szName= ".bss",
	};
	prx->elf.m_pElfSections[5]=prx->elf.m_pElfPrograms[2].iType == PT_PRXRELOC?(ElfSection){
		.iType = SHT_PRXRELOC,
		.iFlags = 0,
		.iAddr = 0,
		.pData = prx->elf.m_pElf + prx->elf.m_pElfPrograms[2].iOffset,
		.iSize = prx->elf.m_pElfPrograms[2].iFilesz,
		.iInfo = 1,// Bind to section 1, not that is matters 
		.szName= ".reloc",
	}:(ElfSection){};

	//ElfDumpSections();
	return 1;
}

int PrxCountRelocs(CProcessPrx* prx){
	int  iRelocCount = 0;

/*
	for(int iLoop = 0; iLoop < prx->iSHCount; iLoop++){
		if((prx->pElfSections[iLoop].iType == SHT_PRXRELOC) || (prx->pElfSections[iLoop].iType == SHT_REL)){
			if(prx->pElfSections[iLoop].iSize % sizeof(Elf32_Rel))
				fprintf(stdout,"Invalid Relocation section #%i\n",iLoop);
			iRelocCount += prx->pElfSections[iLoop].iSize / sizeof(Elf32_Rel);
		}
	}

	for(int iLoop = 0; iLoop < prx->iPHCount; iLoop++){
		if(prx->pElfPrograms[iLoop].iType != PT_PRXRELOC2)
			continue;
		uint8_t *block1, block1s, part1s;
		uint8_t *block2, block2s, part2s;
		uint8_t *pos, *end;
		
		if (prx->pElfPrograms[iLoop].pData[0] || prx->pElfPrograms[iLoop].pData[1])
			return fprintf(stdout,"Should start with 0x00 0x00\n"),0;
		
		part1s = prx->pElfPrograms[iLoop].pData[2];
		part2s = prx->pElfPrograms[iLoop].pData[3];
		block1s = prx->pElfPrograms[iLoop].pData[4];
		block1 = &prx->pElfPrograms[iLoop].pData[4];
		block2 = block1 + block1s;
		block2s = block2[0];
		pos = block2 + block2s;
		end = &prx->pElfPrograms[iLoop].pData[prx->pElfPrograms[iLoop].iFilesz];
		while (pos < end) {
			uint32_t cmd, part1, temp;
			cmd = pos[0] | (pos[1] << 16);
			pos += 2;
			temp = (cmd << (16 - part1s)) & 0xFFFF;
			temp = (temp >> (16 - part1s)) & 0xFFFF;
			if (temp >= block1s)
				return fprintf(stdout,"Invalid cmd1 index\n"),0;
			part1 = block1[temp];
			if (!(part1 & 0x01) && ( part1 & 0x06 ) == 4 )
				pos += 4;
			else {
				switch (part1 & 0x06) {
				case 2:pos += 2;break;
				case 4:pos += 4;break;
				}
				switch (part1 & 0x38) {
				case 0x10:pos += 2;break;
				case 0x18:pos += 4;break;
				}
			}
			iRelocCount++;
		}
	}
	*/
	fprintf(stdout,"Relocation entries %d\n", iRelocCount);
	return iRelocCount;
}

int PrxLoadRelocsTypeA(CProcessPrx* prx,struct ElfReloc *pRelocs){
	int iCurrRel = 0;
/*
	
	for(int iLoop = 0; iLoop < prx->iSHCount; iLoop++){
		if((prx->pElfSections[iLoop].iType == SHT_PRXRELOC) || (prx->pElfSections[iLoop].iType == SHT_REL)){
			const Elf32_Rel *reloc = (const Elf32_Rel *) prx->pElfSections[iLoop].pData;
			for(int i = 0; i < prx->pElfSections[iLoop].iSize / sizeof(Elf32_Rel); i++) {    
				pRelocs[iCurrRel].secname = prx->pElfSections[iLoop].szName;
				pRelocs[iCurrRel].base = 0;
				pRelocs[iCurrRel].type = ELF32_R_TYPE(LW(reloc->r_info));
				pRelocs[iCurrRel].symbol = ELF32_R_SYM(LW(reloc->r_info));
				pRelocs[iCurrRel].info = LW(reloc->r_info);
				pRelocs[iCurrRel].offset = reloc->r_offset;
				reloc++;
				iCurrRel++;
			}
		}
	}
*/
	return iCurrRel;
}

int PrxLoadRelocsTypeB(CProcessPrx* prx,struct ElfReloc *pRelocs){
	uint8_t *block1, *block2, *pos, *end;
	uint32_t block1s, block2s, part1s, part2s;
	uint32_t cmd, part1, part2, lastpart2;
	uint32_t addend = 0, offset = 0;
	uint32_t ofsbase = 0xFFFFFFFF, addrbase;
	uint32_t temp1, temp2;
	uint32_t nbits;
	int iLoop, iCurrRel = 0;
	/*
	for(iLoop = 0; iLoop < prx->iPHCount; iLoop++){
		if(prx->pElfPrograms[iLoop].iType == PT_PRXRELOC2){
			part1s = prx->pElfPrograms[iLoop].pData[2];
			part2s = prx->pElfPrograms[iLoop].pData[3];
			block1s =prx->pElfPrograms[iLoop].pData[4];
			block1 = &prx->pElfPrograms[iLoop].pData[4];
			block2 = block1 + block1s;
			block2s = block2[0];
			pos = block2 + block2s;
			end = &prx->pElfPrograms[iLoop].pData[prx->pElfPrograms[iLoop].iFilesz];
			
			for (nbits = 1; (1 << nbits) < iLoop; nbits++) {
				if (nbits >= 33) {
					fprintf(stdout,"Invalid nbits\n");
					return 0;
				}
			}

	
			lastpart2 = block2s;
			while (pos < end) {
				cmd = pos[0] | (pos[1] << 8);
				pos += 2;
				temp1 = (cmd << (16 - part1s)) & 0xFFFF;
				temp1 = (temp1 >> (16 - part1s)) & 0xFFFF;
				if (temp1 >= block1s) {
					fprintf(stdout,"Invalid part1 index\n");
					return 0;
				}
				part1 = block1[temp1];
				if ((part1 & 0x01) == 0) {
					ofsbase = (cmd << (16 - part1s - nbits)) & 0xFFFF;
					ofsbase = (ofsbase >> (16 - nbits)) & 0xFFFF;
					if (!(ofsbase < iLoop)) {
						fprintf(stdout,"Invalid offset base\n");
						return 0;
					}

					if ((part1 & 0x06) == 0) {
						offset = cmd >> (part1s + nbits);
					} else if ((part1 & 0x06) == 4) {
						offset = pos[0] | (pos[1] << 8) | (pos[2] << 16) | (pos[3] << 24);
						pos += 4;
					} else {
						fprintf(stdout,"Invalid size\n");
						return 0;
					}
				} else {
					temp2 = (cmd << (16 - (part1s + nbits + part2s))) & 0xFFFF;
					temp2 = (temp2 >> (16 - part2s)) & 0xFFFF;
					if (temp2 >= block2s) {
						fprintf(stdout,"Invalid part2 index\n");
						return 0;
					}

					addrbase = (cmd << (16 - part1s - nbits)) & 0xFFFF;
					addrbase = (addrbase >> (16 - nbits)) & 0xFFFF;
					if (!(addrbase < iLoop)) {
						fprintf(stdout,"Invalid address base\n");
						return 0;
					}
					part2 = block2[temp2];
					
					switch (part1 & 0x06) {
					case 0:
						temp1 = cmd;
						if (temp1 & 0x8000) {
							temp1 |= ~0xFFFF;
							temp1 >>= part1s + part2s + nbits;
							temp1 |= ~0xFFFF;
						} else {
							temp1 >>= part1s + part2s + nbits;
						}
						offset += temp1;
						break;
					case 2:
						temp1 = cmd;
						if (temp1 & 0x8000) temp1 |= ~0xFFFF;
						temp1 = (temp1 >> (part1s + part2s + nbits)) << 16;
						temp1 |= pos[0] | (pos[1] << 8);
						offset += temp1;
						pos += 2;
						break;
					case 4:
						offset = pos[0] | (pos[1] << 8) | (pos[2] << 16) | (pos[3] << 24);
						pos += 4;
						break;
					default:
						fprintf(stdout,"invalid part1 size\n");
						return 0;
					}
					
					if (!(offset < prx->pElfPrograms[ofsbase].iFilesz)) {
						fprintf(stdout,"invalid relocation offset\n");
						return 0;
					}
					
					switch (part1 & 0x38) {
					case 0x00:
						addend = 0;
						break;
					case 0x08:
						if ((lastpart2 ^ 0x04) != 0) {
							addend = 0;
						}
						break;
					case 0x10:
						addend = pos[0] | (pos[1] << 8);
						pos += 2;
						break;
					case 0x18:
						addend = pos[0] | (pos[1] << 8) | (pos[2] << 16) | (pos[3] << 24);
						pos += 4;
						fprintf(stdout,"invalid addendum size\n");
						return 0;
					default:
						fprintf(stdout,"invalid addendum size\n");
						return 0;
					}

					lastpart2 = part2;
					pRelocs[iCurrRel].secname = NULL;
					pRelocs[iCurrRel].base = 0;
					pRelocs[iCurrRel].symbol = ofsbase | (addrbase << 8);
					pRelocs[iCurrRel].info = (ofsbase << 8) | (addrbase << 8);
					pRelocs[iCurrRel].offset = offset;

					switch (part2) {
					case 2:
						pRelocs[iCurrRel].type = R_MIPS_32;
						break;
					case 0:
						continue;
					case 3:
						pRelocs[iCurrRel].type = R_MIPS_26;
						break;
					case 6:
						pRelocs[iCurrRel].type = R_MIPS_X_J26;
						break;
					case 7:
						pRelocs[iCurrRel].type = R_MIPS_X_JAL26;
						break;
					case 4:
						pRelocs[iCurrRel].type = R_MIPS_X_HI16;
						pRelocs[iCurrRel].base = (int16_t) addend;
						break;
					case 1:
					case 5:
						pRelocs[iCurrRel].type = R_MIPS_LO16;
						break;
					default:
						fprintf(stdout,"invalid relocation type\n");
						return 0;
					}
					temp1 = (cmd << (16 - part1s)) & 0xFFFF;
					temp1 = (temp1 >> (16 - part1s)) & 0xFFFF;
					temp2 = (cmd << (16 - (part1s + nbits + part2s))) & 0xFFFF;
					temp2 = (temp2 >> (16 - part2s)) & 0xFFFF;					
					fprintf(stdout,"CMD=0x%04X I1=0x%02X I2=0x%02X PART1=0x%02X PART2=0x%02X\n", cmd, temp1, temp2, part1, part2);
					pRelocs[iCurrRel].info |= pRelocs[iCurrRel].type;
					iCurrRel++;
				}
			}
		}
	}
	*/
	return iCurrRel;
}

int PrxLoadRelocs(CProcessPrx* prx){
	int blRet = 0;
	int  iRelocCount = 0;
	int  iCurrRel = 0;
	int  count;
	int  iLoop;

/*
	iRelocCount = this->CountRelocs();

	if(iRelocCount > 0){
		SAFE_ALLOC(prx->pElfRelocs, ElfReloc[iRelocCount]);
		if(prx->pElfRelocs != NULL){

			memset(prx->pElfRelocs, 0, sizeof(ElfReloc) * iRelocCount);
			
			fprintf(stdout,"Loading Type A relocs\n");
			count = this->LoadRelocsTypeA (&prx->pElfRelocs[iCurrRel]);
			if (count) {
				iCurrRel += count;
			} else {
			}

			fprintf(stdout,"Loading Type B relocs\n");
			count = this->LoadRelocsTypeB (&prx->pElfRelocs[iCurrRel]);
			if (count) {
				iCurrRel += count;
			} else {
			}
			prx->iRelocCount = iCurrRel;
			
			if(COutput::GetDebug()){
				fprintf(stdout,"Dumping relocs %d\n", prx->iRelocCount);
				for(iLoop = 0; iLoop < prx->iRelocCount; iLoop++){
					if(prx->pElfRelocs[iLoop].type < 16){
						fprintf(stdout,"Reloc %s:%d Type:%s Symbol:%d Offset %08X Info:%08X\n", 
								prx->pElfRelocs[iLoop].secname, iLoop, g_szRelTypes[prx->pElfRelocs[iLoop].type],
								prx->pElfRelocs[iLoop].symbol, prx->pElfRelocs[iLoop].offset, prx->pElfRelocs[iLoop].info);
					}else{
						fprintf(stdout,"Reloc %s:%d Type:%d Symbol:%d Offset %08X\n", 
								prx->pElfRelocs[iLoop].secname, iLoop, prx->pElfRelocs[iLoop].type,
								prx->pElfRelocs[iLoop].symbol, prx->pElfRelocs[iLoop].offset);
					}
				}
			}
		}
	}
*/
	blRet = 1;

	return blRet;
}

int PrxLoadFromFile(CProcessPrx* prx,const char *szFilename){
	int blRet = 0;
/*
	if(!ElfLoadFromFile(szFilename))
		return 1;
	// Do PRX specific stuff 
	ElfSection *pInfoSect;
	uint8_t *pData = NULL;
	uint32_t iAddr = 0;

	FreeMemory();
	prx->blPrxLoaded = 0;

	prx->vMem = CVirtualMem(prx->pElfBin, prx->iBinSize, prx->iBaseAddr, MEM_LITTLE_ENDIAN);

	pInfoSect = ElfFindSection(PSP_MODULE_INFO_NAME);
	if(pInfoSect == NULL){
		// Get from program headers 
		if(prx->iPHCount > 0){
			iAddr = (prx->pElfPrograms[0].iPaddr & 0x7FFFFFFF) - prx->pElfPrograms[0].iOffset;
			pData = prx->pElfBin + iAddr;
		}
	}else{
		pData = pInfoSect->pData;
		iAddr = pInfoSect->iAddr;
	}

	if(!pData)
		return fprintf(stderr, "Could not find module section\n"),1;
	if(!PrxFillModule((PspModuleInfo *)pData, iAddr))
		return fprintf(stderr, "Could not fill module\n"),1;
	if(!LoadRelocs())
		return fprintf(stderr, "Could not load relocs\n"),1;
	prx->blPrxLoaded = 1;
	if(prx->pElfRelocs){
		FixupRelocs(prx->dwBase, prx->imms);
	}
	if(!LoadExports())
		return fprintf(stderr, "Could not load exports\n"),1;
	if(!LoadImports())
		return fprintf(stderr, "Could not load imports\n"),1;
	if(!CreateFakeSections())
		return fprintf(stderr, "Could not create fake sections\n"),1;
		
	fprintf(stdout, "Loaded PRX %s successfully\n", szFilename);
	BuildMaps();
*/
	return 0;
}

int PrxLoadFromBinFile(CProcessPrx* prx,const char *szFilename, unsigned int dwDataBase){
	int blRet = 0;
/*
	if(ElfLoadFromBinFile(szFilename, dwDataBase)){
		FreeMemory();
		prx->blPrxLoaded = 0;

		prx->vMem = CVirtualMem(prx->pElfBin, prx->iBinSize, prx->iBaseAddr, MEM_LITTLE_ENDIAN);

		fprintf(stdout, "Loaded BIN %s successfully\n", szFilename);
		blRet = 1;
		prx->blPrxLoaded = 1;
		BuildMaps();
	}
*/
	return blRet;
}

void PrxCalcElfSize(CProcessPrx* prx,size_t *iTotal, size_t *iSectCount, size_t *iStrSize){
	int i;
	int iBinBase = 0;
	// Sect count 2 for NULL and string sections 
	*iSectCount = 2;
	*iTotal = 0;
	// 1 for NUL for NULL section 
	*iStrSize = 2 + strlen(".shstrtab"); 

/*
	for(i = 1; i < prx->iSHCount; i++){
		if(prx->pElfSections[i].iFlags & SHF_ALLOC){
			iSectCount++;
			iStrSize += strlen(prx->pElfSections[i].szName) + 1;
		}
	}
*/
	*iTotal = sizeof(Elf32_Ehdr) + (sizeof(Elf32_Shdr)* *iSectCount) + *iStrSize;
}

int PrxOutputElfHeader(CProcessPrx* prx,FILE *fp, size_t iSectCount){
	Elf32_Ehdr hdr;
/*
	memset(&hdr, 0, sizeof(hdr));
	SW(hdr.e_magic, ELF_MAGIC);
	hdr.e_class = 1;
	hdr.e_data = 1;
	hdr.e_idver = 1;
	SH(hdr.e_type, ELF_MIPS_TYPE);
	SH(hdr.e_machine, 8); 
	SW(hdr.e_version, 1);
	SW(hdr.e_entry, prx->dwBase + prx->elfHeader.iEntry); 
	SW(hdr.e_phoff, 0);
	SW(hdr.e_shoff, sizeof(Elf32_Ehdr));
	SW(hdr.e_flags, 0x10a23001);
	SH(hdr.e_ehsize, sizeof(Elf32_Ehdr));
	SH(hdr.e_phentsize, sizeof(Elf32_Phdr));
	SH(hdr.e_phnum, 0);
	SH(hdr.e_shentsize, sizeof(Elf32_Shdr));
	SH(hdr.e_shnum, iSectCount);
	SH(hdr.e_shstrndx, iSectCount-1);

	if(fwrite(&hdr, 1, sizeof(hdr), fp) != sizeof(hdr)){
		return 0;
	}
*/
	return 1;
}

int PrxOutputSections(CProcessPrx* prx,FILE *fp, size_t iElfHeadSize, size_t iSectCount, size_t iStrSize){
	Elf32_Shdr shdr;
	size_t iStrPointer = 1;
	size_t iBinBase;
	int i;
	char *pStrings;
/*
	pStrings = new char[iStrSize];
	if(pStrings == NULL){
		return 0;
	}
	memset(pStrings, 0, iStrSize);

	iBinBase = (iElfHeadSize + 15) & ~15;
	memset(&shdr, 0, sizeof(shdr));
	// Write NULL section 
	if(fwrite(&shdr, 1, sizeof(shdr), fp) != sizeof(shdr)){
		return 0;
	}

	for(i = 1; i < prx->iSHCount; i++){
		if(prx->pElfSections[i].iFlags & SHF_ALLOC){
			SW(shdr.sh_name, iStrPointer);
			SW(shdr.sh_type, prx->pElfSections[i].iType);
			SW(shdr.sh_flags, prx->pElfSections[i].iFlags);
			SW(shdr.sh_addr, prx->pElfSections[i].iAddr + prx->dwBase);
			if(prx->pElfSections[i].iType == SHT_NOBITS){
				SW(shdr.sh_offset, iBinBase + prx->iElfSize);
			}else{
				SW(shdr.sh_offset, iBinBase + prx->pElfSections[i].iAddr);
			}
			SW(shdr.sh_size, prx->pElfSections[i].iSize);
			SW(shdr.sh_link, 0);
			SW(shdr.sh_info, 0);
			SW(shdr.sh_addralign, prx->pElfSections[i].iAddralign);
			SW(shdr.sh_entsize, 0);
			if(fwrite(&shdr, 1, sizeof(shdr), fp) != sizeof(shdr)){
				return 0;
			}
			strcpy(&pStrings[iStrPointer], prx->pElfSections[i].szName);
			iStrPointer += strlen(prx->pElfSections[i].szName) + 1;
		}
	}

	// Write string section 
	SW(shdr.sh_name, iStrPointer);
	SW(shdr.sh_type, SHT_STRTAB);
	SW(shdr.sh_flags, 0);
	SW(shdr.sh_addr, 0);
	SW(shdr.sh_offset, sizeof(Elf32_Ehdr) + (sizeof(Elf32_Shdr)*iSectCount));
	SW(shdr.sh_size, iStrSize);
	SW(shdr.sh_link, 0);
	SW(shdr.sh_info, 0);
	SW(shdr.sh_addralign, 1);
	SW(shdr.sh_entsize, 0);
	if(fwrite(&shdr, 1, sizeof(shdr), fp) != sizeof(shdr)){
		return 0;
	}

	strcpy(&pStrings[iStrPointer], ".shstrtab");
	iStrPointer += strlen(".shstrtab") + 1;

	assert(iStrSize == iStrPointer);

	if(fwrite(pStrings, 1, iStrSize, fp) != (unsigned) iStrSize){
		return 0;
	}

	delete pStrings;
*/
	return 1;
}

int PrxPrxToElf(CProcessPrx* prx,FILE *fp){
	size_t iElfHeadSize = 0;
	size_t iSectCount = 0;
	size_t iStrSize = 0;
	size_t iAlign = 0;
/*
	// Fixup the elf file and output it to fp 
	if((fp == NULL) || (prx->blPrxLoaded == 0)){
		return 0;
	}

	CalcElfSize(iElfHeadSize, iSectCount, iStrSize);
	fprintf(stdout, "size: %d, sectcount: %d, strsize: %d\n", iElfHeadSize, iSectCount, iStrSize);
	if(!OutputElfHeader(fp, iSectCount)){
		fprintf(stdout, "Could not write ELF header\n");
		return 0;
	}

	if(!OutputSections(fp, iElfHeadSize, iSectCount, iStrSize)){
		fprintf(stdout, "Could not write ELF sections\n");
		return 0;
	}

	// Align data size 
	iAlign = iElfHeadSize & 15;
	if(iAlign > 0){
		char align[16];

		iAlign = 16 - iAlign;
		memset(align, 0, sizeof(align));
		if(fwrite(align, 1, iAlign, fp) != iAlign){
			fprintf(stdout, "Could not write alignment\n");
			return 0;
		}
	}

	if(fwrite(prx->pElfBin, 1, prx->iElfSize, fp) != prx->iElfSize){
		fprintf(stdout, "Could not write out binary image\n");
		return 0;
	}

	fflush(fp);
*/
	return 1;
}

void PrxBuildSymbols(CProcessPrx* prx,/*Symbols *syms,*/ uint32_t dwBase){
	// First map in imports and exports 
	PspEntries *pExport;
	PspEntries *pImport;
	int iLoop;
/*
	// If we have a symbol table then no point building from imports/exports 
	if(prx->pElfSymbols){
		int i;

		for(i = 0; i < prx->iSymCount; i++){
			int iType;
			iType = ELF32_ST_TYPE(prx->pElfSymbols[i].info);
			if((iType == STT_FUNC) || (iType == STT_OBJECT)){
				SymbolEntry *s;
				s = syms[prx->pElfSymbols[i].value + dwBase];
				if(s == NULL){
					s = new SymbolEntry;
					s->addr = prx->pElfSymbols[i].value + dwBase;
					if(iType == STT_FUNC){
						s->type = SYMBOL_FUNC;
					}else{
						s->type = SYMBOL_DATA;
					}
					s->size = prx->pElfSymbols[i].size;
					s->name = prx->pElfSymbols[i].symname; 
					syms[prx->pElfSymbols[i].value + dwBase] = s;
				}else{
					if(strcmp(s->name.c_str(), prx->pElfSymbols[i].symname)){
						s->alias.insert(s->alias.end(), prx->pElfSymbols[i].symname);
					}
				}
			}
		}
	}else{
		pExport = prx->module.exports;
		pImport = prx->module.imports;

		while(pExport != NULL){
			if(pExport->f_count > 0){
				for(iLoop = 0; iLoop < pExport->f_count; iLoop++){
					SymbolEntry *s;

					s = syms[pExport->funcs[iLoop].addr + dwBase];
					if(s){
						if(strcmp(s->name.c_str(), pExport->funcs[iLoop].name)){
							s->alias.insert(s->alias.end(), pExport->funcs[iLoop].name);
						}
						s->exported.insert(s->exported.end(), pExport);
					}else{
						s = new SymbolEntry;
						s->addr = pExport->funcs[iLoop].addr + dwBase;
						s->type = SYMBOL_FUNC;
						s->size = 0;
						s->name = pExport->funcs[iLoop].name;
						s->exported.insert(s->exported.end(), pExport);
						syms[pExport->funcs[iLoop].addr + dwBase] = s;
					}
				}
			}

			if(pExport->v_count > 0){
				for(iLoop = 0; iLoop < pExport->v_count; iLoop++){
					SymbolEntry *s;

					s = syms[pExport->vars[iLoop].addr + dwBase];
					if(s){
						if(strcmp(s->name.c_str(), pExport->vars[iLoop].name)){
							s->alias.insert(s->alias.end(), pExport->vars[iLoop].name);
						}
						s->exported.insert(s->exported.end(), pExport);
					}else{
						s = new SymbolEntry;
						s->addr = pExport->vars[iLoop].addr + dwBase;
						s->type = SYMBOL_DATA;
						s->size = 0;
						s->name = pExport->vars[iLoop].name;
						s->exported.insert(s->exported.end(), pExport);
						syms[pExport->vars[iLoop].addr + dwBase] = s;
					}
				}
			}

			pExport = pExport->next;
		}

		while(pImport != NULL){
			if(pImport->f_count > 0){
				for(iLoop = 0; iLoop < pImport->f_count; iLoop++){
					SymbolEntry *s = new SymbolEntry;
					s->addr = pImport->funcs[iLoop].addr + dwBase;
					s->type = SYMBOL_FUNC;
					s->size = 0;
					s->name = pImport->funcs[iLoop].name;
					s->imported.insert(s->imported.end(), pImport);
					syms[pImport->funcs[iLoop].addr + dwBase] = s;
				}
			}

			if(pImport->v_count > 0){
				for(iLoop = 0; iLoop < pImport->v_count; iLoop++){
					SymbolEntry *s = new SymbolEntry;
					s->addr = pImport->vars[iLoop].addr + dwBase;
					s->type = SYMBOL_DATA;
					s->size = 0;
					s->name = pImport->vars[iLoop].name;
					s->imported.insert(s->imported.end(), pImport);
					syms[pImport->vars[iLoop].addr + dwBase] = s;
				}
			}

			pImport = pImport->next;
		}
	}
	*/
}

void PrxFreeSymbols(CProcessPrx* prx/*Symbols &syms*/){
/*
	Symbols::iterator start = syms.begin();
	Symbols::iterator end = syms.end();

	while(start != end){
		SymbolEntry *p;
		p = syms[(*start).first];
		if(p){
			delete p;
			syms[(*start).first] = NULL;
		}
		++start;
	}
	*/
}

void PrxFreeImms(CProcessPrx* prx/*Imms &imms*/){
/*
	Imms::iterator start = imms.begin();
	Imms::iterator end = imms.end();

	while(start != end){
		ImmEntry *i;

		i = imms[(*start).first];
		if(i){
			delete i;
			imms[(*start).first] = NULL;
		}
		++start;
	}
	*/
}

void PrxFixupRelocs(CProcessPrx* prx,uint32_t dwBase/*, Imms &imms*/){
	int iLoop;
	uint32_t *pData;
	uint32_t regs[32];
/*
	// Fixup the elf file and output it to fp 
	if((prx->blPrxLoaded == 0)){
		return;
	}

	if((prx->elfHeader.iPhnum < 1) || (prx->elfHeader.iPhentsize == 0) || (prx->elfHeader.iPhoff == 0)){
		return;
	}

	// We dont support ELF relocs as they are not very special 
	if(prx->elfHeader.iType != ELF_PRX_TYPE){
		return;
	}

	pData = NULL;
	for(iLoop = 0; iLoop < prx->iRelocCount; iLoop++){
		ElfReloc *rel = &prx->pElfRelocs[iLoop];
		uint32_t dwRealOfs;
		uint32_t dwCurrBase;
		int iOfsPH;
		int iValPH;

		iOfsPH = rel->symbol & 0xFF;
		iValPH = (rel->symbol >> 8) & 0xFF;
		if((iOfsPH >= prx->iPHCount) || (iValPH >= prx->iPHCount)){
			fprintf(stdout,"Invalid relocation PH sets (%d, %d)\n", iOfsPH, iValPH);
			continue;
		}
		dwRealOfs = rel->offset + prx->pElfPrograms[iOfsPH].iVaddr;
		dwCurrBase = dwBase + prx->pElfPrograms[iValPH].iVaddr;
		pData = (uint32_t*) VmemGetPtr(dwRealOfs);
		if(pData == NULL){
			fprintf(stdout,"Invalid offset for relocation (%08X)\n", dwRealOfs);
			continue;
		}

		switch(prx->pElfRelocs[iLoop].type){
			case R_MIPS_HI16: {
				uint32_t inst;
				int base = iLoop;
				int lowaddr, hiaddr, addr;
			  	int loinst;
			  	ImmEntry *imm;
			  	int ofsph = prx->pElfPrograms[iOfsPH].iVaddr;
			  	
				inst = LW(*pData);
				addr = ((inst & 0xFFFF) << 16) + dwCurrBase;
				fprintf(stdout,"Hi at (%08X) %d\n", dwRealOfs, iLoop);
			  	while (++iLoop < prx->iRelocCount) {
			  		if (prx->pElfRelocs[iLoop].type != R_MIPS_HI16) break;
			  	}
				fprintf(stdout,"Matching low at %d\n", iLoop);
			  	if (iLoop < prx->iRelocCount) {
					loinst = LW(*((uint32_t*) VmemGetPtr(prx->pElfRelocs[iLoop].offset+ofsph)));
				} else {
					loinst = 0;
				}

				addr = (int32_t) addr + (int16_t) (loinst & 0xFFFF);
				lowaddr = addr & 0xFFFF;
				hiaddr = (((addr >> 15) + 1) >> 1) & 0xFFFF;
				while (base < iLoop) {
					inst = LW(*((uint32_t*)VmemGetPtr(prx->pElfRelocs[base].offset+ofsph)));
					inst = (inst & ~0xFFFF) | hiaddr;
					SW(*((uint32_t*)VmemGetPtr(prx->pElfRelocs[base].offset+ofsph)), inst);
					base++;
				}
			  	while (iLoop < prx->iRelocCount) {
					inst = LW(*((uint32_t*)VmemGetPtr(prx->pElfRelocs[iLoop].offset+ofsph)));
					if ((inst & 0xFFFF) != (loinst & 0xFFFF)) break;
					inst = (inst & ~0xFFFF) | lowaddr;
					SW(*((uint32_t*)VmemGetPtr(prx->pElfRelocs[iLoop].offset+ofsph)), inst);
									
					imm = new ImmEntry;
					imm->addr = dwBase + ofsph + prx->pElfRelocs[iLoop].offset;
					imm->target = addr;
					imm->text = ElfAddrIsText(addr - dwBase);
					imms[dwBase + ofsph + prx->pElfRelocs[iLoop].offset] = imm;

			  		if (prx->pElfRelocs[++iLoop].type != R_MIPS_LO16) break;
				}
				iLoop--;
				fprintf(stdout,"Finished at %d\n", iLoop);
			}
			break;
			case R_MIPS_16:
			case R_MIPS_LO16: {
				uint32_t loinst;
				uint32_t addr;
				ImmEntry *imm;

				loinst = LW(*pData);
				addr = ((int16_t) (loinst & 0xFFFF) & 0xFFFF) + dwCurrBase;
				fprintf(stdout,"Low at (%08X)\n", dwRealOfs);

				imm = new ImmEntry;
				imm->addr = dwRealOfs + dwBase;
				imm->target = addr;
				imm->text = ElfAddrIsText(addr - dwBase);
				imms[dwRealOfs + dwBase] = imm;

				loinst &= ~0xFFFF;
				loinst |= addr;
				SW(*pData, loinst);
			}
			break;
			case R_MIPS_X_HI16: {
				uint32_t hiinst;
				uint32_t addr, hiaddr;
				ImmEntry *imm;

				hiinst = LW(*pData);
				addr = (hiinst & 0xFFFF) << 16;
				addr += rel->base + dwCurrBase;
				hiaddr = (((addr >> 15) + 1) >> 1) & 0xFFFF;
				fprintf(stdout,"Extended hi at (%08X)\n", dwRealOfs);

				imm = new ImmEntry;
				imm->addr = dwRealOfs + dwBase;
				imm->target = addr;
				imm->text = ElfAddrIsText(addr - dwBase);
				imms[dwRealOfs + dwBase] = imm;

				hiinst &= ~0xFFFF;
				hiinst |= (hiaddr & 0xFFFF);
				SW(*pData, hiinst);			
			}
			break;
			case R_MIPS_X_J26: {
				uint32_t dwData, dwInst;
				uint32_t off = 0;
				int base = iLoop;
				ImmEntry *imm;
				ElfReloc *rel2 = NULL;
				uint32_t offs2 = 0;
				while (++iLoop < prx->iRelocCount){
					rel2 = &prx->pElfRelocs[iLoop];
					if (rel2->type == R_MIPS_X_JAL26 && (dwBase + prx->pElfPrograms[(rel2->symbol >> 8) & 0xFF].iVaddr) == dwCurrBase)
						break;
				}

				if (iLoop < prx->iRelocCount) {
					offs2 = rel2->offset + prx->pElfPrograms[rel2->symbol & 0xFF].iVaddr;
					off = LW(*(uint32_t*) VmemGetPtr(offs2));
				}

				dwInst = LW(*pData);
				dwData = dwInst + (dwCurrBase >> 16);
				SW(*pData, dwData);

				if (off & 0x8000)
				    dwInst--;

				if ((dwData >> 26) != 2) // not J instruction
					imm = new ImmEntry;
					imm->addr = dwRealOfs + dwBase;
					imm->target = dwCurrBase + (((dwInst & 0xFFFF) << 16) | (off & 0xFFFF));
					imm->text = ElfAddrIsText(imm->target - dwBase);
					imms[dwRealOfs + dwBase] = imm;
				}
				// already add the JAL26 symbol so we don't have to search for the J26 there
				if (iLoop < prx->iRelocCount && (dwData >> 26) != 3) // not JAL instruction{
					imm = new ImmEntry;
					imm->addr = offs2 + dwBase;
					imm->target = dwCurrBase + (((dwInst & 0xFFFF) << 16) | (off & 0xFFFF));
					imm->text = ElfAddrIsText(imm->target - dwBase);
					imms[offs2 + dwBase] = imm;
				}

				iLoop = base;
			}
			break;
			case R_MIPS_X_JAL26: {
				uint32_t dwData, dwInst;
				ImmEntry *imm;

				dwInst = LW(*pData);
				dwData = dwInst + (dwCurrBase & 0xFFFF);
				SW(*pData, dwData);
			}
			break;
			case R_MIPS_26: {
				uint32_t dwAddr;
				uint32_t dwInst;

				dwInst = LW(*pData);
				dwAddr = (dwInst & 0x03FFFFFF) << 2;
				dwAddr += dwCurrBase;
				dwInst &= ~0x03FFFFFF;
				dwAddr = (dwAddr >> 2) & 0x03FFFFFF;
				dwInst |= dwAddr;
				SW(*pData, dwInst);
			}
			break;
			case R_MIPS_32: {
				uint32_t dwData;
				ImmEntry *imm;

				dwData = LW(*pData);
				dwData += (dwCurrBase & 0x03FFFFFF);
				dwData += (dwBase >> 2) & 0x03FFFFFF;
				SW(*pData, dwData);

				if ((dwData >> 26) != 2) // not J instruction{
					imm = new ImmEntry;
					imm->addr = dwRealOfs + dwBase;
					imm->target = (dwData & 0x03FFFFFF) << 2;;
					imm->text = ElfAddrIsText(dwData - dwBase);
					imms[dwRealOfs + dwBase] = imm;
				}
			}
			break;
			default: // Do nothing 
			break;
		};
	}
	*/
}

// Print a row of a memory dump, up to row_size 
void PrxPrintRow(CProcessPrx* prx,FILE *fp, const uint32_t* row, int32_t row_size, uint32_t addr){
	char buffer[512];
	char *p = buffer;
	int i = 0;
/*
	sprintf(p, "0x%08X - ", addr);
	p += strlen(p);

	for(i = 0; i < 16; i++){
		if(i < row_size){
			sprintf(p, "%02X ", row[i]);
		}else{
			sprintf(p, "-- ");
		}

		p += strlen(p);

		if((i < 15) && ((i & 3) == 3)){
			*p++ = '|';
			*p++ = ' ';
		}
	}

	sprintf(p, "- ");
	p += strlen(p);

	for(i = 0; i < 16; i++){
		if(i < row_size){
			if((row[i] >= 32) && (row[i] < 127)){
				if(prx->blXmlDump && (row[i] == '<')){
					strcpy(p, "&lt;");
					p += strlen(p);
				}else{
					*p++ = row[i];
				}
			}else{
				*p++ =  '.';
			}
		}else{
			*p++ = '.';
		}
	}
	*p = 0;
*/
	fprintf(fp, "%s\n", buffer);
}

void PrxDumpData(CProcessPrx* prx,FILE *fp, uint32_t dwAddr, uint32_t iSize, unsigned char *pData){
	uint32_t i;
	uint32_t row[16];
	int row_size;

	fprintf(fp, "           - 00 01 02 03 | 04 05 06 07 | 08 09 0A 0B | 0C 0D 0E 0F - 0123456789ABCDEF\n");
	fprintf(fp, "-------------------------------------------------------------------------------------\n");
	/*
	memset(row, 0, sizeof(row));
	row_size = 0;
	for(i = 0; i < iSize; i++){
		row[row_size] = pData[i];
		row_size++;
		if(row_size == 16){
			if(prx->blXmlDump){
				fprintf(fp, "<a name=\"0x%08X\"></a>", dwAddr & ~15);
			}
			PrintRow(fp, row, row_size, dwAddr);
			dwAddr += 16;
			row_size = 0;
			memset(row, 0, sizeof(row));
		}
	}
	if(row_size > 0){
		if(prx->blXmlDump){
			fprintf(fp, "<a name=\"0x%08X\"></a>", dwAddr & ~15);
		}
		PrintRow(fp, row, row_size, dwAddr);
	}
	*/
}

#define ISSPACE(x) ((x) == '\t' || (x) == '\r' || (x) == '\n' || (x) == '\v' || (x) == '\f')

int PrxReadString(CProcessPrx* prx,uint32_t dwAddr, char*str, int unicode, uint32_t *dwRet){
	char* curr = "";
	int iSize = 0;//VmemGetSize(dwAddr);
	int iRealLen = 0;

	if(unicode){
		if(dwAddr & 1)// misaligned unicode word => little chance it is a valid
			return 0;
		iSize /= 2;
	}
	strcpy(str,unicode?"L\"":"\"");

	for(int i = 0; i < iSize; i++){
		// Dirty unicode, we dont _really_ care about it being unicode
		// as opposed to being 16bits 
		
		unsigned int ch = unicode?VmemGetU16(&prx->vMem,dwAddr):VmemGetU8(&prx->vMem,dwAddr);
		dwAddr+=unicode?2:1;
		if((ch == 0) && (iRealLen >= MINIMUM_STRING)){
			if(dwRet)
				*dwRet = dwAddr;
			strcpy(str,"\"");
			return 1;
		}
		if(ISSPACE(ch) || ((ch >= 32) && (ch < 127))){
			if((ch >= 32) && (ch < 127)){
				if((prx->blXmlDump) && (ch == '<'))
					strcpy(str,"&lt;");
				else
					strncpy(str,(const char*)&ch,unicode?2:1);
				iRealLen++;
			}else{
				if(ch=='\t')strcpy(str,"\\t"),iRealLen++;
				if(ch=='\r')strcpy(str,"\\r"),iRealLen++;
				if(ch=='\n')strcpy(str,"\\n"),iRealLen++;
				if(ch=='\v')strcpy(str,"\\v"),iRealLen++;
				if(ch=='\f')strcpy(str,"\\f"),iRealLen++;
			}
		}
	}
	return 0;
}

void PrxDumpStrings(CProcessPrx*prx,FILE *fp, uint32_t dwAddr, uint32_t iSize, unsigned char *pData){
	if(iSize <= MINIMUM_STRING)return;
	char* curr = "";
	int head_printed = 0;
	
	for(uint32_t dwEnd = dwAddr + iSize - MINIMUM_STRING;dwAddr < dwEnd;){
		uint32_t dwNext;
		if(PrxReadString(prx,dwAddr - prx->dwBase, curr, 0, &dwNext) || PrxReadString(prx,dwAddr - prx->dwBase, curr, 1, &dwNext)){
			if(!head_printed){
				fprintf(fp, "\n; Strings\n");
				head_printed = 1;
			}
			fprintf(fp, "0x%08X: %s\n", dwAddr, curr);
			dwAddr = dwNext + prx->dwBase;
		}else{
			dwAddr++;
		}
	}
}

void PrxDisasm(CProcessPrx*prx,FILE *fp, uint32_t dwAddr, uint32_t iSize, unsigned char *pData/*, Imms &imms*/, uint32_t dwBase){
	uint32_t iILoop;
	uint32_t *pInst = (uint32_t*) pData;
	uint32_t inst;
/*
	SymbolEntry *lastFunc = NULL;
	unsigned int lastFuncAddr = 0;
	for(iILoop = 0; iILoop < (iSize / 4); iILoop++){
		SymbolEntry *s;
		FunctionType *t;
		ImmEntry *imm;

		inst = LW(pInst[iILoop]);
		s = disasmFindSymbol(dwAddr);
		if(s){
			switch(s->type){
				case SYMBOL_FUNC: fprintf(fp, "\n; ======================================================\n");
						    	  fprintf(fp, "; Subroutine %s - Address 0x%08X ", s->name.c_str(), dwAddr);
								  if(s->alias.size() > 0){
									  fprintf(fp, "- Aliases: ");
									  uint32_t i;
									  for(i = 0; i < s->alias.size()-1; i++){
										  fprintf(fp, "%s, ", s->alias[i].c_str());
									  }
									 fprintf(fp, "%s", s->alias[i].c_str());
								  }
								  fprintf(fp, "\n");
								  t = db_func_find(prx->pCurrNidMgr,s->name.c_str());
								  if(t){
									  fprintf(fp, "; Prototype: %s (*)(%s)\n", t->ret, t->args);
								  }
								  if(s->size > 0){
									  lastFunc = s;
									  lastFuncAddr = dwAddr + s->size;
								  }
								  if(s->exported.size() > 0){
									  unsigned int i;
									  for(i = 0; i < s->exported.size(); i++){
										if(prx->blXmlDump){
											fprintf(fp, "<a name=\"%s_%s\"></a>; Exported in %s\n", 
													s->exported[i]->name, s->name.c_str(), s->exported[i]->name);
										}else{
											fprintf(fp, "; Exported in %s\n", s->exported[i]->name);
										}
									  }
								  }
								  if(s->imported.size() > 0){
									  unsigned int i;
									  for(i = 0; i < s->imported.size(); i++){
										  if((prx->blXmlDump) && (strlen(s->imported[i]->file) > 0)){
											  fprintf(fp, "; Imported from <a href=\"%s.html#%s_%s\">%s</a>\n", 
													  s->imported[i]->file, s->imported[i]->name, 
													  s->name.c_str(), s->imported[i]->file);
										  }else{
											  fprintf(fp, "; Imported from %s\n", s->imported[i]->name);
										  }
									  }
								  }
								  if(prx->blXmlDump){
								 	  fprintf(fp, "<a name=\"%s\">%s:</a>\n", s->name.c_str(), s->name.c_str());
								  }else{
									  fprintf(fp, "%s:", s->name.c_str());
								  }
								  break;
				case SYMBOL_LOCAL: fprintf(fp, "\n");
								   if(prx->blXmlDump){
								 	  fprintf(fp, "<a name=\"%s\">%s:</a>\n", s->name.c_str(), s->name.c_str());
								   }else{
									   fprintf(fp, "%s:", s->name.c_str());
								   }
								   break;
				default: // Do nothing atm 
								   break;
			};

			if(s->refs.size() > 0){
				uint32_t i;
				fprintf(fp, "\t\t; Refs: ");
				for(i = 0; i < s->refs.size(); i++){
					if(prx->blXmlDump){
						fprintf(fp, "<a href=\"#0x%08X\">0x%08X</a> ", s->refs[i], s->refs[i]);
					}else{
						fprintf(fp, "0x%08X ", s->refs[i]);
					}
				}
			}
			fprintf(fp, "\n");
		}

		imm = imms[dwAddr];
		if(imm){
			SymbolEntry *sym = disasmFindSymbol(imm->target);
			if(imm->text){
				if(sym){
					if(prx->blXmlDump){
						fprintf(fp, "; Text ref <a href=\"#%s\">%s</a> (0x%08X)", sym->name.c_str(), sym->name.c_str(), imm->target);
					}else{
						fprintf(fp, "; Text ref %s (0x%08X)", sym->name.c_str(), imm->target);
					}
				}else{
					if(prx->blXmlDump){
						fprintf(fp, "; Text ref <a href=\"#0x%08X\">0x%08X</a>", imm->target, imm->target);
					}else{
						fprintf(fp, "; Text ref 0x%08X", imm->target);
					}
				}
			}else{
				std::string str;

				if(prx->blXmlDump){
					fprintf(fp, "; Data ref <a href=\"#0x%08X\">0x%08X</a>", imm->target & ~15, imm->target);
				}else{
					fprintf(fp, "; Data ref 0x%08X", imm->target);
				}
				if(ReadString(imm->target-dwBase, str, 0, NULL) || ReadString(imm->target-dwBase, str, 1, NULL)){
					fprintf(fp, " %s", str.c_str());
				}else{
					uint8_t *ptr = (uint8_t*) VmemGetPtr(imm->target - dwBase);
					if(ptr){
						// If a valid pointer try and print some data 
						int i;
						fprintf(fp, " ... ");
						if((imm->target & 3) == 0){
							uint32_t *p32 = (uint32_t*) ptr;
							// Possibly words 
							for(i = 0; i < 4; i++){
								fprintf(fp, "0x%08X ", LW(*p32));
								p32++;
							}
						}else{
							// Just guess at printing bytes 
							for(i = 0; i < 16; i++){
								fprintf(fp, "0x%02X ", *ptr++);
							}
						}
					}
				}
			}
			fprintf(fp, "\n");
		}

		// Check if this is a jump 
		if((inst & 0xFC000000) == 0x0C000000){
			uint32_t dwJump = (inst & 0x03FFFFFF) << 2;
			SymbolEntry *s;
			FunctionType *t;
			dwJump |= (dwBase & 0xF0000000);

			s = disasmFindSymbol(dwJump);
			if(s){
				t = db_func_find(prx->pCurrNidMgr,s->name.c_str());
				if(t){
					fprintf(fp, "; Call - %s %s(%s)\n", t->ret, t->name, t->args);
				}
			}
		}

		if(prx->blXmlDump){
			fprintf(fp, "<a name=\"0x%08X\"></a>", dwAddr);
		}
		fprintf(fp, "\t%-40s\n", disasmInstruction(inst, dwAddr, NULL, NULL, 0));
		dwAddr += 4;
		if((lastFunc != NULL) && (dwAddr >= lastFuncAddr)){
			fprintf(fp, "\n; End Subroutine %s\n", lastFunc->name.c_str());
			fprintf(fp, "; ======================================================\n");
			lastFunc = NULL;
			lastFuncAddr = 0;
		}
	}
	*/
}

void PrxDisasmXML(CProcessPrx*prx,FILE *fp, uint32_t dwAddr, uint32_t iSize, unsigned char *pData/*, Imms &imms*/, uint32_t dwBase){
	uint32_t iILoop;
	uint32_t *pInst;
	pInst  = (uint32_t*) pData;
	uint32_t inst;
	int infunc = 0;
/*
	for(iILoop = 0; iILoop < (iSize / 4); iILoop++){
		SymbolEntry *s;
		//FunctionType *t;
		//ImmEntry *imm;

		inst = LW(pInst[iILoop]);
		s = disasmFindSymbol(dwAddr);
		if(s){
			switch(s->type){
				case SYMBOL_FUNC: if(infunc){
									  fprintf(fp, "</func>\n");
								  }else{
									  infunc = 1;
								  }
								
						    	  fprintf(fp, "<func name=\"%s\" link=\"0x%08X\" ", s->name.c_str(), dwAddr);

								  if(s->refs.size() > 0){
									uint32_t i;
									fprintf(fp, "refs=\"");
									for(i = 0; i < s->refs.size(); i++){
										if(i < (s->refs.size() - 1)){
											fprintf(fp, "0x%08X,", s->refs[i]);
										}else{
											fprintf(fp, "0x%08X", s->refs[i]);
										}
									}
									fprintf(fp, "\" ");
								  }
						    	  fprintf(fp, ">\n");

								  //
								  if(s->exported.size() > 0){
									  unsigned int i;
									  for(i = 0; i < s->exported.size(); i++){
										  unsigned int nid = 0;
										  PspEntries *pExp = s->imported[0];

										  for(int i = 0; i < pImp->f_count; i++){
										  	if(strcmp(s->name.c_str(), pImp->funcs[i].name) == 0){
										  		nid = pImp->funcs[i].nid;
										  		break;
										  	}
										  }
									  }
								  }
								  


								  //
								  if(s->alias.size() > 0){
									  fprintf(fp, "- Aliases: ");
									  uint32_t i;
									  for(i = 0; i < s->alias.size()-1; i++){
										  fprintf(fp, "%s, ", s->alias[i].c_str());
									  }
									 fprintf(fp, "%s", s->alias[i].c_str());
								  }
								  fprintf(fp, "\n");
								  t = db_func_find(prx->pCurrNidMgr,s->name.c_str());
								  if(t){
									  fprintf(fp, "; Prototype: %s (*)(%s)\n", t->ret, t->args);
								  }
								  if(s->size > 0){
									  lastFunc = s;
									  lastFuncAddr = dwAddr + s->size;
								  }
								  if(s->exported.size() > 0){
									  unsigned int i;
									  for(i = 0; i < s->exported.size(); i++){
										if(prx->blXmlDump){
											fprintf(fp, "<a name=\"%s_%s\"></a>; Exported in %s\n", 
													s->exported[i]->name, s->name.c_str(), s->exported[i]->name);
										}else{
											fprintf(fp, "; Exported in %s\n", s->exported[i]->name);
										}
									  }
								  }
								  if(s->imported.size() > 0){
									  unsigned int i;
									  for(i = 0; i < s->imported.size(); i++){
										  if((prx->blXmlDump) && (strlen(s->imported[i]->file) > 0)){
											  fprintf(fp, "; Imported from <a href=\"%s.html#%s_%s\">%s</a>\n", 
													  s->imported[i]->file, s->imported[i]->name, 
													  s->name.c_str(), s->imported[i]->file);
										  }else{
											  fprintf(fp, "; Imported from %s\n", s->imported[i]->name);
										  }
									  }
								  }
								  
								  break;
				case SYMBOL_LOCAL: fprintf(fp, "<local name=\"%s\" link=\"0x%08X\" ", s->name.c_str(), dwAddr);
								  if(s->refs.size() > 0){
									uint32_t i;
									fprintf(fp, "refs=\"");
									for(i = 0; i < s->refs.size(); i++){
										if(i < (s->refs.size() - 1)){
											fprintf(fp, "0x%08X,", s->refs[i]);
										}else{
											fprintf(fp, "0x%08X", s->refs[i]);
										}
									}
									fprintf(fp, "\"");
								  }
						    	  fprintf(fp, "/>\n");
								   break;
				default: // Do nothing atm 
								   break;
			};

		}

#if 0
		imm = imms[dwAddr];
		if(imm){
			SymbolEntry *sym = disasmFindSymbol(imm->target);
			if(imm->text){
				if(sym){
					if(prx->blXmlDump){
						fprintf(fp, "; Text ref <a href=\"#%s\">%s</a> (0x%08X)", sym->name.c_str(), sym->name.c_str(), imm->target);
					}else{
						fprintf(fp, "; Text ref %s (0x%08X)", sym->name.c_str(), imm->target);
					}
				}else{
					if(prx->blXmlDump){
						fprintf(fp, "; Text ref <a href=\"#0x%08X\">0x%08X</a>", imm->target, imm->target);
					}else{
						fprintf(fp, "; Text ref 0x%08X", imm->target);
					}
				}
			}else{
				std::string str;

				if(prx->blXmlDump){
					fprintf(fp, "; Data ref <a href=\"#0x%08X\">0x%08X</a>", imm->target & ~15, imm->target);
				}else{
					fprintf(fp, "; Data ref 0x%08X", imm->target);
				}
				if(ReadString(imm->target-dwBase, str, 0, NULL) || ReadString(imm->target-dwBase, str, 1, NULL)){
					fprintf(fp, " %s", str.c_str());
				}else{
					uint8_t *ptr = (uint8_t*) VmemGetPtr(imm->target - dwBase);
					if(ptr){
						// If a valid pointer try and print some data 
						int i;
						fprintf(fp, " ... ");
						if((imm->target & 3) == 0){
							uint32_t *p32 = (uint32_t*) ptr;
							// Possibly words 
							for(i = 0; i < 4; i++){
								fprintf(fp, "0x%08X ", LW(*p32));
								p32++;
							}
						}else{
							// Just guess at printing bytes 
							for(i = 0; i < 16; i++){
								fprintf(fp, "0x%02X ", *ptr++);
							}
						}
					}
				}
			}
			fprintf(fp, "\n");
		}
#endif

		fprintf(fp, "<inst link=\"0x%08X\">%s</inst>\n", dwAddr, disasmInstructionXML(inst, dwAddr));
		dwAddr += 4;
	}

	if(infunc){
		fprintf(fp, "</func>\n");
	}
	*/
}

int PrxFindFuncExtent(CProcessPrx*prx,uint32_t dwStart, uint8_t *pTouchMap){
	return 0;
}

void PrxMapFuncExtents(CProcessPrx*prx/*,Symbols &syms*/){
/*
	Symbols::iterator start = syms.begin();
	Symbols::iterator end = syms.end();
	uint8_t *pTouchMap;

	pTouchMap = new uint8_t[prx->iBinSize];
	memset(pTouchMap, 0, prx->iBinSize);

	while(start != end){
		SymbolEntry *s;
		s = syms[(*start).first];
		if((s->type == SYMBOL_FUNC) && (s->size == 0)){
			int size;

			size = FindFuncExtent(s->addr, pTouchMap);
			if(size > 0){
				s->size = size;
			}
		}

		start++;
	}
	*/
}

int PrxBuildMaps(CProcessPrx*prx){
	int iLoop;
/*
	BuildSymbols(prx->syms, prx->dwBase);

	Imms::iterator start = prx->imms.begin();
	Imms::iterator end = prx->imms.end();

	while(start != end){
		ImmEntry *imm;
		uint32_t inst;

		imm = prx->imms[(*start).first];
		inst = VmemGetU32(imm->target - prx->dwBase);
		if(imm->text){
			SymbolEntry *s;

			s = prx->syms[imm->target];
			if(s == NULL){
				s = new SymbolEntry;
				char name[128];
				// Hopefully most functions will start with a SP assignment 
				if((inst >> 16) == 0x27BD){
					snprintf(name, sizeof(name), "sub_%08X", imm->target);
					s->type = SYMBOL_FUNC;
				}else{
					snprintf(name, sizeof(name), "loc_%08X", imm->target);
					s->type = SYMBOL_LOCAL;
				}
				s->addr = imm->target;
				s->size = 0;
				s->refs.insert(s->refs.end(), imm->addr);
				s->name = name;
				prx->syms[imm->target] = s;
			}else{
				s->refs.insert(s->refs.end(), imm->addr);
			}
		}

		start++;
	}

	// Build symbols for branches in the code 
	for(iLoop = 0; iLoop < prx->iSHCount; iLoop++){
		if(prx->pElfSections[iLoop].iFlags & SHF_EXECINSTR){
			uint32_t iILoop;
			uint32_t dwAddr;
			uint32_t *pInst;
			dwAddr = prx->pElfSections[iLoop].iAddr;
			pInst  = (uint32_t*) VmemGetPtr(dwAddr);

			for(iILoop = 0; iILoop < (prx->pElfSections[iLoop].iSize / 4); iILoop++){
				disasmAddBranchSymbols(LW(pInst[iILoop]), dwAddr + prx->dwBase, prx->syms);
				dwAddr += 4;
			}
		}
	}

	if(prx->syms[prx->elfHeader.iEntry + prx->dwBase] == NULL){
		SymbolEntry *s;
		s = new SymbolEntry;
		// Hopefully most functions will start with a SP assignment 
		s->type = SYMBOL_FUNC;
		s->addr = prx->elfHeader.iEntry + prx->dwBase;
		s->size = 0;
		s->name = "_start";
		prx->syms[prx->elfHeader.iEntry + prx->dwBase] = s;
	}

	MapFuncExtents(prx->syms);
*/
	return 1;
}

void PrxDump(CProcessPrx*prx,FILE *fp, const char *disopts){
	int iLoop;
/*
	disasmSetSymbols(&prx->syms);
	disasmSetOpts(disopts, 1);

	if(prx->blXmlDump){
		disasmSetXmlOutput();
		fprintf(fp, "<html><body><pre>\n");
	}
	for(iLoop = 0; iLoop < prx->iSHCount; iLoop++){
		if(prx->pElfSections[iLoop].iFlags & (SHF_EXECINSTR | SHF_ALLOC)){
			if((prx->pElfSections[iLoop].iSize > 0) && (prx->pElfSections[iLoop].iType == SHT_PROGBITS)){
				fprintf(fp, "\n; ==== Section %s - Address 0x%08X Size 0x%08X Flags 0x%04X\n", 
						prx->pElfSections[iLoop].szName, prx->pElfSections[iLoop].iAddr + prx->dwBase, 
						prx->pElfSections[iLoop].iSize, prx->pElfSections[iLoop].iFlags);
				if(prx->pElfSections[iLoop].iFlags & SHF_EXECINSTR){
					Disasm(fp, prx->pElfSections[iLoop].iAddr + prx->dwBase, 
							prx->pElfSections[iLoop].iSize, 
							(uint8_t*) VmemGetPtr(prx->pElfSections[iLoop].iAddr),
							prx->imms, prx->dwBase);
				}else{
					DumpData(fp, prx->pElfSections[iLoop].iAddr + prx->dwBase, 
							prx->pElfSections[iLoop].iSize,
							(uint8_t*) VmemGetPtr(prx->pElfSections[iLoop].iAddr));
					DumpStrings(fp, prx->pElfSections[iLoop].iAddr + prx->dwBase, 
							prx->pElfSections[iLoop].iSize, 
							(uint8_t*) VmemGetPtr(prx->pElfSections[iLoop].iAddr));
				}
			}
		}
	}
	if(prx->blXmlDump){
		fprintf(fp, "</pre></body></html>\n");
	}
	disasmSetSymbols(NULL);
*/
}

void PrxDumpXML(CProcessPrx*prx,FILE *fp, const char *disopts){
	int iLoop;
	char *slash;
	PspEntries *pExport;
/*
	disasmSetSymbols(&prx->syms);
	disasmSetOpts(disopts, 1);

	slash = strrchr(prx->szFilename, '/');
	if(!slash){
		slash = prx->szFilename;
	}else{
		slash++;
	}

	fprintf(fp, "<prx file=\"%s\" name=\"%s\">\n", slash, prx->module.name);
	fprintf(fp, "<exports>\n");
	pExport = prx->module.exports;
	while(pExport){
		fprintf(fp, "<lib name=\"%s\">\n", pExport->name);
		for(int i = 0; i < pExport->f_count; i++){
			fprintf(fp, "<func nid=\"0x%08X\" name=\"%s\" ref=\"0x%08X\" />\n", pExport->funcs[i].nid, pExport->funcs[i].name,
					pExport->funcs[i].addr);
		}
		fprintf(fp, "</lib>\n");
		pExport = pExport->next;
	}
	fprintf(fp, "</exports>\n");

	for(iLoop = 0; iLoop < prx->iSHCount; iLoop++){
		if(prx->pElfSections[iLoop].iFlags & (SHF_EXECINSTR | SHF_ALLOC)){
			if((prx->pElfSections[iLoop].iSize > 0) && (prx->pElfSections[iLoop].iType == SHT_PROGBITS)){
				if(prx->pElfSections[iLoop].iFlags & SHF_EXECINSTR){
					fprintf(fp, "<disasm>\n");
					DisasmXML(fp, prx->pElfSections[iLoop].iAddr + prx->dwBase, 
							prx->pElfSections[iLoop].iSize, 
							(uint8_t*) VmemGetPtr(prx->pElfSections[iLoop].iAddr),
							prx->imms, prx->dwBase);
					fprintf(fp, "</disasm>\n");
				}
			}
		}
	}
	fprintf(fp, "</prx>\n");
	disasmSetSymbols(NULL);
*/
}

void PrxSetNidMgr(CProcessPrx*prx,DataBase* nidMgr){
	prx->pCurrNidMgr = nidMgr?nidMgr:&prx->defNidMgr;
}

SymbolEntry *PrxGetSymbolEntryFromAddr(CProcessPrx *pPrx,uint32_t dwAddr){
	return NULL;//prx->syms[dwAddr];
}
