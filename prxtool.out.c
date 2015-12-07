#include <limits.h>
#include "out.prx.c"

int comp_sym(const void *left, const void *right){
	return ((int) ((ElfSymbol *) left)->value) - ((int) ((ElfSymbol *) right)->value);
}

int output_sym(PrxCtx* prx,FILE *out){
#if 0
	ElfSymbol *pSymbols=NULL;
	int symbol_count=0;//PrxGetSymbols(&pSymbols);
	if(!symbol_count)
		return fprintf(stderr, "No symbol available"),1;
	
	typedef struct{
		char magic[4];
		char modname[sizeof(((PspModule){}).info.name)];
		uint32_t  symcount;
		uint32_t  strstart;
		uint32_t  strsize;
	} __attribute__ ((packed)) SymfileHeader;
	SymfileHeader fileHead;
	ElfSymbol pSymCopy[symbol_count];

	int iSymCopyCount = 0;
	int iStrSize = 0;
	int iStrPos  = 0;
	// Calculate the sizes
	for(int i = 0; i < symbol_count; i++){
		int type = ELF32_ST_TYPE(pSymbols[i].info);
		if(((type == STT_FUNC) || (type == STT_OBJECT)) && (strlen(pSymbols[i].symname) > 0)){
			memcpy(&pSymCopy[iSymCopyCount++], &pSymbols[i], sizeof(ElfSymbol));
			iStrSize += strlen(pSymbols[i].symname) + 1;
		}
	}

	fprintf(stdout,"Removed %d symbol, leaving %d\n", symbol_count - iSymCopyCount, iSymCopyCount);
	fprintf(stdout,"String size %d/%d\n", symbol_count - iSymCopyCount, iSymCopyCount);
	qsort(pSymCopy, iSymCopyCount, sizeof(ElfSymbol), comp_sym);
	memcpy(fileHead.magic, "SYMS", 4);
	// memcpy(fileHead.modname, PrxGetModuleInfo()->name, PspModuleInfo.name);
	typedef struct{
		uint32_t name;
		uint32_t addr;
		uint32_t size;
	} __attribute__((packed))SymfileEntry;
	
	SW(fileHead.symcount, iSymCopyCount);
	SW(fileHead.strstart, sizeof(fileHead) + (sizeof(SymfileEntry)*iSymCopyCount));
	SW(fileHead.strsize, iStrSize);
	fwrite(&fileHead, 1, sizeof(fileHead), out);
	for(int i = 0; i < iSymCopyCount; i++){
		SymfileEntry sym;

		SW(sym.name, iStrPos);
		SW(sym.addr, pSymCopy[i].value);
		SW(sym.size, pSymCopy[i].size);
		iStrPos += strlen(pSymCopy[i].symname)+1;
		fwrite(&sym, 1, sizeof(sym), out);
	}

	// Write out string table
	for(int i = 0; i < iSymCopyCount; i++)
		fwrite(pSymCopy[i].symname, 1, strlen(pSymCopy[i].symname)+1, out);
#endif
	return 0;
}

int output_asm(PrxCtx* prx,FILE *out,char* disopts){
//	PrxDump(prx,out, disopts);
	return 0;
}

