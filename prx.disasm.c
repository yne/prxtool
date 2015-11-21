//TODO:use print callback to replace txt/xml output
void prx_disasm   (PrxCtx*prx,FILE *fp, uint32_t dwAddr, uint32_t iSize, unsigned char *pData, Imm* imm, size_t imm_count, uint32_t base,
	Protoype*proto,size_t proto_count,Instruction*macro,size_t macro_count,Instruction*instr,size_t instr_count){
	uint32_t *pInst = (uint32_t*) pData;
	Symbol *lastFunc = NULL;
	unsigned int lastFuncAddr = 0;
	for(int iILoop = 0; iILoop < (iSize / 4); iILoop++){
		uint32_t inst = LW(pInst[iILoop]);
		Symbol *s = disasmFindSymbol(dwAddr);
		if(s){
			if(s->type == SYMBOL_FUNC){
				fprintf(fp, "\n; ======================================================\n");
					fprintf(fp, "; Subroutine %s - Address 0x%08X ", s->name, dwAddr);
				if(s->alias){
					fprintf(fp, "- Aliases: ");
					for(uint32_t i = 0; s->alias[i][0] && i < (sizeof(s->alias)/sizeof(s->alias[0])); i++)
						fprintf(fp, "%s ", s->alias[i]);
				}
				fprintf(fp, "\n");
				Protoype *t = db_func_find(proto,proto_count,s->name);
				if(t)
					fprintf(fp, "; Prototype: %s (*)(%s)\n", t->ret, t->args);
				if(s->size > 0){
					lastFunc = s;
					lastFuncAddr = dwAddr + s->size;
				}
				/*
				if(s->exported.size() > 0){
					for(unsigned i = 0; i < s->exported.size(); i++){
						if(prx->isXmlDump)
							fprintf(fp, "<a name=\"%s_%s\"></a>; Exported in %s\n", s->exported[i]->name, s->name, s->exported[i]->name);
						else
							fprintf(fp, "; Exported in %s\n", s->exported[i]->name);
					}
				}
				if(s->imported.size() > 0){
					for(unsigned i = 0; i < s->imported.size(); i++){
						if((prx->isXmlDump) && (strlen(s->imported[i]->file) > 0))
							fprintf(fp, "; Imported from <a href=\"%s.html#%s_%s\">%s</a>\n", s->imported[i]->file, s->imported[i]->name, s->name, s->imported[i]->file);
						else
							fprintf(fp, "; Imported from %s\n", s->imported[i]->name);
					}
				}
				*/
				//if(prx->isXmlDump)
				//	fprintf(fp, "<a name=\"%s\">%s:</a>\n", s->name, s->name);
				//else
					fprintf(fp, "%s:", s->name);
			}
			if(s->type == SYMBOL_LOCAL){
				fprintf(fp, "\n");
				//if(prx->isXmlDump)
				//	fprintf(fp, "<a name=\"%s\">%s:</a>\n", s->name, s->name);
				//else
					fprintf(fp, "%s:", s->name);
			}
			/*
			if(s->refs.size() > 0){
				fprintf(fp, "\t\t; Refs: ");
				for(uint32_t i = 0; i < s->refs.size(); i++){
					if(prx->isXmlDump){
						fprintf(fp, "<a href=\"#0x%08X\">0x%08X</a> ", s->refs[i], s->refs[i]);
					}else{
						fprintf(fp, "0x%08X ", s->refs[i]);
					}
				}
			}
			*/
			fprintf(fp, "\n");
		}

		Imm *imm = &imm[dwAddr];
		if(imm){
		/*
			Symbol *sym = disasmFindSymbol(imm->target);
			if(imm->text){
				if(sym){
					if(prx->isXmlDump){
						fprintf(fp, "; Text ref <a href=\"#%s\">%s</a> (0x%08X)", sym->name, sym->name, imm->target);
					}else{
						fprintf(fp, "; Text ref %s (0x%08X)", sym->name, imm->target);
					}
				}else{
					if(prx->isXmlDump){
						fprintf(fp, "; Text ref <a href=\"#0x%08X\">0x%08X</a>", imm->target, imm->target);
					}else{
						fprintf(fp, "; Text ref 0x%08X", imm->target);
					}
				}
			}else{
				std::string str;

				if(prx->isXmlDump){
					fprintf(fp, "; Data ref <a href=\"#0x%08X\">0x%08X</a>", imm->target & ~15, imm->target);
				}else{
					fprintf(fp, "; Data ref 0x%08X", imm->target);
				}
				if(ReadString(imm->target-base, str, 0, NULL) || ReadString(imm->target-base, str, 1, NULL)){
					fprintf(fp, " %s", str);
				}else{
					uint8_t *ptr = (uint8_t*) VmemGetPtr(imm->target - base);
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
			*/
			fprintf(fp, "\n");
		}

		// Check if this is a jump 
		if((inst & 0xFC000000) == 0x0C000000){
			uint32_t dwJump = (inst & 0x03FFFFFF) << 2;
			dwJump |= (base & 0xF0000000);

			Symbol *s = disasmFindSymbol(dwJump);
			if(s){
				Protoype *t = db_func_find(proto,proto_count,s->name);
				if(t)
					fprintf(fp, "; Call - %s %s(%s)\n", t->ret, t->name, t->args);
			}
		}

		//if(prx->isXmlDump)
		//	fprintf(fp, "<a name=\"0x%08X\"></a>", dwAddr);
		fprintf(fp, "\t%-40s\n", disasmInstruction(inst, dwAddr, NULL, NULL, 0, macro, macro_count, instr, instr_count));
		dwAddr += 4;
		if((lastFunc != NULL) && (dwAddr >= lastFuncAddr)){
			fprintf(fp, "\n; End Subroutine %s\n", lastFunc->name);
			fprintf(fp, "; ======================================================\n");
			lastFunc = NULL;
			lastFuncAddr = 0;
		}
	}
}