int output_mod(PrxCtx* prx,FILE *out){
	ElfCtx*elf=&prx->elf;
	ElfHeader*h = &elf->header;
	fprintf(out,"\n[ELF]\n");
	fprintf(out,"Type:%04X (%s), Start:0x%X Flags:%08X\n",h->type,h->type==ELF_MIPS_TYPE?"ELF":"PRX",h->entry,h->flags);
	fprintf(out,"Programs: %2d*%dB at %06X\n",      h->PHnum,h->PHentSize,h->PHoff);
	fprintf(out,"Sections: %2d*%dB at %06X (.strtab at %d)\n",h->SHnum,h->SHentSize,h->SHoff,h->SHstrIndex);

	if(elf->PH_count){
		char*type2str[]={"NULL","LOAD","DYNAMIC","INTERP","NOTE","SHLIB","PHDR"};
		fprintf(out,"\n[Programs]\nType    Offset VirtAddr PhysAddr FileSz MemSz  Flg Al\n");
		for(ElfProgram*p = elf->program; p < elf->program+elf->PH_count; p++)
			fprintf(out,"%-7s %06X %08X %08X %06X %06X %03X %2X\n",
				p->type<=6?type2str[p->type]:p->type==PT_PRXRELOC?"PRXREL":p->type==PT_PRXRELOC2?"PRXREL2":"?",
				p->iOffset,p->iVaddr,p->iPaddr,p->iFilesz,p->iMemsz,p->flags,p->iAlign);
	}
	if(elf->SH_count){
		char*type2str[]={"NULL","PROGBIT","SYMTAB","STRTAB","RELA","HASH","DYNAMIC","NOTE","NOBITS","REL","SHLIB","DYNSYM"};
		fprintf(out,"\n[Sections]\nType     Addr     Off    Size  ES Flg Lk Inf Al Name\n");
		for(ElfSection*s = elf->section; s < elf->section + elf->SH_count;s++)
			fprintf(out,"%-7s %08X %06X %06X %02X %3X %2X %3X %2d %s\n",
				s->type<=11?type2str[s->type]:s->type==SHT_PRXRELOC?"PRXREL1":"?",
				s->iAddr,s->iOffset,s->iSize,s->iEntsize,s->flags,s->iLink,s->iInfo,s->iAddralign,s->szName);
	}
	if(elf->symbol_count){
		fprintf(out,"\n[Symbols]\nName Value Size Info Other Shndx\n");
		for(ElfSymbol*s=elf->symbol;s < elf->symbol+elf->symbol_count; s++)
			fprintf(out," %d '%s' %08X %08X %02X %02X %04X\n",
				s->name, s->symname,s->value,s->size,s->info,s->other,s->shndx);
	}
	if(elf->reloc_count){
		char* type2str[16] = {
			"NONE","16","32","REL32","26","HI16","LO16","GPREL16","LITERAL",
			"GOT16","PC16","CALL16","GPREL32","HI16","J26","JAL26"
		};
		fprintf(out,"\n[Relocations]\nType       Sym Offset Info Sections\n", elf->reloc_count);
		for(ElfReloc*rel = elf->reloc; rel < elf->reloc+elf->reloc_count; rel++)
			fprintf(out,"%2d:%-7s %3d %06X %04X %s\n", rel->type,
				rel->type<16?type2str[rel->type]:"?", rel->symbol,rel->offset, rel->info, rel->secname?:"");
	}
	
	PspModuleInfo*i=&prx->module.info;
	fprintf(out,"\n[ModuleInfo]\nAddr:%X %.*s Flags:%08X GP:%X Imp:%X..%X Exp:%X..%X\n",prx->module.addr,
		(int)sizeof(i->name),i->name,i->flags, i->gp, i->imports, i->imp_end, i->exports, i->exp_end);
	if(prx->module.imps_count){
		fprintf(out,"\n[ImportedLibs]\nflags    sz var func nids   func   var    name\n");
		for(PspModuleImport*i=prx->module.imps;i<prx->module.imps+prx->module.imps_count;i++)
			fprintf(out,"%08X %2i %3i %4i %06X %06X %06X %s\n",i->flags,i->size,i->vars_count,i->funcs_count,i->nids,i->funcs,i->vars,&prx->elf.elf[elf_translate(&prx->elf,i->name)]);
	}
	if(prx->module.impfuncs_count){
		fprintf(out,"\n[ImportedFuncs]\nNids     Offset\n");
		for(PspModuleFunction*f=prx->module.impfuncs;f<prx->module.impfuncs+prx->module.impfuncs_count;f++)
			fprintf(out,"%08X %06X\n",f->data_addr,f->nid_addr);
	}
	if(prx->module.impvars_count){
		fprintf(out,"\n[ImportedVars]\nNids     Offset\n");
		for(PspModuleVariable*v=prx->module.impvars;v<prx->module.impvars+prx->module.impvars_count;v++)
			fprintf(out,"%08X %06X\n",v->data_addr,v->nid_addr);
	}
	if(prx->module.exps_count){
		fprintf(out,"\n[ExportedLibs]\nflags    sz var func offset name\n");
		for(PspModuleExport*e=prx->module.exps;e<prx->module.exps+prx->module.exps_count;e++)
			fprintf(out,"%08X %2i %3i %4i %06X %s\n",e->flags,e->size,e->vars_count,e->funcs_count,e->exports,elf_translate(&prx->elf,e->name)?(char*)&prx->elf.elf[elf_translate(&prx->elf,e->name)]:"syslib");
	}
	if(prx->module.expfuncs_count){
		fprintf(out,"\n[ExportedFuncs]\nNids     Offset\n");
		for(PspModuleFunction*f=prx->module.expfuncs;f<prx->module.expfuncs+prx->module.expfuncs_count;f++)
			fprintf(out,"%08X %06X\n",f->data_addr,f->nid_addr);
	}
	if(prx->module.expvars_count){
		fprintf(out,"\n[ExportedVars]\nNids     Offset\n");
		for(PspModuleVariable*v=prx->module.expvars;v<prx->module.expvars+prx->module.expvars_count;v++)
			fprintf(out,"%08X %06X\n",v->data_addr,v->nid_addr);
	}
	return 0;
}

int output_elf(PrxCtx* prx,FILE *out){//TODO
//	if(!prx_toElf(prx,out))
//		return fprintf(stderr, "Failed to create a fixed up ELF\n"),1;
	return 0;
}

int output_xml(PrxCtx* prx,FILE *out){
	return 0;
}

int output_map(PrxCtx* prx,FILE *out){
	return 0;
}

int output_idc(PrxCtx* prx,FILE *out){
	return 0;
}

int output_exp(PrxCtx* prx,FILE *out){
#if 0
	fprintf(stdout,"Library %s\n", prx->module.exports->name);
	
	fprintf(out, "PSP_EXPORT_START(%s, 0x%04X, 0x%04X)\n", prx->module.exports->name, prx->module.exports->stub.flags & 0xFFFF, prx->module.exports->stub.flags >> 16);

	char nidName[sizeof(prx->module.exports->name) + 10];
	int nidName_size=strlen(prx->module.exports->name) + 10;
	for(int i = 0; i < prx->module.exports->f_count; i++){
		snprintf(nidName, nidName_size, "%s_%08X", prx->module.exports->name, prx->module.exports->funcs[i].nid);
		if (strcmp(nidName, prx->module.exports->funcs[i].name))
			fprintf(out, "PSP_EXPORT_FUNC_HASH(%s)\n", prx->module.exports->funcs[i].name);
		else
			fprintf(out, "PSP_EXPORT_FUNC_NID(%s, 0x%08X)\n", prx->module.exports->funcs[i].name, prx->module.exports->funcs[i].nid);
	}
	for(int i = 0; i < prx->module.exports->v_count; i++){
		snprintf(nidName, nidName_size, "%s_%08X", prx->module.exports->name, prx->module.exports->vars[i].nid);
		if (strcmp(nidName, prx->module.exports->vars[i].name))
			fprintf(out, "PSP_EXPORT_VAR_HASH(%s)\n", prx->module.exports->vars[i].name);
		else
			fprintf(out, "PSP_EXPORT_VAR_NID(%s, 0x%08X)\n", prx->module.exports->vars[i].name, prx->module.exports->vars[i].nid);
	}
	fprintf(out, "PSP_EXPORT_END\n\n");
#endif
	return 0;
}

int output_S  (PrxCtx* prx,FILE *out,int aliased){
#if 0
	fprintf(stdout,"Library %s\n", prx->module.exports->name);
	if(!prx->module.exports->v_count)
		fprintf(stderr, "%s: Stub output does not currently support variables\n", prx->module.exports->name);

	fprintf(out, "\t.set noreorder\n\n");
	fprintf(out, "#include \"pspimport.s\"\n\n");

	fprintf(out, "// Build List\n");
	fprintf(out, "// %s_0000.o ", prx->module.exports->name);
	for(int i = 0; i < prx->module.exports->f_count; i++)
		fprintf(out, "%s_%04d.o ", prx->module.exports->name, i + 1);
	fprintf(out, "\n\n");

	fprintf(out, "#ifdef F_%s_0000\n", prx->module.exports->name);
	fprintf(out, "\tIMPORT_START\t\"%s\",0x%08X\n", prx->module.exports->name, prx->module.exports->stub.flags);
	fprintf(out, "#endif\n");

	for(int i = 0; i < prx->module.exports->f_count; i++){
		fprintf(out, "#ifdef F_%s_%04d\n", prx->module.exports->name, i + 1);
		Symbol *pSym = NULL; //pPrx?prx_getSymbolEntryFromAddr(pPrx,prx->module.exports->funcs[i].addr)
		if(aliased && (pSym) && (pSym->alias > 0))
			fprintf(out, "\tIMPORT_FUNC_WITH_ALIAS\t\"%s\",0x%08X,%s,%s\n", prx->module.exports->name, prx->module.exports->funcs[i].nid, prx->module.exports->funcs[i].name, strcmp(pSym->name, prx->module.exports->funcs[i].name)?pSym->name:pSym->alias[0]);
		else
			fprintf(out, "\tIMPORT_FUNC\t\"%s\",0x%08X,%s\n", prx->module.exports->name, prx->module.exports->funcs[i].nid, prx->module.exports->funcs[i].name);
		fprintf(out, "#endif\n");
	}
	fclose(out);
#endif
	return 0;
}

int output_htm(PrxCtx* prx,FILE *out){
#if 0
	for(Library *pLib = pNids->libs;pLib;){
		// Convery the Library into a valid PspEntries
		PspEntries prx->module.exports={};
		strcpy(prx->module.exports.name, pLib->lib_name);
		prx->module.exports.f_count = pLib->fcount;
		prx->module.exports.v_count = pLib->vcount;
		prx->module.exports.stub.flags = pLib->flags;

		for(int i = 0; i < prx->module.exports.f_count; i++){
			// prx->module.exports.funcs[i].nid = pLib->pNids[i].nid;
			// strcpy(prx->module.exports.funcs[i].name, pLib->pNids[i].name);
		}

		if(arg_out_stubnew)
			write_stub_new("", &prx->module.exports, NULL);
		else
			write_stub("", &prx->module.exports, NULL);
	}
#endif
	return 0;
}
